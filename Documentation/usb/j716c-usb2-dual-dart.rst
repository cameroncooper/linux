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
sustained operation or installation. Actual serial qualification using the public ACM class and explicit
controllerBULK diagnostic is described below. The later LPM-disabled profile
also qualifies quiet post-open notification progress and simultaneous EEM/ACM. Private keys, identities, raw captures and address values are
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
j716c-acm-type-class-admission.patch reproduces its source from topic commit6b2adde5;
only the cached marker allowlist differs, class/datapath/descriptors unchanged.
No kernel Image/Aura change is needed for the existing explicit mode12.

These passes include bulk-OUT serial traffic before real-notification
qualification. Earlier mode12 notification-only experiments sent no such
traffic, so this does not establish that dual mapping alone changed their
outcome, or that notification progresses independently of bulk traffic.
The subsequent native-class cases below check startup notification before data.
Those class-suppressed cases alone do not qualify the public class, sustained
serial, normal installed boot or a general endpoint-type policy.


Public ACM class without suppression
-----------------------------------

The topic now restores f_acm.c byte-for-byte to pristine public Linux source
(commitdc8f6f77934b471a13c91e7780e2d91ebc904b3d), matching the frozen kernel
source. No board-specific class flag/guard/notification suppression remains.
Class moduleeb21d4d86e24ae7e8a9cbec3ae39a2415de6eb479817cd5691f84725d45351bd
uses the unchanged headers/ABI. Controller diagnostic mode12 stays explicit;
no generic endpoint-type policy is changed by restoring the class.

Two RAM boots qualify the native initial-zero10B notification before any
serial bulkOUT or targetTTY authorization: matching queue/giveback, host10B
completion and cachedDSR/DCDclear. TargetTTY open then queues a separate real
notification, and each boot verifies69632B serial each way,138/138 IN requests
via151TRBs and2/2 total notification givebacks. Both have complete loss-free
host/18CPU captures, clear shared-root DARTs and authenticated physical recovery/
held-FD cleanup. Only the class module replaces the diagnostic class in initrd;
Image/Aura/DT/helpers/worker/observers/otherCPIO payloads remain identical.

The second boot adds a passive2-second wait after targetTTY open. Its real
connect notification does not complete within that idle window; it completes
after serial bulkOUT starts, with cached modem38->358. Native startup delivery
is independent of bulk data, but quiet post-open progress remains a real gap.
This distinction is retained in adjacent j716c-standard-acm-runtime.json;
standard-class source provenance and both complete qualifications are included.
The earlier class-suppressed pair remains reproducible with the adjacent
historical class patch applied to pinned pre-restoration topic commit
6b2adde513f09fac2d70568c1eaa8261cce6da33.


Quiet connection notification and USB2 LPM
----------------------------------------

A matched host-only experiment waits3s with no new control/bulkOUT. The real
notification stays pending. In two other boots, after2s quiet, one standard
read-only device GET_STATUS returns2bytes; real notification and modem update
then complete before the first bulkOUT. Target notification giveback follows
the EP0 request by2.252ms/18.133ms. Exact one successful80/00/0000/0000/0002
control is matched in loss-free USBmon; no SET, claim, unbind or reset occurs.
Each boot also completes the original full serial protocol, all captures and
physical recovery. This identifies progress with control activity, not a
specific internal hardware cause. Curated results and the guarded synchronous
read helper are adjacent (j716c-acm-control-stimulus-runtime.json and
j716c-acm-get-status.py).

A target-side matched experiment changes only the standard zero-length DT
boolean on the exact restricted USB0 peripheral node::

  snps,usb2-gadget-lpm-disable;

The existing DWC3 core parses it, clears its LPM capability and libcomposite
advertises bcdUSB0200 instead of0201. Native public class, Image99b3/Aura7b68,
initrd/upload/helpers/worker/observers, dual retirement/map1 and controller
BULK12 remain byte exact. Control has host runtimeactive/controlon but hardware
USB2LPM enabled, and real notification remains pending through3s. Candidate
passes twice: actual0200, hardwareLPM attributes absent, initial native-zero
then real-connect notification/modem update before any dataOUT or intervening
control, full69632B eachway/138IN and2notifications, clear shared-root DARTs,
complete host/18CPU capture and authenticated physical recovery/cleanup.
See adjacent j716c-acm-lpm-runtime.json.

Advertisement/BOS and existing DCFG.LPM_CAP change together; no actual linkL1
state, exact internal cause or power/performance result is claimed. Disabling
LPM addresses quiet progress for this fixed RAM profile; it does not restore
all native controller policies. A first unmodified-class INT8 input with LPM
disabled still fails configuration with clean captured DART/fault state and
physical recovery. Native transfer-policy restoration remains unresolved;
simultaneous EEM/ACM qualification follows below. No generic board DT/host/USB4/persistent
installation change is made by this topic's documentation.


Simultaneous cable SSH/EEM and serial
------------------------------------

Two bounded RAM boots use one ACM-first/EEM-second gadget with exactly three
interfaces and five endpoint roles. Each verifies69632 serial bytes each way
concurrently with4456448 EEM bytes/128frames each way over pinned cable SSH,
with independent per-end exact hashes and EOF. Both host protocol windows
and kernel IN queue/giveback windows overlap. EEM retires4905/4905 and
4952/4952 IN requests with normalTRB811, original ten-buffer pools, no empty
EEM IN or endpoint completion errors, and final device IN pending0. Native
initial-zero notification completes before any dataOUT; a separate real
notification completes during the wider traffic workflow. This composite
workload does not add a deliberate quiet real-notification pause.

The second boot adds only snps,usb2-gadget-lpm-disable to the restricted DT,
combining the previously qualified quiet-progress setting with the composite
graph. Image99b3, Aura7b68, initrdd276, uploaded functional graph, public ACM/
EEM classes, native helpers and observers are byte exact between these two
boots. LPM-off cached bcdUSB0200 is recorded; baseline0201 is inferred from
frozen source/DT because its cached descriptor decode was not retained.
Both have complete loss-free host/all18CPU captures, clear paired DARTs and
matching valid owned roots, authenticated physical recovery and held-TTY/
scoped namespace/rule cleanup. Adjacent j716c-usb0-composite-runtime.json
includes the stronger per-endpoint, pool, byte/hash/EOF and recovery checks.

These passes retain explicit controllerBULK12 and earlier exact-board DWC
bring-up policies. They establish bounded concurrent serial/network control,
not native controllerINT, sustained throughput/reliability, hotplug or an
installed system. Transfer measurement remains enabled in these two cases;
a repeat without USBmon, target tracing and observer modules is separate.


Native transfer-policy controls
-------------------------------

With LPM disabled, public ACM class and the same dual-DART Image/Aura, two
further inputs retain normal controller interrupt notification type: cached
bulk7/NOOP_SKIP2 and bulk0/NOOP_CONTROL1. These select ordinary bulk transfer
policies with the initial empty START skipped versus retained. Both fail the
first configuration (one host attempt, identity-matched completion status-2),
before serial data. Policy-selection logs, complete host/all18CPU captures,
clear paired DART diagnostics, authenticated physical recovery and scoped
cleanup pass. This status is a host observation, not a hardware root-cause
diagnosis. Adjacent j716c-acm-native-path-runtime.json retains both negatives
and build pins. Successful complete per-endpoint native START/TRB behavior is
not claimed from a startup failure.


Functional composite without transfer measurement
------------------------------------------------

One completed boot removes host USBmon, target transfer tracing and all nine
DART/FIFO/AIC observer modules from the LPM-disabled working composite graph.
It verifies concurrent pinned cable SSH/EEM4456448bytes/128frames each way
and ACM69632bytes each way, exact per-end bytes/hashes/EOF, overlapping host
protocol windows and cached DSR/DCD clear-to-set. Tracefs stays unmounted,
observer modules are absent before/after, all18 CPUs remain online, selected
late-DT metadata and kernel-log checks pass. Host/target TTYs stay held until
authenticated physical watchdog recovery; scoped cleanup passes. Adjacent
j716c-usb0-nodiag-runtime.json records the functional checks and limits.

The first attempt refused a legitimate udev rename after caching the EEM
interface name, before target network-up/data, and recovered physically. A
new host-only revision compares stable ifindex across the rename, preserving
the exact USB generation, interface/driver and isolated-namespace guards.
Target scripts, upload and all raw stage inputs remain byte exact. No packet
notification counts, kernel traffic overlap, TRB/pool counts, DART register
state or FIFO observations are claimed without those instruments. Existing
controller profile12, initial-empty-START skip and LPM disable remain explicit;
this does not qualify native/default controller policies or sustained use.
