J716c serial data with notification suppressed
=============================================

The parent notification-isolation branch permits ACM configuration by skipping
only the redundant initial zero-state notification. Opening the target TTY then
queues a real ten-byte connect notification. With that notification pending,
the target can receive and verify the first4096-byte host payload, but its
bulk-IN reply and interrupt notification have no givebacks. The original
attempt has nine bulk-IN queues/eleven prepared TRBs and zero IN givebacks;
bulk-OUT has nine givebacks. This is a directional failure, not a complete
serial exchange.

This branch changes only acm_connect under the existing cached exact-board,
USB0 and diagnostic-marker guard: update DSR/DCD state as before, but omit the
connect notification. Initial redundant-zero suppression remains. Unmarked
boots retain ordinary ACM behavior. This deliberately disables a notification
in diagnostic boots; it does not repair interrupt transfers or qualify all
CDC ACM behavior.

The notification-disabled mode passes a framed binary exchange of4096 and65536
bytes in each direction. All138 bulk-IN requests retire, across151 TRBs
including zero-length packet TRBs. No notification request is queued. Both
host and target independently check payload bytes and SHA256 receipts. A fresh matched normal-notification control again fails the first reply with
nine queued IN requests and zero givebacks. The suppressed repeat passes the
same full exchange again, with138/138 IN givebacks through151 TRBs. Every
matched capture is complete and loss-free; physical recoveries, held-TTY FD
lifetime and owned udev-rule cleanup pass. The complete result file is adjacent.

Protocol and reproduction
-------------------------

The tested native worker and exact host peer function are in
j716c-acm-serial-test/. The worker is statically linked for AArch64 Linux-musl
against the recorded native ABI. It requires the current boot UUID and a
fresh32-character lowercase hexadecimal nonce. The outer fixture first
verifies kernel notes/DT/guarded120-second held watchdog ownership, direct
configuration and the exact physical-port host TTY generation. It configures
the host TTY raw, waits for all activation controls and a posted interrupt
read, then authorizes the target open. It never exposes a serial shell.

Frames have a64-byte header: J7ACM1 followed by CR/LF (eight bytes), the32-byte
ASCII nonce, little-endian32-bit sequence and length, one direction byte
(zero host-to-target, one target-to-host), then fifteen zero bytes. For each
of lengths4096 and65536, the host sends a frame and receives the target reply.
Payload byte i is (29*i + (i>>8) + nonce[i%32] +97*direction +13*sequence) mod256,
where nonce bytes are ASCII. Headers and every payload byte must match. SHA256
receipts bind the independently observed data. Target TX receipts alone mean
queued-to-TTY; host receipt is required to establish delivery.

The worker has one absolute nine-second data deadline. Its host peer has one
ten-second deadline. Every path after target TTY open retains the FD until
physical reset, including errors. The host also keeps its FD until strict
unmarked-baseline recovery. No signals, early TTY close, USB unbind/unload or
post-loader software reset is used. The two class modules share one build path,
recipe, vermagic and dependencies; their bytes and the only source delta are
recorded. Within the matched pair, Image/DT/native guards/payload are identical
and only usb_f_acm.ko in the initrd differs. Normal module bytes also equal the
module used by the original failed serial attempt.

Qualification requires complete loss-free host and all eighteen CPU traces,
no new kernel fault markers, exact four matching per-direction frame receipts,
all bulk-IN requests retired, successful observer/archive and physical recovery,
then host FD/rule cleanup. This is finite single-unit/cable qualification of
the diagnostic data path, not sustained reliability, standard notification
support, serial SSH or a precise hardware cause. No disk/ESP/install or
boot-policy write is part of the trial. Raw identities, addresses and captures
remain private. Runtime binaries predate publication commits; source and saved
binary pins are distinguished in the adjacent results.
