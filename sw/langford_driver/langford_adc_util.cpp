/***********************************************************************
 * The langford ADC utility. This utility programs the ADC1210S125 chips
 * on board the langford device.
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
#include "langford_spi.h"


using namespace std;


int main(int argc, char **argv) {
	int		DevFD;
	int		val;
	int		nCS, SClk, SDO;
	int		reg;
	int		rc = 0;
	string	CmdStr;

	if (argc != 5) {
		cerr << "Usage:" << endl;
		cerr << "\t" << argv[0] << " device_name ADC Function Value" << endl;
		cerr << "ADC is either A or B" << endl;
		cerr << "Pin can be any of:" << endl;
		cerr << "\tRaw - Write 3 bytes verbatim" << endl;
		cerr << "\tIntRef - Internal reference attenuation in dB (0 to 6), set to -1 to disable internal reference" << endl;
		cerr << "\tClkSel - You can enable ADC clock interleaving by setting one ADC to ClkSel = 0 and the other ClkSel = 1" << endl;
		return 1;
	};

	//Open character device for ioctl
	DevFD = open(argv[1], O_RDWR);
	if (!DevFD) {
		cerr << "Cannot open " << argv[1] << endl;
		return 1;
	};

	//Find which ADC user requested
	CmdStr = argv[2];
	if (CmdStr == "A") {
		nCS = IOCTL_SET_ADCAnCS;
		SClk = IOCTL_SET_ADCASClk;
		SDO = IOCTL_SET_ADCASDIO;
	} else if (CmdStr == "B") {
		nCS = IOCTL_SET_ADCBnCS;
		SClk = IOCTL_SET_ADCBSClk;
		SDO = IOCTL_SET_ADCBSDIO;
	} else {
		cerr << "Invalid ADC specified: " << CmdStr << endl;
		return 1;
	};

	//Find which function user requested
	CmdStr = argv[3];
	//Parse user value
	val = strtoul(argv[4], 0, 0);
	//Perform basic sanity checking on function and value
	if (CmdStr == "Raw") {
		reg = val & 0x00ffffff;
	} else if (CmdStr == "IntRef") {
		if (val == -1) {
			cout << "Turning off internal reference" << endl;
			reg = 0x000800;
		} else if ((val >= 0) && (val <= 6)) {
			cout << "Setting internal reference attenuation to " << val << "dB" << endl;
			reg = 0x000808 | val;
		} else {
			cerr << "Invalid attenuation specified: " << val << endl;
			return 1;
		};
	} else if (CmdStr == "ClkSel") {
		if ((val >= 0) && (val <= 1)) {
			reg = 0x000601 | (val << 4);
		} else {
			cerr << "Invalid choice for ClkSel, valid choices are 0 or 1" << endl;
			return 1;
		};
	} else {
		cerr << "Invalid function specified: " << CmdStr << endl;
		return 1;
	};

	rc = LangordSpiWrite(DevFD, 3, reg, nCS, SClk, SDO);
	cout << "Writing SPI value 0x" << hex << reg << dec << endl;

	close(DevFD);

	cout << "Return code from io_ctl calls: " << rc << endl;

	return rc;
};
