/***********************************************************************
 * The langford ioctl header. 
 * 
 * This includes memory locations and offset for all possible calls within
 * the langford device.
 * 
 * (c) 2013 Per Vices Corporation
 * 
 * See LICENSE.txt for license and copyright information.
 ************************************************************************/

/*After adding more ioctl functions, be sure to update both the driver (ioctl handler) and the driver utility*/

#ifndef __PV_DRIVER_IOCTL_H
#define __PV_DRIVER_IOCTL_H


/*ioctl arguments*/
#define IOCTL_GET_N21at0	100
#define IOCTL_SET_N21at0	101
#define IOCTL_GET_N22at0	102
#define IOCTL_SET_N22at0	103
#define IOCTL_GET_N3GAIN	104
#define IOCTL_SET_N3GAIN	105
#define IOCTL_SET_N3GAIN	105

#define IOCTL_GET_N61CE		200
#define IOCTL_SET_N61CE		201
#define IOCTL_GET_N61DATA	202
#define IOCTL_SET_N61DATA	203
#define IOCTL_GET_N61CLK	204
#define IOCTL_SET_N61CLK	205
#define IOCTL_GET_N61LE		206
#define IOCTL_SET_N61LE		207
#define IOCTL_GET_N62CE		208
#define IOCTL_SET_N62CE		209
#define IOCTL_GET_N62DATA	210
#define IOCTL_SET_N62DATA	211
#define IOCTL_GET_N62CLK	212
#define IOCTL_SET_N62CLK	213
#define IOCTL_GET_N62LE		214
#define IOCTL_SET_N62LE		215
#define IOCTL_GET_N3ENB		216
#define IOCTL_SET_N3ENB		217
#define IOCTL_GET_N3HILO	218
#define IOCTL_SET_N3HILO	219
#define IOCTL_GET_NASRxA	220
#define IOCTL_SET_NASRxA	221
#define IOCTL_GET_NASTxA	222
#define IOCTL_SET_NASTxA	223
#define IOCTL_GET_N7CLK		224
#define IOCTL_SET_N7CLK		225
#define IOCTL_GET_N7CS		226
#define IOCTL_SET_N7CS		227
#define IOCTL_GET_N7SDI		228
#define IOCTL_SET_N7SDI		229
#define IOCTL_GET_DACIQSel	230
#define IOCTL_SET_DACIQSel	231
#define IOCTL_GET_DACReset	232
#define IOCTL_SET_DACReset	233
#define IOCTL_GET_DACCSB	234
#define IOCTL_SET_DACCSB	235
#define IOCTL_GET_DACSDIO	236
#define IOCTL_SET_DACSDIO	237
#define IOCTL_GET_DACSClk	238
#define IOCTL_SET_DACSClk	239
#define IOCTL_GET_ADCAnOE	240
#define IOCTL_SET_ADCAnOE	241
#define IOCTL_GET_ADCAPD	242
#define IOCTL_SET_ADCAPD	243
#define IOCTL_GET_ADCASClk	244
#define IOCTL_SET_ADCASClk	245
#define IOCTL_GET_ADCASDIO	246
#define IOCTL_SET_ADCASDIO	247
#define IOCTL_GET_ADCAnCS	248
#define IOCTL_SET_ADCAnCS	249
#define IOCTL_GET_ADCBnOE	250
#define IOCTL_SET_ADCBnOE	251
#define IOCTL_GET_ADCBPD	252
#define IOCTL_SET_ADCBPD	253
#define IOCTL_GET_ADCBSClk	254
#define IOCTL_SET_ADCBSClk	255
#define IOCTL_GET_ADCBSDIO	256
#define IOCTL_SET_ADCBSDIO	257
#define IOCTL_GET_ADCBnCS	258
#define IOCTL_SET_ADCBnCS	259
#define IOCTL_GET_DACSDO	260
#define IOCTL_SET_DACSDO	261

#define IOCTL_GET_N61MUXOUT	300
#define IOCTL_GET_N61LD		301
#define IOCTL_GET_N62MUXOUT	302
#define IOCTL_GET_N62LD		303
#define IOCTL_GET_N7SDO		304
#define IOCTL_GET_GPIOin	305

#define IOCTL_GET_RXPhase	400
#define IOCTL_SET_RXPhase	401

#define IOCTL_GET_TXPhase	500
#define IOCTL_SET_TXPhase	501

#define IOCTL_GET_RXDecEn	600
#define IOCTL_SET_RXDecEn	601
#define IOCTL_GET_TXIntEn	602
#define IOCTL_SET_TXIntEn	603
#define IOCTL_GET_RXRevFreq	604
#define IOCTL_SET_RXRevFreq	605
#define IOCTL_GET_TXRevFreq	606
#define IOCTL_SET_TXRevFreq	607
#define IOCTL_GET_RxDspEn	608
#define IOCTL_SET_RxDspEn	609
#define IOCTL_GET_TxDspEn	610
#define IOCTL_SET_TxDspEn	611
#define IOCTL_GET_NRXTALSEL	612
#define IOCTL_SET_NRXTALSEL	613
#define IOCTL_GET_NRVCOSEL	614
#define IOCTL_SET_NRVCOSEL	615
#define IOCTL_GET_RxFifoClr	616
#define IOCTL_SET_RxFifoClr	617
#define IOCTL_GET_TxFifoClr	618
#define IOCTL_SET_TxFifoClr	619
#define IOCTL_GET_RxIsSigned	620
#define IOCTL_SET_RxIsSigned	621
#define IOCTL_GET_TxIsSigned	622
#define IOCTL_SET_TxIsSigned	623
#define IOCTL_GET_GPIOout	624
#define IOCTL_SET_GPIOout	625
#define IOCTL_GET_RxBaseband	626
#define IOCTL_SET_RxBaseband    627

#define IOCTL_GET_RxErrorCorrGainQ	        700
#define IOCTL_SET_RxErrorCorrGainQ	        701
#define IOCTL_GET_RxErrorCorrGainI	        702
#define IOCTL_SET_RxErrorCorrGainI	                703
#define IOCTL_GET_RxErrorCorrGroupDelayQ	704
#define IOCTL_SET_RxErrorCorrGroupDelayQ	705
#define IOCTL_GET_RxErrorCorrGroupDelayI	706
#define IOCTL_SET_RxErrorCorrGroupDelayI	707
#define IOCTL_GET_RxErrorCorrDCOffsetQ	708
#define IOCTL_SET_RxErrorCorrDCOffsetQ	        709
#define IOCTL_GET_RxErrorCorrDCOffsetI	        710
#define IOCTL_SET_RxErrorCorrDCOffsetI	        711

#define IOCTL_GET_TxErrorCorrGainQ	        800
#define IOCTL_SET_TxErrorCorrGainQ	        801
#define IOCTL_GET_TxErrorCorrGainI	                802
#define IOCTL_SET_TxErrorCorrGainI	                803
#define IOCTL_GET_TxErrorCorrGroupDelayQ	804
#define IOCTL_SET_TxErrorCorrGroupDelayQ	805
#define IOCTL_GET_TxErrorCorrGroupDelayI	806
#define IOCTL_SET_TxErrorCorrGroupDelayI	807
#define IOCTL_GET_TxErrorCorrDCOffsetQ	        808
#define IOCTL_SET_TxErrorCorrDCOffsetQ	        809
#define IOCTL_GET_TxErrorCorrDCOffsetI	        810
#define IOCTL_SET_TxErrorCorrDCOffsetI	        811


#endif
