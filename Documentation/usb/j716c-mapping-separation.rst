J716c mapping versus controller submission
==========================================

For the subsequent finite-transfer experiment and its newer Image/notes pins,
see ``j716c-transfer-boundaries.rst``. Results here describe the preceding build.

The preceding guarded bulk-IN SINGLE policy passed larger EEM exchanges while
retaining ten normal Ethernet buffers. It also delayed DMA mapping, so that
result alone did not distinguish mapping overlap from descriptor submission.
The finite4.25MiB each-way follow-up passed4,851 IN requests through19 ring wraps;
its exact older build pins are in ``j716c-bulk-policy.rst``.

This branch adds an exact-board diagnostic marker ``usb2-bulk-premap`` (cached
policy5). It maps nonempty linear IN requests at queue admission while retaining
SINGLE preparation. The same DMA API, direction and ``dwc->sysdev`` are used.
The existing private request ``mapped`` bit records ownership: later preparation
skips a duplicate map; every giveback unmaps it, including cancellation before
any TRB was prepared. Mapping failure releases the existing PM reference and
returns before list insertion. SG requests retain preparation-time mapping.

The existing exact J716c USB0/retirement/PHY/core/revision/HS/endpoint geometry
guards remain. PREMAP applies only to non-stream HS512 bulk ep1in; OUT, EP0,
interrupt and other controllers/profiles retain their previous behavior. There
is no DMA/IOMMU bypass, new hardware register programming, added lock, or IRQ-time
DT lookup. An initialized ``dwc3_j716c_premap`` request-event alias records each
successful early map. It uses the existing request class with its name initialized.

Matched RAM rows share one Image, config,36 unmodified modules, initrd, payload,
native helpers, normal Ethernet pools10, MTU1500/qmult5 and native FIFO fields.
Only the chosen test marker changes. One pinned-key SSH connection verifies16
alternating4096/65536-byte EEM2 frames:557056 exact bytes each direction, with
token, sequence, length, SHA256 and EOF. Capture/recovery results and measured
request/mapping/preparation peaks are in ``j716c-mapping-separation-results.json``.

Measured result
-----------------

======================= =========== ============= ============== ===========
Profile                 Class peak  Early map peak Prepared peak  IN retired
======================= =========== ============= ============== ===========
SINGLE                  10          none          1              619/619
PREMAP + SINGLE         10          10            1              621/621
Normal CONTROL          10          none          10             100/110
======================= =========== ============= ============== ===========

"None" means no early-map event: the ordinary preparation-time DMA map still
runs. Both serialized rows verify every557056 bytes each way. PREMAP records621
successful early maps and621 givebacks with no maps left outstanding. All actual
linear IN requests use one NORMAL IOC TRB; enqueue indexes follow modulo255.
Normal CONTROL times out with ten IN requests unretired and three empty host IN
transfers despite59 host receive URBs remaining posted. It is retained as a data
failure. Every row has complete host and18CPU target capture with zero loss,
physical baseline recovery, and scoped cleanup. Both passing observers exit0;
the failed control observer exits1 because its required data receipt is absent.
There are no observed USB completion errors in any row.

Concurrent DMA mappings alone are insufficient to reproduce this stall when
controller descriptors remain serialized. The matched control reproduces it
with overlapping descriptor preparation/submission on the same binary. This
narrows the observed differential; it does not prove a hardware erratum or
exclude finer mapping timing, ring fetch or other controller interactions.

Build pins
------------

* Image.gz SHA256:ff85906ff880e2028d27ad998a8579af4f18732d469d2b485b0e157459b597c9.
* Notes SHA256:1c910347e47844baf0b1f670771c8ae63be4b17094d07e59bda3bfb240c0de38.
* Release:7.3.0-j716c-usb2-fifo-field-probe; notes/marker distinguish the build.
* Config and all66 generated C headers match the preceding serialization build;
  the rebuilt Rust archive is byte-identical. LLVM22.1.8 builds without warnings.
* u_ether.ko SHA256:5f62f0140a95bb97ba8bc2758898b6b52380c507da3c65b12c7f7e70e8f26736.
* usb_f_eem.ko SHA256:2be3513add7405da62488d7808db844ad31ddf3e59be0cd738e835ac7c5262c1.

The qualified native watchdog owner and read-only lease verifier change only
admission constants: these notes, exactly CONTROL/SINGLE/PREMAP markers, and the
new owner hash in the verifier. Their watchdog behavior is unchanged: one held
FD, SET/GET120s, one final explicit keepalive and no userspace feed loop or close.
All post-loader outcomes await physical expiry and authenticate the unmarked
baseline before scoped host cleanup. There is no post-loader software reset,
USB reset/unbind, module unload or early interface/namespace teardown.

The original9s inner/10s outer data and38/58.9s observer deadlines remain. Binary
USBmon captures headers only and checks per-reader loss; all18 target CPUs have
separate loss counters. Raw per-unit UUIDs, addresses, pointers, keys, firmware
and payloads remain private. Tests are RAM-only with no SSD/ESP/install/boot-policy
writes. This remains a finite one-board/port/cable diagnostic, with no sustained
reliability, persistent deployment, broad SG/cancellation runtime qualification
or ACM serial repair claim. Precise controller/ring/DMA timing remains open.
