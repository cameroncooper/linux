.. SPDX-License-Identifier: GPL-2.0

DWC3 request tracing on J716c
=============================

The request trace class reserves an endpoint-name string with ``__string``
but omitted ``__assign_str(name)`` before committing the record. The code
commit adds that assignment, matching the other DWC3 classes with names.
It changes trace initialization, not transfer sequencing.

Source and build
----------------

The single-line `code commit <https://github.com/cameroncooper/linux/commit/91d455ecf8c5308140a31189aee748c760d57abf>`_ is based on the
previous guarded J716c empty-START experiment. All 98,492 tracked source blobs
and Git modes match the frozen hardware-test source; its tree is 4c51e92ad81ecd7985892d8ec51c4b43b71c61cd.
The `configuration <j716c-request-trace-init.config>`_ is supplied. Exact Image,
notes, configuration and review digests are in the `pins <j716c-request-trace-init-pins.json>`_.

The tested Image predates this publication commit. These are source-equivalent
pins, not a claim that the published Git tag produced that existing binary.
The USB diagnostic lineage is separate from the published shallow-WFI work.
Raw initramfs/Stage2 images, firmware, keys and traces are not part of this
source branch.

Bounded hardware result
-----------------------

Two fresh J716c/T6050 RAM boots used the same Image, DT, 34 matching modules,
private initrd, valid BCD 0x0210 ACM descriptor and fixed observation interval.
The sole runtime mode difference enabled ``dwc3_ep_queue`` and
``dwc3_gadget_giveback`` in the owned trace instance. The other three request
aliases stayed disabled; they have not been runtime-qualified by this pair.

Both first SET_CONFIGURATION requests failed. The control had 270 target
records and drained 9,180 bus-wide USBmon input lines; the enabled case had 278
and 9,202 respectively. These line totals were mostly prefiltered traffic, not
target control-record counts. The authenticated control records numbered 2
and 8 respectively. No unrelated transfer payload was retained.
All 18 target CPU loss counters and host text-loss deltas were zero. Each
capture retained the actual first configuration request/cancellation with
more than 6.6 seconds of remaining target observation beyond its host duration.
Earlier descriptor traffic differed between boots; the first-configuration
sequence is the comparison, not a one-to-one match of every record.

The enabled case recorded one ep2in request with actual 0, length 10 and
status -EINPROGRESS. A NORMAL TRB and successful native START followed; EP0
STATUS2 START followed that. No matching notification giveback or subsequent
transfer completion appeared before trace stop. Ten earlier successful EP0
request givebacks had valid names and scalar fields. The exact source and
closed-TTY path support attributing the 10-byte request to SERIAL_STATE;
no notification payload bytes were captured.

Both trials completed guarded capture/archive and recovered the pinned
unmarked baseline after the armed watchdog expired, without a software reboot
request. No hardware reset-cause register was read. Neither trial opened a
host or target data TTY, sent a configuration ACK token, or ran a data helper.

Interpretation
--------------

The pair qualifies the two enabled event formatters for this bounded diagnostic
case. It does not fix or qualify ACM configuration, serial data or USB SSH.
A queue trace occurs before DMA mapping/kick and is not usb_ep_queue's return.
A giveback reports software completion fields before the callback, not remote
wire delivery. EP0 uses a separate queue path and need not emit ep_queue.

Enabled tracing changes execution timing. The evidence does not identify a
unique FIFO, resource-index, ring, DART,
clock, PHY or firmware cause. Request events do not expose the returned START
resource index or cached ring state. Further observations or programming
changes require their own controlled comparison and hardware qualification.
