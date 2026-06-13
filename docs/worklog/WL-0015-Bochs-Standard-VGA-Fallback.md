# WL-0015 -- Bochs boot solved: standard-VGA (mode 0x13) fallback + 8bpp renderer

Issue: **initech-6pj** (closed) -- "stage2 VBE mode-set fix for Bochs (ERR-VBE)".
Epic: initech-bcg (kernel hardening) / tri-emulator (initech-x0i, initech-564).
Branch: `kernel-hardening`.

## Context

WL-0014 left the Bochs tri-emulator leg as the one open robustness gap: the
tracer reached stage2 under SeaBIOS but VBE `0x4F00` failed (`VBE-E00`), and the
native `BIOS-bochs-latest` faulted pre-stage2. The user chose, from three
options, the **standard-VGA fallback + renderer** path: when no VBE LFB is
available, set a standard VGA mode via INT 10h and render the console into it.

## What changed (3 commits, all green)

- **0ad6e58 -- console 8bpp linear renderer.** `os/milton/console.c` gains an
  8bpp branch (put_pixel + scroll-readback: one byte = a VGA palette INDEX,
  fg/bg bound to new `CONSOLE_FG_IDX`/`CONSOLE_BG_IDX`). `console_init` accepts
  bpp 8 and derives the cell grid from the resolution, capped at 80x25
  (640x480 -> 80x25 unchanged; 320x200 -> 40x12) so the blit can never overrun
  a smaller framebuffer. Host oracle `test_console.c` gains a full 320x200 8bpp
  section; mutation-proven (8bpp put_pixel +1 -> 13 RED; cols +1 -> RED).
- **1aca541 -- stage2 mode-0x13 fallback.** `vbe_setup` no longer halts on VBE
  failure: it falls back to standard VGA mode `0x13` (320x200x256 linear @
  0xA0000) via INT 10h, probes the framebuffer is live (fail-loud `ERR-VGA`
  otherwise), and records an 8bpp boot_info. `boot_info` width/height became
  variables (640x480 VBE / 320x200 fallback) instead of `WANT_W/WANT_H`.
- **a2111cd -- kmain 8bpp + DAC palette.** BI validator accepts bpp 8 + the
  320x200 geometry (was `BI-BAD`); `fb_clear` gains an 8bpp branch (fills
  `CONSOLE_BG_IDX`); `vga_set_dac()` (ports 0x3C8/0x3C9, 6-bit channels) maps
  `CONSOLE_BG_IDX` -> seafoam and `CONSOLE_FG_IDX` -> light gray in 8bpp mode.

## Why this path / what the diagnosis turned up

The Bochs boot was a layered mystery, now fully resolved (bd memory
`bochs-boot-solved-initech-6pj`):

1. **`BIOS-bochs-latest` + `pentium`** triple-faults: `cmovnle` at the native
   BIOS's POST -- CMOV is a P6 instruction the pentium model lacks (#UD).
2. **`BIOS-bochs-latest` + `p3_katmai`** clears CMOV but then triple-faults on
   an interrupt with `IDT.limit=0x0` in PM during POST (the WL-0014 fault) --
   APIC/interrupt-related, unresolved. The latest BIOS is unusable here.
3. **SeaBIOS `bios.bin`** boots our OS but its vgabios does NOT drive the VGA
   under Bochs: mode 0x13 `AH=0Fh` reports `13h` yet framebuffer writes to
   0xA0000 are dropped (readback `0xFF`); VBE `0x4F00` returns `E00`.
4. **`BIOS-bochs-legacy` (64K) + `pentium`** is the winner: boots clean (no
   CMOV, no IDT.limit fault), VBE `0x4F00` *succeeds* (`VBE-ENOMODE` -- no
   640x480 LFB mode), and **standard mode 0x13 fully works** (BDA 0x449 -> 0x13,
   framebuffer live, readback verified).

So VBE LFB is genuinely unavailable on Bochs; standard mode 0x13 is the portable
answer (works on every BIOS / 86Box / real hardware), which is why the
user-chosen fallback is the right call.

## Frictions / honest limitation

**Bochs RFB display does NOT render mode 0x13** (bd memory
`bochs-rfb-display-does-not-render-vga-mode`). A *pristine real-mode* fill of
mode 0x13 (index 1 everywhere + DAC programmed), halted before the kernel,
comes back ALL BLACK over RFB (raw framebuffer bytes 0/345600). The OS writes
the framebuffer CORRECTLY -- proven 4 ways: stage2 real-mode write/readback at
0xA0000; a Bochs debugger write-watchpoint on 0xAF9FF firing (kernel fill
reaches the top byte); `test-console` 8bpp host oracle; serial banner trace
through `console_putc`. RFB (the only headless display in this build) simply
cannot show chained 256-color mode 0x13. `bochs-term` also fails headless
(stalls on the graphics mode-set). **Consequence:** the Bochs tri-emulator gate
must assert on SERIAL markers + no-triple-fault (resolution/display-independent,
the rigorous Rule-5 signal), not a screendump. `/tmp/rfb_capture.py` is a
working RFB framebuffer-capture client (returns black for mode 0x13 -- a Bochs
limit, not a client bug).

## Acceptance

`make test` = **55 host + 19 emu GREEN** (no regression). QEMU `test-tracer-boot`
unchanged (VBE 640x480 path intact, BI-OK, banner on seafoam, no triple-fault).
Bochs (legacy BIOS + pentium, headless via the RFB-unblock client) boots
end-to-end with **zero faults**: `S1 JMP2 S2 VBE-ENOMODE VGA13 ... BI-OK ...
BANNER-BEGIN / InitechDOS  Version 3.30 / BANNER-END`.

## Pointers / next

- `os/milton/console.{c,h}` (8bpp), `os/milton/test_console.c` (8bpp oracle),
  `os/boot/stage2.asm` (`.vga_fallback`), `os/milton/kmain.c` (`vga_set_dac`,
  `fb_clear` 8bpp, BI validator).
- `os/milton/console.{c,h}` (8bpp), `os/milton/test_console.c` (8bpp oracle),
  `os/boot/stage2.asm` (`.vga_fallback`), `os/milton/kmain.c` (`vga_set_dac`).

## Addendum -- initech-564: the committed Bochs gate (same session)

Built `harness/emu/bochs.{c,h}` + `bochs_main.c` -- the Bochs leg of the
tri-emulator boot gate, parallel to the QEMU harness and **C-only** (Law 3):
the RFB 3.3 unblock that `rfb_unblock.py` prototyped is now implemented in C.
`bochs_run` generates the working bochsrc (legacy BIOS + LGPL vgabios +
pentium), computes the 2x32 disk geometry, fork/execs the debugger-build Bochs
with `-rc 'c'`, connects + handshakes + drains the RFB socket so Bochs runs
headless, captures serial via `com1=file`, and scans the log for a triple
fault. The guest hlt-loops -> the run times out by design; the verdict is
RFB-unblock + no-fault + the `--expect` marker.

`make test-boot-bochs` boots the SAME tracer image under Bochs and asserts the
mode-0x13 fallback fired (`VBE-ENOMODE` + `VGA13`) and the SAME kernel
milestones as QEMU (`S1 PM OK FONT KERNEL INT21 BI-OK CONSOLE BANNER` -- the
differential) with no triple-fault. SERIAL-only (Bochs RFB can't display mode
0x13). **Mutation-proven:** latest BIOS -> `triple_fault=1` -> RED; bogus
`--expect` -> `marker_found=0` -> RED. Not in the default `make test`
(env-specific Bochs + ~45s); run explicitly. Commit `4a0c38c`.

**Open question for a *visual* Bochs leg:** mode 0x12 (640x480x16 planar) might
render on RFB (untested) but needs a planar renderer -- deferred. **86Box
(initech-44m)** remains the only unbuilt tri-emulator leg (lowest priority;
Qt-offscreen headless automation unbuilt).
