// langford_util.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <winioctl.h>
#include <atlbase.h>
#include <atlstr.h>

#define USER_MODE
#include "..\langford_windows_driver\langford_io.h"

HANDLE OpenLangfordDevice()
{
	return CreateFile(LANGFORD_LINK_DEVICE_NAME ,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
}

void CloseLangfordDevice(HANDLE hDevice)
{
	CloseHandle(hDevice);
}

BOOL LangfordIoControl(HANDLE hDevice, int Command, int *Value, bool Read)
{
	if(!hDevice)
		return FALSE;

	DWORD Returned;
	if(Read)
	{
		return DeviceIoControl(hDevice,Command,NULL,0,Value,sizeof(ULONG),&Returned,NULL);
	}
	else
	{
		return DeviceIoControl(hDevice,Command,Value,sizeof(ULONG),NULL,0,&Returned,NULL);
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	int		Cmd;
	int	    val;
	int		rc = 0;
	CAtlString	CmdStr;

	if ((argc < 3) || (argc > 4))
	{
		printf("Usage:\n");
		printf("\t device_name Pin [Value]\n");
		printf("Pin can be any of:\n");
		printf("\tN21at0, N22at0, N3GAIN\n");
		printf("\tN61CE, N61DATA, N61CLK, N61LE, N61MUXOUT, N61LD\n");
		printf("\tN62CE, N62DATA, N62CLK, N62LE, N62MUXOUT, N62LD\n");
		printf("\tN3ENB, N3HILO, NASRxA, NASTxA\n");
		printf("\tN7CLK, N7CS, N7SDI, N7SDO\n");
		printf("\tDACIQSel, DACReset, DACCSB, DACSDIO, DACSClk, DACSDO\n");
		printf("\tADCAnOE, ADCAPD, ADCASClk, ADCASDIO, ADCAnCS\n");
		printf("\tADCBnOE, ADCBPD, ADCBSClk, ADCBSDIO, ADCBnCS\n");
		printf("\tRXPhase, TXPhase\n");
		printf("\tRXDecEn, TXIntEn\n");
		printf("\tRXRevFreq, TXRevFreq\n");
		printf("\tRxDspEn, TxDspEn\n");
		printf("\tNRXTALSEL, NRVCOSEL\n");
		printf("\tRxFifoClr, TxFifoClr\n");
		printf("\tRxIsSigned, TxIsSigned\n");
		printf("\tRxBaseband\n");
		printf("\tGPIOout, GPIOin\n");
		printf("To set a value, enter a numerical value in the Value field.\n");
		printf("To read a value, leave Value blank.\n");
		
		return 1;
	};

	HANDLE hDevice = OpenLangfordDevice();
	if (hDevice == INVALID_HANDLE_VALUE) 
	{
		printf("error opening device\n");
		return 1;
	}

	CmdStr = argv[2];

	/*Command handler*/
	if (argc == 3)
	{
		//Read a value
		if		(CmdStr == "N21at0")	{Cmd = IOCTL_GET_N21at0;}
		else if	(CmdStr == "N22at0")	{Cmd = IOCTL_GET_N22at0;}
		else if	(CmdStr == "N3GAIN")	{Cmd = IOCTL_GET_N3GAIN;}
		else if	(CmdStr == "N61CE")		{Cmd = IOCTL_GET_N61CE;}
		else if	(CmdStr == "N61DATA")	{Cmd = IOCTL_GET_N61DATA;}
		else if	(CmdStr == "N61CLK")	{Cmd = IOCTL_GET_N61CLK;}
		else if	(CmdStr == "N61LE")		{Cmd = IOCTL_GET_N61LE;}
		else if	(CmdStr == "N62CE")		{Cmd = IOCTL_GET_N62CE;}
		else if	(CmdStr == "N62DATA")	{Cmd = IOCTL_GET_N62DATA;}
		else if	(CmdStr == "N62CLK")	{Cmd = IOCTL_GET_N62CLK;}
		else if	(CmdStr == "N62LE")		{Cmd = IOCTL_GET_N62LE;}
		else if	(CmdStr == "N3ENB")		{Cmd = IOCTL_GET_N3ENB;}
		else if	(CmdStr == "N3HILO")	{Cmd = IOCTL_GET_N3HILO;}
		else if	(CmdStr == "NASRxA")	{Cmd = IOCTL_GET_NASRxA;}
		else if	(CmdStr == "NASTxA")	{Cmd = IOCTL_GET_NASTxA;}
		else if	(CmdStr == "N7CLK")		{Cmd = IOCTL_GET_N7CLK;}
		else if	(CmdStr == "N7CS")		{Cmd = IOCTL_GET_N7CS;}
		else if	(CmdStr == "N7SDI")		{Cmd = IOCTL_GET_N7SDI;}
		else if	(CmdStr == "N61MUXOUT")	{Cmd = IOCTL_GET_N61MUXOUT;}
		else if	(CmdStr == "N61LD")		{Cmd = IOCTL_GET_N61LD;}
		else if	(CmdStr == "N62MUXOUT")	{Cmd = IOCTL_GET_N62MUXOUT;}
		else if	(CmdStr == "N62LD")		{Cmd = IOCTL_GET_N62LD;}
		else if	(CmdStr == "N7SDO")		{Cmd = IOCTL_GET_N7SDO;}
		else if	(CmdStr == "DACIQSel")	{Cmd = IOCTL_GET_DACIQSel;}
		else if	(CmdStr == "DACReset")	{Cmd = IOCTL_GET_DACReset;}
		else if	(CmdStr == "DACCSB")	{Cmd = IOCTL_GET_DACCSB;}
		else if	(CmdStr == "DACSDIO")	{Cmd = IOCTL_GET_DACSDIO;}
		else if	(CmdStr == "DACSClk")	{Cmd = IOCTL_GET_DACSClk;}
		else if	(CmdStr == "DACSDO")	{Cmd = IOCTL_GET_DACSDO;}
		else if	(CmdStr == "ADCAnOE")	{Cmd = IOCTL_GET_ADCAnOE;}
		else if	(CmdStr == "ADCAPD")	{Cmd = IOCTL_GET_ADCAPD;}
		else if	(CmdStr == "ADCASClk")	{Cmd = IOCTL_GET_ADCASClk;}
		else if	(CmdStr == "ADCASDIO")	{Cmd = IOCTL_GET_ADCASDIO;}
		else if	(CmdStr == "ADCAnCS")	{Cmd = IOCTL_GET_ADCAnCS;}
		else if	(CmdStr == "ADCBnOE")	{Cmd = IOCTL_GET_ADCBnOE;}
		else if	(CmdStr == "ADCBPD")	{Cmd = IOCTL_GET_ADCBPD;}
		else if	(CmdStr == "ADCBSClk")	{Cmd = IOCTL_GET_ADCBSClk;}
		else if	(CmdStr == "ADCBSDIO")	{Cmd = IOCTL_GET_ADCBSDIO;}
		else if	(CmdStr == "ADCBnCS")	{Cmd = IOCTL_GET_ADCBnCS;}
		else if	(CmdStr == "RXPhase")	{Cmd = IOCTL_GET_RXPhase;}
		else if	(CmdStr == "TXPhase")	{Cmd = IOCTL_GET_TXPhase;}
		else if	(CmdStr == "RXDecEn")	{Cmd = IOCTL_GET_RXDecEn;}
		else if	(CmdStr == "TXIntEn")	{Cmd = IOCTL_GET_TXIntEn;}
		else if	(CmdStr == "RXRevFreq")	{Cmd = IOCTL_GET_RXRevFreq;}
		else if	(CmdStr == "TXRevFreq")	{Cmd = IOCTL_GET_TXRevFreq;}
		else if	(CmdStr == "RxDspEn")	{Cmd = IOCTL_GET_RxDspEn;}
		else if	(CmdStr == "TxDspEn")	{Cmd = IOCTL_GET_TxDspEn;}
		else if	(CmdStr == "NRXTALSEL")	{Cmd = IOCTL_GET_NRXTALSEL;}
		else if	(CmdStr == "NRVCOSEL")	{Cmd = IOCTL_GET_NRVCOSEL;}
		else if	(CmdStr == "RxFifoClr")	{Cmd = IOCTL_GET_RxFifoClr;}
		else if	(CmdStr == "TxFifoClr")	{Cmd = IOCTL_GET_TxFifoClr;}
		else if	(CmdStr == "RxIsSigned"){Cmd = IOCTL_GET_RxIsSigned;}
		else if	(CmdStr == "TxIsSigned"){Cmd = IOCTL_GET_TxIsSigned;}
		else if (CmdStr == "RxBaseband"){Cmd = IOCTL_GET_RxBaseband;}
		else if	(CmdStr == "GPIOin"){Cmd = IOCTL_GET_GPIOin;}
		else if	(CmdStr == "GPIOout"){Cmd = IOCTL_GET_GPIOout;}
		else 
		{
			printf("Cannot read pin\n");
			return 1;
		}

		rc = LangfordIoControl(hDevice,Cmd, &val, true);
	} 
	else if (argc == 4) 
	{
		//Write a value
		if		(CmdStr == "N21at0")	{Cmd = IOCTL_SET_N21at0;}
		else if	(CmdStr == "N22at0")	{Cmd = IOCTL_SET_N22at0;}
		else if	(CmdStr == "N3GAIN")	{Cmd = IOCTL_SET_N3GAIN;}
		else if	(CmdStr == "N61CE")		{Cmd = IOCTL_SET_N61CE;}
		else if	(CmdStr == "N61DATA")	{Cmd = IOCTL_SET_N61DATA;}
		else if	(CmdStr == "N61CLK")	{Cmd = IOCTL_SET_N61CLK;}
		else if	(CmdStr == "N61LE")		{Cmd = IOCTL_SET_N61LE;}
		else if	(CmdStr == "N62CE")		{Cmd = IOCTL_SET_N62CE;}
		else if	(CmdStr == "N62DATA")	{Cmd = IOCTL_SET_N62DATA;}
		else if	(CmdStr == "N62CLK")	{Cmd = IOCTL_SET_N62CLK;}
		else if	(CmdStr == "N62LE")		{Cmd = IOCTL_SET_N62LE;}
		else if	(CmdStr == "N3ENB")		{Cmd = IOCTL_SET_N3ENB;}
		else if	(CmdStr == "N3HILO")	{Cmd = IOCTL_SET_N3HILO;}
		else if	(CmdStr == "NASRxA")	{Cmd = IOCTL_SET_NASRxA;}
		else if	(CmdStr == "NASTxA")	{Cmd = IOCTL_SET_NASTxA;}
		else if	(CmdStr == "N7CLK")		{Cmd = IOCTL_SET_N7CLK;}
		else if	(CmdStr == "N7CS")		{Cmd = IOCTL_SET_N7CS;}
		else if	(CmdStr == "N7SDI")		{Cmd = IOCTL_SET_N7SDI;}
		else if	(CmdStr == "DACIQSel")	{Cmd = IOCTL_SET_DACIQSel;}
		else if	(CmdStr == "DACReset")	{Cmd = IOCTL_SET_DACReset;}
		else if	(CmdStr == "DACCSB")	{Cmd = IOCTL_SET_DACCSB;}
		else if	(CmdStr == "DACSDIO")	{Cmd = IOCTL_SET_DACSDIO;}
		else if	(CmdStr == "DACSClk")	{Cmd = IOCTL_SET_DACSClk;}
		else if	(CmdStr == "DACSDO")	{Cmd = IOCTL_SET_DACSDO;}
		else if	(CmdStr == "ADCAnOE")	{Cmd = IOCTL_SET_ADCAnOE;}
		else if	(CmdStr == "ADCAPD")	{Cmd = IOCTL_SET_ADCAPD;}
		else if	(CmdStr == "ADCASClk")	{Cmd = IOCTL_SET_ADCASClk;}
		else if	(CmdStr == "ADCASDIO")	{Cmd = IOCTL_SET_ADCASDIO;}
		else if	(CmdStr == "ADCAnCS")	{Cmd = IOCTL_SET_ADCAnCS;}
		else if	(CmdStr == "ADCBnOE")	{Cmd = IOCTL_SET_ADCBnOE;}
		else if	(CmdStr == "ADCBPD")	{Cmd = IOCTL_SET_ADCBPD;}
		else if	(CmdStr == "ADCBSClk")	{Cmd = IOCTL_SET_ADCBSClk;}
		else if	(CmdStr == "ADCBSDIO")	{Cmd = IOCTL_SET_ADCBSDIO;}
		else if	(CmdStr == "ADCBnCS")	{Cmd = IOCTL_SET_ADCBnCS;}
		else if	(CmdStr == "RXPhase")	{Cmd = IOCTL_SET_RXPhase;}
		else if	(CmdStr == "TXPhase")	{Cmd = IOCTL_SET_TXPhase;}
		else if	(CmdStr == "RXDecEn")	{Cmd = IOCTL_SET_RXDecEn;}
		else if	(CmdStr == "TXIntEn")	{Cmd = IOCTL_SET_TXIntEn;}
		else if	(CmdStr == "RXRevFreq")	{Cmd = IOCTL_SET_RXRevFreq;}
		else if	(CmdStr == "TXRevFreq")	{Cmd = IOCTL_SET_TXRevFreq;}
		else if	(CmdStr == "RxDspEn")	{Cmd = IOCTL_SET_RxDspEn;}
		else if	(CmdStr == "TxDspEn")	{Cmd = IOCTL_SET_TxDspEn;}
		else if	(CmdStr == "NRXTALSEL")	{Cmd = IOCTL_SET_NRXTALSEL;}
		else if	(CmdStr == "NRVCOSEL")	{Cmd = IOCTL_SET_NRVCOSEL;}
		else if	(CmdStr == "RxFifoClr")	{Cmd = IOCTL_SET_RxFifoClr;}
		else if	(CmdStr == "TxFifoClr")	{Cmd = IOCTL_SET_TxFifoClr;}
		else if	(CmdStr == "RxIsSigned"){Cmd = IOCTL_SET_RxIsSigned;}
		else if	(CmdStr == "TxIsSigned"){Cmd = IOCTL_SET_TxIsSigned;}
		else if (CmdStr == "RxBaseband"){Cmd = IOCTL_SET_RxBaseband;}
		else if	(CmdStr == "GPIOout"){Cmd = IOCTL_SET_GPIOout;}
		else 
		{
			printf("Cannot write pin\n");
			return 1;
		}

		val = wcstoul(argv[3], 0, 0);

		rc = LangfordIoControl(hDevice,Cmd, &val, false);
	}

	CloseLangfordDevice(hDevice);

	if(rc)
		printf("action completed succesfully\n");

	return rc;
}

