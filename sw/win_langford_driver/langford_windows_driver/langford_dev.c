#include "stdinc.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, langfordEvtDeviceAdd)
#pragma alloc_text (PAGE, langfordEvtDeviceContextCleanup)
#pragma alloc_text (PAGE, langfordEvtDevicePrepareHardware)
#pragma alloc_text (PAGE, langfordEvtDeviceReleaseHardware)
#pragma alloc_text (PAGE, langfordEvtDeviceD0Entry)
#pragma alloc_text (PAGE, langfordEvtDeviceD0Exit)
#endif

NTSTATUS langfordEvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
	NTSTATUS								status = STATUS_SUCCESS;
	WDF_PNPPOWER_EVENT_CALLBACKS			pnpPowerCallbacks;
	WDF_OBJECT_ATTRIBUTES					ldoAttributes;
	WDFDEVICE								hDevice;
	PLDO_DATA								ldoData = NULL;
	WDF_IO_QUEUE_CONFIG						queueConfig;
	//WDF_INTERRUPT_CONFIG					interruptConfig;
	DECLARE_CONST_UNICODE_STRING(DosDeviceName, LANGFORD_DOS_DEVICE_NAME);
	WDF_DMA_ENABLER_CONFIG          dmaConfig;
	ULONG                           maximumLength;
	HANDLE									hThread = NULL;

	UNREFERENCED_PARAMETER(Driver);

	WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

	pnpPowerCallbacks.EvtDevicePrepareHardware = langfordEvtDevicePrepareHardware;
	pnpPowerCallbacks.EvtDeviceReleaseHardware = langfordEvtDeviceReleaseHardware;
	pnpPowerCallbacks.EvtDeviceD0Entry = langfordEvtDeviceD0Entry;
	pnpPowerCallbacks.EvtDeviceD0Exit  = langfordEvtDeviceD0Exit;

	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ldoAttributes, LDO_DATA);

	ldoAttributes.EvtCleanupCallback = langfordEvtDeviceContextCleanup;

	status = WdfDeviceCreate(&DeviceInit, &ldoAttributes, &hDevice);

	ldoData = langfordDeviceGetData(hDevice);
	RtlZeroMemory(ldoData,sizeof(PLDO_DATA));

	ldoData->WdfDevice = hDevice;

	//
	// Get the BUS_INTERFACE_STANDARD for our device so that we can
	// read & write to PCI config space.
	//
	//status = WdfFdoQueryForInterface(ldoData->WdfDevice,
	//	&GUID_BUS_INTERFACE_STANDARD,
	//	(PINTERFACE) &ldoData->BusInterface,
	//	sizeof(BUS_INTERFACE_STANDARD),
	//	1, // Version
	//	NULL); //InterfaceSpecificData
	//if (!NT_SUCCESS (status))
	//{
	//	return status;
	//}

	//langfordGetDeviceInformation(ldoData);

	//
	status = WdfDeviceCreateSymbolicLink(
		ldoData->WdfDevice,
		&DosDeviceName
		);   // symbolic link
	if (!NT_SUCCESS(status)) 
	{
		return status;
	}

	WDF_IO_QUEUE_CONFIG_INIT ( &queueConfig, WdfIoQueueDispatchSequential);

	queueConfig.EvtIoWrite = langfordEvtIoWrite;

    __analysis_assume(queueConfig.EvtIoStop != 0);
	status = WdfIoQueueCreate(hDevice, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &ldoData->WriteQueue);
	__analysis_assume(queueConfig.EvtIoStop != 0);
	if (!NT_SUCCESS(status)) 
	{
		return status;
	}

	//
	// Set the Write Queue forwarding for IRP_MJ_WRITE requests.
	//
	status = WdfDeviceConfigureRequestDispatching( ldoData->WdfDevice,
		ldoData->WriteQueue,
		WdfRequestTypeWrite);

	if(!NT_SUCCESS(status)) 
	{
		return status;
	}
	//
	// Create a new IO Queue for IRP_MJ_READ requests in sequential mode.
	//
	WDF_IO_QUEUE_CONFIG_INIT( &queueConfig,
		WdfIoQueueDispatchSequential);

	queueConfig.EvtIoRead = langfordEvtIoRead;

	__analysis_assume(queueConfig.EvtIoStop != 0);
	status = WdfIoQueueCreate( ldoData->WdfDevice,&queueConfig,WDF_NO_OBJECT_ATTRIBUTES,&ldoData->ReadQueue );
	__analysis_assume(queueConfig.EvtIoStop == 0);

	if(!NT_SUCCESS(status))
	{
		return status;
	}

	//
	// Set the Read Queue forwarding for IRP_MJ_READ requests.
	//
	status = WdfDeviceConfigureRequestDispatching( ldoData->WdfDevice,
		ldoData->ReadQueue,
		WdfRequestTypeRead);

	if(!NT_SUCCESS(status)) 
	{
		return status;
	}

	// Create a new IO Queue for IRP_MJ_DEVICE_CONTROL requests in sequential mode.
	//
	WDF_IO_QUEUE_CONFIG_INIT( &queueConfig,
		WdfIoQueueDispatchSequential);

	queueConfig.EvtIoDeviceControl = langfordEvtIoDeviceControl;

	__analysis_assume(queueConfig.EvtIoStop != 0);
	status = WdfIoQueueCreate( ldoData->WdfDevice,&queueConfig,WDF_NO_OBJECT_ATTRIBUTES,&ldoData->IoControlQueue );
	__analysis_assume(queueConfig.EvtIoStop == 0);

	if(!NT_SUCCESS(status))
	{
		return status;
	}

	//
	// Set the Ioctl Queue forwarding for IRP_MJ_DEVICE_CONTROL requests.
	//
	status = WdfDeviceConfigureRequestDispatching( ldoData->WdfDevice,
		ldoData->IoControlQueue,
		WdfRequestTypeDeviceControl);

	if(!NT_SUCCESS(status)) 
	{
		return status;
	}
	
	//Don't use the interrupt until the hardware really use it

	//WDF_INTERRUPT_CONFIG_INIT(&interruptConfig, langfordEvtInterruptIsr, langfordEvtInterruptDpc);

	//interruptConfig.EvtInterruptEnable  = langfordEvtInterruptEnable;
	//interruptConfig.EvtInterruptDisable = langfordEvtInterruptDisable;

	//status = WdfInterruptCreate(ldoData->WdfDevice, &interruptConfig, WDF_NO_OBJECT_ATTRIBUTES, &ldoData->WdfInterrupt);

	KeInitializeEvent(&ldoData->ReadRequestEvent,SynchronizationEvent,FALSE);
	KeInitializeEvent(&ldoData->WriteRequestEvent,SynchronizationEvent,FALSE);
	KeInitializeEvent(&ldoData->TerminatedThreadEvent,NotificationEvent,FALSE);

	status = PsCreateSystemThread(&hThread,THREAD_ALL_ACCESS,NULL,NULL,NULL,ReadRequestProcessor,ldoData);
	if(!NT_SUCCESS(status)) 
	{
		return status;
	}

	status = ObReferenceObjectByHandle(hThread,THREAD_ALL_ACCESS,*PsThreadType,KernelMode,(PVOID*)&ldoData->ReadRequestThread,NULL);
	if(!NT_SUCCESS(status)) 
	{
		return status;
	}

	status = PsCreateSystemThread(&hThread,THREAD_ALL_ACCESS,NULL,NULL,NULL,WriteRequestProcessor,ldoData);
	if(!NT_SUCCESS(status)) 
	{
		return status;
	}

	status = ObReferenceObjectByHandle(hThread,THREAD_ALL_ACCESS,*PsThreadType,KernelMode,(PVOID*)&ldoData->WriteRequestThread,NULL);
	if(!NT_SUCCESS(status)) 
	{
		return status;
	}
	
	//
	// Alignment requirement for this device. This alignment
	// value will be inherits by the DMA enabler and used when you allocate
	// common buffers.
	// TOCHECK: the file descriptor offset is 4 bytes, so we will try wit this 
	//
	WdfDeviceSetAlignmentRequirement( ldoData->WdfDevice, FILE_LONG_ALIGNMENT);

	maximumLength = DMABUFFSIZE * DMARXBUFFS;

	WDF_DMA_ENABLER_CONFIG_INIT( &dmaConfig,
		WdfDmaProfileScatterGatherDuplex,
		maximumLength );
	//
	WDF_DMA_ENABLER_CONFIG_INIT( &dmaConfig,
		WdfDmaProfileScatterGatherDuplex,
		DMABUFFSIZE * DMARXBUFFS);

	status = WdfDmaEnablerCreate( ldoData->WdfDevice,
		&dmaConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&ldoData->WdfDmaEnabler );

	if (!NT_SUCCESS (status))
	{
		return status;
	}

	WdfDmaEnablerSetMaximumScatterGatherElements(ldoData->WdfDmaEnabler,DMARXBUFFS);

	//
	// Since we are using sequential queue and processing one request
	// at a time, we will create transaction objects upfront and reuse
	// them to do DMA transfer. Transactions objects are parented to
	// DMA enabler object by default. They will be deleted along with
	// along with the DMA enabler object. So need to delete them
	// explicitly.
	//
	status = WdfDmaTransactionCreate( ldoData->WdfDmaEnabler,
		WDF_NO_OBJECT_ATTRIBUTES,
		&ldoData->WdfReadTransaction);

	if(!NT_SUCCESS(status))
	{
		return status;
	}

	// Create a new DmaTransaction.
	//
	status = WdfDmaTransactionCreate( ldoData->WdfDmaEnabler,
		WDF_NO_OBJECT_ATTRIBUTES,
		&ldoData->WdfWriteTransaction);

	if(!NT_SUCCESS(status)) 
	{
		return status;
	}


	return status;
}


VOID langfordEvtDeviceContextCleanup(WDFOBJECT Device)
{
	PLDO_DATA LdoData;
	PVOID WaitEvents[2];
	int ThreadIndex = 0;
	NTSTATUS status;

	LdoData = langfordDeviceGetData(Device);

	KeSetEvent(&LdoData->TerminatedThreadEvent,IO_NO_INCREMENT,FALSE);

	if(LdoData->ReadRequestThread != NULL)
	{
		WaitEvents[ThreadIndex] = LdoData->ReadRequestThread;
		ThreadIndex++;
	}
	if(LdoData->WriteRequestThread != NULL)
	{
		WaitEvents[ThreadIndex] = LdoData->WriteRequestThread;
		ThreadIndex++;
	}

	status = KeWaitForMultipleObjects(ThreadIndex,WaitEvents,WaitAll,Executive,KernelMode,FALSE,NULL,NULL);

	if(status == STATUS_SUCCESS)
	{
		if(LdoData->ReadRequestThread != NULL)
			ObDereferenceObject(LdoData->ReadRequestThread);
		if(LdoData->WriteRequestThread != NULL)
			ObDereferenceObject(LdoData->WriteRequestThread);
	}
}

NTSTATUS langfordEvtDevicePrepareHardware (WDFDEVICE Device, WDFCMRESLIST Resources, WDFCMRESLIST ResourcesTranslated)
{
	NTSTATUS status = STATUS_SUCCESS;

	PLDO_DATA LdoData;
	
	LdoData = langfordDeviceGetData(Device);

	status = langfordMapHWResources(LdoData,Resources,ResourcesTranslated);

	return status;
}

NTSTATUS langfordEvtDeviceReleaseHardware(IN WDFDEVICE Device, IN WDFCMRESLIST ResourcesTranslated)
{
	NTSTATUS status = STATUS_SUCCESS;
	PLDO_DATA LdoData;

	UNREFERENCED_PARAMETER(ResourcesTranslated);

	LdoData = langfordDeviceGetData(Device);

	status = langfordUnmapHWResources(LdoData);

	return status;
}

NTSTATUS langfordEvtDeviceD0Entry(IN WDFDEVICE Device, IN WDF_POWER_DEVICE_STATE PreviousState)
{
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(PreviousState);

	return status;
}

NTSTATUS langfordEvtDeviceD0Exit(IN WDFDEVICE Device, IN WDF_POWER_DEVICE_STATE TargetState)
{
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(TargetState);

	return status;
}

NTSTATUS langfordGetDeviceInformation(IN PLDO_DATA LdoData)
/*++
Routine Description:

    This function reads the PCI config space and make sure that it's our
    device and stores the device IDs and power information in the device
    extension. Should be done in the StartDevice.

Arguments:

    LdoData     Pointer to our LdoData

Return Value:

     None

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) UCHAR buffer[LANGFORD_PCI_HDR_LENGTH];
    PPCI_COMMON_CONFIG  pPciConfig = (PPCI_COMMON_CONFIG) buffer;
    ULONG               bytesRead =0;

    PAGED_CODE();

    RtlZeroMemory(buffer, sizeof(buffer));
    bytesRead = LdoData->BusInterface.GetBusData(
                        LdoData->BusInterface.Context,
                         PCI_WHICHSPACE_CONFIG, //READ
                         buffer,
                         FIELD_OFFSET(PCI_COMMON_CONFIG, VendorID),
                         LANGFORD_PCI_HDR_LENGTH);

    if (bytesRead != LANGFORD_PCI_HDR_LENGTH) 
	{
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Is this our device?
    //

    if (pPciConfig->VendorID != LANGFORD_PCI_VENDOR_ID ||
        pPciConfig->DeviceID != LANGFORD_PCI_DEVICE_ID)
    {
		//return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // save TRACE_LEVEL_INFORMATION from config space
    //
    LdoData->RevisionID = pPciConfig->RevisionID;

    return status;
}