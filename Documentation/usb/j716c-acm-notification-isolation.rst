J716c ACM notification isolation
===============================

This guarded RAM diagnostic extends the finite bulk-transfer experiment to
actual CDC ACM endpoints on one J716c/T6050/M5 Max. It does not restore serial
or enable any behavior on an ordinary unmarked boot.

The finite ACM profile adds final-request LST and endpoint XFER_COMPLETE_EN
only to high-speed, non-stream bulk ep1in/MPS512 and notification ep2in/MPS10.
A class diagnostic caches the exact board, USB0 node and chosen-marker guard
at bind and skips only an already-zero DSR/DCD disconnect notification.
Opening the target TTY still queues the standard real ten-byte notification.
The ordinary class module and notification timing comparison are retained
as separate earlier experiments; an initial invalid OF path guard is also
retained and is not counted as a treatment.

Observed behavior
-----------------

* Finite transfers alone leave the first configuration stalled while the
  initial real notification is pending.
* Skipping the redundant initial zero-state notification permits the first
  configuration and all host TTY activation controls to complete.
* Host cdc_acm starts notification polling at TTY activation. The experiment
  opens the exact physical-port host TTY, observes a posted interrupt read
  and completion of activation controls, then authorizes one target TTY open.
* The resulting actual ten-byte notification still has no device giveback
  or host completion. No notification was queued before open authorization.
* Changing only the controller interval field from eight to zero does not
  help. The USB descriptor bInterval remains nine. Both cached rings retain
  one NORMAL ten-byte LST/IOC/HWO-owned TRB, enqueue one/dequeue zero.

The matched FIFO experiment also fails at the real notification. It changes
only the notification FIFO field from native bank two to literal physical
bank five, after successful configuration. Bulk IN/OUT FIFOs and interrupt
interval eight stay unchanged. Tracing confirms parameter0 XOR 0xe0000 for
notification only; all other endpoint parameters match. Both coherent-RAM
rings still contain one10B LST/IOC/HWO-owned NORMAL TRB. Both host and all
eighteen CPU captures are complete/loss-free; watchdog recovery and owned
TTY/rule cleanup pass. The existing cached geometry admission covers the
target bank; no FIFO selector/register file is used by the observer. This
is a negative result for this isolated change, not proof of FIFO correctness.

Reproduction and limits
-----------------------

The four chosen markers are usb2-acm-control (ordinary transfers),
usb2-acm-last (finite transfers), usb2-acm-interval0 (finite plus controller
interval zero), and usb2-acm-notifyfifo (finite plus notification FIFO five).
All depend on the existing exact-board USB0 retired-handoff, fixed device,
PHY, revision, high-speed and controller geometry admission. The class
skip-zero diagnostic accepts the latter three markers; it is not a global
ACM behavior change. The interval and FIFO comparisons each use identical
Image, class module, initrd and payload within their pair, changing only the
chosen marker. Different experiments have separate binary/source pins.

An independent native watchdog owner sets and reads back 120 seconds,
issues one final explicit keepalive, then retains its FD. Host and target
TTY FDs remain held until physical recovery, including every failed case.
The host releases its FD and exact-device temporary udev rule only after
strict unmarked-baseline authentication. No post-loader software reset,
early USB teardown, disk, ESP, install or boot-policy write is used.

Cached TRB reads run after the failed outcome and explicitly take/release
a runtime-PM reference, with configured/active state checked before/after.
Only coherent RAM is inspected; raw addresses and USB/boot identities remain
private. Host and all eighteen target CPU captures must be complete and
loss-free for a qualified observation. Serial payload, sustained operation,
other ports and a precise hardware/ring cause remain unqualified.

The adjacent JSON files preserve exact build pins, finite observations and
limits. Published source commits package the tested source; the saved runtime
binaries predate these publication commits and are not claimed as rebuilds
of their new Git version tags. Private raw captures and identity/key material
are excluded.
