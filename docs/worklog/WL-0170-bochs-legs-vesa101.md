# WL-0170 -- The Bochs legs after the VESA 0101h fallback: the Bochs framebuffer AGREES with QEMU, pixel for pixel

Lane bochs3 (branch lane/bochs3, clone initech-os-bochs2): an Opus lane
left the RFB/snapshot capture harness, the one-checker script, the pixel
leg and the kmain DAC seam uncommitted when the PM paused everything; this
shard is the Fable takeover that committed that state as WIP (fbc14918),
merged origin/main (the DPB, SAMIR size round 2, kernel size round 5
merges; no conflicts; the panic-screen mkdir fix 28e14d7f was already on
main, the lane's duplicate dropped) and re-ran everything on the MERGED
kernel. Nothing pushed; CLAUDE.md, HANDOFF and beads untouched; `make
test` not run (brief); one emulator at a time.

## Context: a tri-emulator disagreement to investigate, not pin

After lane vesa8 (initech-2gva, commit 9246ad60) taught IO.SYS the VESA
0101h 640x480x8 LFB tier between the 24/32bpp modes and the mode-13h
floor, three Bochs legs went RED on main:

    !!! test-boot-bochs FAIL: fallback marker VBE-ENOMODE missing (stage2 did not take the mode-0x13 path)

The serial said why: Bochs 2.7's LGPL vgabios offers 0101h with an LFB at
0xE0000000, so stage2 now prints `VBE-MODE=0101`, the kernel reports
`VIDEO 640x480 bpp=8 pitch=640 lfb=0xE0000000 dac=8` and runs on to the
desktop -- while every Bochs leg (and the Makefile comment block near the
bochs_legs include, written by lane cqej BEFORE vesa8) still expected
`VBE-ENOMODE` + `VGA13` + the "LFB smaller than 640x480" guard panic.
CLAUDE.md's stop condition applies: an oracle disagreeing across emulators
is investigated, never pinned to QEMU.

## The finding: the Bochs 0101h framebuffer agrees with the QEMU golden EXACTLY

Harness (harness/emu/bochs.c, `--screendump-after LINE`): after the
complete exact serial line, the harness asks Bochs's RFB server (its only
headless display) for full-window FramebufferUpdates until two consecutive
full frames are byte-identical (a torn frame cannot be graded), writes the
raw BGR233 codes (`<name>.rfb.pgm`), then clicks Bochs's own header-bar
snapshot button over RFB (gui.cc snapshot_handler) for the LOSSLESS twin:
the guest's 8bpp indices plus the emulated DAC as a BMP, converted to
exact-RGB P6 (`<name>.snap.ppm`). `--rtc-time0` pins the guest RTC (TZ=UTC,
since cmos.cc localtime()s a numeric time0) to the same instant QEMU's
`--rtc-base` uses, so the menu-bar clock agrees.

Checker (tools/ppm_bochs_check.c): grades the snapshot for EXACT RGB
equality on all 307200 pixels against a 640x480 QEMU QMP capture, and the
RFB frame through the ONE lossy map Bochs 2.7 can show (rfb.cc BGR233:
`(R>>5) | (G>>5)<<3 | (B>>6)<<6`, the guest at y = rfbHeaderbarY). No
tolerance anywhere.

Golden independence (Law 2): the golden is QEMU booting the SHIPPED
flair_tenants.img / flair_desktop.img on QEMU's 32bpp direct-colour VBE
path -- a different emulator AND a different depth path (no DAC at all) --
captured at the same serial line with the same data-volume bytes and the
same pinned RTC. The Bochs side is the 8bpp indexed LFB + the DAC that
flair_dac_load programs. Agreement is therefore not by construction.

Result on the merged kernel (85bb4104 + this shard), scratchpad
pixel-flagship.log / pixel-tenants.log and the gate logs:

    GREEN bochs-exact: all 307200 RGB pixels of the Bochs snapshot equal the QEMU golden
    GREEN bochs-rfb: all 307200 displayed pixels equal the QEMU golden through Bochs BGR233 (window 720x530, guest at y=32, 9 distinct codes)
    GREEN bochs-pixel flagship: Bochs == QEMU at 'FLAIR-TENANTS-READY hello+notes co-resident; HELLO foreground' (exact + displayed)
    GREEN bochs-pixel tenants:  Bochs == QEMU at 'FLAIR-TENANTS-READY hello+notes co-resident; HELLO foreground' (exact + displayed)

for BOTH the production IO.SYS (pitch 640) and the FORCE_VBE_101 gate
IO.SYS (vesa8.mk; the 4F06h call pads the logical scanline to 648 and the
row copy must honour it). The desktop PAINTS correctly through the Bochs
VBE LFB; there is no emulator-ism in the artifact and none in Bochs's VBE.
The only Bochs-specific fact is the display: RFB serves BGR233 only, which
is why the lossless snapshot is the EXACT grade and the RFB frame the
"what was really shown" grade.

The mutants bite under Bochs (Rule 6; test-flair-desktop-8bpp-bochs-mutant,
each FLAIR_DAC_MUT_* kernel under Bochs against the CLEAN QEMU golden):

    omit119: RED bochs-exact RGB (5,0) qemu=#777777 bochs=#1C7000
    omit165: RED bochs-exact RGB (593,51) qemu=#A5A5A5 bochs=#687050
    rowcopy: RED bochs-exact RGB (0,1) qemu=#000000 bochs=#E7E7E7
    dac6:    RED bochs-exact RGB (5,0) qemu=#777777 bochs=#1D1D1D

Note the kmain change the Opus lane made and this shard kept
(os/milton/kmain.c flair_dac_load): the boot screen (initech-tdnl.105) had
grown a second, UNMUTATED copy of the DAC loop that pre-loaded the whole
table before the mutated presenter loop ran, so the presenter-only
omit119/omit165 mutants survived on QEMU and Bochs alike. Both the boot
screen and every desktop present now load through the ONE seam, so the
FLAIR_DAC_MUT_* mutants reach the DAC the desktop is actually shown with.

## What changed

* harness/emu/bochs.c, bochs.h, bochs_main.c -- the RFB FramebufferUpdate
  shadow + snapshot capture (`--screendump-after`), `--rtc-time0`,
  `--vga-ext`; the screendump run executes Bochs with cwd=output dir on
  absolute paths (snapshot.bmp is cwd-relative). `ok` now requires BOTH
  captures when a screendump was asked for (the header already said so).
* harness/emu/bochs_leg_check.sh -- the ONE Bochs serial verdict (RFB
  unblock, no triple fault, the video path, the shared milestones, the
  leg's exact-line checks, `!PANIC`). Video paths: `vbe101` (stock ROM),
  `vbe101pad` (the FORCE_VBE_101 gate IO.SYS, pitch 648), `vga13` (ISA
  Cirrus: VBE present, 0101h offered WITHOUT an LFB -> VBE-ENOMODE ->
  mode 13h), `e00` (plain VGA, no VBE -> VBE-E00 -> mode 13h). The
  takeover fixed the vga13/e00 `bad` lists: stage2 prints the `VBE` step
  marker on every path, so it must not be a forbidden line there.
* harness/emu/bochs_pixel_leg.sh + tools/ppm_bochs_check.c -- the pixel
  leg (QEMU golden boot, Bochs capture, both grades).
* harness/bochs_legs.mk -- the generic macro re-keyed to the real video
  path (5th arg, default vbe101), profiles `flair` (FLAIR-HEAP-OK,
  FLAIR-FAT-MOUNT-OK, FLAIR-DESKTOP, FLAIR-LIVE-READY, no PANIC),
  `guard`/`guardmount` (the 640x480 guard under mode 13h), `shell`,
  `nodrive`, `exec`, `mzexec`; every leg stops at its completion line
  (`--quit-after`), so a leg takes ~2-5 s instead of a 45 s timeout. New:
  test-flair-desktop-8bpp-bochs (vbe101pad boot leg + the pixel leg) and
  its -mutant; the FORCED mode-13h legs test-boot-vga13-bochs,
  test-boot-novbe-bochs, test-flair-desktop-vga13-bochs,
  test-flair-appswitch-vga13-bochs, test-flair-desktop-novbe-bochs. The
  ISA Cirrus ROM is the vgabios package's (bochs Depends: vgabios |
  seabios); a missing ROM is a LOUD FAIL with the apt hint, never a skip.
* harness/emu/test_bochs_legs.c/.sh -- the inventory oracle re-keyed
  (completion line = --expect + --quit-after, the checker invocation with
  the leg's video path and checks, VIDEO rule for the forced legs, the DAC
  rule = the 8bpp leg grades the padded scanline); a sixth wiring mutant
  (VIDEO). The retired RUN verb (3c32c4df; test-samir-dml-pending by
  name) is dropped from the roster: main's own test-bochs-legs was RED
  since that commit because SAMIR_DML_CASES no longer lists `run`.
* Makefile -- test-boot-bochs, test-flair-desktop-bochs and
  test-flair-appswitch-bochs re-keyed: the boot leg through the one
  checker on the vbe101 path to SHELL-READY (+ the KHI/KAT placement
  check), the two FLAIR legs are now the pixel legs (QEMU 32bpp golden
  vs Bochs at FLAIR-DESKTOP / FLAIR-TENANTS-READY) plus the serial
  verdict; the stale "Bochs has no VBE LFB / ENOMODE -> guard" comment
  blocks and the help line corrected.
* harness/emu/panic_screen.mk -- unchanged from main (28e14d7f already
  carries the order-only `| $(PANIC_SCREEN_DIR)`); test-panic-screen-bochs
  runs.

## Frictions

* The Opus checker's `vga13`/`e00` paths forbade the `VBE` line, which
  stage2 prints as its step marker on every path; caught on the first
  probe, fixed before wiring.
* The DAC wiring mutant was caught by the BOUNDARY rule first (the check
  line's video argument is part of the boundary); the DAC rule now runs
  ahead of it so the mutant harness sees `RED bochs-legs DAC:`.
* The kernel strings are packed (serial_trace_pack), so `grep` on an image
  cannot tell which completion line a gate kernel prints; the universal
  BOOT_FLAIR_LIVE line FLAIR-LIVE-READY is the flair profile's completion.

## Acceptance (merged tree; every target by name, one emulator at a time; scratchpad bochs3/vector.log + logs/)

    test-bochs-legs + -mutant        GREEN: 72 named boot legs; all six wiring mutants caught (MISSING VECTOR IMAGE BOUNDARY VIDEO DAC)
    test-bochs-completion (+3 mut)   GREEN
    test-bochs-reboot (+mutant)      GREEN: READY -> 8042 pulse -> DONE -> READY (second boot)
    test-boot-bochs                  PASS: tracer on the VESA 0101h path to SHELL-READY, KHI/KAT placement green
    test-flair-desktop-bochs         PASS: FLAIR-DESKTOP; Bochs == QEMU 32bpp on all 307200 pixels (exact + RFB BGR233)
    test-flair-appswitch-bochs       PASS: FLAIR-TENANTS-READY; Bochs == QEMU 32bpp on all 307200 pixels (exact + RFB BGR233)
    test-panic-screen-bochs          GREEN: real FAT .COM #UD, debug register dump, no triple fault
    test-flair-desktop-8bpp-bochs    green: vbe101pad boot leg + the flagship pixel leg (pitch 648)
    test-flair-desktop-8bpp-bochs-mutant  GREEN: omit119 / omit165 / rowcopy / dac6 each RED under Bochs (lines above)
    test-boot-vga13-bochs            green: ISA Cirrus -> VBE-ENOMODE + VGA13 + 'VIDEO 320x200 bpp=8 pitch=320 lfb=0x000A0000 dac=6' to SHELL-READY
    test-boot-novbe-bochs            green: plain VGA -> VBE-E00 + VGA13 to SHELL-READY
    test-flair-desktop-vga13-bochs   green: the 640x480 guard PANIC + HALTED under mode 13h (Cirrus)
    test-flair-appswitch-vga13-bochs green: FAT mount then the guard PANIC + HALTED under mode 13h (Cirrus)
    test-flair-desktop-novbe-bochs   green: the guard under mode 13h (plain VGA)
    the 72 roster legs + the un-rostered generic legs (menubar-clock, iword, exit-to-dos, tps-journey, file-copy,
      paint, itext(-scroll), calc, calcda, apple-menu, dos-break, dos-not-ready, chkdsk, xcopy-tree, dosutil,
      edlin, live-scene, shutdown, restart)   all green on the vbe101 path
    QEMU regression: test-boot PASS, test-flair-desktop PASS, test-flair-desktop-8bpp GREEN, test-flair-appswitch PASS

Whole Bochs vector wall time about six minutes (every leg stops at its
completion line instead of a 45 s timeout).

## CLAUDE.md correction needed (NOT edited by this lane)

CLAUDE.md "Build & test" says bochs runs "the Bochs legs of the boot /
FLAIR-desktop / FLAIR-appswitch gates" -- still true. No sentence in
CLAUDE.md itself claims Bochs ENOMODEs; the claim lived in the Makefile
comment blocks (corrected here), harness/bochs_legs.mk (corrected) and
harness/emu/bochs.h's old bochsrc comment (corrected). If the operator
wants a CLAUDE.md note, the truthful one is: "Bochs 2.7 (LGPL vgabios)
takes the VESA 0101h 640x480x8 LFB path; the Bochs FLAIR legs grade the
presented desktop EXACT against the QEMU 32bpp capture (WL-0170); the
mode-13h floor + VBE-ENOMODE/E00 are graded on an ISA Cirrus / plain VGA
by the *-vga13-/-novbe-bochs legs."

## Pointers

* harness/emu/bochs.c (RfbShadow, rfb_click_header, snapshot_to_ppm),
  harness/emu/bochs_leg_check.sh, harness/emu/bochs_pixel_leg.sh,
  tools/ppm_bochs_check.c, harness/bochs_legs.mk, harness/emu/
  test_bochs_legs.c, os/milton/kmain.c flair_dac_load.
* Bochs 2.7 sources (REL_2_7_FINAL): gui/rfb.cc (BGR233Format, ServerInit,
  show_headerbar), gui/gui.cc (headerbar_click, snapshot_handler),
  iodev/display/vga.cc (VBE_DISPI_GETCAPS, update()), vga.h
  (VBE_DISPI_LFB_PHYSICAL_ADDRESS = 0xE0000000), iodev/cmos.cc (time0).
* docs/research/vesa8-ground-truth.md; vesa8.mk; WL-0156 (the cqej legs).
