#ifndef _LANGFORD_H
#define _LANGFORD_H


// NIC PCI Device and vendor IDs
#define LANGFORD_PCI_DEVICE_ID               0x1172
#define LANGFORD_PCI_VENDOR_ID               0x0004

/*Offsets for BAR0*/
#define CRA					0x00000000	/**< Base address of CRA port of the HIP*/
#define CRA_ATT				0x00001000	/**< Location of address translation table for Altera HIP (on CRA)*/
#define PIO0				0x00008000	/**< Base address of PIO0 relative to BAR0*/
#define PIO1				0x00008100	/**< Base address of PIO1 relative to BAR0*/
#define PIO2				0x00008200	/**< Base address of PIO2 relative to BAR0*/
#define PIO3				0x00008300	/**< Base address of PIO3 relative to BAR0*/
#define PIO4				0x00008400	/**< Base address of PIO4 relative to BAR0*/
#define PIO5				0x00008500	/**< Base address of PIO5 relative to BAR0*/

/*Offsets for BAR1*/
#define WRCSR				0x00000000	/**< Write DMA controller (FPGA to memory, radio rx) command and status registers*/
#define WRCSRSTAT			0x00000000	/**< Write DMA controller CSR - status register*/
#define WRCSRCTRL			0x00000004	/**< Write DMA controller CSR - control register*/
#define WRCSRFILLLEVEL		0x00000008	/**< Write DMA controller CSR - fill level of write descriptor buffer */
#define WRDESC				0x00001000	/**< Write DMA controller (FPGA to memory, radio rx) descriptor registers*/
#define WRDESCWRADDR		0x00001004	/**< Write DMA controller - Descriptor write address*/
#define WRDESCLEN			0x00001008	/**< Write DMA controller - Descriptor length*/
#define WRDESCCTRL			0x0000100C	/**< Write DMA controller - Descriptor control*/
#define RDCSR				0x00002000	/**< Read DMA controller (memory to FPGA, radio tx) command and status registers*/
#define RDCSRSTAT			0x00002000	/**< Read DMA controller - status register*/
#define RDCSRCTRL			0x00002004	/**< Read DMA controller - control register*/
#define RDCSRFILLLEVEL		0x00002008	/**< Read DMA controller - fill level of read descriptor buffer */
#define RDDESC				0x00003000	/**< Read DMA controller (memory to FPGA, radio tx) descriptor registers*/
#define RDDESCRDADDR		0x00003000	/**< Read DMA controller - Descriptor read address*/
#define RDDESCLEN			0x00003008	/**< Read DMA controller - Descriptor length*/
#define RDDESCCTRL			0x0000300C	/**< Read DMA controller - Descriptor control*/

/*Data required for DMA*/
#define DMAPAGEWIDTH		20			/**< Size of address page (i.e. number of pass through bits)*/
#define DMARXBUFFS			32			/**< Number of buffers for Rx (device to memory). Combined with DMATXBUFFS must not exceed number of address pages.*/
#define DMARXDESCS			128			/**< Maximum number of descriptors for rx that the device can handle*/
#define DMARXDESCDELAY		4			/**< Number of rx descriptors to complete before a read will take place*/
#define DMATXBUFFS			32			/**< Number of buffers for Tx (memory to device). Combined with DMARXBUFFS must not exceed number of address pages.*/
#define DMATXDESCS			128			/**< Maximum number of descriptors for tx that the device can handle*/
#define DMATXDESCDELAY		4			/**< Number of rx descriptors to complete before a read will take place*/
#define DMABUFFSIZE			0x1000      //262144		/**< Size of each Rx or Tx buffer*/
//we are implementing scatter gather DMA in order to use the most number of descriptor the device can handle
//this will reduce the amount of data transfered by each descriptors to one page (0x1000)
//this allow us to start simultaneously the bigger buffer size in the device that Windows can handle per adapter object 
//we need to check if the device really can handle until 128 descriptors
//for now we will start with what is already working in Linux, 32

/*Bits for WRCSR and RDCSR*/
#define	CSR_STAT_RESETTING	(1 << 6)	/**< Status bit indicating that reset is still underway*/
#define	CSR_CTRL_RESET		(1 << 1)	/**< Reset bit for the CSR control register*/

/*Bits for WRDESC and RDDESC*/
#define DESC_CTRL_GO		(1 << 31)	/**< Commit descriptor to buffer*/

__forceinline
	VOID WRITE_LANGFORD_REGISTER_ULONG ( PUCHAR BaseAddress,ULONG Offset,ULONG Value)
{
	PULONG Register = (PULONG) (BaseAddress + Offset);
	WRITE_REGISTER_ULONG(Register,Value);
}

__forceinline
	ULONG READ_LANGFORD_REGISTER_ULONG ( PUCHAR BaseAddress,ULONG Offset)
{
	PULONG Register = (PULONG) (BaseAddress + Offset);
	return READ_REGISTER_ULONG(Register);
}

NTSTATUS langfordMapHWResources( IN OUT PLDO_DATA LdoData, 
								IN WDFCMRESLIST ResourcesRaw, 
								IN WDFCMRESLIST ResourcesTranslated );
NTSTATUS langfordUnmapHWResources( IN OUT PLDO_DATA LdoData ) ;

VOID langfordResetDma(IN PLDO_DATA LdoData);

NTSTATUS langfordIo(IN PLDO_DATA LdoData, IN ULONG IoControl, IN OUT PULONG Value);

VOID langfordPowerPeriph(IN PLDO_DATA LdoData, IN ULONG State);




#endif