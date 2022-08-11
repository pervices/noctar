#!/bin/sh

UTIL=langford_util
FSYNTH=langford_rf_fsynth
DEVICE=/dev/langford

#Enable HF chain
$UTIL $DEVICE NASTxA 0

#Set HF gain
$UTIL $DEVICE N22at0 1

#Set frequency synthesizer
$FSYNTH $DEVICE tx 500e6

#Enable interpolation of 2
$UTIL $DEVICE TXIntEn 2

gnuradio-companion langford_tx_hi.grc

