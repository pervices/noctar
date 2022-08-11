
#ifndef _LANGFORD_DRV_H
#define _LANGFORD_DRV_H


typedef struct _DRIVER_CONTEXT
{
	WDFLOOKASIDE RecvLookaside;
} DRIVER_CONTEXT, * PDRIVER_CONTEXT;


WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DRIVER_CONTEXT, langfordDriverGetData)

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD langfordEvtDriverUnload;
EVT_WDF_OBJECT_CONTEXT_CLEANUP langfordEvtDriverContextCleanup;


#endif