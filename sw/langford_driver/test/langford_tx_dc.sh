#!/bin/sh

UTIL=langford_util
FSYNTH=langford_rf_fsynth
DEVICE=/dev/langford

#Enable HF chain
$UTIL $DEVICE NASTxA 1

#Enable interpolation of 2^3 
$UTIL $DEVICE TXIntEn 0x7

gnuradio-companion langford_tx_dc.grc

