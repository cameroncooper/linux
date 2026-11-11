J716c: EEM single-TX diagnostic
==============================

This branch deliberately allocates one u_ether TX request, retaining the normal
RX pool. It isolates overlapping USB IN requests; it is not an upstream-ready
global throughput limit or a general DWC3/ACM fix.

On J716c/T6050 (M5 Max), unmodified bulk-only CDC-EEM completes configuration,
direct SET_INTERFACE and three echoes each way, then stalls in the fixed SSH
exchange. MTU480 and qmult1 alone also stall. With TX preallocation changed from
n to1, the same data row passes at MTU1500/qmult5:

* One strict pinned-key SSH session with the existing Dropbear port2222.
* Exact4096 and65536 synthetic bytes in each direction;69632 bytes per direction.
* All102 bulk-IN queues have successful givebacks; peak outstanding1, none left.
* All18 target CPU trace-loss counters zero; complete host capture with zero loss.
* Observer exit0, physical-only watchdog recovery, authenticated original baseline,
  and cleanup of only the owned empty host namespace/rule.

The original MTU1500/qmult5 module times out on that row. Two earlier single-TX
MTU480 runs pass every byte but have host text-monitor drops, so they are not full
capture qualifications. All results are retained in ``eem-runtime.json``.

Exact runtime inputs
--------------------

The RAM kernel was not rebuilt with this change. A matching external module was
substituted; the other35 modules stayed unchanged. Its unmodified external
control has identical executable/provider sections to the original module.
__FILE__ string pooling and the absent intree metadata differ; the diagnostic
honestly carries the expected external-module taint O. Types, imports, exports,
generated headers, config, Image, notes, DT, DWC3 and FIFO fields are unchanged.

* Release:7.3.0-j716c-usb2-request-trace.
* Image.gz SHA256:e2d2105e285187867bb6264db08e7208a4d80461e1a0abdf573aba70bc2a6a54.
* Notes SHA256:807d8869ee3cd4de250a3751e9d4e4df6e4bfaa277562f9e380b447ac75d0135.
* Diagnostic u_ether.ko SHA256:b1ff58a3f09e490ac8a909c116a73eff3c318f369dec49d17c412aedc4f0da5d.
* usb_f_eem.ko SHA256:9ebe449b0ebd9d00a1c19cb3c95caaf59055267fdb9bf768555a9c324a327756.

The source base includes the initial-empty-START experiment and request trace-name
initialization fix. USB0 uses that exact qualified handoff profile; these results
cannot be attributed to the one line on an arbitrary kernel/loader. Only the
connected high-speed USB0 path was tested. No storage, ESP, enrollment, policy or
installed boot configuration changed. Raw per-unit traces stay private; the JSON
contains synthetic results, public pins and receipt hashes.

``eem-data/data.c`` is the finite SSH stdio peer, with its SHA256 helper beside it.
It accepts one32-lowercase-hex token and two H2T frames (4096,65536), checks their
deterministic bytes/headers, returns H2T receipts and T2H frames, then requires
EOF and emits one PASS line. Headers carry token/direction/sequence/length/hash.
The caller must check every byte, EOF and the separately authenticated machine
identity. The peer provides no shell, network, device or watchdog control.

Interpretation and next work
----------------------------

Serializing TX is a bounded working workaround and points toward overlapping-IN
handling. The precise DWC3 ring/resource/DMA cause is not established. Next is a
matched external unmodified-control rerun and cache-only ring observation. ACM
notification/configuration still fails. Throughput, long duration, suspend,
other ports and persistent USB networking remain unqualified.
