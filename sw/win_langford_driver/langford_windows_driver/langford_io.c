#include "stdinc.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, langfordEvtIoRead)
#pragma alloc_text (PAGE, langfordEvtIoWrite)
#pragma alloc_text (PAGE, langfordEvtIoDeviceControl)
#endif


VOID langfordEvtIoRead (WDFQUEUE Queue, WDFREQUEST Request, size_t Length)
{
	NTSTATUS                status = STATUS_UNSUCCESSFUL;
	PLDO_DATA				ldoData;

	PAGED_CODE();
	//
	// Get the DevExt from the Queue handle
	//
	ldoData = langfordDeviceGetData(WdfIoQueueGetDevice(Queue));

	do 
	{
		//
		// Validate the Length parameter.
		//
		if (Length > DMARXBUFFS*DMABUFFSIZE) 
		{
			status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		//
		// Initialize this new DmaTransaction.
		//
		status = WdfDmaTransactionInitializeUsingRequest(
			ldoData->WdfReadTransaction,
			Request,
			langfordEvtProgramDma,
			WdfDmaDirectionReadFromDevice );

		if(!NT_SUCCESS(status)) 
		{
			break;
		}

		//
		// Execute this DmaTransaction.
		//
		status = WdfDmaTransactionExecute( ldoData->WdfReadTransaction, 
			WDF_NO_CONTEXT);

		if(!NT_SUCCESS(status)) 
		{
			//
			// Couldn't execute this DmaTransaction, so fail Request.
			//
			break;
		}

		//
		// Indicate that Dma transaction has been started successfully.
		// The request will be complete by the Dpc routine when the DMA
		// transaction completes.
		//
		status = STATUS_SUCCESS;

	} 
	 __pragma(warning(suppress: 4127)) while(FALSE);

	//
	// If there are errors, then clean up and complete the Request.
	//
	if (!NT_SUCCESS(status )) 
	{
		WdfDmaTransactionRelease(ldoData->WdfReadTransaction);
		WdfRequestComplete(Request, status);
	}

	return;
}


VOID langfordEvtIoWrite (WDFQUEUE Queue, WDFREQUEST Request, size_t Length)
{
	NTSTATUS                status = STATUS_UNSUCCESSFUL;
	PLDO_DATA				ldoData;

	PAGED_CODE();
	//
	// Get the DevExt from the Queue handle
	//
	ldoData = langfordDeviceGetData(WdfIoQueueGetDevice(Queue));

	do 
	{
		//
		// Validate the Length parameter.
		//
		if (Length > DMARXBUFFS*DMABUFFSIZE) 
		{
			status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		//
		// Initialize this new DmaTransaction.
		//
		status = WdfDmaTransactionInitializeUsingRequest(
			ldoData->WdfReadTransaction,
			Request,
			langfordEvtProgramDma,
			WdfDmaDirectionWriteToDevice);

		if(!NT_SUCCESS(status)) 
		{
			break;
		}

		//
		// Execute this DmaTransaction.
		//
		status = WdfDmaTransactionExecute( ldoData->WdfReadTransaction, 
			WDF_NO_CONTEXT);

		if(!NT_SUCCESS(status)) 
		{
			//
			// Couldn't execute this DmaTransaction, so fail Request.
			//
			break;
		}

		//
		// Indicate that Dma transaction has been started successfully.
		// The request will be complete by the Dpc routine when the DMA
		// transaction completes.
		//
		status = STATUS_SUCCESS;

	} 
	 __pragma(warning(suppress: 4127)) while(FALSE);

	//
	// If there are errors, then clean up and complete the Request.
	//
	if (!NT_SUCCESS(status )) 
	{
		WdfDmaTransactionRelease(ldoData->WdfReadTransaction);
		WdfRequestComplete(Request, status);
	}

	return;
}

VOID langfordEvtIoDeviceControl(IN WDFQUEUE Queue, 
								IN WDFREQUEST Request, 
								IN size_t OutputBufferLength, 
								IN size_t InputBufferLength,
								IN ULONG IoControlCode)
{
	NTSTATUS          status = STATUS_INVALID_DEVICE_REQUEST;
	ULONG_PTR         Information = 0;
	size_t            BufSize;
	PLDO_DATA		  ldoData;

	PAGED_CODE ();

	ldoData = langfordDeviceGetData(WdfIoQueueGetDevice(Queue));

	switch (IoControlCode)
	{
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
	case IOCTL_GET_GPIOin:		case IOCTL_GET_GPIOout:
	case IOCTL_GET_N21at0:		case IOCTL_GET_N22at0:		case IOCTL_GET_N3GAIN:
	case IOCTL_GET_RXDecEn:		case IOCTL_GET_TXIntEn:
	case IOCTL_GET_RXPhase:		case IOCTL_GET_TXPhase:
		{
			if (OutputBufferLength == sizeof(ULONG))
			{
				PULONG Value;
				status = WdfRequestRetrieveOutputBuffer(Request, sizeof(ULONG),&Value, &BufSize);
				if (NT_SUCCESS(status)) 
				{
					status = langfordIo(ldoData,IoControlCode,Value);
					Information = sizeof(LONG);
				}     
			} 
			else 
			{
				status = STATUS_INVALID_PARAMETER;
			}
			break;
		}
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
	case IOCTL_SET_GPIOout:
	case IOCTL_SET_N21at0:		case IOCTL_SET_N22at0:		case IOCTL_SET_N3GAIN:
	case IOCTL_SET_RXDecEn:		case IOCTL_SET_TXIntEn:
	case IOCTL_SET_RXPhase:		case IOCTL_SET_TXPhase:
		{
			if (InputBufferLength == sizeof(ULONG))
			{
				PULONG Value;
				status = WdfRequestRetrieveInputBuffer(Request, sizeof(ULONG),&Value, &BufSize);
				if (NT_SUCCESS(status)) 
				{
					status = langfordIo(ldoData,IoControlCode,Value);
					Information = sizeof(LONG);
				}     
			} 
			else 
			{
				status = STATUS_INVALID_PARAMETER;
			}
			break;
		}
	default:
		break;
	}
	WdfRequestCompleteWithInformation(Request, status, Information);
}


VOID
	langfordRequestComplete(
	IN WDFDMATRANSACTION  DmaTransaction,
	IN NTSTATUS           Status
	)
	/*++

	Routine Description:

	Arguments:

	Return Value:

	--*/
{
	WDFREQUEST         request;
	size_t             bytesTransferred;

	//
	// Get the associated request from the transaction.
	//
	request = WdfDmaTransactionGetRequest(DmaTransaction);

	ASSERT(request);

	//
	// Get the final bytes transferred count.
	//
	bytesTransferred =  WdfDmaTransactionGetBytesTransferred( DmaTransaction );

	WdfDmaTransactionRelease(DmaTransaction);

	//
	// Complete this Request.
	//
	WdfRequestCompleteWithInformation( request, Status, bytesTransferred);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
BOOLEAN
langfordEvtProgramDma(
    IN  WDFDMATRANSACTION       Transaction,
    IN  WDFDEVICE               Device,
    IN  WDFCONTEXT              Context,
    IN  WDF_DMA_DIRECTION       Direction,
    IN  PSCATTER_GATHER_LIST    SgList
    )
/*++

Routine Description:

  The framework calls a driver's EvtProgramDma event callback function
  when the driver calls WdfDmaTransactionExecute and the system has
  enough map registers to do the transfer. The callback function must
  program the hardware to start the transfer. A single transaction
  initiated by calling WdfDmaTransactionExecute may result in multiple
  calls to this function if the buffer is too large and there aren't
  enough map registers to do the whole transfer.


Arguments:

Return Value:

--*/
{
    PLDO_DATA				 ldoData;
	ULONG                    descAddress;
	ULONG                    descNumber;
    UNREFERENCED_PARAMETER( Context );
    UNREFERENCED_PARAMETER( Transaction );

    //
    // Initialize locals
    //
    ldoData = langfordDeviceGetData(Device);

	//from this point there is no way we could detect fails
	//so there is no difference in setting the desc number to the scatter gather list size
	//and wake up the polling threads
	//also the framework will dispatch the request sequentially
	//so one read/write request will no get here until we completed the current
	//this have to change to optimize performance
	//and the access to Read/WriteDescNumber variable must be protected(redesigned) 
    //
	if(Direction ==  WdfDmaDirectionWriteToDevice)
	{
		ldoData->WriteDescNumber = SgList->NumberOfElements;

	}
	else if(Direction == WdfDmaDirectionReadFromDevice)
	{
		ldoData->ReadDescNumber = SgList->NumberOfElements;
	}

    //set the descriptors with the scatter gather list
	//and start the transaction for each one
    //
    for (descNumber = 0; descNumber < SgList->NumberOfElements; descNumber++) 
	{
		/*Write a descriptor*/
		//descAddress = (BuffEnd << DMAPAGEWIDTH) | (DevPrivData->RxDmaBuffsBusAddr[BuffEnd] & ~DevPrivData->BusTranslationMask);
		//we will do our best to make something similar
		if(Direction == WdfDmaDirectionReadFromDevice )
		{
			int i;
			/*Calculate address translation table for Avalon-MM to PCIe translation and write it to device*/
			for (i = 0; i < DMARXBUFFS; i++) 
			{
				//writel(DevPrivData->RxDmaBuffsBusAddr[i] & DevPrivData->BusTranslationMask, DevPrivData->pBar0 + CRA_ATT + i * 8);
				WRITE_LANGFORD_REGISTER_ULONG(ldoData->Bar0LogicalAddress,
					CRA_ATT + i * 8,
					SgList->Elements[descNumber].Address.LowPart & ldoData->BusTranslationMask);

				//writel(0, DevPrivData->pBar0 + CRA_ATT + 4 + i * 8);
				WRITE_LANGFORD_REGISTER_ULONG(ldoData->Bar0LogicalAddress,
					CRA_ATT + 4 + i * 8,
					0);
			}
			//we set the device to write to the descriptors
			descAddress = (descNumber << DMAPAGEWIDTH) | (SgList->Elements[descNumber].Address.LowPart & ~ldoData->BusTranslationMask);
			WRITE_LANGFORD_REGISTER_ULONG(ldoData->Bar1LogicalAddress,WRDESCWRADDR,descAddress);
			WRITE_LANGFORD_REGISTER_ULONG(ldoData->Bar1LogicalAddress,WRDESCLEN,SgList->Elements[descNumber].Length);
			WRITE_LANGFORD_REGISTER_ULONG(ldoData->Bar1LogicalAddress,WRDESCCTRL,(ULONG)DESC_CTRL_GO);
		}
		else if(Direction == WdfDmaDirectionWriteToDevice )
		{
			int i;
			for (i = 0; i < DMATXBUFFS; i++) 
			{
				//writel(DevPrivData->TxDmaBuffsBusAddr[i] & DevPrivData->BusTranslationMask, DevPrivData->pBar0 + CRA_ATT + (i + DMARXBUFFS) * 8);
				WRITE_LANGFORD_REGISTER_ULONG(ldoData->Bar0LogicalAddress,
					CRA_ATT + (i + DMARXBUFFS) * 8,
					SgList->Elements[descNumber].Address.LowPart & ldoData->BusTranslationMask);

				//writel(0, DevPrivData->pBar0 + CRA_ATT + 4 + (i + DMARXBUFFS) * 8);
				WRITE_LANGFORD_REGISTER_ULONG(ldoData->Bar0LogicalAddress,
					CRA_ATT + 4 + (i + DMARXBUFFS) * 8,
					0);
			}
			//we set the device to read from the descriptors
			descAddress = (descNumber << DMAPAGEWIDTH) | (SgList->Elements[descNumber].Address.LowPart & ~ldoData->BusTranslationMask);
			WRITE_LANGFORD_REGISTER_ULONG(ldoData->Bar1LogicalAddress,RDDESCRDADDR,descAddress);
			WRITE_LANGFORD_REGISTER_ULONG(ldoData->Bar1LogicalAddress,RDDESCLEN,SgList->Elements[descNumber].Length);
			WRITE_LANGFORD_REGISTER_ULONG(ldoData->Bar1LogicalAddress,RDDESCCTRL,(ULONG)DESC_CTRL_GO);
		}
    }

	if(Direction == WdfDmaDirectionReadFromDevice )
	{
		KeSetEvent(&ldoData->ReadRequestEvent,IO_NO_INCREMENT,FALSE);
	}
	else
	{
		KeSetEvent(&ldoData->WriteRequestEvent,IO_NO_INCREMENT,FALSE);
	}

    return TRUE;
}