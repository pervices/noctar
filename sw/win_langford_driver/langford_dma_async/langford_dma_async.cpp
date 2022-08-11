// langford_dma_async.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <conio.h>
#include <windows.h>
#include <winioctl.h>
#include <atlbase.h>
#include <atlstr.h>

#define USER_MODE
#include "..\langford_windows_driver\langford_io.h"

#define DMARXBUFFS			32
#define DMABUFFSIZE			0x1000 

HANDLE OpenLangfordDevice()
{
	return CreateFile(LANGFORD_LINK_DEVICE_NAME ,
		GENERIC_READ|GENERIC_WRITE,
		0,NULL,OPEN_EXISTING,FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,NULL);
}

void CloseLangfordDevice(HANDLE hDevice)
{
	CloseHandle(hDevice);
}

typedef struct BUFFER_DATA
{
	OVERLAPPED overLapped;
	LPVOID lpData;
	BUFFER_DATA()
	{
		ZeroMemory(&overLapped,sizeof(OVERLAPPED));
		lpData = NULL;
	}
}BUFFER_DATA, *PBUFFER_DATA;

#define MAX_ASYNC_BUFFERS  50
BUFFER_DATA BuffersArray[MAX_ASYNC_BUFFERS];

typedef enum _COMPLETION_KEY
{
	BUFFER_FILLED,
	ABORT
}COMPLETION_KEY;

HANDLE hDevice;
HANDLE hCompletionPort; 

DWORD WINAPI ReadCompletionWorker(LPVOID Param)
{
	BOOL  bSuccess;
	DWORD bytesTransferred;
	DWORD key;
	LPOVERLAPPED overlappedComp;
	PBUFFER_DATA pBufferData;

	while(TRUE)
	{
		// Worker Thread will be waiting on IO Completion Port 
		// to process IO Completion packets.
		bSuccess = GetQueuedCompletionStatus(hCompletionPort,
			&bytesTransferred,
			&key,
			&overlappedComp,
			(DWORD)-1);

		if( !bSuccess && (overlappedComp == NULL))
		{
			printf("GetQueuedCompletionStatus Failed::\n");
			return 0;
		}

		// Typecasting the OVERLAPPED structure to BUFFER_DATA, so that we can
		// have an access to data which has been read() from file.
		pBufferData = (PBUFFER_DATA)overlappedComp;

		if (key == BUFFER_FILLED)
		{
			//print the data
			//printf("reading %d bytes using DMA\n", bytesTransferred);
			//print just the first 10
			for(int i = 0; i < 10 /*bytesTransferred*/; i++)
			{
				printf("%x ",((PCHAR)pBufferData->lpData)[i]);
			}

			//enqueue the buffer again
			DWORD BytesRead = DMABUFFSIZE*DMARXBUFFS;
			BOOL bSuccess = ReadFile(hDevice,pBufferData->lpData,BytesRead,&BytesRead,&pBufferData->overLapped);
			if(!bSuccess && (GetLastError() != ERROR_IO_PENDING)) 
			{
				printf("error queuing read operation\n");
			}
		}
		else if(key == ABORT)
		{
			break;
		}
	}

	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	//read from the device async using completion port

	hDevice = OpenLangfordDevice();
	if (hDevice == INVALID_HANDLE_VALUE) 
	{
		printf("error opening device\n");
		return 1;
	}

	int i;
	for(i = 0; i < MAX_ASYNC_BUFFERS; i++)
	{
		BuffersArray[i].lpData = (PCHAR)VirtualAlloc( NULL,
			DMABUFFSIZE*DMARXBUFFS,
			MEM_COMMIT,
			PAGE_READWRITE);
		if(!BuffersArray[i].lpData)
		{
			printf("error allocating buffer memory\n");
			break;
		}
	}

	if(i > 0)
	{
		//now we have the buffers, then setup the completion port and the thread
		hCompletionPort = CreateIoCompletionPort(hDevice,NULL,BUFFER_FILLED,1);
		if(hCompletionPort != NULL)
		{
			//enqueue MAX_ASYNC_BUFFERS reads
			for(int t = 0; t < i; t++)
			{
				DWORD BytesRead = DMABUFFSIZE*DMARXBUFFS;
				BOOL bSuccess = ReadFile(hDevice,BuffersArray[t].lpData,BytesRead,&BytesRead,&BuffersArray[t].overLapped);
				if(!bSuccess && (GetLastError() != ERROR_IO_PENDING)) 
				{
					printf("error queuing read operation\n");
					break;
				}
			}

			HANDLE hThread = CreateThread(NULL,0,ReadCompletionWorker,NULL,0,NULL);
			if(hThread)
			{
				printf("thread capturing data, press any key to finish\n");

				_getch();

				PostQueuedCompletionStatus(hCompletionPort,0,ABORT,NULL);

				printf("waiting for thread to finish\n");

				WaitForSingleObject(hThread,INFINITE);
			}
			else
			{
				printf("error creating thread: %d\n",GetLastError());
			}
		}
		else
		{
			printf("error creating completion port: %d\n",GetLastError());
		}
	}

	for(int j = 0; i < j; j++)
	{
		VirtualFree(BuffersArray[j].lpData,DMABUFFSIZE*DMARXBUFFS,MEM_FREE);
	}

	return 0;
}

