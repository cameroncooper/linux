J716c finite bulk-IN transfer boundaries
=========================================

The preceding matched mapping diagnostic passes with ten concurrently mapped
buffers and one prepared IN request, while normal overlapping submission stalls.
See ``j716c-mapping-separation.rst`` for that separate Image and its measured rows.
This experiment asks whether explicit hardware transfer boundaries permit normal
mapping/preparation overlap. It is exact-board, marker-gated and disabled by default.

``usb2-bulk-last`` selects cached policy6 under the existing exact J716c USB0,
retirement, PHY/core/revision/HS/geometry admission. For non-stream HS512 bulk
ep1in only, it sets LST on the final request TRB and enables XFER_COMPLETE_EN in
endpoint configuration. The existing transfer-complete handler clears the active
flag, cleans completed requests and starts remaining work. Ordinary DMA mapping,
full TRB preparation, IN_PROGRESS_EN, FIFO fields, commands, normal Ethernet
buffers10 and MTU1500/qmult5 remain. OUT, EP0, interrupt and other profiles retain
their prior behavior. There is no new DMA/IOMMU programming, selector probe,
additional stop/reset sequence, lock or scheduling delay.

An initial LST-only fixture omitted XFER_COMPLETE_EN. Ordinary non-stream Linux
bulk configuration enables only transfer-in-progress events; that first fixture
failed during ping with seven prepared requests and no givebacks/IN events.
It is retained as a fixture deficiency in ``j716c-transfer-boundary-initial.json``;
it does not establish that correctly configured finite transfers cannot work.
That report also preserves an earlier short normal-control success and an
incomplete host capture. The corrected matched pair is reported separately in
``j716c-transfer-boundary-results.json``.

The corrected pair uses one Image, config,36 unmodified modules, initrd, payload,
native helpers and EEM3 workload. Only the chosen marker differs between stages.
One pinned-key SSH connection requires128 alternating4KiB/64KiB frames and
4,456,448 exact bytes (4.25MiB) each direction, including nonce, sequence, length,
SHA256 and EOF. Host capture is binary headers only with per-reader loss counters
and a bounded final drain; all18 target CPUs have separate trace-loss counters.
The intended hardware deltas are TRB bit1 (LST) and endpoint configuration param1
bit8 (transfer-complete enable), with all other endpoint parameters unchanged.

Measured results
------------------

* control: class/prepared peaks10/10, 127/137 IN retired, data timeout.
* last: class/prepared peaks10/10, 4864/4864 IN retired, all bytes pass.
* last_repeat: class/prepared peaks10/10, 4878/4878 IN retired, all bytes pass.

Both LAST rows verify every4,456,448 bytes each direction with ten mapped/prepared
requests permitted concurrently. Each retired request has one transfer-complete
event and one polled-successful START; no transfer-in-progress events occur on
that IN endpoint. Both traverse19 exact modulo255 ring wraps. They have no empty
host IN transfers, USB completion errors, pending device requests or new kernel
fault markers. All target18CPU and complete host captures have zero loss, and
both observers, physical recoveries and scoped cleanups pass. The matched normal
CONTROL remains a data failure despite complete zero-loss captures and recovery.
Its ten prepared requests are left unretired; all other endpoint configuration
parameters match, apart from the intended IN transfer-complete-enable bit.

These rows show normal concurrent DMA mappings and TRB preparation can work with
finite transfers and the existing completion/restart path. The observed failure
is more specific than concurrent mapping/preparation alone. The exact behavior
of open-ended IN transfers, UPDATE timing and controller/ring fetch remains
unresolved. No-response UPDATE's Successful label is a software default without
polling; START statuses are actually polled. This is a guarded diagnostic result,
not a general hardware erratum or sustained reliability claim.

Build pins
------------

* Image.gz SHA256:02881e43009faaa0feea8beb265c9d577bab1100e216c6e66013004086f5054a.
* Notes SHA256:8c0cc1a1501c43af0f8070058e6feba733d8aeca9844f8b936dd7b0ad87e2c9f.
* Release:7.3.0-j716c-usb2-fifo-field-probe; identify the build by notes and marker.
* Config/all66 generated C headers and the Rust archive match the previous build.
  LLVM22.1.8 builds without warnings; class modules/source remain unchanged.
* u_ether.ko SHA256:5f62f0140a95bb97ba8bc2758898b6b52380c507da3c65b12c7f7e70e8f26736.
* usb_f_eem.ko SHA256:2be3513add7405da62488d7808db844ad31ddf3e59be0cd738e835ac7c5262c1.
* EEM3 peer SHA256:cadc377709e4110d9acd0c5238bce0d36a9b2734f916c1dd7a22a4888374942f.

The qualified watchdog owner/verifier change only admission constants: notes
and the new owner hash. Their behavior remains one held FD, SET/GET120s, one
final explicit keepalive and no periodic userspace feed or close. Every outcome
after loading waits for physical expiry, authenticates the unmarked baseline
and only then performs scoped host cleanup. Data deadlines remain9s inner/10s
outer, target observation38s, finish58.9s. Trace RAM is16MiB per CPU. Host capture
runs at least60s plus at most1s final drain checked between operations; the first
empty-queue snapshot defines its finite boundary. Later arrivals are outside it.
Every missed bound, lost event or unresolved control pair leaves a capture incomplete.

These finite tests concern one board/port/cable. They do not qualify sustained
reliability, persistent deployment, general SG/ZLP behavior or ACM serial repair,
and do not establish a hardware erratum. Raw per-unit UUIDs, addresses, pointers,
keys, firmware and payloads remain private. Tests use RAM only, without SSD/ESP/
installation/boot-policy writes. No upstream issue/comment/PR is created.
