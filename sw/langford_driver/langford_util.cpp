/***********************************************************************
 * Langford Utility
 * 
 * This utility programs, reads, and provides access to, (most) of the 
 * onboard chips and devices. This abstracts bit-banging, and reading 
 * datasheets and provides a straightforward means of adjusting things.
 * 
 * (c) 2013 Per Vices Corporation
 * 
 * See LICENSE.txt for license and copyright information.
 ************************************************************************/

/*To add a new ioctl function, be sure to add it to the help message and the command handler*/

#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include "langford_ioctl.h"

using namespace std;


int main(int argc, char **argv) {
	int		DevFD;
	int		Cmd;
	unsigned long long	val;
	int		rc = 0;
	string	CmdStr;

	if ((argc < 3) || (argc > 4)) {
		cerr << "Usage:" << endl;
		cerr << "\t" << argv[0] << " device_name Pin [Value]" << endl;
		cerr << "Pin can be any of:" << endl;
		cerr << "\tN21at0, N22at0, N3GAIN" << endl;
		cerr << "\tN61CE, N61DATA, N61CLK, N61LE, N61MUXOUT, N61LD" << endl;
		cerr << "\tN62CE, N62DATA, N62CLK, N62LE, N62MUXOUT, N62LD" << endl;
		cerr << "\tN3ENB, N3HILO, NASRxA, NASTxA" << endl;
		cerr << "\tN7CLK, N7CS, N7SDI, N7SDO" << endl;
		cerr << "\tDACIQSel, DACReset, DACCSB, DACSDIO, DACSClk, DACSDO" << endl;
		cerr << "\tADCAnOE, ADCAPD, ADCASClk, ADCASDIO, ADCAnCS" << endl;
		cerr << "\tADCBnOE, ADCBPD, ADCBSClk, ADCBSDIO, ADCBnCS" << endl;
		cerr << "\tRXPhase, TXPhase" << endl;
		cerr << "\tRXDecEn, TXIntEn" << endl;
		cerr << "\tRXRevFreq, TXRevFreq" << endl;
		cerr << "\tRxDspEn, TxDspEn" << endl;
		cerr << "\tNRXTALSEL, NRVCOSEL" << endl;
		cerr << "\tRxFifoClr, TxFifoClr" << endl;
		cerr << "\tRxIsSigned, TxIsSigned" << endl;
                cerr << "\tRxBaseband" << endl;
                cerr << "\tGPIOout, GPIOin" << endl;
                cerr << "\tRxErrorCorrGainQ, RxErrorCorrGainI" << endl;
                cerr << "\tTxErrorCorrGainQ, TxErrorCorrGainI" << endl;
                cerr << "\tRxErrorCorrGroupDelayQ, RxErrorCorrGroupDelayI" << endl;
                cerr << "\tTxErrorCorrGroupDelayQ, TxErrorCorrGroupDelayI" << endl;
                cerr << "\tRxErrorCorrDCOffsetQ, RxErrorCorrDCOffsetI" << endl;
                cerr << "\tTxErrorCorrDCOffsetQ, TxErrorCorrDCOffsetI" << endl;
		cerr << "To set a value, enter a numerical value in the Value field." << endl;
		cerr << "To read a value, leave Value blank." << endl;
		return 1;
	};

	DevFD = open(argv[1], O_RDWR);
	if (!DevFD) {
		cerr << "Cannot open " << argv[1] << endl;
		return 1;
	};

	CmdStr = argv[2];

	/*Command handler*/
	if (argc == 3) {
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
		else if	(CmdStr == "RxErrorCorrGainQ"){Cmd = IOCTL_GET_RxErrorCorrGainQ;}
		else if	(CmdStr == "RxErrorCorrGainI"){Cmd = IOCTL_GET_RxErrorCorrGainI;}
		else if	(CmdStr == "RxErrorCorrGroupDelayQ"){Cmd = IOCTL_GET_RxErrorCorrGroupDelayQ;}
		else if	(CmdStr == "RxErrorCorrGroupDelayI"){Cmd = IOCTL_GET_RxErrorCorrGroupDelayI;}
		else if	(CmdStr == "RxErrorCorrDCOffsetQ"){Cmd = IOCTL_GET_RxErrorCorrDCOffsetQ;}
		else if	(CmdStr == "RxErrorCorrDCOffsetI"){Cmd = IOCTL_GET_RxErrorCorrDCOffsetI;}
		else if	(CmdStr == "TxErrorCorrGainQ"){Cmd = IOCTL_GET_TxErrorCorrGainQ;}
		else if	(CmdStr == "TxErrorCorrGainI"){Cmd = IOCTL_GET_TxErrorCorrGainI;}
		else if	(CmdStr == "TxErrorCorrGroupDelayQ"){Cmd = IOCTL_GET_TxErrorCorrGroupDelayQ;}
		else if	(CmdStr == "TxErrorCorrGroupDelayI"){Cmd = IOCTL_GET_TxErrorCorrGroupDelayI;}
		else if	(CmdStr == "TxErrorCorrDCOffsetQ"){Cmd = IOCTL_GET_TxErrorCorrDCOffsetQ;}
		else if	(CmdStr == "TxErrorCorrDCOffsetI"){Cmd = IOCTL_GET_TxErrorCorrDCOffsetI;}
		else {
			cerr << "Cannot read pin " << CmdStr << endl;
			return 1;
		};
		rc |= ioctl(DevFD, Cmd, &val);
		cout << CmdStr << " is set to " << val << endl;
	} else if (argc == 4) {
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
		else if	(CmdStr == "RxErrorCorrGainQ"){Cmd = IOCTL_SET_RxErrorCorrGainQ;}
		else if	(CmdStr == "RxErrorCorrGainI"){Cmd = IOCTL_SET_RxErrorCorrGainI;}
		else if	(CmdStr == "RxErrorCorrGroupDelayQ"){Cmd = IOCTL_SET_RxErrorCorrGroupDelayQ;}
		else if	(CmdStr == "RxErrorCorrGroupDelayI"){Cmd = IOCTL_SET_RxErrorCorrGroupDelayI;}
		else if	(CmdStr == "RxErrorCorrDCOffsetQ"){Cmd = IOCTL_SET_RxErrorCorrDCOffsetQ;}
		else if	(CmdStr == "RxErrorCorrDCOffsetI"){Cmd = IOCTL_SET_RxErrorCorrDCOffsetI;}
		else if	(CmdStr == "TxErrorCorrGainQ"){Cmd = IOCTL_SET_TxErrorCorrGainQ;}
		else if	(CmdStr == "TxErrorCorrGainI"){Cmd = IOCTL_SET_TxErrorCorrGainI;}
		else if	(CmdStr == "TxErrorCorrGroupDelayQ"){Cmd = IOCTL_SET_TxErrorCorrGroupDelayQ;}
		else if	(CmdStr == "TxErrorCorrGroupDelayI"){Cmd = IOCTL_SET_TxErrorCorrGroupDelayI;}
		else if	(CmdStr == "TxErrorCorrDCOffsetQ"){Cmd = IOCTL_SET_TxErrorCorrDCOffsetQ;}
		else if	(CmdStr == "TxErrorCorrDCOffsetI"){Cmd = IOCTL_SET_TxErrorCorrDCOffsetI;}
		else {
			cerr << "Cannot write pin " << CmdStr << endl;
			return 1;
		};
		val = strtoul(argv[3], 0, 0);
		rc |= ioctl(DevFD, Cmd, &val);
		cout << CmdStr << " set to " << val << endl;
	};

	close(DevFD);

	cout << "Return code from io_ctl calls: " << rc << endl;

	return rc;
};
