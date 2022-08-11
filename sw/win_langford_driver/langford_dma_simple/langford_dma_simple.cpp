// langford_dma_simple.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
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
	return CreateFile(LANGFORD_LINK_DEVICE_NAME ,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
}

void CloseLangfordDevice(HANDLE hDevice)
{
	CloseHandle(hDevice);
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc =! 2) 
	{
		printf("Usage: read,write\n");
		return 1;
	}

	CAtlString CmdStr = argv[1];

	if(CmdStr == L"read")
	{
		HANDLE hDevice = OpenLangfordDevice();
		if (hDevice == INVALID_HANDLE_VALUE) 
		{
			printf("error opening device\n");
			return 1;
		}

		PCHAR Buffer = new CHAR[DMABUFFSIZE*DMARXBUFFS];
		if(!Buffer)
		{
			printf("error allocating buffer memory\n");
			return 1;
		}

		DWORD BytesRead = 0;
		BOOL ReadResult = ReadFile(hDevice,Buffer,DMARXBUFFS*DMABUFFSIZE,&BytesRead,NULL);
		if(ReadResult)
		{
			printf("reading %d bytes using DMA\n", BytesRead);
			for(int i = 0; i < BytesRead; i++)
			{
				printf("%x ",Buffer[i]);
			}
		}
		else
		{
			printf("error: %d",GetLastError());
		}

		return 0;
	}

	if(CmdStr == L"write")
	{
		HANDLE hDevice = OpenLangfordDevice();
		if (hDevice == INVALID_HANDLE_VALUE) 
		{
			printf("error opening device\n");
			return 1;
		}

		PCHAR Buffer = new CHAR[DMABUFFSIZE*DMARXBUFFS];
		if(!Buffer)
		{
			printf("error allocating buffer memory\n");
			return 1;
		}

		DWORD BytesWritten = 0;
		BOOL ReadResult = WriteFile(hDevice,Buffer,DMARXBUFFS*DMABUFFSIZE,&BytesWritten,NULL);
		if(ReadResult)
		{
			printf("writing %d bytes using DMA\n", BytesWritten);
			for(int i = 0; i < BytesWritten; i++)
			{
				printf("%x ",Buffer[i]);
			}
		}
		else
		{
			printf("error: %d",GetLastError());
		}

		return 0;
	}

	printf("Usage: read,write\n");

	return 0;
}

