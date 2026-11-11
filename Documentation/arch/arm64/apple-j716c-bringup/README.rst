.. SPDX-License-Identifier: GPL-2.0

J716c bring-up status, 2026-11-11
================================

This records bounded bring-up on Mac17,6 (J716c / T6050, M5 Max), using
macOS 27.2.1 build 26B131 firmware and mBoot-20457.41.3. The tested system
is an Arch Linux ARM engineering root in RAM with systemd and Hyprland.
It is not a full Omarchy installation or a persistent Linux installation.

Source and attribution
----------------------

The qualified unified kernel source is `integration a59e709c7377
<https://github.com/cameroncooper/linux/commit/a59e709c737777495c84541c6afb6c862bb79227>`_,
based on `Aurora dc8f6f77934b
<https://github.com/aurora-silicon/linux/commit/dc8f6f77934b471a13c91e7780e2d91ebc904b3d>`_.
Its paired opt-in graphics Stage2 source is `Aura 9da61857f9d9
<https://github.com/cameroncooper/aura/commit/9da61857f9d9262bd64c023fbc1b18315b12d0af>`_,
based on `bd9f3e8273dc
<https://github.com/aurora-silicon/aura/commit/bd9f3e8273dc79ae9095c19c2c4f32218446ad4e>`_.
That Stage2 pin describes the graphics handoff, not the installed Stage1
or a complete recovery/boot-policy configuration.

The kernel integration preserves 14 public backports and their authors:
ten M5 InitData/layout commits by Rolando Trevino from `PR 1173
<https://github.com/aurora-silicon/linux/pull/1173>`_, three GPU/panel DT
commits by Erik Bourget from `PR 1276
<https://github.com/aurora-silicon/linux/pull/1276>`_, and Jeff Queisser's
`DCP firmware method-table commit
<https://github.com/aurora-silicon/linux/commit/290b7de9f12953a6d18555c1bed0899ef9bdb8cb>`_
from `PR 1174 <https://github.com/aurora-silicon/linux/pull/1174>`_.
Their original commit IDs and trailers remain in the history.

The five local changes describe and validate live J716c power tables,
identify the measured G17C firmware text, expose individually checked
read-only SMC telemetry, and add a default-off shallow-WFI probe opt-in.
They build on the upstream GPU, DCP, N1 and platform drivers. Bluetooth
enablement is a configuration choice on that base, not a new driver.

Observed behavior and limits
----------------------------

* All 18 CPUs online; N1 Wi-Fi and key-authenticated SSH work in the RAM root.
* Native internal-panel use, GLES readback and native Vulkan were exercised.
  The latest checkerboard compared 24,829,472 pixels over 32 frames with
  zero mismatches and no new kernel messages. A separate native Wayland
  cube run requested 600 frames; those frames were not individually
  validated by pixel readback.
* The native panel reports 3456 x 2234 modes, including 120 Hz. This does
  not independently measure physical refresh, qualify every mode, or
  establish external-display support.
* Bluetooth initialization and bounded LE discovery were tested. Pairing
  and Bluetooth audio remain unqualified.
* Explicit SMC temperature, fan-speed and power reads work. Firmware
  retains fan control. The engineering runtime limits CPU ceilings to
  approximately 2 GHz and raises them only after valid temperature
  telemetry; this is not unrestricted power-management qualification.
* ``j716c.wfi=probe`` enables the board-scoped shallow-WFI path; it remains
  off by default. Register/timer checks ran on all 18 CPUs.
  ``apple_keep_idle_nop`` still overrides it. Deeper idle and suspend/resume
  are not qualified.
* The engineering runtime keeps RTC and the unqualified external ATC2
  group disabled. RTC writeback is not qualified, even though the config
  compiles RTC support. Network time synchronization was observed.
* Bounded physical-watchdog recovery was tested, including loss of the
  host renewal process. The host-side guardian is a separate deployment
  component; its installation and private connection settings are not
  provided by these kernel commits.

The separate `USB2 diagnostics topic
<https://github.com/cameroncooper/linux/commit/1b321df7897b5172eb5674aabaa183e9d979175c>`_
qualifies checked USB0 retirement, provider/UDC registration and ACM
configuration-object binding. Host SET_CONFIGURATION still times out.
There is no working USB serial or USB SSH channel yet. Its guarded
snapshots are observations at specific capture points, not a wire-level
ACK or a proven failure cause. USB topics branch before the WFI commit;
a combined USB-plus-WFI tree has not been qualified. Do not bypass their
board, ownership or power/lifetime checks to activate devices.

Build pins and boundary
-----------------------

`pins.json <pins.json>`_ records source, toolchain, config,
Image.gz and official userland package hashes. The complete, audited
`kernel.config <kernel.config>`_ is the frozen tested config,
not a fragment. Its configured logo is already tracked in the source.
It embeds no initramfs, firmware, command line, private key or private path.

The original build used arm64 LLVM, Rust and bindgen with the versions in
``pins.json``; targets were ``Image.gz``, ``modules`` and
``apple/t6050-j716c.dtb``. Build identity was
``KBUILD_BUILD_USER=j716c``, ``KBUILD_BUILD_HOST=bringup``,
``KBUILD_BUILD_VERSION=1`` and ``SOURCE_DATE_EPOCH=1794338095``; the build
timestamp was derived from that epoch. The command-line ``LOCALVERSION``
was empty; the release suffix is in the config. Rust standard-library
sources and libclang must match the toolchain; bindgen must be available.

The public integration tree was checked equal to the frozen source used
for the qualified Image.gz. This documentation commit changes no code.
The published commit was not rebuilt to demonstrate byte-identical output:
Git state, source paths and build identities can affect generated bytes.
The Image hash identifies the tested bytes, not a downloadable binary or
a promise that an arbitrary build has that hash.

Hardware-specific loader-filled properties, independently obtained Apple
firmware, a suitable private initramfs/root, runtime DT safety gates and
recovery controls remain additional inputs. No firmware payload, per-unit
ADT/DROM, private root or raw capture is included here. Package hashes
identify the admitted official asset bytes; they do not claim a package
signature check or qualification of later repacks. These source/config
pins are not an installer or an enrollment/boot-policy recipe.
