J716c initial-transfer configuration experiment
===============================================

This branch contains a controlled USB0 device-mode experiment for the J716c
Mac17,6 / T6050 M5 Max using macOS 27.2.1 firmware, mBoot-20457.41.3.
It is based on ``j716c/usb2-diagnostics``. The USB branch precedes the separate
shallow-WFI branch; combined WFI/USB qualification is not claimed.

The Apple glue caches an exact admitted profile before gadget registration.
The existing contract2 Aura handoff supplies the firmware-qualified retirement
chain; the kernel does not independently parse the mBoot version. Default
markers and other controllers retain their original behavior.

The paired test markers are:

* ``usb2-noopcontrol``: validate the diagnostic profile and retain the initial
  bulk/interrupt STARTTRANSFER against the zeroed first TRB.
* ``usb2-noopstart``: validate the same profile and omit only that initial start
  for a fresh, empty endpoint. Configuration, resource allocation, DALEPENA,
  ring/link initialization and the EP0 sequence are retained. The existing
  first real-request START and subsequent UPDATE paths are unchanged.

Bounded hardware result
-----------------------

Both cases used the same kernel Image, 34 matching modules and one vendor
interrupt-IN endpoint: MPS10, FS interval32 / HS interval9, with no userspace
noncontrol endpoint open or data queue.

The control recorded host cancellation status -2 after 5.210697 seconds,
consistent with the configuration timeout.
Its trace records the native initial start and TRANSFER_STARTED set, followed
by an EP0 status start without completion during the observed window.

The skip case completed EP0 STATUS2 with HWO clearing 1162 microseconds after
the target request. The host independently recorded configuration ACK status0
in 1349 microseconds and a subsequent successful string request. These are
separate clock-local durations, without a shared host/target clock anchor.
All 450 target trace records matched the header and 18 per-CPU entry totals;
every loss counter was zero. The complete host capture also had zero loss.
Control-FD lifetime, trace/event-disable return codes and readbacks, archive
integrity, and natural watchdog recovery to the original baseline passed.

The successful capture used a three-second postbind window. Thirty later
same-boot SSH metadata samples kept the gadget configured while uptime
advanced from 23.18 to 81.24 seconds. Those samples add N1 traffic and do not
qualify unattended idle or long-duration reliability. Earlier incomplete
capture attempts were retained as failures.

This supports the initial-start sequence differential on this exact profile.
ACM performs a real notification queue during configuration, which the
vendor/noqueue test does not cover. ACM data, serial, USB SSH, reconnect,
suspend and a general DWC31 erratum remain unqualified. This is an experiment,
not a proposed production-wide quirk.

Build and source pins
---------------------

The `source and build pins <j716c-empty-start-pins.json>`_ record the
source-equivalent commit/tree,
runtime Image/notes and configuration digests. The `kernel configuration
<j716c-empty-start.config>`_ contains no initramfs or firmware payload.
The executed binaries were built from the frozen pre-publication workspace;
every source blob and mode matches the code commit's tree. Rebuilding after
publication can change Git-derived version metadata and binary digests.

The source-only branch contains no private initramfs, device keys, firmware,
raw runtime device tree or unit-specific identifiers. No installer or boot
policy change is supplied by this experiment.
