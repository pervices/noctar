
#ifndef _LANGFORD_DEV_H
#define _LANGFORD_DEV_H

#define LANGFORD_DOS_DEVICE_NAME L"\\DosDevices\\Langford"
// PCI config space including the Device Specific part of it/
#define LANGFORD_PCI_HDR_LENGTH         0xe2

typedef struct _LDO_DATA
{
	BUS_INTERFACE_STANDARD BusInterface;
	UCHAR RevisionID;

	WDFDEVICE WdfDevice;
	WDFQUEUE WriteQueue;
	WDFQUEUE ReadQueue;
	WDFQUEUE IoControlQueue;
	//WDFINTERRUPT WdfInterrupt;
	WDFDMAENABLER WdfDmaEnabler;
	WDFDMATRANSACTION WdfReadTransaction;
	WDFDMATRANSACTION WdfWriteTransaction;

	KEVENT WriteRequestEvent;
	KEVENT ReadRequestEvent;
	KEVENT TerminatedThreadEvent;

	PETHREAD ReadRequestThread;
	PETHREAD WriteRequestThread;

	ULONG ReadDescNumber;
	ULONG WriteDescNumber;

	PHYSICAL_ADDRESS Bar0PhysAddress;
	PHYSICAL_ADDRESS Bar1PhysAddress;

	ULONG Bar0MemoryLength;
	ULONG Bar1MemoryLength;

	PUCHAR Bar0LogicalAddress;
	PUCHAR Bar1LogicalAddress;

	ULONG BusTranslationMask;

} LDO_DATA, *PLDO_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(LDO_DATA, langfordDeviceGetData)

EVT_WDF_DRIVER_DEVICE_ADD langfordEvtDeviceAdd;
EVT_WDF_DEVICE_CONTEXT_CLEANUP langfordEvtDeviceContextCleanup;
EVT_WDF_DEVICE_D0_ENTRY langfordEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT langfordEvtDeviceD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE langfordEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE langfordEvtDeviceReleaseHardware;

NTSTATUS langfordGetDeviceInformation(IN PLDO_DATA LdoData);


#endif