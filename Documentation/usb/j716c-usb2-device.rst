J716c USB0 USB2 device experiment
===============================

This exact-unit, fixed-cable experiment is independent of the existing T6050
host-only combo PHY and DWC3 compatibles. It has no host, Type-C notification,
SuperSpeed, GPIO, ACIO or firmware startup path. It has not been qualified on
hardware. Its source DT never enables USB0, and request 2 is validation only.

Aura must validate J716c/Mac17,6/J716cAP, board 0xe, T6050 and the trusted unit
ADT/firmware, stop the exact CDC gadget, verify all endpoints/events halted,
retire DMA and clear SID1 roots with completed invalidation, then hold DWC3
reset and force clamp on the dummy PIPE. Only after those checked operations
may it atomically publish /chosen/j716c,usb0-handoff-status = "retired" and
enable the whole dependency group in the exact final FDT Linux receives.
The opt-in property /chosen/j716c,usb0-handoff is one u32 cell of value 1.
No marker means no kernel admission, even if someone enables the nodes.

All hardware nodes are under /soc@200000000. The seven-member group consists of:

* phy@2a2a90000: apple,j716c-usb2-device-phy, reg banks USB2PHY
  0x2a2a90000/0x4000, digital 0x2a2800000/0x4000, PIPE
  0x2a2a84000/0x4000; zero-cell PHY and reset provider.
* usb@2a2280000: apple,j716c-usb2-device-dwc3, core resource
  0x2a2280000/0xcd00, peripheral, high-speed, one USB2 PHY/reset,
  interrupt 1419, IOMMU DART1/SID1 only.
* iommu@2a2f80000: apple,j716c-usb2-device-dart plus apple,t8110-dart,
  resource 0x2a2f80000/0xc000, interrupt 1423. DART0 stays disabled.
* power-management@280600000/power-controller@208: ATC0_COMMON,
  with no physical parent after flattening the trusted ADT virtual ancestry.
* power-management@288300000/power-controller@{148,150,158}:
  ATC0_USB_AON, ATC0_USB, ATC0_PHYMXWRAP. USB and WRAP each depend
  on COMMON and AON. The PHY consumes WRAP; DART/DWC3 consume USB.

The inventory FAB3_SOC50 and FAB0_PIOGW49 are ADT no_ps virtual rows, not
physical MMIO genpd providers. Both inventory nodes at PMGR offsets1e0/1d8
must remain disabled. COMMON55 reaches no real parent through them; this
J716c USB2 overlay removes COMMON's fake inventory power-domains edge.
The kernel refuses a reinserted edge or either virtual provider enabled.

USB2 PHY properties are BE32 offset/mask/value triples, validated exactly:

* apple,tunable-usb2-default: <8 0x7003 3 0x18 0x703 0x103>
* apple,tunable-usb2-device: <8 0x7003 3>

The first reset assertion adopts only PIPE MUX_CTRL&0x73==0x42 and
AON_GEN&0x11==0x10. Device-mode selection precedes reset release, all USB2
errors are fatal and reset/PHY state must latch. The public Aura USB2 default
then DEV sequence supplies only the dummy PIPE; no generic CIO table applies.
DART probe requires unlocked registers, SID1 TCR==6 and all TTBRs zero.
Every other stream must have TCR 0 or 6 and zero roots. It only resets SID1,
only enables SID1 globally and rejects other consumers or SIDs. Unrelated
live state makes the entire DART probe fail without resetting it.

Run tools/asahi/j716c-usb2-tests.py for source-integrated admission and MMIO
sequence fixtures. OF/MMIO timing and electrical behavior are modeled; these
checks do not qualify hardware, disconnect/reconnect, suspend or DMA traffic.
First trial requires root-owned watchdog and proven Wi-Fi/SSH recovery, the
exact paired Image/config/modules/initramfs, a distinct gadget VID/PID and
inspection of UDC/descriptors before a bounded ACM transfer. No getty or
serial console is required. Reboot must restore the unchanged Stage1 proxy.
