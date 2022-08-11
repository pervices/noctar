/***********************************************************************
 * This file programs the LMH6521 on board the langford device.
 * 
 * (c) 2013 Per Vices Corporation
 * See LICENSE.txt for license and copyright information.
 **********************************************************************/

#include <iostream>
#include "lmh6521_regs.hpp"
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "../langford_ioctl.h"


int WriteReg(std::string FileName, uint32_t reg_val);


int main(int argc, char **argv) {
  double gain;
  int channel;
  int rc;
  int verbosity;

  //Initialize
  lmh6521_regs_t lmh6521_regs;

  if ( (argc < 4) || (argc > 5) ){
	  std::cerr << "Usage: " << std::endl;
	  std::cerr << "\t" << argv[0] << " [DEVICE_NAME] [CHANNEL] [GAIN] (OUTPUT)" << std::endl;
	  std::cerr << "[DEVICE_NAME] default is /dev/langford" << std::endl;
	  std::cerr << "[CHANNEL] = {0|1} to program I or Q channel." << std::endl;
	  std::cerr << "[GAIN] (0 <= GAIN <= 31.5), Set desired channel gain (31.5dB range across (-5.5..26)dB )" << std::endl;
	  std::cerr << "[OUTPUT] = {0|1|2} is the (optional) output format (0 = silent (default), 1 = verbose, 2=driver)" << std::endl;
	  return 1;
  };

  if ( (std::string(argv[2]) == "0") ||  (std::string(argv[2]) == "1") ){ channel = atof(argv[2]); }
  
  else {
    std::cerr << "Invalid Channel Selection: Valid values for [CHANNEL] are: {0|1}" << std::endl;
    return 1;
  };

  if ( (atof(argv[3]) >= 0) && (atof(argv[3]) <= 31.5) ) { gain = atof(argv[3]); }
  else {
      std::cerr << "Invalid Gain: Valid values for [GAIN] is limited to set [0,31.5]" << std::endl;
      return 1;
  };
  
  if ( (argc == 5) ){ 
    if (std::string(argv[4]) == "0") { verbosity = 0; }
    else if (std::string(argv[4]) == "1") { verbosity = 1; std::cout.precision(10); }
    else if (std::string(argv[4]) == "2") { verbosity = 2; std::cout.precision(10); }
    else { std::cerr << "Invalid output selection: 0 for verbose mode, 1 for driver"; return 1;};
  }
  else { verbosity = 0; };

  
  gain = lmh6521_regs.set_gain(gain, channel);
  
  rc = WriteReg(argv[1], lmh6521_regs.get_regs());
  
  if (verbosity == 1){
    std::cout << "Programming Channel: " << channel
	      << " => " << gain
	      << std::endl;
    lmh6521_regs.show_regs();
    std::cout << "Return code from io_ctl calls: " << rc << std::endl;
  }
  else if (verbosity == 2){
    std::cout << channel
	      << " " << gain
	      << std::endl;
  };

  return rc;
};


int WriteReg(std::string FileName, uint32_t reg_val) {
	int		DevFD;
	int		val;
	int		rc = 0;
	const unsigned long long	val0 = 0, val1 = 1;
	int		i;

	DevFD = open(FileName.c_str(), O_RDWR);
	if (!DevFD) {
		std::cerr << "Cannot open " << FileName << std::endl;
		return 1;
	};

	rc |= ioctl(DevFD, IOCTL_SET_N7CS, &val0);
	for (i = 0; i < 16; i++) {
		rc |= ioctl(DevFD, IOCTL_SET_N7CLK, &val0);
		val = (reg_val >> (15 - i)) & 0x01;
		rc |= ioctl(DevFD, IOCTL_SET_N7SDI, &val);
		rc |= ioctl(DevFD, IOCTL_SET_N7CLK, &val1);
	};
	rc |= ioctl(DevFD, IOCTL_SET_N7CLK, &val0);
	rc |= ioctl(DevFD, IOCTL_SET_N7SDI, &val0);
	rc |= ioctl(DevFD, IOCTL_SET_N7CS, &val1);

	close(DevFD);

	return rc;
};
