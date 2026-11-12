J716c bulk-IN submission experiment
===================================

For the later mapping/submission differential on this branch, including its
new Image/notes pins, see ``j716c-mapping-separation.rst``. Results below describe
the preceding guarded-serialization build.

On J716c/T6050 (M5 Max), bulk-only CDC-EEM can provide cable SSH with unmodified
Ethernet modules. Smaller exchanges sometimes pass, while a larger fixed exchange
reproduces a stall: the host retains receive URBs, the gadget leaves queued IN
TRBs unretired, and unexpected empty IN transfers appear. ACM notification and
configuration are a separate unresolved failure.

This branch contains a deliberately gated controller-level diagnostic workaround.
It is not an enabled default policy or a general DWC3/Apple Silicon fix.

Matched RAM results
--------------------

All rows share one Image, config,36 unmodified modules, initrd, helpers and payload.
The normal Ethernet TX/RX pools stay10; MTU1500/qmult5 and native FIFO fields stay
unchanged. A chosen RAM-test marker selects the immutable cached policy before
USB gadget registration. One pinned-key SSH session checks16 alternating4096- and
65536-byte frames,557056 exact bytes (544KiB) in each direction, token/sequence/
length/SHA256 and EOF. Three ICMP echoes each direction precede data.

================== =========== ================ =========== =============
Profile            Class peak  Prepared IN peak  IN retired  Data result
================== =========== ================ =========== =============
Normal control     10          10               119/129     Timeout
ACK UPDATE         10          10               154/162     Timeout
Serialized IN #1   10          1                628/628     All bytes pass
Serialized IN #2   10          1                621/621     All bytes pass
================== =========== ================ =========== =============

The control and acknowledged rows produce3 and5 empty host IN transfers;
serialized rows produce none. Every host capture is complete with zero per-reader
loss, every target's18 CPU loss counters are zero, and every physical watchdog
recovery and scoped host cleanup passes. Successful rows exit the observer0.
The failures and partial outputs remain retained; they are not repaired by later
success. Raw per-unit traces/addresses/boot UUIDs stay private; curated receipts
and hashes are in ``j716c-bulk-policy-results.json``.

A subsequent eightfold EEM3 row uses the exact same Image, modules, initrd, DT
and stage2, with128 alternating4KiB/64KiB frames:4,456,448 exact bytes (4.25MiB)
each direction. All4,851 IN requests are prepared and retired, the normal class
peak remains10, and prepared peak remains1. Enqueue indexes follow exact modulo255
progression through19 ring wraps. There are no empty host IN transfers, USB
completion errors or host/all18CPU capture loss; observer exit, physical recovery
and scoped cleanup all pass. Original finite data/recovery deadlines are retained;
only trace RAM and host metadata bounds grow. This is one additional finite row,
not sustained reliability or a throughput benchmark. See
``j716c-bulk-wrap-results.json`` and ``j716c-eem-wrap-data/``.

Earlier paired FIFO-field tests changed only bits17..21 for ep1in1->3 and
EP1out0->2, retaining all other endpoint parameters. Both larger exchanges still
failed. See ``j716c-eem-fifo-results.json``. A cached-ring witness and header-only
host capture in ``j716c-eem-stress-results.json`` retain the original failure:
59 host IN reads pending, no observed USB completion errors, a512-byte result
then an empty result, two host net RX errors and ten unretired device IN TRBs.
Those observations do not establish a precise descriptor-fetch or DMA cause.

Policies and scope
-------------------

The experiment uses the existing exact-board USB0 peripheral/retirement/PHY/core/
HS/revision/endpoint-geometry admission and initial-empty-START skip policy.
Only these new markers select the added behavior:

* ``usb2-bulk-control`` caches policy2: normal preparation and no-response UPDATE.
* ``usb2-bulk-ack`` caches policy3: exact HS512 non-stream bulk ep1in uses CmdAct
  and the existing bounded status poll for UPDATE; CmdIOC remains0.
* ``usb2-bulk-single`` caches policy4: exact same endpoint prepares one request,
  deferring the rest until existing completion cleanup kicks pending work.

The single-request path finishes partial SG preparation for the current request
before deferring later ones. Actual tested EEM requests are linear and padded
away from512-byte multiples, so each has one normal TRB; prepare/giveback deltas
therefore establish the prepared peak. A complete_trb trace entry with HWO1 is
not counted as successful retirement.

Other endpoints, EP0, OUT, interrupt/ISO/stream traffic, controllers and default
profiles retain their existing behavior. No IRQ-time DT lookup, additional lock,
FIFO resize, PHY/DART/PMGR programming or register-selector probe is introduced.
The tested change bounds request mapping/preparation/submission overlap, so it
does not distinguish ring, mapping/DMA or finer hardware timing as the cause.

Normal no-response UPDATE clears CmdAct/CmdIOC, skips polling and leaves status0
in software; its Successful trace label is not hardware acceptance. The ACK row
actually sets CmdAct, polls a zero hardware return for all161 IN UPDATEs, and
still fails. Acknowledgment alone is insufficient in that observed row.

Build and test pins
--------------------

* Source baseline:fc7b007c157eaf67c7f0ef460b8196fda578e154, plus the FIFO diagnostic
  scaffolding and the three bulk-policy source changes on this branch.
* Release:7.3.0-j716c-usb2-fifo-field-probe; use the notes pin and marker to identify
  the variant, not release text alone.
* Image.gz SHA256:9f92c67b5ed1b55a9708c06f41c7293f1827dd35864017f484b4a500663d6937.
* Notes SHA256:3199d828daf516509affa8192d4629a3be6c557155a35efe4b218a067fde8ca3.
* u_ether.ko SHA256:5f62f0140a95bb97ba8bc2758898b6b52380c507da3c65b12c7f7e70e8f26736.
* usb_f_eem.ko SHA256:2be3513add7405da62488d7808db844ad31ddf3e59be0cd738e835ac7c5262c1.

The config and all66 generated C headers match the preceding FIFO kernel. Only
gadget.c and dwc3-apple.c require recompilation; the private admission header also
changes. Class modules/source remain unmodified. MODVERSIONS is unset, so this
is source/config/provider provenance and runtime evidence, not strong CRC proof.
The external class builds carry their expected module taint. Compiler usesLLVM22
and the existing public bring-up environment. ``j716c-bulk-policy.config`` retains
the actual config; no cross-environment bit-identical rebuild is promised.

``j716c-eem-data/data.c`` is the finite EEM2 SSH stdio peer and SHA256 helper.
It exposes no shell/device/network/watchdog control and requires the caller to
validate every byte plus a separately authenticated machine identity. Credentials,
unit firmware and bootable initrd are deliberately not distributed in this source
branch. Host USBmon uses GETX64-byte headers with alloc0/dataNULL and a4MiB private
ring; no transfer payload is copied. Binary timestamps are CLOCK_REALTIME with
sampled offset guards; target admission is causally bounded using BOOTTIME.

Recovery and limits
--------------------

A detached native owner opens one watchdog FD, sets/reads120seconds, issues one
final explicit keepalive, then holds without feeding/closing. Its new admission
constants bind the exact Image notes and these three markers; watchdog logic is
unchanged. An explicit75-second pre-open boot lease covers early refusal. All
post-loader outcomes wait physical expiry, fresh factory USB generation and
strictly authenticated original baseline before removing only the empty owned
host namespace/rule. Kernel-owned network links stay UP until physical reset.

The data command keeps a9-second inner/10-second outer ceiling,38-second data
phase,48.9-second body and58.9-second observer ceiling. No data retry or software
reset repairs failure. No SSD/ESP, installation, enrollment or boot-policy write
was performed. This is bounded single-unit evidence, not sustained throughput,
long-duration stability, other-port/board coverage, suspend or ACM repair.
