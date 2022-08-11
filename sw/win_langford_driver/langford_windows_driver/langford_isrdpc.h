
#ifndef _LANGFORD_ISRDPC_H
#define _LANGFORD_ISRDPC_H

EVT_WDF_INTERRUPT_ISR langfordEvtInterruptIsr;
EVT_WDF_INTERRUPT_DPC langfordEvtInterruptDpc;
EVT_WDF_INTERRUPT_ENABLE langfordEvtInterruptEnable;
EVT_WDF_INTERRUPT_DISABLE langfordEvtInterruptDisable;

VOID ReadRequestProcessor( IN PVOID StartContext );
VOID WriteRequestProcessor( IN PVOID StartContext );

#endif