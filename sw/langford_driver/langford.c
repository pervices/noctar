/***********************************************************************
 * The langford driver. This initializes and handles the DMA transfers,
 * and dataflow between the FPGA and computer.
 * 
 * (c) 2017 Per Vices Corporation
 * 
 * This code is licensed under the GPLv2.
 ************************************************************************/




/*Sources:
http://lwn.net/Kernel/LDD3/
http://free-electrons.com/doc/pci-drivers.pdf
linux-2.6.38/drivers/i2c/busses/i2c-i801.c
*/

#include "langford.h"


// NOTE: Linux kernel commit 63a29f744fe1c19742039ce7526663a98f172f7e removes
// the use of __dev (and __devinit, __devexit_p, __devinitdata, _devinitconst,
// and __devexit from ther kernel because CONFIG_HOTPLUG is going away.
//
// To address this, we redefine these functions for kernels 3.8+

// NOTE re: cdev_read_buff_mutex, cdev_write_buff_mutex:
// For writes: cdev_write and dev_write_task use cdev_write_buff_mutex to share beginning and end pointers to cdev_write_buff (cdev_write_buff_start,cdev_write_buff_end)
// while dev_write_task only updates cdev_write_buff_start, and reads cdev_write_buff_end, cdev_write function does opposite: only updates cdev_write_buff_end, and reads cdev_write_buff_start
// cdev_write writes data at location cdev_write_buff+cdev_write_buff_end, while dev_write_task reads at location cdev_write_buff+cdev_write_buff_start, which will never overlap due to wait loops
// checking for such condition. Therefore locks were moved out of memcpy/copyfromuser areas, reducing unnecisary waiting in the write thread and cdev_write function.
// Same applies to Read thread and cdev_read. 

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
#define __devinit
#define __devexit
#define __devexit_p(x) x
#endif

static DevPrivDataType	*DevPrivData;

/*Character device data structures*/
static dev_t			cdev;
static struct cdev		driver_cdev;

static DEFINE_MUTEX(cdev_open_close_mutex);		/**< Mutex for file open and close*/

static DEFINE_MUTEX(cdev_read_buff_mutex);		/**< Read buffer mutex*/
static uint8_t			*cdev_read_buff;	/**< Read buffer*/
static int			cdev_read_buff_start;	/**< Start pointer to read buffer*/
static int			cdev_read_buff_end;	/**< End pointer to read buffer*/

static DEFINE_MUTEX(cdev_write_buff_mutex);		/**< Write buffer mutex*/
static uint8_t			*cdev_write_buff;	/**< Write buffer*/
static int			cdev_write_buff_start;	/**< Start pointer to write buffer*/
static int			cdev_write_buff_end;	/**< End pointer to write buffer*/


/*DMA read and write threads*/
static struct task_struct	*dev_read_thread;	/**< Thread in charge of reading data from PCIe device*/
static struct task_struct	*dev_write_thread;	/**< Thread in charge of writing data to PCIe device*/



inline int circ_buff_filled(int start, int end, int len) {
	int		rc;

	rc = end - start;

	if (rc < 0) {
		rc = len + rc;
	};

	return rc;
};


/**Figures out how many elements of a circular buffer are available to be filled
*/
inline int circ_buff_avail(int start, int end, int len) {
	return len - circ_buff_filled(start, end, len) - 1;
};



/**Device read thread

This thread:
- constantly performs read operations from the device via DMA
- copy transferred rx data to cdev_read_buff
*/
int dev_read_task(void *data) {
	int		DescLevel;			/*Descriptor FIFO level read from device*/
	int		DescActive, DescComp;		/*Number of descriptors to be serviced and the number of completed descriptors*/
	int		BuffStart, BuffEnd;		/*Indices to beginning and end of DMA buffers ring array (DevPrivData->RxDmaBuffsBusAddr)*/
	int		Ctr;
	int		BuffsToWrite;
	uint32_t	DescAddr;
	int 		DescFlag;			/*Set when descriptors were written in 'Cnt' for loop, used to sleep if no descriptors were written, giving cdev_read a timeslice*/

	printk(KERN_DEBUG DRIVER_NAME " Read thread created\n");

	BuffStart = BuffEnd = 0;
	DescActive = DescComp = 0;
	for (Ctr = 0; ; Ctr++) { 

		mb();
		do{/*Loop until at least one descriptor was processed*/
			if (kthread_should_stop()) {return 0;}
			/*Find how many descriptors are remaining in the FPGA write buffer: if full, then 1ms-5ms sleep, giving priority to other threads*/
			if (((readl(DevPrivData->pBar1 + WRCSRFILLLEVEL) >> 16) & 0x0000ffff)>=DMARXDESCS) msleep(RX_THREAD_MSLEEP);/*1ms is around 2 DMA buffs at 500MB/s or 3 at 800MB/s */
			else break;
		} while (1);
		/*Find how many descriptors are remaining in the FPGA write buffer*/
		DescLevel = (readl(DevPrivData->pBar1 + WRCSRFILLLEVEL) >> 16) & 0x0000ffff;
#if VERBOSE_LOGGING_R /*----v----v----v----v----v----v----v----v----v----v----v----v*/
		printk(KERN_DEBUG DRIVER_NAME " Device write desc. level: %d, active: %d, buffs filled: %d, buff start: %d, buff end: %d\n",
			DescLevel,
			DescActive,
			circ_buff_filled(BuffStart, BuffEnd, DMARXBUFFS),
			BuffStart,
			BuffEnd
		); 
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/

		if (DescLevel > DMARXDESCS) printk(KERN_ERR DRIVER_NAME "Read Thread ERROR: more descriptors than maximum enqueued in FPGA!\n");

#if EMPTY_LOGGING /*----v----v----v----v----v----v----v----v----v----v----v----v*/
		if (DescLevel == 0) { 
			printk(KERN_ERR DRIVER_NAME " Device write descriptor buffer empty!\n");
		};
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/

		/*Enqueue more descriptors. We want to write as many descriptors as we can. We are limited by:
			1 - Descriptor FIFO depth
			2 - Number of DMA buffers*/

		BuffsToWrite = MIN(
			DMARXDESCS - DescLevel,					/*Number of entries in the descriptor FIFO that we can potentially enqueue*/
			circ_buff_avail(BuffStart, BuffEnd, DMARXBUFFS)		/*Number of DMA buffers that we have free*/
		);

		for (; BuffsToWrite; BuffsToWrite--) {
			/*Write a descriptor*/
			DescAddr = (BuffEnd << DMAPAGEWIDTH) | (DevPrivData->RxDmaBuffsBusAddr[BuffEnd] & ~DevPrivData->BusTranslationMask);
#if VERBOSE_LOGGING_R /*----v----v----v----v----v----v----v----v----v----v----v----v*/
			printk(KERN_DEBUG DRIVER_NAME " Writing device write descriptor: %d, bus address: 0x%0llx, address on device: 0x%08x\n", BuffEnd, DevPrivData->RxDmaBuffsBusAddr[BuffEnd], DescAddr);
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/
			writel(DescAddr, DevPrivData->pBar1 + WRDESCWRADDR);
			writel(DMABUFFSIZE, DevPrivData->pBar1 + WRDESCLEN);
			wmb();
			writel(DESC_CTRL_GO, DevPrivData->pBar1 + WRDESCCTRL);/*GO = descriptor will processed ASAP*/
			wmb();
			BuffEnd = (BuffEnd + 1) % DMARXBUFFS;
			DescActive++;
			DescLevel++;

		};	

		/*Read completed DMA transfers (after a delay of a few descriptor completions)*/
		DescComp += DescActive - DescLevel;
		DescActive = DescLevel;
#if VERBOSE_LOGGING_R /*----v----v----v----v----v----v----v----v----v----v----v----v*/
		printk(KERN_DEBUG DRIVER_NAME " %d write descriptors completed\n", DescComp);
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/

		DescFlag=0;
		wmb();

		for (; DescComp >DMARXDESCDELAY; DescComp--) {
			DescFlag=1;
			/*Copy transferred data to file buffer*/
			dma_sync_single_for_cpu(
				&(DevPrivData->dev->dev),
				DevPrivData->RxDmaBuffsBusAddr[BuffStart],
				DMABUFFSIZE,
				DMA_FROM_DEVICE
			);
#if VERBOSE_LOGGING_R /*----v----v----v----v----v----v----v----v----v----v----v----v*/
			printk(KERN_DEBUG DRIVER_NAME " Device write DMA completed on buffer: %d, descriptor address: 0x%0llx\n", BuffStart, DevPrivData->RxDmaBuffsBusAddr[BuffStart]);
			printk(KERN_DEBUG DRIVER_NAME " Content: 0x%02x 0x%02x 0x%02x 0x%02x    0x%02x 0x%02x 0x%02x 0x%02x\n",
				DevPrivData->pRxDmaBuffs[BuffStart][0], DevPrivData->pRxDmaBuffs[BuffStart][1], DevPrivData->pRxDmaBuffs[BuffStart][2], DevPrivData->pRxDmaBuffs[BuffStart][3],
				DevPrivData->pRxDmaBuffs[BuffStart][4], DevPrivData->pRxDmaBuffs[BuffStart][5], DevPrivData->pRxDmaBuffs[BuffStart][6], DevPrivData->pRxDmaBuffs[BuffStart][7]
			);
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/
			/*Copy data from DMA buffer to character device buffer*/
			
#if VERBOSE_LOGGING_R /*----v----v----v----v----v----v----v----v----v----v----v----v*/
			printk(KERN_DEBUG DRIVER_NAME " Read character buffer start: %d, end: %d, filled: %d, available: %d\n", cdev_read_buff_start, cdev_read_buff_end, 
				circ_buff_filled(cdev_read_buff_start, cdev_read_buff_end, CDEVBUFFSIZE), circ_buff_avail(cdev_read_buff_start, cdev_read_buff_end, CDEVBUFFSIZE)
			);
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/

			mutex_lock(&cdev_read_buff_mutex); 
			if (circ_buff_avail(cdev_write_buff_start, cdev_write_buff_end, CDEVBUFFSIZE) > DMABUFFSIZE){
				mutex_unlock(&cdev_read_buff_mutex);/*unlock since memcpy is writing to buff_end, while cdev_read writes at buff_start: improves thrasfer rate*/
				/*Copy data before the wrap around in the circular buffer*/
				memcpy(cdev_read_buff + cdev_read_buff_end, DevPrivData->pRxDmaBuffs[BuffStart], MIN(DMABUFFSIZE, CDEVBUFFSIZE - cdev_read_buff_end));
				/*Copy data after the wrap around if there is one*/
				if ((cdev_read_buff_end + DMABUFFSIZE) > CDEVBUFFSIZE) {
					memcpy(cdev_read_buff, DevPrivData->pRxDmaBuffs[BuffStart] + CDEVBUFFSIZE - cdev_read_buff_end, DMABUFFSIZE - CDEVBUFFSIZE + cdev_read_buff_end);
				};
				mb(); 

				/*Copy transferred data to file buffer*/
				dma_sync_single_for_device(
					&(DevPrivData->dev->dev),
					DevPrivData->RxDmaBuffsBusAddr[BuffStart],
					DMABUFFSIZE,
					DMA_FROM_DEVICE
				);

				mutex_lock(&cdev_read_buff_mutex);
				cdev_read_buff_end = (cdev_read_buff_end + DMABUFFSIZE) % CDEVBUFFSIZE;
				mutex_unlock(&cdev_read_buff_mutex);
				BuffStart = (BuffStart + 1) % DMARXBUFFS;

#if VERBOSE_LOGGING_R /*----v----v----v----v----v----v----v----v----v----v----v----v*/
				printk(KERN_DEBUG DRIVER_NAME " Character device read buffer start: %d, end: %d, fill: %d, available: %d\n",
					cdev_read_buff_start,
					cdev_read_buff_end,
					circ_buff_filled(cdev_read_buff_start, cdev_read_buff_end, CDEVBUFFSIZE),
					circ_buff_avail(cdev_read_buff_start, cdev_read_buff_end, CDEVBUFFSIZE)
				);
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/
			  
			}else{/*cdev buffer is not ready yet (it's slow reading the data, we have to wait for it to be processed at cdev_read) */
				mutex_unlock(&cdev_read_buff_mutex);  
				yield();
				msleep(RX_THREAD_MSLEEP);/*5ms is around 8 DMA buffs at 400MB/s or 16 at 800MB/s */
#if EMPTY_LOGGING /*----v----v----v----v----v----v----v----v----v----v----v----v*/
				printk(KERN_ERR DRIVER_NAME " Device write buffer full. Data lost. Read data faster!\n");
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/
			};/*IF : data in cdev_read was read, and there is space?*/
			             
		};//FOR: DescComp >DMARXDESCDELAY
		
		if (!DescFlag){//nothing was coppied, meaning that cdev_read is slow. sleep (gives timeslice to cdev_read) until there is something available
			yield();
			msleep(RX_THREAD_MSLEEP);
		}
		wmb(); 

		/*Detect if parent thread is signalling for a shutdown*/
		if (kthread_should_stop()) {
/*
		if (Ctr > 4000 && kthread_should_stop()) {
*/
			printk(KERN_DEBUG DRIVER_NAME " Read thread termination sequence intiated...\n");
			printk(KERN_DEBUG DRIVER_NAME " Waiting for remaining write DMA transfers to complete...\n");
			while(DescLevel) {
				DescLevel = (readl(DevPrivData->pBar1 + WRCSRFILLLEVEL) >> 16) & 0x0000ffff;
#if VERBOSE_LOGGING_R /*----v----v----v----v----v----v----v----v----v----v----v----v*/
				printk(KERN_DEBUG DRIVER_NAME " Write descriptor level is %d...\n", DescLevel);
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/
				yield();
			};
			printk(KERN_DEBUG DRIVER_NAME " Read thread complete\n");
			return 0;
		};

		/*Give other threads a chance to do processing*/
		yield();	
	};

	printk(KERN_ERR DRIVER_NAME " You should never see this printed\n");

	return 1;
};


/**Device write thread

This thread:
- reads tx data from cdev_write_buff
- constantly performs write operations to the device via DMA
*/
int dev_write_task(void *data) {
	int		DescLevel;				/*Descriptor FIFO level read from device*/
	int		DescActive, DescComp;			/*Number of descriptors to be serviced and the number of completed descriptors*/
	int		BuffStart, BuffEnd;			/*Indices to beginning and end of DMA buffers ring array (DevPrivData->TxDmaBuffsBusAddr)*/
	int		Ctr;
	int		BuffsToWrite;
	uint32_t	DescAddr;
	int 		DescFlag;

	printk(KERN_DEBUG DRIVER_NAME " Write thread created\n");

	BuffStart = BuffEnd = 0;
	DescActive = DescComp = 0;
	for (Ctr = 0; ; Ctr++) {
		mb(); 
		
		do{/*Loop until at least one descriptor was processed*/
			if (kthread_should_stop()) {return 0;}
			msleep(1);
			/*Find how many descriptors are remaining in the FPGA write buffer: if full, then 1ms-5ms sleep, giving priority to other threads*/
			if ((readl(DevPrivData->pBar1 + RDCSRFILLLEVEL) & 0x0000ffff)>=DMATXDESCS) msleep(TX_THREAD_MSLEEP);/*1ms is around 2 DMA buffs at 500MB/s or 3 at 800MB/s */
			else break;
		} while (1);
		/*Find how many descriptors are remaining in the FPGA read buffer*/
		DescLevel = readl(DevPrivData->pBar1 + RDCSRFILLLEVEL) & 0x0000ffff;

#if VERBOSE_LOGGING_W /*----v----v----v----v----v----v----v----v----v----v----v----v*/
		printk(KERN_DEBUG DRIVER_NAME " Device read desc. level: %d, active: %d, buffs filled: %d, buff start: %d, buff end: %d\n",
			DescLevel,
			DescActive,
			circ_buff_filled(BuffStart, BuffEnd, DMARXBUFFS),
			BuffStart,
			BuffEnd
		);
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/

	if (DescLevel > DMARXDESCS) printk(KERN_ERR DRIVER_NAME "Write Thread ERROR: more descriptors than maximum enqueued in FPGA!\n");

#if EMPTY_LOGGING /*----v----v----v----v----v----v----v----v----v----v----v----v*/
		if (DescLevel == 0) {
			printk(KERN_ERR DRIVER_NAME " Device read descriptor buffer empty!\n");
		};
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/

		/*Write a few buffers for DMA transfers (after a delay of a few descriptor completions)*/
		DescComp = DMARXBUFFS - DescLevel;
		DescActive = DescLevel;

#if VERBOSE_LOGGING_W /*----v----v----v----v----v----v----v----v----v----v----v----v*/
		printk(KERN_DEBUG DRIVER_NAME " %d read descriptors completed\n", DescComp);
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/

		for (; DescComp > DMATXDESCDELAY; DescComp--) {
			/*Copy data from character device buffer to DMA buffer*/

#if VERBOSE_LOGGING_W /*----v----v----v----v----v----v----v----v----v----v----v----v*/
			printk(KERN_DEBUG DRIVER_NAME " Write character buffer start: %d, end: %d, filled: %d, available: %d\n", cdev_write_buff_start, cdev_write_buff_end, circ_buff_filled(cdev_write_buff_start, cdev_write_buff_end, CDEVBUFFSIZE), 					circ_buff_avail(cdev_write_buff_start, cdev_write_buff_end, CDEVBUFFSIZE));
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/
			
			/*Write descriptors only if data is available*/
			mutex_lock(&cdev_write_buff_mutex);
			if (circ_buff_filled(cdev_write_buff_start, cdev_write_buff_end, CDEVBUFFSIZE) > DMABUFFSIZE) {
				mutex_unlock(&cdev_write_buff_mutex);

				/*Copy transferred data to file buffer*/
				dma_sync_single_for_cpu(
					&(DevPrivData->dev->dev),
					DevPrivData->TxDmaBuffsBusAddr[BuffEnd],
					DMABUFFSIZE,
					DMA_TO_DEVICE
				);
				
				/*Copy data before the wrap around in the circular buffer*/
				memcpy(DevPrivData->pTxDmaBuffs[BuffEnd], cdev_write_buff + cdev_write_buff_start, MIN(DMABUFFSIZE, CDEVBUFFSIZE - cdev_write_buff_start));
				/*Copy data after the wrap around if there is one*/
				if ((cdev_write_buff_start + DMABUFFSIZE) > CDEVBUFFSIZE) {
					memcpy(DevPrivData->pTxDmaBuffs[BuffEnd] + CDEVBUFFSIZE - cdev_write_buff_start, cdev_write_buff, DMABUFFSIZE - CDEVBUFFSIZE + cdev_write_buff_start);
				};
				mutex_lock(&cdev_write_buff_mutex);
				cdev_write_buff_start = (cdev_write_buff_start + DMABUFFSIZE) % CDEVBUFFSIZE;
				mutex_unlock(&cdev_write_buff_mutex);
				mb();

#if VERBOSE_LOGGING_W /*----v----v----v----v----v----v----v----v----v----v----v----v*/
				printk(KERN_DEBUG DRIVER_NAME " Character device write buffer start: %d, end: %d, fill: %d, available: %d\n",
					cdev_write_buff_start,
					cdev_write_buff_end,
					circ_buff_filled(cdev_write_buff_start, cdev_write_buff_end, CDEVBUFFSIZE),
					circ_buff_avail(cdev_write_buff_start, cdev_write_buff_end, CDEVBUFFSIZE)
				);
				printk(KERN_DEBUG DRIVER_NAME " Device read DMA buffer initialized on buffer: %d, descriptor address: 0x%0llx\n", BuffEnd, DevPrivData->TxDmaBuffsBusAddr[BuffEnd]);
				printk(KERN_DEBUG DRIVER_NAME " Content: 0x%02x 0x%02x 0x%02x 0x%02x    0x%02x 0x%02x 0x%02x 0x%02x\n",
					DevPrivData->pTxDmaBuffs[BuffEnd][0], DevPrivData->pTxDmaBuffs[BuffEnd][1], DevPrivData->pTxDmaBuffs[BuffEnd][2], DevPrivData->pTxDmaBuffs[BuffEnd][3],
					DevPrivData->pTxDmaBuffs[BuffEnd][4], DevPrivData->pTxDmaBuffs[BuffEnd][5], DevPrivData->pTxDmaBuffs[BuffEnd][6], DevPrivData->pTxDmaBuffs[BuffEnd][7]
				);
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/

				/*Copy transferred data to file buffer*/
				dma_sync_single_for_device(
					&(DevPrivData->dev->dev),
					DevPrivData->TxDmaBuffsBusAddr[BuffEnd],
					DMABUFFSIZE,
					DMA_TO_DEVICE
				);
				BuffEnd = (BuffEnd + 1) % DMARXBUFFS;
			} else {
				mutex_unlock(&cdev_write_buff_mutex);
				yield();				
#if EMPTY_LOGGING /*----v----v----v----v----v----v----v----v----v----v----v----v*/
				printk(KERN_ERR DRIVER_NAME " Character device write buffer empty. Write more before read descriptors complete!\n");
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/
			
			};
			
		};
		/*Enqueue more descriptors. We want to write as many descriptors as we can. We are limited by:
			1 - Descriptor FIFO depth
			2 - Number of DMA buffers*/
		BuffsToWrite = MIN(
			DMATXDESCS - DescLevel,		/*even the ones we havent coppied data to, in above (due to desc delay)?*/							/*Number of entries in the descriptor FIFO that we can potentially enqueue*/
			circ_buff_filled(BuffStart, BuffEnd, DMATXBUFFS)		/*Number of DMA buffers that we have free*/
		);
		DescFlag=0;
		for (; BuffsToWrite; BuffsToWrite--) {
			/*Write a descriptor*/
			DescAddr = ((BuffStart + DMARXBUFFS) << DMAPAGEWIDTH) | (DevPrivData->TxDmaBuffsBusAddr[BuffStart] & ~DevPrivData->BusTranslationMask);

#if VERBOSE_LOGGING_W /*----v----v----v----v----v----v----v----v----v----v----v----v*/
			printk(KERN_DEBUG DRIVER_NAME " Writing device read descriptor: %d, bus address: 0x%0llx, address on device: 0x%08x\n", BuffStart, DevPrivData->TxDmaBuffsBusAddr[BuffStart], DescAddr);
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/

			writel(DescAddr, DevPrivData->pBar1 + RDDESCRDADDR);/*what is the write address?*/
			writel(DMABUFFSIZE, DevPrivData->pBar1 + RDDESCLEN);
			wmb();
			writel(DESC_CTRL_GO, DevPrivData->pBar1 + RDDESCCTRL);/*GO = descriptor will processed ASAP*/
			wmb();
			BuffStart = (BuffStart + 1) % DMATXBUFFS;
			DescActive++;
			DescLevel++;
		};


		/*Detect if parent thread is signalling for a shutdown*/
		if (kthread_should_stop()) {
/*
		if (Ctr > 4000 && kthread_should_stop()) {
*/
			printk(KERN_DEBUG DRIVER_NAME " Write thread termination sequence intiated...\n");
			printk(KERN_DEBUG DRIVER_NAME " Waiting for remaining read DMA transfers to complete...\n");
			while(DescLevel) {
				DescLevel = readl(DevPrivData->pBar1 + RDCSRFILLLEVEL) & 0x0000ffff;
#if VERBOSE_LOGGING_W /*----v----v----v----v----v----v----v----v----v----v----v----v*/
				printk(KERN_DEBUG DRIVER_NAME " Read desc level is %d...\n", DescLevel);
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/
				yield();
			};
			printk(KERN_DEBUG DRIVER_NAME " Write thread complete\n");
			return 0;
		};
		yield();
	};//thread for loop
	printk(KERN_ERR DRIVER_NAME " You should never see this printed\n");

	return 1;
};


/**Service user mode reads
*/
static ssize_t cdev_read(struct file *file, char __user *buf, size_t len, loff_t *ppos) {
	int		cdev_buff_fill;
#if VERBOSE_LOGGING_R /*----v----v----v----v----v----v----v----v----v----v----v----v*/
	printk(KERN_DEBUG DRIVER_NAME " User requested read of %zu bytes\n", len);
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/
/*
	printk(KERN_ERR DRIVER_NAME " Read requested on user buffer: %p, length: %zu, offset: %p\n", buf, len, ppos);
	printk(KERN_ERR DRIVER_NAME " Read character buffer: %p, start: %d, end: %d\n", cdev_read_buff, cdev_read_buff_start, cdev_read_buff_end);
*/
	if (len > (CDEVBUFFSIZE - 1)) {
		printk(KERN_ERR DRIVER_NAME " User requested read of %zu bytes is larger than character device buffer of %lu bytes\n", len, CDEVBUFFSIZE - 1);
	};
   	do {	
		mutex_lock(&cdev_read_buff_mutex);
		cdev_buff_fill = circ_buff_filled(cdev_read_buff_start, cdev_read_buff_end, CDEVBUFFSIZE);
		mutex_unlock(&cdev_read_buff_mutex);
		yield();	
		if (cdev_buff_fill < len) msleep(RX_CDEV_MSLEEP);
   	} while (cdev_buff_fill < len);


#if VERBOSE_LOGGING_R /*----v----v----v----v----v----v----v----v----v----v----v----v*/
	printk(KERN_DEBUG DRIVER_NAME " Read buffer prior to data copy start: %d, end: %d\n", cdev_read_buff_start, cdev_read_buff_end);
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/

	/*Copy everything before the wrap around*/
	copy_to_user(buf, cdev_read_buff + cdev_read_buff_start, MIN(len, (int)CDEVBUFFSIZE - cdev_read_buff_start));
	/*Copy everything after the wrap around if there is one*/
	if ((cdev_read_buff_start + len) > CDEVBUFFSIZE) {
		copy_to_user(buf + CDEVBUFFSIZE - cdev_read_buff_start, cdev_read_buff, (int)len - (int)CDEVBUFFSIZE + cdev_read_buff_start);
	};
	mutex_lock(&cdev_read_buff_mutex);
	cdev_read_buff_start = (cdev_read_buff_start + len) % CDEVBUFFSIZE;
	mb();
	mutex_unlock(&cdev_read_buff_mutex);

#if VERBOSE_LOGGING_R /*----v----v----v----v----v----v----v----v----v----v----v----v*/
	printk(KERN_DEBUG DRIVER_NAME " User read request completed\n");
#endif /*----^----^----^----^----^----^----^----^----^----^----^----^----^----^----^*/
  
	return len;
};


/**Service user mode writes
*/
static ssize_t cdev_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos) {
	int		cdev_buff_avail;
//u64 startj;
#if VERBOSE_LOGGING_W
	printk(KERN_DEBUG DRIVER_NAME " User requested write of %zu bytes\n", len);
#endif
/*
	printk(KERN_ERR DRIVER_NAME " Write requested on user buffer: %p, length: %zu, offset: %p\n", buf, len, ppos);
	printk(KERN_ERR DRIVER_NAME " Write character buffer: %p, start: %d, end: %d\n", cdev_write_buff, cdev_write_buff_start, cdev_write_buff_end);
*/
	if (len > (CDEVBUFFSIZE - 1)) {
		printk(KERN_ERR DRIVER_NAME " User requested write of %zu bytes is larger than character device buffer of %lu bytes\n", len, CDEVBUFFSIZE - 1);
	};
	do {
		mutex_lock(&cdev_write_buff_mutex);
		cdev_buff_avail = circ_buff_avail(cdev_write_buff_start, cdev_write_buff_end, CDEVBUFFSIZE);
		mutex_unlock(&cdev_write_buff_mutex);
		yield();
		if (cdev_buff_avail<len) msleep(TX_CDEV_MSLEEP);
    	} while ((cdev_buff_avail < len));

#if VERBOSE_LOGGING_W
	printk(KERN_DEBUG DRIVER_NAME " Write buffer prior to data copy start: %d, end: %d\n", cdev_write_buff_start, cdev_write_buff_end);
#endif
	/*Copy everything before the wrap around*/
	copy_from_user(cdev_write_buff + cdev_write_buff_end, buf, MIN(len, (int) CDEVBUFFSIZE - cdev_write_buff_end));
	/*Copy everything after the wrap around if there is one*/
	if ((cdev_write_buff_end + len) > CDEVBUFFSIZE) {
		copy_from_user(cdev_write_buff, buf + CDEVBUFFSIZE - cdev_write_buff_end, (int)len - (int)CDEVBUFFSIZE + cdev_write_buff_end);
	};
	mutex_lock(&cdev_write_buff_mutex);
	cdev_write_buff_end = (cdev_write_buff_end + len) % CDEVBUFFSIZE;
	mb();
	mutex_unlock(&cdev_write_buff_mutex);

#if VERBOSE_LOGGING_W
	printk(KERN_DEBUG DRIVER_NAME " User write request completed\n");
#endif
	return len;
};


/**Helper function for ioctrl (called ioctl handler and driver)
*/
int ioctl_helper(int cmd, int *parg) {
	int		AddrOffset;
	int		BitOffset;

	/*Bit offset list*/
	switch (cmd) {
		case IOCTL_GET_N21at0:		case IOCTL_SET_N21at0:		AddrOffset = PIO0;	BitOffset = 0;	break;
		case IOCTL_GET_N22at0:		case IOCTL_SET_N22at0:		AddrOffset = PIO0;	BitOffset = 8;	break;
		case IOCTL_GET_N3GAIN:		case IOCTL_SET_N3GAIN:		AddrOffset = PIO0;	BitOffset = 16;	break;
		case IOCTL_GET_N61CE:		case IOCTL_SET_N61CE:		AddrOffset = PIO1;	BitOffset = 0;	break;
		case IOCTL_GET_N61DATA:		case IOCTL_SET_N61DATA:		AddrOffset = PIO1;	BitOffset = 1;	break;
		case IOCTL_GET_N61CLK:		case IOCTL_SET_N61CLK:		AddrOffset = PIO1;	BitOffset = 2;	break;
		case IOCTL_GET_N61LE:		case IOCTL_SET_N61LE:		AddrOffset = PIO1;	BitOffset = 3;	break;
		case IOCTL_GET_N62CE:		case IOCTL_SET_N62CE:		AddrOffset = PIO1;	BitOffset = 4;	break;
		case IOCTL_GET_N62DATA:		case IOCTL_SET_N62DATA:		AddrOffset = PIO1;	BitOffset = 5;	break;
		case IOCTL_GET_N62CLK:		case IOCTL_SET_N62CLK:		AddrOffset = PIO1;	BitOffset = 6;	break;
		case IOCTL_GET_N62LE:		case IOCTL_SET_N62LE:		AddrOffset = PIO1;	BitOffset = 7;	break;
		case IOCTL_GET_N3ENB:		case IOCTL_SET_N3ENB:		AddrOffset = PIO1;	BitOffset = 8;	break;
		case IOCTL_GET_N3HILO:		case IOCTL_SET_N3HILO:		AddrOffset = PIO1;	BitOffset = 9;	break;
		case IOCTL_GET_NASRxA:		case IOCTL_SET_NASRxA:		AddrOffset = PIO1;	BitOffset = 10;	break;
		case IOCTL_GET_NASTxA:		case IOCTL_SET_NASTxA:		AddrOffset = PIO1;	BitOffset = 11;	break;
		case IOCTL_GET_N7CLK:		case IOCTL_SET_N7CLK:		AddrOffset = PIO1;	BitOffset = 12;	break;
		case IOCTL_GET_N7CS:		case IOCTL_SET_N7CS:		AddrOffset = PIO1;	BitOffset = 13;	break;
		case IOCTL_GET_N7SDI:		case IOCTL_SET_N7SDI:		AddrOffset = PIO1;	BitOffset = 14;	break;
		case IOCTL_GET_DACIQSel:	case IOCTL_SET_DACIQSel:	AddrOffset = PIO1;	BitOffset = 15;	break;
		case IOCTL_GET_DACReset:	case IOCTL_SET_DACReset:	AddrOffset = PIO1;	BitOffset = 16;	break;
		case IOCTL_GET_DACCSB:		case IOCTL_SET_DACCSB:		AddrOffset = PIO1;	BitOffset = 17;	break;
		case IOCTL_GET_DACSDIO:		case IOCTL_SET_DACSDIO:		AddrOffset = PIO1;	BitOffset = 18;	break;
		case IOCTL_GET_DACSClk:		case IOCTL_SET_DACSClk:		AddrOffset = PIO1;	BitOffset = 19;	break;
		case IOCTL_GET_ADCAnOE:		case IOCTL_SET_ADCAnOE:		AddrOffset = PIO1;	BitOffset = 20;	break;
		case IOCTL_GET_ADCAPD:		case IOCTL_SET_ADCAPD:		AddrOffset = PIO1;	BitOffset = 21;	break;
		case IOCTL_GET_ADCASClk:	case IOCTL_SET_ADCASClk:	AddrOffset = PIO1;	BitOffset = 22;	break;
		case IOCTL_GET_ADCASDIO:	case IOCTL_SET_ADCASDIO:	AddrOffset = PIO1;	BitOffset = 23;	break;
		case IOCTL_GET_ADCAnCS:		case IOCTL_SET_ADCAnCS:		AddrOffset = PIO1;	BitOffset = 24;	break;
		case IOCTL_GET_ADCBnOE:		case IOCTL_SET_ADCBnOE:		AddrOffset = PIO1;	BitOffset = 25;	break;
		case IOCTL_GET_ADCBPD:		case IOCTL_SET_ADCBPD:		AddrOffset = PIO1;	BitOffset = 26;	break;
		case IOCTL_GET_ADCBSClk:	case IOCTL_SET_ADCBSClk:	AddrOffset = PIO1;	BitOffset = 27;	break;
		case IOCTL_GET_ADCBSDIO:	case IOCTL_SET_ADCBSDIO:	AddrOffset = PIO1;	BitOffset = 28;	break;
		case IOCTL_GET_ADCBnCS:		case IOCTL_SET_ADCBnCS:		AddrOffset = PIO1;	BitOffset = 29;	break;
		case IOCTL_GET_DACSDO:		case IOCTL_SET_DACSDO:		AddrOffset = PIO1;	BitOffset = 30;	break;
		case IOCTL_GET_N61MUXOUT:								AddrOffset = PIO2;	BitOffset = 0;	break;
		case IOCTL_GET_N61LD:									AddrOffset = PIO2;	BitOffset = 1;	break;
		case IOCTL_GET_N62MUXOUT:								AddrOffset = PIO2;	BitOffset = 2;	break;
		case IOCTL_GET_N62LD:									AddrOffset = PIO2;	BitOffset = 3;	break;
		case IOCTL_GET_N7SDO:									AddrOffset = PIO2;	BitOffset = 4;	break;
		case IOCTL_GET_GPIOin:									AddrOffset = PIO2;	BitOffset = 5;	break;
		case IOCTL_GET_RXPhase:		case IOCTL_SET_RXPhase:		AddrOffset = PIO3;	BitOffset = 0;	break;
		case IOCTL_GET_TXPhase:		case IOCTL_SET_TXPhase:		AddrOffset = PIO4;	BitOffset = 0;	break;
		case IOCTL_GET_RXDecEn:		case IOCTL_SET_RXDecEn:		AddrOffset = PIO5;	BitOffset = 0;	break;
		case IOCTL_GET_TXIntEn:		case IOCTL_SET_TXIntEn:		AddrOffset = PIO5;	BitOffset = 8;	break;
		case IOCTL_GET_RXRevFreq:	case IOCTL_SET_RXRevFreq:	AddrOffset = PIO5;	BitOffset = 16;	break;
		case IOCTL_GET_TXRevFreq:	case IOCTL_SET_TXRevFreq:	AddrOffset = PIO5;	BitOffset = 17;	break;
		case IOCTL_GET_RxDspEn:		case IOCTL_SET_RxDspEn:		AddrOffset = PIO5;	BitOffset = 18;	break;
		case IOCTL_GET_TxDspEn:		case IOCTL_SET_TxDspEn:		AddrOffset = PIO5;	BitOffset = 19;	break;
		case IOCTL_GET_NRXTALSEL:	case IOCTL_SET_NRXTALSEL:	AddrOffset = PIO5;	BitOffset = 20;	break;
		case IOCTL_GET_NRVCOSEL:	case IOCTL_SET_NRVCOSEL:	AddrOffset = PIO5;	BitOffset = 21;	break;
		case IOCTL_GET_RxFifoClr:	case IOCTL_SET_RxFifoClr:	AddrOffset = PIO5;	BitOffset = 22;	break;
		case IOCTL_GET_TxFifoClr:	case IOCTL_SET_TxFifoClr:	AddrOffset = PIO5;	BitOffset = 23;	break;
		case IOCTL_GET_RxIsSigned:	case IOCTL_SET_RxIsSigned:	AddrOffset = PIO5;	BitOffset = 24;	break;
		case IOCTL_GET_TxIsSigned:	case IOCTL_SET_TxIsSigned:	AddrOffset = PIO5;	BitOffset = 25;	break;
		case IOCTL_GET_GPIOout:		case IOCTL_SET_GPIOout:		AddrOffset = PIO5;	BitOffset = 26;	break;
		case IOCTL_GET_RxBaseband:      case IOCTL_SET_RxBaseband:      AddrOffset = PIO5;      BitOffset = 30; break;
                case IOCTL_GET_RxErrorCorrGainQ:  case IOCTL_SET_RxErrorCorrGainQ: AddrOffset = PIO6;	BitOffset = 0;	break;
                case IOCTL_GET_RxErrorCorrGainI:    case IOCTL_SET_RxErrorCorrGainI:  AddrOffset = PIO6;	BitOffset = 4;	break;
                case IOCTL_GET_RxErrorCorrGroupDelayQ:    case IOCTL_SET_RxErrorCorrGroupDelayQ:  AddrOffset = PIO6;	BitOffset = 8;	break;
                case IOCTL_GET_RxErrorCorrGroupDelayI:      case IOCTL_SET_RxErrorCorrGroupDelayI:   AddrOffset = PIO6;	BitOffset = 12;	break;
                case IOCTL_GET_RxErrorCorrDCOffsetQ:         case IOCTL_SET_RxErrorCorrDCOffsetQ:      AddrOffset = PIO6;	BitOffset = 16;	break;
                case IOCTL_GET_RxErrorCorrDCOffsetI:          case IOCTL_SET_RxErrorCorrDCOffsetI:        AddrOffset = PIO6;	BitOffset = 24;	break;
                case IOCTL_GET_TxErrorCorrGainQ:  case IOCTL_SET_TxErrorCorrGainQ: AddrOffset = PIO7;	BitOffset = 0;	break;
                case IOCTL_GET_TxErrorCorrGainI:   case IOCTL_SET_TxErrorCorrGainI:   AddrOffset = PIO7;	BitOffset = 4;	break;
                case IOCTL_GET_TxErrorCorrGroupDelayQ:    case IOCTL_SET_TxErrorCorrGroupDelayQ:  AddrOffset = PIO7;	BitOffset = 8;	break;
                case IOCTL_GET_TxErrorCorrGroupDelayI:      case IOCTL_SET_TxErrorCorrGroupDelayI:   AddrOffset = PIO7;	BitOffset = 12;	break;
                case IOCTL_GET_TxErrorCorrDCOffsetQ:         case IOCTL_SET_TxErrorCorrDCOffsetQ:      AddrOffset = PIO7;	BitOffset = 16;	break;
                case IOCTL_GET_TxErrorCorrDCOffsetI:          case IOCTL_SET_TxErrorCorrDCOffsetI:        AddrOffset = PIO7;	BitOffset = 24;	break;
                
		default:
			printk(KERN_ERR DRIVER_NAME " Invalid command: %d\n", cmd);
			return -EINVAL;
			break;
	};
	mb();

	/*Read and write lists*/
	switch (cmd) {
		/*1 bit read*/
		case IOCTL_GET_N61CE:		case IOCTL_GET_N61DATA:		case IOCTL_GET_N61CLK:		case IOCTL_GET_N61LE:
		case IOCTL_GET_N62CE:		case IOCTL_GET_N62DATA:		case IOCTL_GET_N62CLK:		case IOCTL_GET_N62LE:
		case IOCTL_GET_N3ENB:		case IOCTL_GET_N3HILO:		case IOCTL_GET_NASRxA:		case IOCTL_GET_NASTxA:
		case IOCTL_GET_N7CLK:		case IOCTL_GET_N7CS:		case IOCTL_GET_N7SDI:		case IOCTL_GET_N7SDO:
		case IOCTL_GET_N61MUXOUT:	case IOCTL_GET_N61LD:		case IOCTL_GET_N62MUXOUT:	case IOCTL_GET_N62LD:
		case IOCTL_GET_DACIQSel:	case IOCTL_GET_DACReset:	case IOCTL_GET_DACCSB:		case IOCTL_GET_DACSDIO:		case IOCTL_GET_DACSClk:		case IOCTL_GET_DACSDO:
		case IOCTL_GET_ADCAnOE:		case IOCTL_GET_ADCAPD:		case IOCTL_GET_ADCASClk:	case IOCTL_GET_ADCASDIO:	case IOCTL_GET_ADCAnCS:
		case IOCTL_GET_ADCBnOE:		case IOCTL_GET_ADCBPD:		case IOCTL_GET_ADCBSClk:	case IOCTL_GET_ADCBSDIO:	case IOCTL_GET_ADCBnCS:
		case IOCTL_GET_RXRevFreq:	case IOCTL_GET_TXRevFreq:
		case IOCTL_GET_RxDspEn:		case IOCTL_GET_TxDspEn:
		case IOCTL_GET_NRXTALSEL:	case IOCTL_GET_NRVCOSEL:
		case IOCTL_GET_RxFifoClr:	case IOCTL_GET_TxFifoClr:	case IOCTL_GET_RxIsSigned: 	case IOCTL_GET_TxIsSigned: case IOCTL_GET_RxBaseband:
			*parg = (readl(DevPrivData->pBar0 + AddrOffset) >> BitOffset) & 0x00000001;
			break;
		/*1 bit write*/
		case IOCTL_SET_N61CE:		case IOCTL_SET_N61DATA:		case IOCTL_SET_N61CLK:		case IOCTL_SET_N61LE:
		case IOCTL_SET_N62CE:		case IOCTL_SET_N62DATA:		case IOCTL_SET_N62CLK:		case IOCTL_SET_N62LE:
		case IOCTL_SET_N3ENB:		case IOCTL_SET_N3HILO:		case IOCTL_SET_NASRxA:		case IOCTL_SET_NASTxA:
		case IOCTL_SET_N7CLK:		case IOCTL_SET_N7CS:		case IOCTL_SET_N7SDI:
		case IOCTL_SET_DACIQSel:	case IOCTL_SET_DACReset:	case IOCTL_SET_DACCSB:		case IOCTL_SET_DACSDIO:		case IOCTL_SET_DACSClk:		case IOCTL_SET_DACSDO:
		case IOCTL_SET_ADCAnOE:		case IOCTL_SET_ADCAPD:		case IOCTL_SET_ADCASClk:	case IOCTL_SET_ADCASDIO:	case IOCTL_SET_ADCAnCS:
		case IOCTL_SET_ADCBnOE:		case IOCTL_SET_ADCBPD:		case IOCTL_SET_ADCBSClk:	case IOCTL_SET_ADCBSDIO:	case IOCTL_SET_ADCBnCS:
		case IOCTL_SET_RXRevFreq:	case IOCTL_SET_TXRevFreq:
		case IOCTL_SET_RxDspEn:		case IOCTL_SET_TxDspEn:
		case IOCTL_SET_NRXTALSEL:	case IOCTL_SET_NRVCOSEL:
		case IOCTL_SET_RxFifoClr:	case IOCTL_SET_TxFifoClr:	case IOCTL_SET_RxIsSigned:	case IOCTL_SET_TxIsSigned:	case IOCTL_SET_RxBaseband:
			if (*parg > 1) {
				printk(KERN_ERR DRIVER_NAME " Invalid argument: %d\n", *parg);
				return -EINVAL;
			};
			writel((readl(DevPrivData->pBar0 + AddrOffset) & ~(0x00000001 << BitOffset)) | (*parg << BitOffset), DevPrivData->pBar0 + AddrOffset);
			break;
                /*4 bit read*/
                case IOCTL_GET_RxErrorCorrGainQ:               case IOCTL_GET_RxErrorCorrGainI:
                case IOCTL_GET_RxErrorCorrGroupDelayQ:    case IOCTL_GET_RxErrorCorrGroupDelayI:
                case IOCTL_GET_TxErrorCorrGainQ:               case IOCTL_GET_TxErrorCorrGainI:
                case IOCTL_GET_TxErrorCorrGroupDelayQ:    case IOCTL_GET_TxErrorCorrGroupDelayI: 
			*parg = (readl(DevPrivData->pBar0 + AddrOffset) >> BitOffset) & 0x0000000f;
			break;
		/*4 bit write*/
                case IOCTL_SET_RxErrorCorrGainQ:               case IOCTL_SET_RxErrorCorrGainI:
                case IOCTL_SET_RxErrorCorrGroupDelayQ:    case IOCTL_SET_RxErrorCorrGroupDelayI:
                case IOCTL_SET_TxErrorCorrGainQ:               case IOCTL_SET_TxErrorCorrGainI:
                case IOCTL_SET_TxErrorCorrGroupDelayQ:    case IOCTL_SET_TxErrorCorrGroupDelayI: 
			if (*parg > 15) {
				printk(KERN_ERR DRIVER_NAME " Invalid argument: %d\n", *parg);
				return -EINVAL;
			};
			writel((readl(DevPrivData->pBar0 + AddrOffset) & ~(0x0000000f << BitOffset)) | (*parg << BitOffset), DevPrivData->pBar0 + AddrOffset);
			break;
                        
		/*5 bit read*/
		case IOCTL_GET_GPIOin:		case IOCTL_GET_GPIOout:
			*parg = (readl(DevPrivData->pBar0 + AddrOffset) >> BitOffset) & 0x0000001f;
			break;
		/*5 bit write*/
		case IOCTL_SET_GPIOout:
			if (*parg > 31) {
				printk(KERN_ERR DRIVER_NAME " Invalid argument: %d\n", *parg);
				return -EINVAL;
			};
			writel((readl(DevPrivData->pBar0 + AddrOffset) & ~(0x0000001f << BitOffset)) | (*parg << BitOffset), DevPrivData->pBar0 + AddrOffset);
			break;
		/*8 bit read*/
		case IOCTL_GET_N21at0:		case IOCTL_GET_N22at0:		case IOCTL_GET_N3GAIN:
                case IOCTL_GET_RXDecEn:		case IOCTL_GET_TXIntEn: 
                case IOCTL_GET_RxErrorCorrDCOffsetQ:   case IOCTL_GET_RxErrorCorrDCOffsetI:           
                case IOCTL_GET_TxErrorCorrDCOffsetQ:   case IOCTL_GET_TxErrorCorrDCOffsetI:
			*parg = (readl(DevPrivData->pBar0 + AddrOffset) >> BitOffset) & 0x000000ff;
			break;
		/*8 bit write*/
		case IOCTL_SET_N21at0:		case IOCTL_SET_N22at0:		case IOCTL_SET_N3GAIN:
		case IOCTL_SET_RXDecEn:		case IOCTL_SET_TXIntEn: 
                case IOCTL_SET_RxErrorCorrDCOffsetQ:   case IOCTL_SET_RxErrorCorrDCOffsetI:
                case IOCTL_SET_TxErrorCorrDCOffsetQ:   case IOCTL_SET_TxErrorCorrDCOffsetI:
			if (*parg > 255) {
				printk(KERN_ERR DRIVER_NAME " Invalid argument: %d\n", *parg);
				return -EINVAL;
			};
			writel((readl(DevPrivData->pBar0 + AddrOffset) & ~(0x000000ff << BitOffset)) | (*parg << BitOffset), DevPrivData->pBar0 + AddrOffset);
			break;
		/*32 bit read*/
		case IOCTL_GET_RXPhase:		case IOCTL_GET_TXPhase:
			*parg = (readl(DevPrivData->pBar0 + AddrOffset) >> BitOffset) & 0xffffffff;
			break;
		/*32 bit write*/
		case IOCTL_SET_RXPhase:		case IOCTL_SET_TXPhase:
			writel((readl(DevPrivData->pBar0 + AddrOffset) & ~(0xffffffff << BitOffset)) | (*parg << BitOffset), DevPrivData->pBar0 + AddrOffset);
			break;
	};
	mb();

	return 0;
};


/**Service user ioctl calls
*/
/*To add a new ioctl call, add the entry to the bit offset list and read/write list*/
static long cdev_ioctl(struct file *file, unsigned int cmd, unsigned long parg) {
	int32_t	tmp;
	int		rc;

	printk(KERN_DEBUG DRIVER_NAME " ioctl called with cmd = %d, parg = 0x%016lx\n", cmd, parg);

	if (copy_from_user(&tmp, (void *)parg, 4)) {
		printk(KERN_ERR DRIVER_NAME " Cannot copy from program memory\n");
		rc = -ENOMEM;
		goto exit_func;
	};

	printk(KERN_DEBUG DRIVER_NAME " ioctl being served with arg = %d\n", tmp);
	rc = ioctl_helper(cmd, &tmp);

	if (copy_to_user((void *)parg, &tmp, 4)) {
		printk(KERN_ERR DRIVER_NAME " Cannot copy to program memory\n");
		rc = -ENOMEM;
		goto exit_func;
	};

	return 0;

/*rc should be set to a meaningful value which is appropriate for the type of failure prior to a jump to any of the following labels*/
exit_func:
	printk(KERN_ERR DRIVER_NAME " ioctl call failed: %d\n", rc);
	return rc;
};


/**Create buffers required for DMA
*/
static int create_dma_buffs(uint8_t ***dma_buffs, dma_addr_t **dma_buffs_bus_addr, int num_buffs, int buff_size, enum dma_data_direction dir) {
	int		i;
	int		rc = 0;

	if (!(*dma_buffs = (uint8_t**)kmalloc(sizeof(uint8_t*) * num_buffs, GFP_KERNEL))) {
		rc = -ENOMEM;
		goto exit_func;
	};
	for (i = 0; i < num_buffs; i++) {
		if (!((*dma_buffs)[i] = (uint8_t*)kmalloc(buff_size, GFP_KERNEL))) {
			rc = -ENOMEM;
			goto exit_kfree;
		};
	};

	if (!(*dma_buffs_bus_addr = (dma_addr_t*)kmalloc(sizeof(dma_addr_t) * num_buffs, GFP_KERNEL))) {
		rc = -ENOMEM;
		goto exit_kfree;
	};
	for (i = 0; i < num_buffs; i++) {
		if (!((*dma_buffs_bus_addr)[i] = dma_map_single(&(DevPrivData->dev->dev), (*dma_buffs)[i], buff_size, dir))) {
			rc = -ENOMEM;
			goto exit_dma_unmap;
		};
		printk(KERN_DEBUG DRIVER_NAME " Created buffer %d memory address: %p = bus address: 0x%0llx\n", i + 1, (*dma_buffs)[i], (long long) (*dma_buffs_bus_addr)[i]);
	};

	return 0;

/*rc should be set to a meaningful value which is appropriate for the type of failure prior to a jump to any of the following labels*/
exit_dma_unmap:
	for (; i >= 0; i--) {
		dma_unmap_single(&(DevPrivData->dev->dev), (*dma_buffs_bus_addr)[i], buff_size, dir);
	};
	i = 15;
	kfree(*dma_buffs_bus_addr);
exit_kfree:
	for (; i >= 0; i--) {
		kfree((*dma_buffs)[i]);
	};
	kfree(*dma_buffs);
exit_func:
	return rc;
};


/**Destroy DMA buffers created by create_dma_buffs
*/
static void destroy_dma_buffs(uint8_t ***dma_buffs, dma_addr_t **dma_buffs_bus_addr, int num_buffs, int buff_size, enum dma_data_direction dir) {
	int		i;

	for (i = 0; i < num_buffs; i++) {
/*
		printk(KERN_DEBUG DRIVER_NAME " Destroying buffer %d memory address: %p = bus address: 0x%0llx\n", i + 1, (*dma_buffs)[i], (long long) (*dma_buffs_bus_addr)[i]);
*/
		dma_unmap_single(&(DevPrivData->dev->dev), (*dma_buffs_bus_addr)[i], buff_size, dir);
	};
	kfree(*dma_buffs_bus_addr);
	for (i = 0; i < num_buffs; i++) {
		kfree((*dma_buffs)[i]);
	};
	kfree(*dma_buffs);
};


/**Enables or disables ADC and DAC

State = 1 - Power on
State = 0 - Power off
*/
void PowerPeriph(int State) {
	int		nState = !State;
	int		Val1 = 1;
	State = !nState;

	printk(KERN_DEBUG DRIVER_NAME " State: %d nState: %d\n", State, nState);

	/*All comments presented for power on scenario, invert logic for power off*/

	/*ADCs*/
	/*Disable power down (0)*/
	ioctl_helper(IOCTL_SET_ADCAPD, &nState);
	ioctl_helper(IOCTL_SET_ADCBPD, &nState);

	/*DACs*/
	/*Enable pin mode always*/
	ioctl_helper(IOCTL_SET_DACReset, &Val1);
	/*Data on both ports (0)*/
	ioctl_helper(IOCTL_SET_DACSClk, &nState);
	/*Data format is signed binary (0)*/
	ioctl_helper(IOCTL_SET_DACSDIO, &nState);
	/*Mixed mode off (0)*/
	ioctl_helper(IOCTL_SET_DACCSB, &nState);
	/*Power down off - all systems running (0)*/
	ioctl_helper(IOCTL_SET_DACSDO, &nState);

	/*Enable DSP chains (1)*/
	ioctl_helper(IOCTL_SET_RxDspEn, &State);
	ioctl_helper(IOCTL_SET_TxDspEn, &State);

	/*Set sane values for signed and unsigned inputs. */
	ioctl_helper(IOCTL_SET_RxIsSigned, &nState);
	ioctl_helper(IOCTL_SET_TxIsSigned, &nState);
};


/**Handle user device open calls
*/
static int cdev_open(struct inode *inode, struct file *file) {
	int		m;
	int		i;
	int		rc;


	mutex_lock(&cdev_open_close_mutex);
	printk(KERN_DEBUG DRIVER_NAME " File being opened...\n");
	/*We can support only one device for now*/
	printk(KERN_DEBUG DRIVER_NAME " Checking device minor number...\n");
	m = iminor(inode);
	if (m < 0) {
		printk(KERN_ERR DRIVER_NAME " Bad device minor number: %d\n", m);
		rc = -ENODEV;
		goto exit_func; 
	};
	printk(KERN_DEBUG DRIVER_NAME " File being opened with minor number: %d\n", m);

	if (DevPrivData->OpenCtr) {
		/*File is open elsewhere, do not attempt to reinitialize*/
		printk(KERN_DEBUG DRIVER_NAME " File already open, not redoing initialization routine...\n");
	} else {
		/*Initialize device DMA buffers*/
		printk(KERN_DEBUG DRIVER_NAME " Powering up device\n");
		printk(KERN_DEBUG DRIVER_NAME " Allocating DMA buffers\n");
		if ((rc = create_dma_buffs(&DevPrivData->pRxDmaBuffs, &DevPrivData->RxDmaBuffsBusAddr, DMARXBUFFS, DMABUFFSIZE, DMA_FROM_DEVICE))) {
			printk(KERN_ERR DRIVER_NAME " No memory for rx DMA buffers\n");
			goto exit_func;
		};
		if ((rc = create_dma_buffs(&DevPrivData->pTxDmaBuffs, &DevPrivData->TxDmaBuffsBusAddr, DMATXBUFFS, DMABUFFSIZE, DMA_TO_DEVICE))) {
			printk(KERN_ERR DRIVER_NAME " No memory for tx DMA buffers\n");
			goto exit_destroy_dma_buffs_rx;
		};
		/*Calculate address translation table for Avalon-MM to PCIe translation and write it to device*/
		printk(KERN_DEBUG DRIVER_NAME " Writing address translation table...\n");
		for (i = 0; i < DMARXBUFFS; i++) {
			writel(DevPrivData->RxDmaBuffsBusAddr[i] & DevPrivData->BusTranslationMask, DevPrivData->pBar0 + CRA_ATT + i * 8);
			writel(0, DevPrivData->pBar0 + CRA_ATT + 4 + i * 8);
			mb();
			printk(KERN_DEBUG DRIVER_NAME " Address translation table entry %d: 0x%08x\n", i, readl(DevPrivData->pBar0 + CRA_ATT + i * 8));
		};
		for (i = 0; i < DMATXBUFFS; i++) {
			writel(DevPrivData->TxDmaBuffsBusAddr[i] & DevPrivData->BusTranslationMask, DevPrivData->pBar0 + CRA_ATT + (i + DMARXBUFFS) * 8);
			writel(0, DevPrivData->pBar0 + CRA_ATT + 4 + (i + DMARXBUFFS) * 8);
			mb();
			printk(KERN_DEBUG DRIVER_NAME " Address translation table entry %d: 0x%08x\n", i, readl(DevPrivData->pBar0 + CRA_ATT + (i + DMARXBUFFS) * 8));
		};
		printk(KERN_DEBUG DRIVER_NAME " Address translation table written");
		/*Initialize character device buffers*/
		printk(KERN_DEBUG DRIVER_NAME " Allocating character device buffers...\n");
		if (!(cdev_read_buff = vmalloc(CDEVBUFFSIZE))) {
			printk(KERN_ERR DRIVER_NAME " Cannot create character device read buffer, not enough memory?\n");
			rc = -ENOMEM;
			goto exit_destroy_dma_buffs_tx;
		};
		cdev_read_buff_start = cdev_read_buff_end = 0;
		if (!(cdev_write_buff = vmalloc(CDEVBUFFSIZE))) {
			printk(KERN_ERR DRIVER_NAME " Cannot create character device write buffer, not enough memory?\n");
			rc = -ENOMEM;
			goto exit_vfree_cdev_read_buff;
		};
		cdev_write_buff_start = cdev_write_buff_end = 0;
		printk(KERN_DEBUG DRIVER_NAME " Read buffer: %p, write buffer: %p\n", cdev_read_buff, cdev_write_buff);

		wmb();

		/*Power on ADCs and DACs before spawning threads*/
		printk(KERN_DEBUG DRIVER_NAME " Turning on ADCs and DACs\n");
		PowerPeriph(1);

		/*Spawn DMA transfer threads*/
		printk(KERN_DEBUG DRIVER_NAME " Spawning read and write threads\n");
#if ENABLEREADTHREAD
		dev_read_thread = kthread_run(dev_read_task, 0, DRIVER_NAME);
		if (IS_ERR(dev_read_thread)) {
			printk(KERN_ERR DRIVER_NAME " Unable to start read thread: 0x%016llx\n", (uint64_t) dev_read_thread);
			rc = -ECHILD;
			goto exit_power_down;
		};
#endif
#if ENABLEWRITETHREAD
		dev_write_thread = kthread_run(dev_write_task, 0, DRIVER_NAME);
		if (IS_ERR(dev_write_thread)) {
			printk(KERN_ERR DRIVER_NAME " Unable to start write thread: 0x%016llx\n", (uint64_t) dev_write_thread);
			rc = -ECHILD;
			goto exit_stop_read_thread;
		};
		printk(KERN_DEBUG DRIVER_NAME " Device powered up\n");
#endif
	};

	DevPrivData->OpenCtr++;
	printk(KERN_DEBUG DRIVER_NAME " Device has %d handles open\n", DevPrivData->OpenCtr);
	printk(KERN_DEBUG DRIVER_NAME " File opened successfully\n");
	mutex_unlock(&cdev_open_close_mutex);
	return nonseekable_open(inode, file);

/*rc should be set to a meaningful value which is appropriate for the type of failure prior to a jump to any of the following labels*/
exit_stop_read_thread:
#if ENABLEREADTHREAD
	kthread_stop(dev_read_thread);
#endif
exit_power_down:
	PowerPeriph(0);
	vfree(cdev_write_buff);
exit_vfree_cdev_read_buff:
	vfree(cdev_read_buff);
exit_destroy_dma_buffs_tx:
	destroy_dma_buffs(&DevPrivData->pTxDmaBuffs, &DevPrivData->RxDmaBuffsBusAddr, DMATXBUFFS, DMABUFFSIZE, DMA_TO_DEVICE);
exit_destroy_dma_buffs_rx:
	destroy_dma_buffs(&DevPrivData->pRxDmaBuffs, &DevPrivData->RxDmaBuffsBusAddr, DMARXBUFFS, DMABUFFSIZE, DMA_FROM_DEVICE);
exit_func:
	printk(KERN_ERR DRIVER_NAME " File open failed: %d\n", rc);
	return rc;
};


/**Handle user device open calls
*/
static int cdev_release(struct inode *inode, struct file *file) {
	int		m;
	int		rc;

	mutex_lock(&cdev_open_close_mutex);
	//module_put(THIS_MODULE);

	printk(KERN_DEBUG DRIVER_NAME " File being closed...\n");
	m = iminor(inode);
	if (m < 0) {
		printk(KERN_ERR DRIVER_NAME " Bad device minor number: %d...\n", m);
		rc = -ENODEV;
		goto exit_func;
	};
	printk(KERN_DEBUG DRIVER_NAME " File being closed with minor number: %d\n", m);

	if (DevPrivData->OpenCtr > 1) {
		/*File is open elsewhere, do not attempt to shutdown threads*/
		printk(KERN_DEBUG DRIVER_NAME " File still open by other processes, not shutting down device...\n");
	} else {
		/*Shut down device*/
		printk(KERN_DEBUG DRIVER_NAME " Shutting down device\n");
		printk(KERN_DEBUG DRIVER_NAME " Stopping read and write threads...\n");
#if ENABLEREADTHREAD
		kthread_stop(dev_read_thread);
#endif
#if ENABLEWRITETHREAD
		kthread_stop(dev_write_thread);
#endif
		printk(KERN_DEBUG DRIVER_NAME " Turning off ADCs and DACs\n");
		PowerPeriph(0);
		printk(KERN_DEBUG DRIVER_NAME " Freeing character device buffers...\n");
		vfree(cdev_write_buff);
		vfree(cdev_read_buff);
		printk(KERN_DEBUG DRIVER_NAME " Freeing rx and tx buffers...\n");
		destroy_dma_buffs(&DevPrivData->pTxDmaBuffs, &DevPrivData->TxDmaBuffsBusAddr, DMATXBUFFS, DMABUFFSIZE, DMA_TO_DEVICE);
		destroy_dma_buffs(&DevPrivData->pRxDmaBuffs, &DevPrivData->RxDmaBuffsBusAddr, DMARXBUFFS, DMABUFFSIZE, DMA_FROM_DEVICE);
		printk(KERN_DEBUG DRIVER_NAME " Device shut down\n");
	};

	DevPrivData->OpenCtr--;
	printk(KERN_DEBUG DRIVER_NAME " Device has %d handles open\n", DevPrivData->OpenCtr);
	printk(KERN_DEBUG DRIVER_NAME " File closed\n");
	mutex_unlock(&cdev_open_close_mutex);

	return 0;

/*rc should be set to a meaningful value which is appropriate for the type of failure prior to a jump to any of the following labels*/
exit_func:
	printk(KERN_ERR DRIVER_NAME " File close failed: %d\n", rc);
	return rc;
};


static const struct file_operations cdev_fops = {
	.owner = THIS_MODULE,
	.write = cdev_write,
	.read = cdev_read,
	.unlocked_ioctl = cdev_ioctl,
	.open = cdev_open,
	.release = cdev_release,
	.llseek = no_llseek
};


/**Reset DMA controller on device
*/
static void reset_dma(void) {
    printk(KERN_DEBUG DRIVER_NAME " Resetting DMA controllers...\n");
	writel(CSR_CTRL_RESET, DevPrivData->pBar1 + WRCSRCTRL);
	writel(CSR_CTRL_RESET, DevPrivData->pBar1 + RDCSRCTRL);
	wmb();
	/*Wait for reset to complete*/
	while (readl(DevPrivData->pBar1 + WRCSRSTAT) & CSR_STAT_RESETTING) {
		rmb();
		printk(KERN_DEBUG DRIVER_NAME " Waiting for Rx reset to complete\n");
	};
	while (readl(DevPrivData->pBar1 + RDCSRSTAT) & CSR_STAT_RESETTING) {
		rmb();
		printk(KERN_DEBUG DRIVER_NAME " Waiting for Tx reset to complete\n");
	};
    printk(KERN_DEBUG DRIVER_NAME " DMA controllers reset\n");
};


/**This function is called when a new device has become available that we could potentially want

Be sure to perform the exact opposite of this function in driver_remove
*/
static int __devinit driver_probe(struct pci_dev *dev, const struct pci_device_id *id) {
	int		rc;

	printk(KERN_INFO DRIVER_NAME " Probing and setting up device...\n");

	/*Enable device*/
	printk(KERN_DEBUG DRIVER_NAME " Enabling device...\n");
	rc = pci_enable_device(dev);
	if (rc) {
		printk(KERN_ERR DRIVER_NAME " Cannot enable device: %d\n", rc);
		goto exit_func;
	};
	printk(KERN_DEBUG DRIVER_NAME " Device enabled\n");


	/*Allocate memory for device specific settings*/
	printk(KERN_DEBUG DRIVER_NAME " Allocating memory for device settings...\n");
	if (!(DevPrivData = kmalloc(sizeof(DevPrivDataType), GFP_KERNEL))) {
		printk(KERN_ERR DRIVER_NAME " No memory available\n");
		rc = -ENOMEM;
		goto exit_pci_disable_device;
	};
	dev_set_drvdata(&dev->dev, DevPrivData);
	DevPrivData->dev = dev;
	printk(KERN_DEBUG DRIVER_NAME " Memory for device settings allocated\n");

	/*Check for revision number to ensure support*/
	printk(KERN_DEBUG DRIVER_NAME " Checking device version...\n");
	DevPrivData->DevVer = 0;
	pci_read_config_byte(dev, PCI_REVISION_ID, (u8*)&DevPrivData->DevVer);
	if (DevPrivData->DevVer != 1) {
		printk(KERN_ERR DRIVER_NAME " Device version not supported: %d\n", DevPrivData->DevVer);
		rc = -ENODEV;
		goto exit_kfree_priv_data;
	};
	printk(KERN_DEBUG DRIVER_NAME " Device version: %d\n", DevPrivData->DevVer);

	/*Request BAR regions and check to make sure that they are memory spaces*/
	printk(KERN_DEBUG DRIVER_NAME " Requesting and checking BAR regions...\n");
	/*Request use of BAR0 address range*/
	rc = pci_request_region(dev, 0, DRIVER_NAME);
	if (rc) {
		printk(KERN_ERR DRIVER_NAME " Unable to grab BAR0: %d\n", rc);
		goto exit_kfree_priv_data;
	};
	/*Check to ensure that BAR0 is a memory space (not IO space)*/
	if (!(pci_resource_flags(dev, 0) & IORESOURCE_MEM)) {
		printk(KERN_ERR DRIVER_NAME " Device's BAR0 is not an memory space\n");
		rc = -ENODEV;
		goto exit_pci_release_regions;
	};
	printk(KERN_DEBUG DRIVER_NAME " BAR0: 0x%08x\n", (uint32_t)pci_resource_start(dev, 0));
	/*Request use of BAR0 address range*/
	rc = pci_request_region(dev, 1, DRIVER_NAME);
	if (rc) {
		printk(KERN_ERR DRIVER_NAME " Unable to grab BAR1: %d\n", rc);
		goto exit_pci_release_regions;
	};
	/*Check to ensure that BAR1 is a memory space (not IO space)*/
	if (!(pci_resource_flags(dev, 1) & IORESOURCE_MEM)) {
		printk(KERN_ERR DRIVER_NAME " Device's BAR1 is not an memory space\n");
		rc = -ENODEV;
		goto exit_pci_release_regions;
	};
	printk(KERN_DEBUG DRIVER_NAME " BAR0: 0x%08x\n", (uint32_t)pci_resource_start(dev, 0));
	printk(KERN_DEBUG DRIVER_NAME " BAR regions are OK");

	/*Map bar regions for driver access*/
	printk(KERN_DEBUG DRIVER_NAME " Mapping BARs to memory addresses...\n");
	if (!(DevPrivData->pBar0 = pci_iomap(dev, 0, 0))) {
		printk(KERN_ERR DRIVER_NAME " Cannot map BAR0 to a memory address\n");
		rc = -ENODEV;
		goto exit_pci_release_regions;
	};
	printk(KERN_DEBUG DRIVER_NAME " BAR0 accessible through memory address: 0x%016llx\n", (int64_t)DevPrivData->pBar0);
	if (!(DevPrivData->pBar1 = pci_iomap(dev, 1, 0))) {
		printk(KERN_ERR DRIVER_NAME " Cannot map BAR1 to a memory address\n");
		rc = -ENODEV;
		goto exit_pci_iounmap_0;
	};
	printk(KERN_DEBUG DRIVER_NAME " BAR1 accessible through memory address: 0x%016llx\n", (int64_t)DevPrivData->pBar1);
	printk(KERN_DEBUG DRIVER_NAME " BAR regions are mapped to memory addresses\n");


	/*Set up DMA*/
	printk(KERN_DEBUG DRIVER_NAME " Setting DMA mask for 32bit operation...\n");
	pci_set_master(dev);


	if (dma_set_mask(&(dev->dev), DMA_BIT_MASK(32))) {
		printk(KERN_ERR DRIVER_NAME " Failed to set DMA mask\n");
		rc = -ENODEV;
		goto exit_pci_iounmap_1;
	};
	printk(KERN_DEBUG DRIVER_NAME " DMA mask set\n");

	/*Detect address translation mask*/
	printk(KERN_DEBUG DRIVER_NAME " Determining bus translation mask...\n");
	writel(0xfffffffc, DevPrivData->pBar0 + CRA_ATT);
	mb();
	DevPrivData->BusTranslationMask = readl(DevPrivData->pBar0 + CRA_ATT);
	printk(KERN_DEBUG DRIVER_NAME " Bus translation mask: 0x%016llx...\n", (uint64_t)DevPrivData->BusTranslationMask);
	reset_dma();

	/*Turn off device to save power*/
	printk(KERN_DEBUG DRIVER_NAME " Turning off ADCs and DACs\n");
	PowerPeriph(0);

	printk(KERN_INFO DRIVER_NAME " Probing complete\n");
	return 0;

/*rc should be set to a meaningful value which is appropriate for the type of failure prior to a jump to any of the following labels*/
exit_pci_iounmap_1:
	pci_iounmap(dev, DevPrivData->pBar1);
exit_pci_iounmap_0:
	pci_iounmap(dev, DevPrivData->pBar0);
exit_pci_release_regions:
	pci_release_regions(dev);
exit_kfree_priv_data:
	kfree(DevPrivData);
exit_pci_disable_device:
	pci_disable_device(dev);
exit_func:
	printk(KERN_ERR DRIVER_NAME " Probe and setup failed: %d\n", rc);
	return rc;
};


/**This function is called as a clean up routine

Be sure to undo all the changes made by driver_probe
*/
static void __devexit driver_remove(struct pci_dev *dev) {
	printk(KERN_INFO DRIVER_NAME " Removing device...\n");
	reset_dma();
	pci_iounmap(dev, DevPrivData->pBar1);
	pci_iounmap(dev, DevPrivData->pBar0);
	pci_release_regions(dev);
	kfree(DevPrivData);
	pci_disable_device(dev);
	printk(KERN_INFO DRIVER_NAME " Device removed\n");
};


static const struct pci_device_id driver_ids[] = {
	{ PCI_DEVICE(0x1172, 0x0004), },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, driver_ids);


static struct pci_driver driver_pci_driver = {
	.name = DRIVER_NAME,
	.id_table = driver_ids,
	.probe = driver_probe,
	.remove = __devexit_p(driver_remove),
};


/**Called by insmod

Everything done here must be undone in driver_exit
*/
static int __init driver_initial(void) {
	int		rc;

	printk(KERN_INFO DRIVER_NAME " Loading... (build 20150120)\n");
	rc = pci_register_driver(&driver_pci_driver);
	if (rc) {
		goto exit_func;
	};

	/*Make cahracter device*/
	printk(KERN_DEBUG DRIVER_NAME " Allocating character device...\n");
	rc = alloc_chrdev_region(&cdev, 0, 1, DRIVER_NAME);
	if (rc < 0) {
		printk(KERN_ERR DRIVER_NAME " Error making character device\n");
		goto exit_pci_unregister_driver;
	};
	printk(KERN_DEBUG DRIVER_NAME " Charater device allocated (major: %d, minor: %d)\n", MAJOR(cdev), MINOR(cdev));
	printk(KERN_DEBUG DRIVER_NAME " Initializing and adding character device...\n");
	cdev_init(&driver_cdev, &cdev_fops);
	rc = cdev_add(&driver_cdev, cdev, 1);
	if (rc < 0) {
		printk(KERN_ERR DRIVER_NAME " Cannot add character device: %d\n", rc);
		goto exit_unregister_chrdev;
	};
	if (DevPrivData) { DevPrivData->OpenCtr = 0; };
	printk(KERN_INFO DRIVER_NAME " To make file system device, run mknod %s c %d %d\n", DRIVER_NAME, MAJOR(cdev), MINOR(cdev));
	printk(KERN_DEBUG DRIVER_NAME " Character device added\n");

	printk(KERN_INFO DRIVER_NAME " Loaded\n");
	return 0;

/*rc should be set to a meaningful value which is appropriate for the type of failure prior to a jump to any of the following labels*/
exit_unregister_chrdev:
	unregister_chrdev_region(cdev, 1);
exit_pci_unregister_driver:
	pci_unregister_driver(&driver_pci_driver);
exit_func:
	printk(KERN_ERR DRIVER_NAME " Loading failed: %d\n", rc);
	return rc;
};


/**Called by rmmod

Undo everything done in driver_initial
*/
static void __exit driver_exit(void) {
	printk(KERN_INFO DRIVER_NAME " Unloading...\n");
	cdev_del(&driver_cdev);
	unregister_chrdev_region(cdev, 1);
	pci_unregister_driver(&driver_pci_driver);
	printk(KERN_INFO DRIVER_NAME " Unloaded\n");
};


MODULE_LICENSE("GPL");


module_init(driver_initial);
module_exit(driver_exit);
