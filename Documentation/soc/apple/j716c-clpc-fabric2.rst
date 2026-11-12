J716c T6050 GPU fabric-floor backport qualification
=================================================

T6050 CLPC requests DCS6/fabric3 for host GPU work in this base. Carry the
one-line correction from upstream ca3e29829786bf20805fb4c5ab665dee414dfe08
by Diljit Singh: keep DCS6 and request fabric2. The broader upstream
platform-profile/SMC power polling changes are not part of this topic.

On one J716c/Mac17,6/M5 Max with macOS27.2.1 firmware, matched RAM boots
use the same Aura M5 all-core IRQ routing, GPU handoff1, disabled native
DCP, initrd, native Vulkan loader/ICD, unchanged checkerboard and CPU caps
1992000/1992000/1980000kHz. Each arm verifies32 frames and24829472 pixels
with zero mismatches. All250 readonly CLPC samples are retained per run.
Host-GPU publication sequence reaches99, and GPU-reason votes are DCS6/
fabric3 in11 samples for reference and DCS6/fabric2 in8 for the candidate.
Controller update errors remain0. Kernel logs show no new fault markers.
Physical native120s watchdog recovery authenticates the unmarked baseline
in both runs. No post-loader software reboot or watchdog handoff is used.

This qualifies bounded correctness and observed host-voter requests. It
measures no performance, power saving, winning combined hardware state,
thermal behavior, sustained stability, native panel/compositor or USB4.
Other firmware agents continue to vote. TCMb telemetry was unavailable;
no new SMC probe was introduced. An earlier fixture stopped before GPU
loading because the USB initrd lacked cpufreq policies. Its capture and
successful physical recovery remain private; v2 packages the unchanged
cpufreq source module with the matching vermagic before applying the caps.

Code and build provenance
-------------------------

This clean topic extends j716c/aic-effective-affinity at0498a4d2fea14.
Its CLPC source is byte-identical to the tested candidate. The actual
engineering Image also carries the previously published USB geometry and
J716c integration; this topic is not a byte-identical full-image release.
The tested CLPC change is one constant. Config, all66 generated C headers,
Rust archive and vmlinux/Module export CRCs match the reference.

Reference Image SHA256
f3b728f64809271981a31722336caf1dac3f5a5beeae7601df6b817308d470a6
and kernel notes SHA256
5fcc965e9a09b499f74be4141e1ac45c6bf8274b8adc11009de1452a402c05ca.
Candidate Image SHA256
cb95542702bc157520077efe05110dcef206f57997edbf3852d8a812c4b3404f
and kernel notes SHA256
489673ae7e16993ab848631fd3953bcc45a98aa81ee01a60138c0e5f2e8e9cf8.
Release is7.3.0-j716c-usb2-fifo-field-probe. Full tested Aura source is
cameroncooper/aura:j716c/usb2-irq-route-refresh ata18ec683918f65;
raw4063232-byte SHA256
bbc4a007acb43e0afecd01847d0cf034d00bc9b660a804dfe60a1895749cdfe3.
Runtime binaries predate this documentation commit.

Build Image.gz with the existing pinned LLVM22/Rust environment, fixed
SOURCE_DATE_EPOCH and LOCALVERSION empty. A matching cpufreq module was
built using M=drivers/cpufreq; on this kernel, external-M outputs are in
the source M directory rather than the kernel O directory. Module SHA256
80202e78842066fcc675543f344f681a0857d21fc0508718cc7c701432603767.
Native watchdog/helper source bodies remain unchanged; note/expected-owner
hash admission constants differ between the two Images. Both retain their
watchdog FD and stop feeding after one explicit final keepalive until reset.

The adjacent unchanged worker/shader sources produce the previously
qualified native ELF9f96ef58cfbe95bb4d4db3d6ce775ffe2a483813db54abc8ce49f535f64b8fc8.
The readonly sample/worker launch is shown in each guarded target script.
Native userspace pins include the default loader/ICD/libc exact-byte
checks and29 selected runtime file hashes. No loader/library/ICD environment
override is used. Libraries run in a RAM chroot with live /dev,/proc,/sys;
no installation or systemd switch_root occurs. Fixed lease/SSH controllers,
firmware, keys, root archives, raw identities and raw captures are not shipped.
Results.json contains curated outcomes and hashes; raw captures stay private.
