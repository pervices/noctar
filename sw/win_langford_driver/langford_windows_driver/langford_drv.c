#include "stdinc.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, langfordEvtDriverContextCleanup)
#pragma alloc_text (PAGE, langfordEvtDriverUnload)
#endif

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_DRIVER_CONFIG config = {0};
	WDF_OBJECT_ATTRIBUTES  attrib;
	WDFDRIVER              driver;
	PDRIVER_CONTEXT        driverContext;

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attrib, DRIVER_CONTEXT);
	attrib.EvtCleanupCallback = langfordEvtDriverContextCleanup;
	WDF_DRIVER_CONFIG_INIT(&config, langfordEvtDeviceAdd);
	config.EvtDriverUnload = langfordEvtDriverUnload;
	status = WdfDriverCreate(DriverObject, RegistryPath, &attrib, &config, &driver);
	driverContext = langfordDriverGetData(driver);

	//DbgBreakPoint();

	/*  TODO - Useful???
	//
    // Create a driver wide lookside list used for allocating memory  for the
    // MP_RFD structure for all device instances (if there are multiple present).
    //
    status = WdfLookasideListCreate(WDF_NO_OBJECT_ATTRIBUTES, // LookAsideAttributes
                                sizeof(MP_RFD),
                                NonPagedPool,
                                WDF_NO_OBJECT_ATTRIBUTES, // MemoryAttributes
                                PCIDRV_POOL_TAG,
                                &driverContext->RecvLookaside
                                );
	*/
	
	return status;
}

VOID langfordEvtDriverContextCleanup(IN WDFOBJECT Driver)
{
	UNREFERENCED_PARAMETER(Driver);
}

VOID langfordEvtDriverUnload(IN WDFDRIVER Driver)
{
	UNREFERENCED_PARAMETER(Driver);

	PAGED_CODE ();
}
