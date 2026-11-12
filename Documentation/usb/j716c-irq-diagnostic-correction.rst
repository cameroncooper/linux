Shared USB0 DART IRQ diagnostic correction
==========================================

A subsequent read-only audit of73 retained target captures found16 failed
EEM cases with ``irq 67: nobody cared`` followed by ``Disabling IRQ #67``.
The listed handler is apple-dart; both USB0 DART banks share that DT IRQ.
The previous fault regex did not match these two messages. Affected derived
JSON records now include both markers and kernel_diagnostics_clean=false.

Raw receipts, byte/request counts, transfer failures, capture completeness
and physical-recovery outcomes remain unchanged. None of the successful
cases scanned had either IRQ diagnostic. Successful serial/AIC and finite
transfer observations retain their stated scope. Failed EEM captures do not
qualify healthy I/O merely because every CPU received interrupts. This audit
does not establish that every transfer stall has the same cause.
