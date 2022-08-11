/***********************************************************************
 * Langford SPI files.
 * 
 * This implements the SPI protocol for when programming data.
 * (Bit banging)
 * 
 * (c) 2013 Per Vices Corporation
 * 
 * See LICENSE.txt for license and copyright information.
 ************************************************************************/


#include "langford_spi.h"

int LangordSpiWrite(int DevFD, int NumBytes, int Content, int nCS, int SClk, int SDO) {
  int	RetVal = 0;
  int	Val, Val0 = 0, Val1 = 1;
  int	i;

  //Set pins to known state
  RetVal |= ioctl(DevFD, nCS, &Val1);
  RetVal |= ioctl(DevFD, SClk, &Val0);

  //Start transfer
  RetVal |= ioctl(DevFD, nCS, &Val0);

  //Transfer data
  for (i = NumBytes * 8 - 1; i >= 0; i--) {
    Val = (Content >> i) & 1;
    RetVal |= ioctl(DevFD, SDO, &Val);
    RetVal |= ioctl(DevFD, SClk, &Val1);
    RetVal |= ioctl(DevFD, SClk, &Val0);
  };

  //Signal end of transfer
  RetVal |= ioctl(DevFD, nCS, &Val1);

  return RetVal;
};
