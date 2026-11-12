T6050 effective IRQ affinity qualification
========================================

T6050 advertises apple,t6050-aic3 followed by the t8122 fallback, but uses
generic AIC3 data. Active hardware IRQs consequently report empty effective
affinity while receiving interrupts. The fourteen-line patch adds exact
T6050 information selecting the existing EL2 SIQ acceptance read and routed
AIC2 chip, as already used for T8140. It adds no register write, IRQ steering,
suspend capability or DT change. Requested affinity cannot steer AIC2;
effective affinity describes the accepting CPU pool.

One Mac17,6/J716c/T6050/M5 Max was tested: eighteen CPUs, firmware27.2.1/26B131,
mboot20457.41.3. The independently built observer checks CurrentEL2 on each
CPU and reads only the known public SIQ route field once per CPU, after
exact board/DT/CPU admission. It never writes that register or changes IRQ
affinity/masks. Observed route bits are0 (accept) or3 (reject); actual hardware
IRQ delivery is counted separately.

With USB-fixed Aura f07c7b, CPU0 reads route0 and CPUs1-17 read route3. The
patched kernel reports effective CPU0 for the active DWC IRQ, matching actual
delivery on CPU0. With the upstream M5 route-zero backport in Aura bbc4a0,
all eighteen route fields read0 and effective affinity becomes CPUs0-17.
The serial workload delivers DWC hardware interrupts on twelve CPUs; this
finite observation does not establish delivery to every CPU.

Both patched-kernel boots verify4096+65536 payload bytes each direction,
with138/138 bulk-IN requests retired across151 TRBs including ZLPs. Every
byte and independent per-end SHA256 receipt matches. Host and all eighteen
CPU captures are complete/loss-free, with no new kernel fault markers.
Physical watchdog recovery and held-TTY FD/rule cleanup pass. The strict
unpatched comparison passes the same full serial exchange but again reports
an empty DWC effective mask, while the independently measured SIQ state is
identical to the patched CPU0 control. Image changes are confined to this
AIC patch; native helper changes are admission-note/hash constants only.
Exact binary/source pins and all three cases are in the adjacent JSON.

DWC requests its IRQ at gadget bind, after the initial snapshot. An absent
initial IRQ/mask is recorded as absent; its effective mask is required after
traffic. Initial masks are also checked for IRQs already registered at the
first snapshot. Actual active AIC2 rows are counted; FIQ/IPI rows are excluded.

Build and reproduction
----------------------

Image.gz SHA256 f3b728f64809271981a31722336caf1dac3f5a5beeae7601df6b817308d470a6;
notes SHA256 5fcc965e9a09b499f74be4141e1ac45c6bf8274b8adc11009de1452a402c05ca;
release7.3.0-j716c-usb2-fifo-field-probe. Configuration, all66 generated C headers,
Rust archive and vmlinux/Module export-CRC files equal the prior kernel. The
only kernel source delta is the fourteen-line AIC information/match entry.
Compilation has no warnings.

The clean topic starts at dc8f6f77934b471a13c91e7780e2d91ebc904b3d; its AIC file
is byte-identical to the tested source. The patch applies cleanly to primary
integration05f3aabb2ae08f84506a3462638885bf77a8c5f8; unrelated T8140 suspend work
is not imported into this runtime.

The broader USB test Image uses j716c/acm-notification-geometry at98ffd177f15f8,
plus this AIC patch. The separately built class module is the qualified
notification-suppression variant at d92448214fc78, SHA256 beginning5bb77d0a;
the native held-TTY worker and host peer are published on that branch. This
guarded diagnostic omits connect notification because the real interrupt
notification remains broken. The AIC patch does not repair it. Observer source,
Makefile and kernel configuration are adjacent. Runtime binaries predate
publication commits; new-version-tag rebuilds are not claimed. Private
firmware, keys, boot identities, addresses and raw captures are excluded.

Every trial uses a verified native120-second watchdog owner with one final
explicit keepalive and retained FD. Both TTY FDs stay held until physical
reset; host FD/rule release follows strict baseline authentication. No
post-loader software reset, unload, CPU offline, suspend, affinity write,
disk/ESP/install/NVRAM or boot-policy write is used. Other peripherals,
hotplug, suspend/resume, sustained reliability and exhaustive per-CPU IRQ
delivery remain unqualified. No upstream PR/comment is created here.

Subsequent larger EEM workload
------------------------------

On the same patched kernel/all-core Aura, finite bulk transfers verify
4456448 payload bytes each direction,128 frames each way,4850/4850 IN
givebacks and19 ring wraps with peaks10/10. Normal open-ended transfers
still stall208/218 with ten pending and four empty IN completions. Thus
the IRQ changes alone do not repair that separate controller failure.
Both complete loss-free captures, physical recoveries and scoped cleanup
pass. Hardware AIC2 IRQ delivery is observed on all eighteen CPUs in the
normal-control capture, and seventeen during the full finite-transfer pass.
These distinct outcomes are retained in the adjacent EEM result file.
