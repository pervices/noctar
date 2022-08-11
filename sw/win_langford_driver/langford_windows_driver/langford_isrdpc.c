#include "stdinc.h"

BOOLEAN langfordEvtInterruptIsr(IN WDFINTERRUPT Interrupt, IN ULONG MessageID)
{
	UNREFERENCED_PARAMETER(Interrupt);
	UNREFERENCED_PARAMETER(MessageID);

	return 1;
}

VOID langfordEvtInterruptDpc(IN WDFINTERRUPT WdfInterrupt, IN WDFOBJECT WdfDevice)
{
	UNREFERENCED_PARAMETER(WdfInterrupt);
	UNREFERENCED_PARAMETER(WdfDevice);
}

NTSTATUS langfordEvtInterruptEnable(IN WDFINTERRUPT Interrupt, IN WDFDEVICE AssociatedDevice)
{
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(Interrupt);
	UNREFERENCED_PARAMETER(AssociatedDevice);

	return status;
}

NTSTATUS langfordEvtInterruptDisable(IN WDFINTERRUPT Interrupt, IN WDFDEVICE AssociatedDevice)
{
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(Interrupt);
	UNREFERENCED_PARAMETER(AssociatedDevice);

	return status;
}

VOID ReadRequestProcessor( IN PVOID StartContext )
{
	ULONG DescLevel,DescComp;
	PVOID WaitEvents[2];
	PLDO_DATA ldoData = (PLDO_DATA)StartContext;

	WaitEvents[0] = &ldoData->ReadRequestEvent;
	WaitEvents[1] = &ldoData->TerminatedThreadEvent;

	PAGED_CODE ();

	__pragma(warning(suppress: 4127)) while(TRUE)
	{
		// wait any one of the 2 events to become signaled
		NTSTATUS status = KeWaitForMultipleObjects(2,WaitEvents,WaitAny,Executive,KernelMode,FALSE,NULL,NULL);

		if(status == STATUS_WAIT_0)
		{
			NTSTATUS TimeOutStatus;
			LARGE_INTEGER TimeOut;
			TimeOut.QuadPart = -1;

			do 
			{
				//read descriptor level
				//DescLevel = readl(DevPrivData->pBar1 + WRCSRFILLLEVEL) & 0x0000ffff;
				DescLevel = READ_LANGFORD_REGISTER_ULONG(ldoData->Bar1LogicalAddress,WRCSRFILLLEVEL) & 0x0000FFFF ;
				DescComp = ldoData->ReadDescNumber /*presumably the number of descriptor we sent to device*/ - DescLevel;
				if(DescComp == ldoData->ReadDescNumber)
				{
					//complete the request
					BOOLEAN transactionComplete;
					//
					// Indicate this DMA operation has completed:
					// This may drive the transfer on the next packet if
					// there is still data to be transfered in the request.
					// 
					transactionComplete = WdfDmaTransactionDmaCompleted( ldoData->WdfReadTransaction,
						&status );

					if (transactionComplete) 
					{
						//
						// Complete this DmaTransaction.
						//
						langfordRequestComplete(ldoData->WdfReadTransaction,status);
					}

					break;
				}
				TimeOutStatus = KeWaitForSingleObject(&ldoData->TerminatedThreadEvent,Executive,KernelMode,FALSE,&TimeOut);
			} 
			while(TimeOutStatus == STATUS_TIMEOUT);
		}
		else if(status == STATUS_WAIT_1)
		{
			break;
		}
	}

	PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID WriteRequestProcessor( IN PVOID StartContext )
{
	ULONG DescLevel,DescComp;
	PVOID WaitEvents[2];
	PLDO_DATA ldoData = (PLDO_DATA)StartContext;

	WaitEvents[0] = &ldoData->WriteRequestEvent;
	WaitEvents[1] = &ldoData->TerminatedThreadEvent;

	PAGED_CODE ();

	__pragma(warning(suppress: 4127)) while(TRUE)
	{
		// wait any one of the 2 events to become signaled
		NTSTATUS status = KeWaitForMultipleObjects(2,WaitEvents,WaitAny,Executive,KernelMode,FALSE,NULL,NULL);

		if(status == STATUS_WAIT_0)
		{
			NTSTATUS TimeOutStatus;
			LARGE_INTEGER TimeOut;
			TimeOut.QuadPart = -1;

			do 
			{
				//write descriptor level
				//DescLevel = readl(DevPrivData->pBar1 + RDCSRFILLLEVEL) & 0x0000ffff;
				DescLevel = READ_LANGFORD_REGISTER_ULONG(ldoData->Bar1LogicalAddress,RDCSRFILLLEVEL) & 0x0000FFFF ;
				DescComp = ldoData->ReadDescNumber /*presumably the number of descriptor we sent to device*/ - DescLevel;
				if(DescComp == ldoData->ReadDescNumber)
				{
					//complete the request
					BOOLEAN transactionComplete;
					//
					// Indicate this DMA operation has completed:
					// This may drive the transfer on the next packet if
					// there is still data to be transfered in the request.
					// 
					transactionComplete = WdfDmaTransactionDmaCompleted( ldoData->WdfWriteTransaction,
						&status );

					if (transactionComplete) 
					{
						//
						// Complete this DmaTransaction.
						//
						langfordRequestComplete(ldoData->WdfWriteTransaction,status);
					}

					break;
				}
				TimeOutStatus = KeWaitForSingleObject(&ldoData->TerminatedThreadEvent,Executive,KernelMode,FALSE,&TimeOut);
			} 
			while(TimeOutStatus == STATUS_TIMEOUT);
		}
		else if(status == STATUS_WAIT_1)
		{
			break;
		}
	}

	PsTerminateSystemThread(STATUS_SUCCESS);
}
