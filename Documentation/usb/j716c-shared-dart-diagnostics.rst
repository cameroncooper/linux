USB0 shared DART fault investigation on J716c
=============================================

Three RAM-only open-ended EEM controls reproduce a previously missed fault:
bank0/SID0 exception bitmap becomes1, while bank1 remains clear. Both were
clear before gadget bind. A follow-up reads bank0/SID0 status0: read, TTBR
invalid, using the public T8110 exception decoder. Linux calls only the bank1
apple-dart handler on this shared interrupt, reports nobody-cared and disables
IRQ67. Both DT banks use AIC IRQ1423 and the same physical USB power domain.
This is evidence of a pending peer fault, not proof of the originating DMA
transaction or permission to ACK/reset/enable that peer.

A matched38-second passive control creates/binds no gadget and enables no
link/traffic. Both banks remain clear, UDC stays unattached, target DWC trace
has zero events and no new fault markers. Three active runs fail the EEM
exchange; the passive observation succeeds. Each has complete loss-free
host (active arms) and18CPU target captures, authenticated physical-watchdog
recovery and scoped cleanup. No precise fault onset timing is claimed.

All arms retain Imagef3b728f64809271981a31722336caf1dac3f5a5beeae7601df6b817308d470a6,
notes5fcc965e9a09b499f74be4141e1ac45c6bf8274b8adc11009de1452a402c05ca,
release7.3.0-j716c-usb2-fifo-field-probe and Aura rawbbc4a007acb43e0afecd01847d0cf034d00bc9b660a804dfe60a1895749cdfe3.
The raw Aura is built from j716c/usb2-irq-route-refresh at a18ec683918f65b88263d9fcfe3bc549f2dd00c2.
Runtime pin details and original archive hashes are in the adjacent JSON.

Observer scope and reproduction
-------------------------------

The adjacent modules are separately named one-shot before/after/info observers.
They export cached read-only module parameters; no callback rereads hardware.
The first two source variants differ only in module name. The info variant
adds public per-SID status reads only for pending SIDs on a16-stream bank.
No DMA/IOVA addresses, TCR/TTBR or selector registers are read. No MMIO write,
ACK, reset, IRQ mask/steering or bank0 platform device/driver is added.

Admission requires exact J716c/T6050,18onlineCPUs, full experimental USB0
retirement-contract2 guard, control/read-only DTmarkers, public bank ranges,
shared IRQ cells/parent and physical power domain. Bank1 must be bound and
already active; get-if-active holds its shared power reference while both
banks are sampled and put-noidle releases only that temporary reference.
Unknown versions/stream counts retain parameters with fault_valid0. Captures
show version2.4,16streams per bank and active status throughout every sample.

Build the external modules with make -C SOURCE O=OUTPUT ARCH=arm64 LLVM=1
M=$PWD modules against the qualified kernel configuration/generated headers/
export CRCs. The frozen USB2 admission header is included for reproducibility;
the Makefile adds only its local include search path. C sources and admission
header are byte-identical to the tested variants; source hashes are adjacent.
The clean publication branch carries diagnostics and evidence, not a complete
installable USB2 bring-up stack. Native helper admission, test kernel bytes
and guarded experimental DT are required. No release-tag rebuild is claimed.

Each load follows a fresh native proof of at least70seconds remaining on the
120-second watchdog lease. The owner keeps its FD with all catchable signals
blocked after one final keepalive. Modules/DMA memory remain until physical
reset; host ownership is released only after strict baseline authentication.
No software reboot after the loader, unload, persistent disk/ESP/NVRAM,
enrollment or boot-policy write occurs. The source diagnostics do not repair
standard ACM notifications or qualify sustained/native USB operation.
