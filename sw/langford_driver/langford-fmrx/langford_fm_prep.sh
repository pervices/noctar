#!/bin/sh

UTIL=langford_util
UTIL_ADC=langford_adc_util
FSYNTH=langford_rf_fsynth
VGA=langford_rx_rf_bb_vga
DEVICE=/dev/langford

#Enable HF chain
$UTIL $DEVICE NASRxA 0

#Set HF gain
#If the demo doesn't immediately work, or you don't
#observe a signal, try adjusting this value higher.
$UTIL $DEVICE N21at0 125

#Set frequency synthesizer
$FSYNTH $DEVICE rx 97.5e6

#Set ADC VGA to reasonable values on both channels
#If the demo doesn't immediately work, or you don't
#Observe a signal, try adjusting this value.
$VGA $DEVICE 0 15.5
$VGA $DEVICE 1 15.5

#Ensure Simultanious ADC Clocking (no interleaving)
$UTIL_ADC $DEVICE A ClkSel 0
$UTIL_ADC $DEVICE B ClkSel 0

#Set decimation of 7
$UTIL $DEVICE RXDecEn 0xf

gnuradio-companion dual-channel/fmrx_langford_dual.grc
