#!/bin/sh

UTIL=langford_util
UTIL_ADC=langford_adc_util
FSYNTH=langford_rf_fsynth
VGA=langford_rx_rf_bb_vga
DEVICE=/dev/langford

#Enable HF chain
$UTIL $DEVICE NASRxA 0

#Set HF gain
$UTIL $DEVICE N21at0 1

#Set frequency synthesizer
$FSYNTH $DEVICE rx 500e6

#Set ADC VGA to reasonable values on both channels
$VGA $DEVICE 0 8
$VGA $DEVICE 1 8

#Ensure Simultanious ADC Clocking (no interleaving)
$UTIL_ADC $DEVICE A ClkSel 0
$UTIL_ADC $DEVICE B ClkSel 0

#Enable decimation of 2
$UTIL $DEVICE RXDecEn 0x1

gnuradio-companion langford_rx_hi.grc

