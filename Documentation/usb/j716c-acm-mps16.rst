J716c notification packet geometry differential
===============================================

This matched guarded RAM experiment changes only the notification endpoint
controller MPS field from10 to16. The descriptor MPS remains10, the real
notification length10, interval/FIFO fields unchanged, and finite LST plus
XFER_COMPLETE_EN retained. Both profiles use identical Image/module/initrd
and payload; only chosen marker usb2-acm-last versus usb2-acm-mps16 differs.

Both complete direct configuration and host TTY activation controls with an
interrupt read posted before target TTY open. Both then retain the actual10B
notification without a giveback or host completion. Cached coherent-RAM rings
show one NORMAL10B LST/IOC/HWO-owned TRB, enqueue1/dequeue0. Tracing confirms
only notification DEPCFG parameter0 XOR0xd0; all other endpoint parameters
match. Complete host and all18 CPU captures are loss-free; physical watchdog
recovery and held-TTY FD/rule cleanup pass. This isolated change is insufficient;
it does not establish a hardware packet-size limitation.

The adjacent JSON records exact build pins and bounded limits. Raw identities,
addresses and captures remain private. This source branch packages the tested
kernel geometry diagnostic and normal-connect notification class source. The
separate notification-suppression module is on j716c/acm-notification-suppression.
Runtime binaries predate publication commits and are not claimed as rebuilds
of their new version tags. No upstream issue/comment/PR is created here.

This branch's class guard accepts acm-last and acm-mps16 only. Earlier
interval/FIFO records and their class variants remain pinned in the parent
notification-isolation branch; their presence in historical documentation
does not qualify those marker combinations with this geometry module.
