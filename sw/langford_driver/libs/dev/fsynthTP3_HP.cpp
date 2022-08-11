#include <iostream>
#include <string>
#include "adf4351_regs.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include "fsynthTP3_HP.hpp"
#include "../../langford_ioctl.h"


int	SetDDCFreq(std::string FileName, int Channel, double ActualFreq, double TargetFreq);
int WriteReg(std::string FileName, int Channel, uint32_t reg_val);


int main(int argc, char **argv) {
  bool debug = true; //debug mode results in verbose output.
  unsigned long TargetFreq;
//  unsigned long RefIn=156250000; //Set Reference Input in Hz
  unsigned long RefIn=125000000; //Set Reference Input in Hz
  unsigned int chan_mod = 125; // Modulus used to figure Channel Spacing
  double ActualFreq;
  int	Channel, rc, i;

	if (argc != 4) {
		std::cerr << "Usage:" << std::endl;
		std::cerr << "\t" << argv[0] << " device_name channel target_freq" << std::endl;
		std::cerr << "channel = 1 corresponds to N61CE, N61DATA, N61CLK and N61LE" << std::endl;
		std::cerr << "channel = 2 corresponds to N62CE, N62DATA, N62CLK and N62LE" << std::endl;
		return 1;
	};

	Channel = strtoul(argv[2], 0, 0);
	TargetFreq = atof(argv[3]);

  //Initialize
  fsynth_t fsynth;
  
   if (debug == true){
    //Default Constructor for TP3 (initializes at 555MHz).
    std::cout 
    << "\nDefault Constructor, Fout = 555MHz @ Refin = " << (float)RefIn/1e6 
    << "MHz, using MOD=" << chan_mod 
    << std::endl;
    fsynth.show_regs();
   };
   

  //Get Target Frequency (in MHz).
   if (debug == true){
    std::cout 
    << "\nRequesting Desired Frequency (@ Refin = " << (float)RefIn/1e6 
    << "MHz) in MHz: " << std::dec << TargetFreq / 1000000 
    << ", using MOD=" << chan_mod 
    << std::endl;
   };
   
   //  std::cin >> TargetFreq;
   //  TargetFreq = TargetFreq*1000000; //To Hz.

  ActualFreq = fsynth.set_freq(TargetFreq, RefIn, chan_mod, debug);
  
   if (debug == true){
    fsynth.show_regs();
   };

  //Write register content
  rc = 0;
  for (i = 0; i < 6; i++) {
	rc |= WriteReg(argv[1], Channel, fsynth.get_reg(i));
  };

  //Compensate for the remaining frequency difference in DSP
  rc |= SetDDCFreq(argv[1], Channel, ActualFreq, TargetFreq);

  if (debug == true){
    std::cout << "Return code from io_ctl calls: " << rc << std::endl;
  };
  
  return rc;
};



int	SetDDCFreq(std::string FileName, int Channel, double ActualFreq, double TargetFreq) {
	int		DevFD;
	int		SET_Phase;
	int		SET_RevFreq;
	int		RevFreq = (ActualFreq > TargetFreq);
	int		Phase;
	int		rc = 0;

	if (Channel == 1) {
		SET_Phase = IOCTL_SET_RXPhase;
		SET_RevFreq = IOCTL_SET_RXRevFreq;
	} else if (Channel == 2) {
		SET_Phase = IOCTL_SET_TXPhase;
		SET_RevFreq = IOCTL_SET_TXRevFreq;
	} else {
		std::cerr << "Invalid selection for channel" << std::endl;
		return 1;
	};

	DevFD = open(FileName.c_str(), O_RDWR);
	if (!DevFD) {
		std::cerr << "Cannot open " << FileName << std::endl;
		return 1;
	};

	Phase = fabs(ActualFreq - TargetFreq) / 125e6 * 0xffffffff;
	rc |= ioctl(DevFD, SET_Phase, &Phase);
	rc |= ioctl(DevFD, SET_RevFreq, &RevFreq);

	close(DevFD);
	return rc;
};


int WriteReg(std::string FileName, int Channel, uint32_t reg_val) {
	int		DevFD;
	int		val;
	int		rc = 0;
	const unsigned long long	val0 = 0, val1 = 1;
	int		NCE, NDATA, NCLK, NLE;
	int		i;

	if (Channel == 1) {
		NCE = IOCTL_SET_N61CE;
		NDATA = IOCTL_SET_N61DATA;
		NCLK = IOCTL_SET_N61CLK;
		NLE = IOCTL_SET_N61LE;
	} else if (Channel == 2) {
		NCE = IOCTL_SET_N62CE;
		NDATA = IOCTL_SET_N62DATA;
		NCLK = IOCTL_SET_N62CLK;
		NLE = IOCTL_SET_N62LE;
	} else {
		std::cerr << "Invalid selection for channel" << std::endl;
		return 1;
	};

	DevFD = open(FileName.c_str(), O_RDWR);
	if (!DevFD) {
		std::cerr << "Cannot open " << FileName << std::endl;
		return 1;
	};

	rc |= ioctl(DevFD, NLE, &val0);
	rc |= ioctl(DevFD, NCE, &val1);
	for (i = 0; i < 32; i++) {
		rc |= ioctl(DevFD, NCLK, &val0);
		val = (reg_val >> (31 - i)) & 0x00000001;
		rc |= ioctl(DevFD, NDATA, &val);
		rc |= ioctl(DevFD, NCLK, &val1);
	};
	rc |= ioctl(DevFD, NCLK, &val0);
	rc |= ioctl(DevFD, NDATA, &val0);
	rc |= ioctl(DevFD, NLE, &val1);
	rc |= ioctl(DevFD, NLE, &val0);

	close(DevFD);
	return rc;
};
