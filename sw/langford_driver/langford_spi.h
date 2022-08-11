/***********************************************************************
 * Langford SPI header.
 * 
 * This is an aid to help implement the SPI protocol to program on board
 * chips.
 * 
 * (c) 2013 Per Vices Corporation
 * 
 * See LICENSE.txt for license and copyright information.
 ************************************************************************/

#include <sys/ioctl.h>
#define _GLIBCXX_USE_CXX11_ABI 0

/**
 * Do SPI writes to the Langord device
 * @param DevFD - Device file descriptor (from open())
 * @param NumBytes - Number of bytes to transfer (maximum of 4 = bytes in int)
 * @param Content - Data to transfer (NumBytes least significant bytes transfered)
 * @param nCS - ioctl command to write SPI nCS pin
 * @param SClk - ioctl command to write SPI SCLK pin
 * @param SDO - ioctl command to write SPI SDO pin

 * For example, to write 3 bytes 0x123 to ADC A, you can use:
@code
  int	FD = open("/dev/langford", o_RDWR);
  LangordSpiWrite(FD, 3, 0x123, IOCTL_SET_ADCAnCS, IOCTL_SET_ADCASClk, IOCTL_SET_ADCASDIO);
@endcode
*/
int LangordSpiWrite(int DevFD, int NumBytes, int Content, int nCS, int SClk, int SDO);
