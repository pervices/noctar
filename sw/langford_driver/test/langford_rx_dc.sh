#!/bin/sh

UTIL=langford_util
UTIL_ADC=langford_adc_util
FSYNTH=langford_rf_fsynth
VGA=langford_rx_rf_bb_vga
DEVICE=/dev/langford

#Enable LF chain
$UTIL $DEVICE NASRxA 1

#Set LF gain
$UTIL $DEVICE N3ENB 1
$UTIL $DEVICE N3HILO 1
$UTIL $DEVICE N3GAIN 90

#No DSP (Upconversion or Decimation) in baseband.
$UTIL $DEVICE RXDecEn 0x0

#Interleave ADC clocks
$UTIL_ADC $DEVICE A ClkSel 1
$UTIL_ADC $DEVICE B ClkSel 0

gnuradio-companion langford_rx_dc.grc

