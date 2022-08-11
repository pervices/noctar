#include "stdinc.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, langfordMapHWResources)
#pragma alloc_text (PAGE, langfordUnmapHWResources)
#pragma alloc_text (PAGE, langfordResetDma)
#pragma alloc_text (PAGE, langfordIo)
#pragma alloc_text (PAGE, langfordPowerPeriph)
#endif

NTSTATUS
langfordMapHWResources(
    IN OUT PLDO_DATA LdoData,
    IN WDFCMRESLIST  ResourcesRaw,
    IN WDFCMRESLIST  ResourcesTranslated
    )
/*++
Routine Description:

    Gets the HW resources assigned by the bus driver and:
    1) Maps them to system address space. 
    2) If PCIDRV_CREATE_INTERRUPT_IN_PREPARE_HARDWARE is defined, 
        it creates a WDFINTERRUPT object.

    Called during EvtDevicePrepareHardware callback.

	Arguments:

    FdoData     Pointer to our FdoData
    ResourcesRaw - Pointer to list of raw resources passed to 
                        EvtDevicePrepareHardware callback
    ResourcesTranslated - Pointer to list of translated resources passed to
                        EvtDevicePrepareHardware callback

Return Value:

    NTSTATUS

--*/
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptorBar0;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptorBar1;
    ULONG       ResourceCount;
	LARGE_INTEGER Interval;
    NTSTATUS    status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(ResourcesRaw);

    PAGED_CODE();

	ResourceCount = WdfCmResourceListGetCount(ResourcesTranslated);
	if(ResourceCount < 2)
	{
		return STATUS_DEVICE_CONFIGURATION_ERROR;
	}

	//get BAR0
	descriptorBar0 = WdfCmResourceListGetDescriptor(ResourcesTranslated, 0);
	//get BAR1
	descriptorBar1 = WdfCmResourceListGetDescriptor(ResourcesTranslated, 1);
	if(!descriptorBar0 || !descriptorBar1)
	{
		return STATUS_DEVICE_CONFIGURATION_ERROR;
	}

	if((descriptorBar0->Type != CmResourceTypeMemory) || (descriptorBar1->Type != CmResourceTypeMemory))
	{
		return STATUS_DEVICE_CONFIGURATION_ERROR;
	}

	LdoData->Bar0PhysAddress = descriptorBar0->u.Memory.Start;
	LdoData->Bar0MemoryLength = descriptorBar0->u.Memory.Length;
	LdoData->Bar0LogicalAddress = MmMapIoSpace(
		descriptorBar0->u.Memory.Start,
		descriptorBar0->u.Memory.Length,
		MmNonCached);

	LdoData->Bar1PhysAddress = descriptorBar0->u.Memory.Start;
	LdoData->Bar1MemoryLength = descriptorBar0->u.Memory.Length;
	LdoData->Bar1LogicalAddress = MmMapIoSpace(
		descriptorBar0->u.Memory.Start,
		descriptorBar0->u.Memory.Length,
		MmNonCached);

	//Configure Bus Master DMA 32 bits in adddevice

	/*Detect address translation mask*/
	WRITE_LANGFORD_REGISTER_ULONG(LdoData->Bar0LogicalAddress,CRA_ATT,0xfffffffc);

	//give a chance to the device to reply
	Interval.QuadPart = -500; //interval of 500 nanoseconds seems enough
	KeDelayExecutionThread(KernelMode,FALSE,&Interval);

    LdoData->BusTranslationMask = READ_LANGFORD_REGISTER_ULONG(LdoData->Bar0LogicalAddress, CRA_ATT);

	langfordResetDma(LdoData);

// 	/*Turn off device to save power*/
 	langfordPowerPeriph(LdoData, 0);
	
    return status;
}

NTSTATUS
langfordUnmapHWResources(
    IN OUT PLDO_DATA LdoData
    )
/*++
Routine Description:

    Disconnect the interrupt and unmap all the memory and I/O resources.

Arguments:

    FdoData     Pointer to our FdoData

Return Value:

     None

--*/
{
    PAGED_CODE();

    //
    // Free hardware resources
    //
    if (LdoData->Bar0LogicalAddress)
    {
        MmUnmapIoSpace(LdoData->Bar0LogicalAddress, LdoData->Bar0MemoryLength);
        LdoData->Bar0LogicalAddress = NULL;
    }

	if (LdoData->Bar1LogicalAddress)
	{
		MmUnmapIoSpace(LdoData->Bar1LogicalAddress, LdoData->Bar1MemoryLength);
		LdoData->Bar1LogicalAddress = NULL;
	}

    return STATUS_SUCCESS;

}

VOID langfordResetDma( IN OUT PLDO_DATA LdoData)
{
	//printk(KERN_DEBUG DRIVER_NAME " Resetting DMA controllers...\n");
	//writel(CSR_CTRL_RESET, DevPrivData->pBar1 + WRCSRCTRL);
	WRITE_LANGFORD_REGISTER_ULONG(LdoData->Bar1LogicalAddress, RDCSRCTRL, CSR_CTRL_RESET);
	//wmb();
	/*Wait for reset to complete*/
	while (READ_LANGFORD_REGISTER_ULONG(LdoData->Bar1LogicalAddress,WRCSRSTAT) & CSR_STAT_RESETTING)
	{
		//rmb();
		//printk(KERN_DEBUG DRIVER_NAME " Waiting for Rx reset to complete\n");
	}
	while (READ_LANGFORD_REGISTER_ULONG(LdoData->Bar1LogicalAddress,RDCSRSTAT) & CSR_STAT_RESETTING)
	{
		//rmb();
		//printk(KERN_DEBUG DRIVER_NAME " Waiting for Tx reset to complete\n");
	}

	//printk(KERN_DEBUG DRIVER_NAME " DMA controllers reset\n");
}

/**Helper function for ioctrl (called ioctl handler and driver)
*/
NTSTATUS langfordIo(IN PLDO_DATA LdoData, IN ULONG IoControl, IN OUT PULONG Value) 
{
	LONG		AddrOffset;
	LONG		BitOffset;

	/*Bit offset list*/
	switch (IoControl) 
	{
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
	default:
		return STATUS_UNSUCCESSFUL;
		break;
	};
	//mb();

	/*Read and write lists*/
	switch (IoControl)
	{
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
		*Value = (READ_LANGFORD_REGISTER_ULONG(LdoData->Bar0LogicalAddress,AddrOffset) >> BitOffset) & 0x00000001;
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
		if (*Value > 1) 
		{
			return STATUS_INVALID_PARAMETER;
		};
		//writel((readl(DevPrivData->pBar0 + AddrOffset) & ~(0x00000001 << BitOffset)) | (*parg << BitOffset), DevPrivData->pBar0 + AddrOffset);
		WRITE_LANGFORD_REGISTER_ULONG(LdoData->Bar0LogicalAddress, 
			AddrOffset, 
			(READ_LANGFORD_REGISTER_ULONG(LdoData->Bar0LogicalAddress,AddrOffset) & ~(0x00000001 << BitOffset)) | (*Value << BitOffset));
		break;
		/*5 bit read*/
	case IOCTL_GET_GPIOin:		case IOCTL_GET_GPIOout:
		*Value = (READ_LANGFORD_REGISTER_ULONG(LdoData->Bar0LogicalAddress, AddrOffset) >> BitOffset) & 0x0000001f;
		break;
		/*5 bit write*/
	case IOCTL_SET_GPIOout:
		if (*Value > 31) 
		{
			return STATUS_UNSUCCESSFUL;
		};
		WRITE_LANGFORD_REGISTER_ULONG( LdoData->Bar0LogicalAddress, AddrOffset, (READ_LANGFORD_REGISTER_ULONG(LdoData->Bar0LogicalAddress, AddrOffset) & ~(0x0000001f << BitOffset)) | (*Value << BitOffset));
		break;
		/*8 bit read*/
	case IOCTL_GET_N21at0:		case IOCTL_GET_N22at0:		case IOCTL_GET_N3GAIN:
	case IOCTL_GET_RXDecEn:		case IOCTL_GET_TXIntEn:
		*Value = (READ_LANGFORD_REGISTER_ULONG(LdoData->Bar0LogicalAddress, AddrOffset) >> BitOffset) & 0x000000ff;
		break;
		/*8 bit write*/
	case IOCTL_SET_N21at0:		case IOCTL_SET_N22at0:		case IOCTL_SET_N3GAIN:
	case IOCTL_SET_RXDecEn:		case IOCTL_SET_TXIntEn:
		if (*Value > 255) 
		{
			return STATUS_UNSUCCESSFUL;
		};
		WRITE_LANGFORD_REGISTER_ULONG(LdoData->Bar0LogicalAddress,
			AddrOffset, 
			(READ_LANGFORD_REGISTER_ULONG(LdoData->Bar0LogicalAddress, AddrOffset) & ~(0x000000ff << BitOffset)) | (*Value << BitOffset));
		break;
		/*32 bit read*/
	case IOCTL_GET_RXPhase:		case IOCTL_GET_TXPhase:
		*Value = (READ_LANGFORD_REGISTER_ULONG(LdoData->Bar0LogicalAddress, AddrOffset) >> BitOffset) & 0xffffffff;
		break;
		/*32 bit write*/
	case IOCTL_SET_RXPhase:		case IOCTL_SET_TXPhase:
		WRITE_LANGFORD_REGISTER_ULONG(LdoData->Bar0LogicalAddress, 
			AddrOffset,
			(READ_LANGFORD_REGISTER_ULONG(LdoData->Bar0LogicalAddress, AddrOffset) & ~(0xffffffff << BitOffset)) | (*Value << BitOffset));
		break;
	};
	//mb();

	return STATUS_SUCCESS;
};

/**Enables or disables ADC and DAC

State = 1 - Power on
State = 0 - Power off
*/
VOID langfordPowerPeriph(IN PLDO_DATA LdoData, IN ULONG State) 
{
	ULONG		nState = !State;
	ULONG		Val1 = 1;

	State = !nState;

	/*All comments presented for power on scenario, invert logic for power off*/
	/*ADCs*/
	/*Disable power down (0)*/
	langfordIo(LdoData, (ULONG)IOCTL_SET_ADCAPD, &nState);
	langfordIo(LdoData, (ULONG)IOCTL_SET_ADCBPD, &nState);

	///*DACs*/
	/*Enable pin mode always*/
	langfordIo(LdoData, (ULONG)IOCTL_SET_DACReset, &Val1);
	///*Data on both ports (0)*/
	langfordIo(LdoData, (ULONG)IOCTL_SET_DACSClk, &nState);
	/*Data format is signed binary (0)*/
	langfordIo(LdoData, (ULONG)IOCTL_SET_DACSDIO, &nState);
	/*Mixed mode off (0)*/
	langfordIo(LdoData, (ULONG)IOCTL_SET_DACCSB, &nState);
	/*Power down off - all systems running (0)*/
	langfordIo(LdoData, (ULONG)IOCTL_SET_DACSDO, &nState);

	/*Enable DSP chains (1)*/
	langfordIo(LdoData, (ULONG)IOCTL_SET_RxDspEn, &State);
	langfordIo(LdoData, (ULONG)IOCTL_SET_TxDspEn, &State);

	/*Set sane values for signed and unsigned inputs. */
	langfordIo(LdoData, (ULONG)IOCTL_SET_RxIsSigned, &nState);
	langfordIo(LdoData, (ULONG)IOCTL_SET_TxIsSigned, &nState);
}

