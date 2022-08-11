/***********************************************************************
 * The langford header. This includes the memory location and offset
 * for various items in the PCIe BAR device.
 * 
 * (c) 2013 Per Vices Corporation
 * 
 * This code is licensed under the GPLv3
 ************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/pci_regs.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/jiffies.h>
#include <linux/timex.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include "langford_ioctl.h"


/** Data structure to hold data specific to each instance of the device on the system

This data structure can be "attached" to a device using dev_set_drvdata:
\code
	DevPrivDataType	*DevPrivData;
	DevPrivData = kmalloc(sizeof(DevPrivDataType), GFP_KERNEL);
	dev_set_drvdata(&dev->dev, DevPrivData);
\endcode 

To access the contents of this data structure after it has been allocated, you can use dev_get_drvdata:
\code
	DevPrivDataType	*DevPrivData = dev_get_drvdata(&dev->dev);
\endcode

This data structure should only be allocated once per device. It should be destroyed when the device is unloaded.
*/
typedef struct {
	struct pci_dev	*dev;				/**< PCI device which this data structure belongs to*/
	unsigned int	DevVer;				/**< Version of the device as seen from the PCI configuartion registers*/
	void __iomem	*pBar0;				/**< Pointer to BAR0 accessible in program space*/
	void __iomem	*pBar1;				/**< Pointer to BAR1 accessible in program space*/
	int		OpenCtr;			/**< Number of open handles (need to close device as many items as it is opened)*/
	dma_addr_t	BusTranslationMask;		/**< Mask used to remove LSBs from memory address prior to being writen to translation table*/
	uint8_t		**pRxDmaBuffs;			/**< Rx DMA buffers*/ 
	uint8_t		**pTxDmaBuffs;			/**< Rx DMA buffers*/
	dma_addr_t	*RxDmaBuffsBusAddr;		/**< Addresses of pRxDmaBuffs as seen by the PCI bus*/
	dma_addr_t	*TxDmaBuffsBusAddr;		/**< Addresses of pTxDmaBuffs as seen by the PCI bus*/

} DevPrivDataType;


#define RX_THREAD_MSLEEP	1
#define TX_THREAD_MSLEEP	1
#define RX_CDEV_MSLEEP		1
#define TX_CDEV_MSLEEP		1

/*Delete this define and the block of code it contains in production*/
#define DELETEME 0
 
/*Program constants*/
#define DRIVER_NAME			"langford"	/**< Driver name as how it shows up in the kernel logs*/
#define VERBOSE_LOGGING_R		0		/**< Extra verbose logging in rx DMA thread*/
#define VERBOSE_LOGGING_W		0		/**< Extra verbose logging in tx DMA thread*/
#define EMPTY_LOGGING			0		/**< Print messages if read or write buffers are empty*/
#define CDEVBUFFSIZE			33554432UL	/**< Size of buffers for the character device read and write buffers*/
#define ENABLEINTERRUPTS		0
#define ENABLEREADTHREAD		1
#define ENABLEWRITETHREAD		1

/*Data required for DMA*/ 

#define DMAPAGEWIDTH			20		/**< Size of address page (i.e. number of pass through bits)*/
#define DMARXBUFFS			32		/**< Number of buffers for Rx (device to memory). Combined with DMATXBUFFS must not exceed number of address pages.*/
#define DMARXDESCS			128		/**< Maximum number of descriptors for rx that the device can handle*/
#define DMARXDESCDELAY			4		/**< Number of rx descriptors to complete before a read will take place*/
#define DMATXBUFFS			32		/**< Number of buffers for Tx (memory to device). Combined with DMARXBUFFS must not exceed number of address pages.*/
#define DMATXDESCS			128		/**< Maximum number of descriptors for tx that the device can handle*/
#define DMATXDESCDELAY			4		/**< Number of rx descriptors to complete before a read will take place*/
#define DMABUFFSIZE			262144UL	/**< Size of each Rx or Tx buffer = 2^18*/ 

/*Offsets relative to PCI device base address (configuration space)*/
#define MSI_CONTROL_SATUS_REGISTER	0x00000050	/*32-bit register in PCI configuration space*/
#define MSI_CONTROL_ENABLE			(1<<16) 	/*tables 5-10, 6-4 of Altera's "IP compiler for PCI express user guide"*/


/*Offsets for BAR0*/
#define CRA				0x00000000	/**< Base address of CRA port of the HIP*/
#define AVALON_TO_PCI_IRQ_STATUS	0x00000040	/*PCI express Avalon-MM bridge Register for PCI express interrupt Status*/
#define AVALON_TO_PCI_IRQ_ENABLE	0x00000050	/*PCI express Avalon-MM bridge Register for PCI express interrupt Enable*/
#define PCI_TO_AVALON_IRQ_STATUS	0x00000360	/*PCI express Avalon-MM bridge Register for Avalon-MM interrupt Status*/
#define PCI_TO_AVALON_IRQ_ENABLE	0x00000370	/*PCI express Avalon-MM bridge Register for Avalon-MM interrupt Enable*/
#define CRA_ATT				0x00001000	/**< Location of address translation table for Altera HIP (on CRA)*/
#define PIO0				0x00008000	/**< Base address of PIO0 relative to BAR0*/
#define PIO1				0x00008100	/**< Base address of PIO1 relative to BAR0*/
#define PIO2				0x00008200	/**< Base address of PIO2 relative to BAR0*/
#define PIO3				0x00008300	/**< Base address of PIO3 relative to BAR0*/
#define PIO4				0x00008400	/**< Base address of PIO4 relative to BAR0*/
#define PIO5				0x00008500	/**< Base address of PIO5 relative to BAR0*/
#define PIO6				0x00008600	/**< Base address of PIO5 relative to BAR0*/
#define PIO7				0x00008700	/**< Base address of PIO5 relative to BAR0*/

/*Offsets for BAR1*/
/*reference: .../ ip/SGDMA_dispatcher/Modular_SGDMA_Dispatcher_Core_UG.pdf*/
#define WRCSR				0x00000000	/**< Write DMA controller (FPGA to memory, radio rx) command and status registers*/
#define WRCSRSTAT			0x00000000	/**< Write DMA controller CSR - status register*/
#define WRCSRCTRL			0x00000004	/**< Write DMA controller CSR - control register*/
#define WRCSRFILLLEVEL			0x00000008	/**< Write DMA controller CSR - fill level of write descriptor buffer */
#define WRDESC				0x00001000	/**< Write DMA controller (FPGA to memory, radio rx) descriptor registers*/
#define WRDESCWRADDR			0x00001004	/**< Write DMA controller - Descriptor write address*/
#define WRDESCLEN			0x00001008	/**< Write DMA controller - Descriptor length*/
#define WRDESCCTRL			0x0000100C	/**< Write DMA controller - Descriptor control*/
#define RDCSR				0x00002000	/**< Read DMA controller (memory to FPGA, radio tx) command and status registers*/
#define RDCSRSTAT			0x00002000	/**< Read DMA controller - status register*/
#define RDCSRCTRL			0x00002004	/**< Read DMA controller - control register*/
#define RDCSRFILLLEVEL			0x00002008	/**< Read DMA controller - fill level of read descriptor buffer */
#define RDDESC				0x00003000	/**< Read DMA controller (memory to FPGA, radio tx) descriptor registers*/
#define RDDESCRDADDR			0x00003000	/**< Read DMA controller - Descriptor read address*/
#define RDDESCLEN			0x00003008	/**< Read DMA controller - Descriptor length*/
#define RDDESCCTRL			0x0000300C	/**< Read DMA controller - Descriptor control*/

/*Bits for WRCSR and RDCSR*/
//STATUS REGISTER MASKS:
#define	CSR_STAT_RESETTING		(1 << 6)	/**< Status bit indicating that reset is still underway*/
#define CSR_DESCRBUFF_EMPTY		(1 << 1)	/*Set when both the read and write command buffers are empty*/
#define CSR_DESCRBUFF_FULL		(1 << 2)	/*Set when either the read or write cmd buffers are full*/
#define CSR_RESPBUFF_EMPTY		(1 << 3)	/*Set when the response buffer is empty*/
#define CSR_RESPBUFF_FULL		(1 << 4)	/*Set when the response buffer is full*/
#define CSR_IRQ_FLAG			(1 << 9)  	/*Set when interrupt condition occurs*/
#define CSR_BUSY			(1 << 0)	/*Set when the dispatcher still has commands buffered or one of the masters is still transferring data */


//CONTROL REGISTER MASKS
#define	CSR_CTRL_RESET			(1 << 1)	/**< Reset bit for the CSR control register*/
#define CSR_INTR_ENABLE			(1 << 4) 	/*Setting this bit will allow interrupts to propagate to the interrupt sender port. This mask occurs after the register logic so that interrupts are not missed when the mask is disabled. */


/*Bits for WRDESC and RDDESC*/		
#define DESC_CTRL_GO			(1 << 31)	/**< Commit descriptor to buffer*/
#define DESC_CTRL_DMA_CHANNEL		0x000000ff
#define DESC_CTRL_DMA_COMPLETE_IRQMASK	(1 << 14)	/*DMA transfer complete IRQ mask*/
#define DESC_CTRL_DMA_EARLYTERM_IRQMASK	(1 << 15)	/*DMA transfer early termination IRQ mask*/
#define DESC_CTRL_DMA_ERROR_IRQMASK	0x00ff0000	/*DMA transmit Error/ Error IRQ Mask*/
#define DESC_CTRL_DMA_EARLYDONE_ENABLE	(1 << 24)	/*DMA early done enable*/


/*Program macros*/

/**Given 2 numbers, return the larger one*/
#define MAX(a, b)			((a > b) ? a : b)

/**Given 2 numbers, return the smaller one*/
#define MIN(a, b)			((a < b) ? a : b)

