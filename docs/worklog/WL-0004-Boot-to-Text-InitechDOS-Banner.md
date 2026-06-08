<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0004 — Programme Engineering Work Record (PEWR)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Engineering Work Record (Worklog)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | PE-WL-0004 |
| Title | Boot-to-Text: stage2 → C Kernel Handoff, LFB Console, and the InitechDOS Banner |
| Version | 1.0 |
| Status | Issued |
| Classification | Internal Use Only |
| Period Covered | 2026-06-08 |
| Recording Function | Build Orchestration (supervised multi-agent) |
| Related | WL-0003; ADR-0003 (DEC-08, DEC-12, App D.1); PRD Sec 5, Sec 11 (M1); beads initech-d00, initech-yqb, initech-bea, initech-slz/a9w/dt9 |

---

## 1. Purpose

This Record memorializes the M1 (Boot-to-Text) increment: the boot chain now hands
off from the assembly second-stage loader into a freestanding C kernel, brings up an
80x25 text console over the VESA linear framebuffer, and renders the controlled
InitechDOS boot banner. The increment was executed as a supervised, serial
multi-agent orchestration (one research pass, three implementation passes, an
orchestrator verification pass), each pass verified independently against the locked
spec and a mechanical oracle before the next commenced.

## 2. Conformance Reconciliation (Predecessor Work)

Four M1 beads were found to be already satisfied by the tracer (initech-f8v.2) and
its green oracle, and were closed against that evidence: initech-dt9 (MBR CHS load of
stage2), initech-a9w (A20 / GDT / 32-bit protected far jump), initech-slz (VBE 2.0
linear-framebuffer init). The remaining robustness work (multi-bpp, LBA fallback,
tri-emulator VBE) is retained under initech-ta2. This is heritage accretion, not
regression — the work existed; the issue records lagged it.

## 3. Ground Truth Established (Law 1)

A research pass produced `docs/research/boot-to-text-ground-truth.md`: the VGA ROM
8x16 font acquisition path (INT 10h AX=1130h BH=06h → ES:BP, 256 glyphs x 16 bytes,
MSB-leftmost); the real-mode-before-protected-mode ordering constraint for all BIOS
calls; a boot-information handoff contract; the flat-kernel link/load recommendation
(link at 0x10000, load by INT 13h below 1 MiB); the framebuffer bit-depth branch (32
preferred, 24 fallback); and the boot-oracle plan. The orchestrator independently
re-verified the memory map and confirmed that the second stage reads but does not
persist the display width/height.

## 4. Artifacts Delivered

- **Boot handoff (initech-d00, closed):** `os/boot/stage2.asm` extended to capture the
  ROM font (stashed at physical 0x1000), persist a 24-byte `boot_info` structure at
  physical 0x500 (LFB address/pitch/bpp, width, height, font pointer), load a flat C
  kernel from disk (sector 17+, geometry queried at runtime via INT 13h AH=08h — a
  root-cause fix for a non-1.44M raw-image geometry), and far-jump into it. New C
  kernel: `os/milton/{kstart.asm,kernel.ld,kmain.c,boot_info.h,io.h}` (flat binary
  linked at 0x10000). New serial milestones FONT / KERNEL / BI-OK.
- **Text console (initech-yqb, closed):** `os/milton/console.{h,c}` — a freestanding
  80x25 console over the LFB: bit-depth-branching glyph blit (MSB-leftmost), printable
  and control-character handling, line wrap, and one-row scroll; fail-loud
  initialization. Background is the house seafoam; foreground light gray.
- **Banner (initech-bea, QEMU-green; open for tri-emulator):** `os/milton/kmain.c`
  prints the controlled banner from `spec/dos_banner.txt` (byte-exact, ADR-0003
  DEC-12 / Appendix D.1) to the console and mirrors it to serial. New screendump
  oracle `tools/ppm_text_check.c`.

## 5. Oracle Disposition (Law 2)

- **`make test-boot` (new gate):** asserts (1) no triple-fault; (2) the serial
  milestone set plus both banner lines; (3) the serial banner equals
  `spec/dos_banner.txt` byte-for-byte (a diff tying the running artifact to the locked
  spec); (4) a screendump text check (foreground pixels at the banner's known glyph
  cells AND seafoam preserved below) — proving rendering, not a solid fill.
- **`make test-tracer-boot` was STRENGTHENED, not weakened.** The banner causes a small
  number of the prior pure-seafoam sample grid points to land on rendered text; per the
  stop-condition (never weaken an oracle to pass), the screendump check was replaced by
  the strictly stronger `ppm_text_check` (banner-rendered AND seafoam-below). The
  pure-seafoam property survives as one of its assertions.
- **Mutation-proven (Rule 6)** at every layer and independently by the orchestrator:
  the handoff markers, the console blit (MSB-flip, cell-origin), and the banner
  (single-character drift → serial-vs-spec diff red; banner-skip → screendump red).

## 6. Verification of Record

At close of period the following pass without exception: `make test-boot`,
`make test-tracer-boot`, `make test-spec`, `make test-console`, `make test-fat`, and
the three FAT12 unit oracles. No triple-fault occurs on the boot chain. `kernel.bin`
is byte-identical across clean rebuilds (Rule 11). All new sources are ASCII-clean
(Rule 12). The boot screen was additionally confirmed by eye (Law 4) from a screendump
crop: the two banner lines render in light gray on seafoam.

## 7. Phase Disposition

**InitechDOS boots from a raw disk image into C, brings up a text console, and prints
its banner.** This is the visible spine of M1. The single remaining M1-acceptance item
for initech-bea is **tri-emulator agreement** (QEMU == Bochs == 86Box), which is
blocked on initech-x0i (Bochs/86Box harness drivers, not yet built) — recorded
honestly; the gate runs QEMU-only and says so. The natural next increments toward a
booting, usable DOS are the IO.SYS/INITDOS.SYS two-file kernel partition + SYSINIT
(initech-509.2), the INT 21h dispatcher (initech-509.5), and the COMMAND.COM shell
(initech-7pc) reading the now-proven FAT12 volume — at which point the f8v.4 keystone
(boot → banner → COMMAND.COM → DIR → TYPE) closes.

## 8. Follow-On Items

- **initech-bea** remains open for tri-emulator agreement (depends on initech-x0i).
- **initech-ta2** retains boot robustness (multi-bpp, LBA fallback, tri-emulator VBE).
- A durable note (issue-tracker memory) records the host-test low-memory allocation
  requirement (MAP_32BIT) for tests that place a host pointer into a `uint32_t`
  `boot_info` field.

---

*— End of Record —*

<!-- Tedium certified compliant with NFR-7. -->
