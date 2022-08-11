/*After adding more ioctl functions, be sure to update both the driver (ioctl handler) and the driver utility*/

#ifndef __LANGFORD_IO_H
#define __LANGFORD_IO_H

#define LANGFORD_LINK_DEVICE_NAME  L"\\\\.\\Langford"

#define FILE_DEVICE_LANGFORD  0x8000

// Define Interface reference/dereference routines for
// Interfaces exported by IRP_MN_QUERY_INTERFACE

#define LANGFORDDEVICE_IOCTL(index) \
	CTL_CODE(FILE_DEVICE_LANGFORD, index, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GET_N21at0 LANGFORDDEVICE_IOCTL(0x800) //0-255, RX HF variable gain 0=0V - 255=5V
#define IOCTL_SET_N21at0 LANGFORDDEVICE_IOCTL(0x801) //0-255, RX HF variable gain 0=0V - 255=5V
#define IOCTL_GET_N22at0 LANGFORDDEVICE_IOCTL(0x802)  //0-255, TX HF variable gain 0=0V - 255=5V
#define IOCTL_SET_N22at0 LANGFORDDEVICE_IOCTL(0x803) //0-255, TX HF variable gain 0=0V - 255=5V
#define IOCTL_GET_N3GAIN LANGFORDDEVICE_IOCTL(0x804)//0-255, RX LF variable gain 0=0V - 255=1V
#define IOCTL_SET_N3GAIN LANGFORDDEVICE_IOCTL(0x805)//0-255, RX LF variable gain 0=0V - 255=1V

#define IOCTL_GET_N61CE	 LANGFORDDEVICE_IOCTL(0x806)//0-1, RX Chip Enable 																(Status of Receiver)
#define IOCTL_SET_N61CE	 LANGFORDDEVICE_IOCTL(0x807) //0-1, RX Chip Enable 																(Enable Receiver)
#define IOCTL_GET_N61DATA LANGFORDDEVICE_IOCTL(0x808)//0-1, RX Serial interface frequency synth data pin								(Status of data pin)
#define IOCTL_SET_N61DATA LANGFORDDEVICE_IOCTL(0x809)//0-1, RX Serial interface frequency synth data pin								(Enable data via serial interface)
#define IOCTL_GET_N61CLK LANGFORDDEVICE_IOCTL(0x80A)//0-1. RX clock pin																(Status of clock)
#define IOCTL_SET_N61CLK LANGFORDDEVICE_IOCTL(0x80B)//0-1, RX clock pin																(Enable clock)
#define IOCTL_GET_N61LE	 LANGFORDDEVICE_IOCTL(0x80C)//0-1, RX Frequency synth															(Status of synth)
#define IOCTL_SET_N61LE LANGFORDDEVICE_IOCTL(0x80D)//0-1, RX Frequency synth															(Enable synth)
#define IOCTL_GET_N62CE	 LANGFORDDEVICE_IOCTL(0x80E)//0-1, TX Chip Enable 																(Status of Transmitter)
#define IOCTL_SET_N62CE	 LANGFORDDEVICE_IOCTL(0x80F)//0-1, TX Chip Enable 																(Enable Transmitter)
#define IOCTL_GET_N62DATA LANGFORDDEVICE_IOCTL(0x810)//0-1, TX Serial interface frequency synth data pin								(Status of data pin)
#define IOCTL_SET_N62DATA LANGFORDDEVICE_IOCTL(0x811)//0-1, TX Serial interface frequency synth data pin								(Enable data via serial interface)
#define IOCTL_GET_N62CLK LANGFORDDEVICE_IOCTL(0x812)//0-1. TX clock pin																(Status of clock)
#define IOCTL_SET_N62CLK LANGFORDDEVICE_IOCTL(0x813)//0-1, TX clock pin																(Enable clock)
#define IOCTL_GET_N62LE LANGFORDDEVICE_IOCTL(0x814)//0-1, TX Frequency synth															(Status of synth)
#define IOCTL_SET_N62LE LANGFORDDEVICE_IOCTL(0x815)//0-1, TX Frequency synth															(Enable synth)
#define IOCTL_GET_N3ENB LANGFORDDEVICE_IOCTL(0x816)//0-1, LF amplifier																(Status of Amplifier)
#define IOCTL_SET_N3ENB LANGFORDDEVICE_IOCTL(0x817)//0-1, LF amplifier																(Enable Amplifier)
#define IOCTL_GET_N3HILO LANGFORDDEVICE_IOCTL(0x818)//0-1, High-Low Gain Stages (Low gain = -4.5..43.5dB, High gain = 7.5..55.5dB)  	(Status of high-low gain)
#define IOCTL_SET_N3HILO LANGFORDDEVICE_IOCTL(0x819)//0-1, High-Low Gain Stages (Low gain = -4.5..43.5dB, High gain = 7.5..55.5dB)  	(Enable high-low gain)
#define IOCTL_GET_NASRxA LANGFORDDEVICE_IOCTL(0x81A)//0-1, RX 'A' control pin for RF switch, 'B' will take opposite value
#define IOCTL_SET_NASRxA LANGFORDDEVICE_IOCTL(0x81B)//0-1, RX 'A' control pin for RF switch, 'B' will take opposite value
#define IOCTL_GET_NASTxA LANGFORDDEVICE_IOCTL(0x81C)//0-1, TX 'A' control pin for RF switch, 'B' will take opposite value
#define IOCTL_SET_NASTxA LANGFORDDEVICE_IOCTL(0x81D)//0-1, TX 'A' control pin for RF switch, 'B' will take opposite value
#define IOCTL_GET_N7CLK LANGFORDDEVICE_IOCTL(0x81E)//0-1, Clock pin for serial interface of differential ADC VGA						(Status of clock pin)
#define IOCTL_SET_N7CLK	 LANGFORDDEVICE_IOCTL(0x81F)//0-1, Clock pin for serial interface of differential ADC VGA						(Enable clock pin)
#define IOCTL_GET_N7CS LANGFORDDEVICE_IOCTL(0x820)//0-1, Chip select pin for serial interface of differential ADC VGA
#define IOCTL_SET_N7CS LANGFORDDEVICE_IOCTL(0x821)//0-1, Chip select pin for serial interface of differential ADC VGA
#define IOCTL_GET_N7SDI LANGFORDDEVICE_IOCTL(0x822)//0-1, Serial data to the serial interface for differential ADC VGA
#define IOCTL_SET_N7SDI	LANGFORDDEVICE_IOCTL(0x823)//0-1, Serial data to the serial interface for differential ADC VGA
#define IOCTL_GET_DACIQSel LANGFORDDEVICE_IOCTL(0x824)//0-1, IQ framing pin for the DAC. Used for single port mode.
#define IOCTL_SET_DACIQSel LANGFORDDEVICE_IOCTL(0x825)//0-1, IQ framing pin for the DAC. Used for single port mode.
#define IOCTL_GET_DACReset LANGFORDDEVICE_IOCTL(0x826)//0-1, Reset pin for the DAC, also enables pin mode								(Status of DAC reset and pin mode)
#define IOCTL_SET_DACReset LANGFORDDEVICE_IOCTL(0x827)//0-1, Reset pin for the DAC, also enables pin mode								(Enable DAC reset and pin mode)
#define IOCTL_GET_DACCSB LANGFORDDEVICE_IOCTL(0x828)//0-1, Enable DAC mixmode, chipselect pin for serial interface
#define IOCTL_SET_DACCSB LANGFORDDEVICE_IOCTL(0x829)//0-1, Enable DAC mixmode, chipselect pin for serial interface
#define IOCTL_GET_DACSDIO LANGFORDDEVICE_IOCTL(0x82A)//0-1, Unsigned or signed binary data format, also data pin of serial interface (DAC)
#define IOCTL_SET_DACSDIO LANGFORDDEVICE_IOCTL(0x82B)//0-1, Unsigned or signed binary data format, also data pin of serial interface (DAC)
#define IOCTL_GET_DACSClk LANGFORDDEVICE_IOCTL(0x82C)//0-1, Enable single port mode, clock pin for serial interface
#define IOCTL_SET_DACSClk LANGFORDDEVICE_IOCTL(0x82D)//0-1, Enable single port mode, clock pin for serial interface
#define IOCTL_GET_ADCAnOE LANGFORDDEVICE_IOCTL(0x82E)
#define IOCTL_SET_ADCAnOE LANGFORDDEVICE_IOCTL(0x82F)
#define IOCTL_GET_ADCAPD LANGFORDDEVICE_IOCTL(0x830)//0-1 Power down ADC
#define IOCTL_SET_ADCAPD LANGFORDDEVICE_IOCTL(0x831)//0-1 Power down ADC
#define IOCTL_GET_ADCASClk LANGFORDDEVICE_IOCTL(0x832)//0-1, Unsigned or signed binary data format, also data pin of serial interface (ADC)
#define IOCTL_SET_ADCASClk LANGFORDDEVICE_IOCTL(0x833)//0-1, Unsigned or signed binary data format, also data pin of serial interface (ADC)
#define IOCTL_GET_ADCASDIO LANGFORDDEVICE_IOCTL(0x834)//0-1, Enable LVDs outputs for ADC or use CMOS, data pin ADC's serial interface
#define IOCTL_SET_ADCASDIO LANGFORDDEVICE_IOCTL(0x835)//0-1, Enable LVDs outputs for ADC or use CMOS, data pin ADC's serial interface
#define IOCTL_GET_ADCAnCS LANGFORDDEVICE_IOCTL(0x836)//0-1,  Enable pin mode, chip select pin for ADC serial interface
#define IOCTL_SET_ADCAnCS LANGFORDDEVICE_IOCTL(0x837)//0-1,  Enable pin mode, chip select pin for ADC serial interface
#define IOCTL_GET_ADCBnOE LANGFORDDEVICE_IOCTL(0x838)
#define IOCTL_SET_ADCBnOE LANGFORDDEVICE_IOCTL(0x839)
#define IOCTL_GET_ADCBPD LANGFORDDEVICE_IOCTL(0x83A)//0-1, Power down ADC
#define IOCTL_SET_ADCBPD LANGFORDDEVICE_IOCTL(0x83B)//0-1, Power down ADC
#define IOCTL_GET_ADCBSClk LANGFORDDEVICE_IOCTL(0x83C)//0-1, Unsigned or signed binary data format, also data pin of serial interface (ADC)
#define IOCTL_SET_ADCBSClk LANGFORDDEVICE_IOCTL(0x83D)//0-1, Unsigned or signed binary data format, also data pin of serial interface (ADC)
#define IOCTL_GET_ADCBSDIO LANGFORDDEVICE_IOCTL(0x83E)//0-1, Enable LVDs outputs for ADC or use CMOS, data pin ADC's serial interface
#define IOCTL_SET_ADCBSDIO LANGFORDDEVICE_IOCTL(0x83F)//0-1, Enable LVDs outputs for ADC or use CMOS, data pin ADC's serial interface
#define IOCTL_GET_ADCBnCS LANGFORDDEVICE_IOCTL(0x840)//0-1, Enable pin mode, chip select pin for ADC serial interface
#define IOCTL_SET_ADCBnCS LANGFORDDEVICE_IOCTL(0x841)//0-1, Enable pin mode, chip select pin for ADC serial interface
#define IOCTL_GET_DACSDO LANGFORDDEVICE_IOCTL(0x842)//0-1, Power down DAC, data pin of DAC's serial interface 
#define IOCTL_SET_DACSDO LANGFORDDEVICE_IOCTL(0x843)//0-1, Power down DAC, data pin of DAC's serial interface 

#define IOCTL_GET_N61MUXOUT LANGFORDDEVICE_IOCTL(0x844)//0-1, RX Multiplexor output pin on frequency synth							(Status of multiplexor)
#define IOCTL_GET_N61LD LANGFORDDEVICE_IOCTL(0x845)//0-1, RX lock detect pin														(status of lock)
#define IOCTL_GET_N62MUXOUT LANGFORDDEVICE_IOCTL(0x846)//0-1, TX Multiplexor output pin on frequency synth							(Status of multiplexor)
#define IOCTL_GET_N62LD LANGFORDDEVICE_IOCTL(0x847)//0-1, TX lock detect pin														(status of lock)
#define IOCTL_GET_N7SDO LANGFORDDEVICE_IOCTL(0x848)//0-1, Serial data to the serial interface for differential ADC VGA

#define IOCTL_GET_RXPhase LANGFORDDEVICE_IOCTL(0x849)//0-2^32-1 Phase increment for RX NCO
#define IOCTL_SET_RXPhase LANGFORDDEVICE_IOCTL(0x84A)//0-2^32-1 Phase increment for RX NCO

#define IOCTL_GET_TXPhase LANGFORDDEVICE_IOCTL(0x84B)//0-2^32-1 Phase increment for TX NCO
#define IOCTL_SET_TXPhase LANGFORDDEVICE_IOCTL(0x84C)//0-2^32-1 Phase increment for TX NCO

#define IOCTL_GET_RXDecEn LANGFORDDEVICE_IOCTL(0x84D)//0-1, Enable RX decimation
#define IOCTL_SET_RXDecEn LANGFORDDEVICE_IOCTL(0x84E)//0-1, Enable RX decimation
#define IOCTL_GET_TXIntEn LANGFORDDEVICE_IOCTL(0x84F)//0-1, Eanable TX interpolation  
#define IOCTL_SET_TXIntEn LANGFORDDEVICE_IOCTL(0x850)//0-1, Eanable TX interpolation
#define IOCTL_GET_RXRevFreq LANGFORDDEVICE_IOCTL(0x851)//0-1, Negate RX DDC and DUC frequency
#define IOCTL_SET_RXRevFreq LANGFORDDEVICE_IOCTL(0x852)//0-1, Negate RX DDC and DUC frequency
#define IOCTL_GET_TXRevFreq LANGFORDDEVICE_IOCTL(0x853)//0-1, Negate TX DDC and DUC frequency
#define IOCTL_SET_TXRevFreq LANGFORDDEVICE_IOCTL(0x854)//0-1, Negate TX DDC and DUC frequency
#define IOCTL_GET_RxDspEn LANGFORDDEVICE_IOCTL(0x855)//0-1, Enable RX DSP chain
#define IOCTL_SET_RxDspEn LANGFORDDEVICE_IOCTL(0x856)//0-1, Enable RX DSP chain
#define IOCTL_GET_TxDspEn LANGFORDDEVICE_IOCTL(0x857)//0-1, Enable TX DSP chain
#define IOCTL_SET_TxDspEn LANGFORDDEVICE_IOCTL(0x858)//0-1, Enable TX DSP chain
#define IOCTL_GET_NRXTALSEL LANGFORDDEVICE_IOCTL(0x859)//0-1, Select between internal and external oscillator (external 20Mhz reference)		(Status of selection)
#define IOCTL_SET_NRXTALSEL LANGFORDDEVICE_IOCTL(0x85A)//0-1, Select between internal and external oscillator (external 20Mhz reference)		(Enable external reference)
#define IOCTL_GET_NRVCOSEL LANGFORDDEVICE_IOCTL(0x85B)//0-1, Bypass clock dist. IC
#define IOCTL_SET_NRVCOSEL LANGFORDDEVICE_IOCTL(0x85C)//0-1, Bypass clock dist. IC

//New IOCTLs (Windows)
#define IOCTL_GET_PHITEST LANGFORDDEVICE_IOCTL(0x85D)//0-1, Test IOCTLs against card														(Status of testing)
#define IOCTL_SET_PHITEST LANGFORDDEVICE_IOCTL(0x85E)//0-1, Test IOCTLs against card														(Enable test)
#define IOCTL_GET_READBUF LANGFORDDEVICE_IOCTL(0x85F)//0-1, Use driver for IOCTL_GET (cache)												(Status of buffer)
#define IOCTL_SET_READBUF LANGFORDDEVICE_IOCTL(0x860)//0-1, Use driver for IOCTL_GET (cache)												(Enable buffer)

//this was not include in the original windows driver. why????

#define IOCTL_GET_RxFifoClr	LANGFORDDEVICE_IOCTL(0x861)
#define IOCTL_SET_RxFifoClr	LANGFORDDEVICE_IOCTL(0x862)
#define IOCTL_GET_TxFifoClr	LANGFORDDEVICE_IOCTL(0x863)
#define IOCTL_SET_TxFifoClr LANGFORDDEVICE_IOCTL(0x864)
#define IOCTL_GET_RxIsSigned LANGFORDDEVICE_IOCTL(0x865)
#define IOCTL_SET_RxIsSigned LANGFORDDEVICE_IOCTL(0x866)
#define IOCTL_GET_TxIsSigned LANGFORDDEVICE_IOCTL(0x867)
#define IOCTL_SET_TxIsSigned LANGFORDDEVICE_IOCTL(0x868)
#define IOCTL_GET_GPIOout LANGFORDDEVICE_IOCTL(0x869)
#define IOCTL_SET_GPIOout LANGFORDDEVICE_IOCTL(0x86A)
#define IOCTL_GET_RxBaseband LANGFORDDEVICE_IOCTL(0x86B)
#define IOCTL_SET_RxBaseband LANGFORDDEVICE_IOCTL(0x86C)

#define IOCTL_GET_GPIOin LANGFORDDEVICE_IOCTL(0x86D)

#ifndef USER_MODE 

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL langfordEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_WRITE langfordEvtIoWrite;
EVT_WDF_IO_QUEUE_IO_READ langfordEvtIoRead;

BOOLEAN langfordEvtProgramDma( IN WDFDMATRANSACTION Transaction, 
								  IN WDFDEVICE Device, 
								  IN WDFCONTEXT Context, 
								  IN WDF_DMA_DIRECTION Direction, 
								  IN PSCATTER_GATHER_LIST SgList );
VOID langfordRequestComplete( IN WDFDMATRANSACTION DmaTransaction, IN NTSTATUS Status );

#endif

#endif
