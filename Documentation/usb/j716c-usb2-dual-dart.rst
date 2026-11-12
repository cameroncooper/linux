J716c USB0 dual-DART handoff
===========================

The restricted RAM USB2 profile mapped bank1/SID1 only. Normal open-ended
EEM transfers stalled and bank0/SID0 latched a read/invalid-TTBR exception,
leading Linux to disable the shared unhandled fault IRQ. Native T6050 DTS
assigns DWC0 both bank0/SID0 and bank1/SID1. A passive lookup confirmed the
retained fault IOVA was currently valid in DWC0's existing software domain;
no address value was retained/exported and no exact transaction attribution
is claimed.

Contract3 independently retires SID0 under full controller reset, then
optionally gives the exact DWC0 consumer its native two-DART domain. Normal
EEM3 now passes twice:4456448bytes/128frames each direction,4978/4978 and
4938/4938 IN requests retired,19ringwraps each, class/preparation peaks10.
Both use normal TRB811, one START and4977/4937 normal no-response UPDATEs,
no LST or transfer-complete subscription, unchanged pools10/MTU1500/qmult5.
There are no empty IN transfers, host USB completion errors or new kernel
fault markers. Both banks remain clear and share valid owned roots through
the38-second observation window. All host/18CPU captures are loss-free;
physical-watchdog recovery and scoped cleanup pass.

The counterbalanced map0/map1/map0/map1 controls use identical Image, Aura,
initrd, native helpers, observers and workload. Only the input mapping flag
differs. Retiring and blocking SID0 without mapping it quiets the fault but
still stalls151/158 and104/114 IN requests. Thus suppressing the fault alone
does not repair the data path. A new-binary old-contract2 reference also
reproduces153/160 and the original fault. All five recover physically.

Retirement and admission
------------------------

Static input retains bank0 disabled/native-compatible and a single bank1
IOMMU reference. Existing handoff1 with no new flags remains contract2.
The new exact u32 chosen properties are::

  j716c,usb0-dual-retire = <1>;
  j716c,usb0-bank0-map = <0>; /* retirement-only control, or1 to map */

Aura validates exact board/firmware/ADT/port/resources/shared IRQ/power and
prepares every late-tree change before MMIO, retaining the unchanged-tree
CRC check. A one-shot receipt from the existing SID1 device halt/reset and
full wrapper clamp/reset permits SID0 retirement without reading DWC behind
reset. Bank0 must have the observed v2.4/16stream/page14/42bit layout, no
protection, enabled streams, masks or faults, and drained transactions.
Only SID0 TCR/root are cleared and FLUSH_SID(0) issued; foreign raw TCR/root
words and controls must remain unchanged. No unlock/global flush/foreign
ACK/enable/root write occurs. Any uncertainty refuses the handoff.

Only after both retirements does Aura publish contract3. Map0 retains the
old single-bank topology. Map1 enables the exact bank0 provider, creates its
phandle if needed and publishes ordered bank0/SID0 plus bank1/SID1 references.
The kernel's full metadata guard admits only that exact topology. Each
experimental DART caches its one owned SID; the existing multiple-DART
mapper installs shared page tables. No stream bitmap is widened. Supplier
references, mappings and kexec blocks remain until physical reset. Paired
invalidation, PM or publication errors cannot return to callers that might
release memory: diagnostics/memory are retained via panic0 and physical WD.

Pins and scope
--------------

Image99b3a44cc08fcbcc95a4a3ec3dc70da0c25a959230b972b085e9f056c0863704;
notesc6fd6340d24cbc64fd0bbc2dfff635c398d097bcb370db231e640cfc17f11ff3;
release7.3.0-j716c-usb2-fifo-field-probe. Aura raw
7b68e97701d1b63098a961dc41aef560e037af3f7fe9704e1c337a451d5188ae,
4063232bytes. Configuration,66generated C headers, Rust and export CRCs
match the prior qualified kernel. New kernel source is confined to the DART
and shared admission header; DWC/AIC source is unchanged from tested5524.
This working topic also carries the prior AIC reporting fix and cached
DWC diagnostics inherited by that build; they are not new default policies.
Selected kernel files match the tested source; publication-tag rebuilds
are not claimed. Full build pins/configuration and all cases are adjacent.

Aura source and guarded handoff are on cameroncooper/aura branch
j716c/usb2-dual-dart-handoff. Generic PHY initialization/clock22, broader USB4
and reserve-and-boot-on-failed-retirement changes are not mixed into this
experiment. Clock42 and the previously qualified all-core routing remain.

One Mac17,6/J716c/T6050, mBoot20457.41.3, fixed left-back USB0 cable/port was
tested. Every run uses a verified native120second lease, one final keepalive
and retained FD, explicit panic0 and authenticated physical recovery. No
post-loader software reset, unbind/unload, SSD/ESP/install/NVRAM or boot-policy
write occurs. Original unmarked recovery baseline remains unchanged. This
qualifies bounded RAM EEM traffic, not USB3/USB4/host/hotplug/suspend,
sustained operation or installation. Real ACM interrupt notification remains
a separate failing test even with both DART banks clear; no serial fix is
claimed here. Private keys, identities, raw captures and address values are
excluded.


Extended EEM and serial boundary
-------------------------------

A longer unchanged-map1 boot verifies17825792bytes each direction and512
alternating4096/65536-byte EEM4 frames through77 ring wraps. All19759 IN
queues/TRBs/givebacks retire, peaks remain10, one START plus19758 ordinary
UPDATEs, normalTRB811/noLST/noCMPLEN. Both DART banks stay clear with matching
valid roots, no emptyIN/hostUSBerrors/newkernel faults, zero host/18CPU
capture loss, and authenticated physical recovery/cleanup. This adds bounded
coverage, not sustained reliability. Image/Aura/DT/initrd/native helpers and
observers are byte-exact to the earlier map1 fixture. Only the synthetic data
helper/protocol and its bounded SSH timeout20s/outer21s change inside the
same38s observation/58.9s finish/120s physical-watchdog envelope.

Adjacent j716c-dual-dart-long-runtime.json records complete checks and build
pins. j716c-eem-512frames.c/.h are the finite native stdio payload checker
(header include renamed only for publication); they control no device,
watchdog or network. Per-end receipts and EOF are checked independently by
the private host harness; output queued by the target alone is not a pass.

The initial interrupt-type ACM case stalls9 bulk-IN requests plus one real10B notification,
all with zero givebacks, despite healthy dual mapping. Full captures and
physical recovery/held-FD cleanup pass. A passive25-read/zero-write FIFO
module confirms FIFO1/2 each199words/3184B, starts35/234words and distinct
unchanged ranges; sharedRX stays1000words/16000B. Only FIFO2 bit15 changes
0to1 after configuration. The private observer parameter tx_ram_number was
overnamed: the introducing Synopsys patch labels that field asynchronous/
periodic TXFIFO, not a proven RAM selector. See Linux commit
0cab8d26d6e5e053b2bed3356992aaa71dc93628. Its exact polarity/update mechanism
and stall causation are not established. Curated adjacent ACM/FIFO results
use tx_bit15. No missing/overlapping allocation was observed; no FIFO writes,
resize change or standard-ACM qualification is claimed.


Actual notification with controller BULK type
--------------------------------------------

A fresh matched pair on healthy dual mapping changes only controller dep5
EPtype INT3 toBULK2 (cached diagnostic mode8 versus12). Host descriptors remain
identicalINT/MPS10/interval9, FIFO2 and all other observed endpoint parameters
match. INT still stalls9 bulk-IN requests plus1 real10B notification. BULK
passes twice: real10B notification1/1, cachedDSR/DCD clear-to-set and69632bytes
each direction,138/138 bulk-IN givebacks via151TRBs. Both DARTs stay clear
with shared valid roots; all host/18CPU captures are complete/loss-free and
physical-watchdog recovery/held-TTY cleanup pass. FIFO starts/depths remain
unchanged. Bit15's asynchronous/periodic classification differs with type;
no precise internal controller cause is established.

Adjacent j716c-dual-dart-notification-type-runtime.json includes both
positives, negative control, observed type-only parameter comparison and all
pins. Both arms use one common external class modulecc8ea23c, preserving the
initial-zero notification suppression diagnostic. The adjacent one-line
j716c-acm-type-class-admission.patch reproduces its source from this topic;
only the cached marker allowlist differs, class/datapath/descriptors unchanged.
No kernel Image/Aura change is needed for the existing explicit mode12.

These passes include bulk-OUT serial traffic before real-notification
qualification. Earlier mode12 notification-only experiments sent no such
traffic, so this does not establish that dual mapping alone changed their
outcome, or that notification progresses independently of bulk traffic.
Unmodified-class startup notification before any bulk-OUT is the next test.
This remains bounded diagnostic bring-up, not standard ACM, sustained serial,
normal installed boot or a general endpoint-type policy recommendation.
