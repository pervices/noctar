/***********************************************************************
 * This utility programs the ADF4351 frequency synthesizers, and 
 * initializes the Langford Device.
 * 
 * (c) 2013 Per Vices Corporation
 * See LICENSE.txt for license and copyright information.
 **********************************************************************/

#include <iostream>
#include <string>
#include "adf4351_regs.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include "fsynth.hpp"
#include "../langford_ioctl.h"


int	SetDDCFreq(std::string FileName, int Channel, double ActualFreq, double TargetFreq, int verbosity);
int WriteReg(std::string FileName, int Channel, uint32_t reg_val);


int main(int argc, char **argv) {
  unsigned long TargetFreq;
//  unsigned long RefIn=156250000; //Set Reference Input in Hz
  unsigned long RefIn=125000000; //Set Reference Input in Hz
  unsigned int chan_mod = 125; // Modulus used to figure Channel Spacing
  double ActualFreq;
  int	rc, i;
  int verbosity, lowspur;
  
  enum Channel_t{
	rx = 1,
	tx = 2
    };
  Channel_t Channel;

  if ( (argc < 4) || (argc > 6) ){
	  std::cerr << "Frequency Programmer: Phi production." << std::endl;
	  std::cerr << "Usage:" << std::endl;
	  std::cerr << "\t" << argv[0] << " [DEVICE_NAME] [CHANNEL] [TARGET_FREQ] (OUTPUT)" << std::endl;
	  std::cerr << "[DEVICE_NAME] default is /dev/langford" << std::endl;
	  std::cerr << "[CHANNEL] = {tx|rx} to program transmission or recieve branch" << std::endl;
	  std::cerr << "[TARGET_FREQ] is the desired frequency to be tuned, measured in Hertz (Hz). 0 for power down." << std::endl;
	  std::cerr << "[OUTPUT] = {0|1|2} is the (optional) output format (0 = silent (default), 1 = verbose, 2=driver)" << std::endl;
	  std::cerr << "[NOISE MODE] = {0|1} moderates output spurs (0 = low spur (default), 1 = low noise)" << std::endl;
	  //std::cerr << "channel = Tx corresponds to N61CE, N61DATA, N61CLK and N61LE" << std::endl;
	  //std::cerr << "channel = 2 corresponds to N62CE, N62DATA, N62CLK and N62LE" << std::endl;
	  return 1;
  };

  if ( (std::string(argv[2]) == "rx") ||  (int( strtoul(argv[2], 0, 0) ) == 1) ){
    Channel = rx;
  }
  else if ( (std::string(argv[2]) == "tx") ||  (int( strtoul(argv[2], 0, 0) ) == 2) ){
    Channel = tx;
  }
  else {
    std::cerr << "Invalid Channel Selection: Valid values for [CHANNEL] are: {rx|tx}" << std::endl;
    return 1;
  };

  if ( ( (atof(argv[3]) >= 34375000) && (atof(argv[3]) <=4400000000) ) || (atof(argv[3]) == 0) ) {
      TargetFreq = atof(argv[3]);
  }
  else {
      std::cerr << "Invalid Frequency: Valid values for [TARGET_FREQ] limited to set [34375000,4400000000], or 0 for power down." << std::endl;
      return 1;
  };
  
  if ( (argc == 5) ){
    if (std::string(argv[4]) == "0") { verbosity = 0; }
    else if (std::string(argv[4]) == "1") { verbosity = 1; std::cout.precision(10); }
    else if (std::string(argv[4]) == "2") { verbosity = 2; std::cout.precision(10); }
    else { std::cerr << "Invalid output selection: 0 for verbose mode, 1 for driver"; return 1;};
  }
  else { verbosity = 0; };
  
    if ( (argc == 6) ){
      if (std::string(argv[5]) == "0") { lowspur = 0; }
      else if (std::string(argv[4]) == "1") { lowspur = 1; std::cout.precision(10); }
      else { std::cerr << "Invalid Noise mode selection: 0 for low spur, 1 for low noise"; return 1;};
  }
  else { lowspur = 0; };
  
  //Initialize
  fsynth_t fsynth;

//Get Target Frequency (in MHz).
  if (verbosity == 1){
    std::cout 
      << "\nRequesting Desired Frequency (@ Refin = " << (float)RefIn/1e6 
      << "MHz) in MHz: " << std::dec << TargetFreq / 1000000 
      << ", using MOD=" << chan_mod 
      << std::endl;
  };
//If target frequency is 0, power down VCO.
  ActualFreq = (int(TargetFreq) == 0 ? fsynth.power_down = fsynth_t::POWER_DOWN_ENABLED : fsynth.set_freq(TargetFreq, RefIn, chan_mod, bool( int(verbosity) == 1 ? true : false ) ) );

  fsynth.low_noise_and_spur = ( int(lowspur) == 0 ? fsynth_t::LOW_NOISE_AND_SPUR_LOW_SPUR : fsynth_t::LOW_NOISE_AND_SPUR_LOW_NOISE );
  
  if (verbosity == 2) {
     std::cout << ActualFreq << std::endl;
  };
  
   //Debug Information:
  if (verbosity == 1){
    std::cout << "Programming Channel: "
	      << std::string( int(Channel) == 1 ? "rx" : "tx")
	      << std::endl;
    //fsynth.show_regs();
  };

  //Write register content
  rc = 0;
  //for (i = 0; i < 6; i++) {
  for (i = 5; i >= 0; i--) {
    if (verbosity == 1){ std::cout << "Register " << i << " => 0x"<< std::hex << fsynth.get_reg(i) << std::dec << std::endl; };
    rc |= WriteReg(argv[1], Channel, fsynth.get_reg(i));
  };

  //Compensate for the remaining frequency difference in DSP
  rc |= SetDDCFreq(argv[1], Channel, ActualFreq, TargetFreq, verbosity);
  
//   //Debug Information:
//   if (debug == true){
//     std::cout 
//       << "DDC Phase is: "
//       << std::string( bool(RevFreq) == 0 ? "-" : "+")  
//       << Phase << std::endl;
//   };
// 
//   if (driver == true){
//     std::cout 
//       << std::string( float( (ActualFreq - TargetFreq) / 125e6  )) << std::endl;
//   };

  if (verbosity == 1){
    std::cout << "Return code from io_ctl calls: " << rc << std::endl;
  };
  
  return rc;
};


int SetDDCFreq(std::string FileName, int Channel, double ActualFreq, double TargetFreq, int verbose) {
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
