# R3.6 menu-bar clock (initech-tdnl.13)

The lane reads the bead from the clone's exported issues.jsonl because bd show
reports no local database. No beads or Git metadata is changed.

The host formatter oracle was written first and failed to link before the API
existed. The locked QEMU trace then booted without a clock and failed MC5:
186 of 1600 pixels differed. The RTC is pinned with the existing harness
--rtc-base / --rtc-deterministic switches. The twelve-second gate-only pump
crosses a minute boundary in one boot; the normal pump budget stays unchanged.

The independent Mac OS 8 capture is
system7-decomp/goldens/captures/s8_desktop_menubar.png (12:56 am), corroborated
by s8_menu_dropped.png (1:00 am). The narrower captured clock font is replaced
by Chicago 12 as requested and as PRD 6.4 specifies. The oracle preserves the
captured bar colors, baseline and rightmost ink anchor, and uses the ORIGINAL
reference NFNT_5478.bin for the substituted glyphs. It never uses the shipped
font header or formatter to compute an expectation.

Deliberate Rule-8 spec change under this bead: the original Chicago clock
comparison exposed eleven incorrect pixels in the existing authored digit 2,
digit 5 and lowercase a. Correct their shoulders/diagonal/bowl in chicago12.art
and regenerate chicago12.h with font_art2h. No metrics change. The clock gate
stays exact; no mismatch tolerance is introduced.

The clock binds to the shell's top MenuBar without changing the disk-tenant
MenuBar ABI. The cooperative pump samples INT 21h AH=2Ch once per PIT second;
only a changed displayed minute repaints. DrawMenuBar preserves the bound
clock when a menu closes or a scene is recomposed. The mutations are
CLOCK_WRONG_HOUR and CLOCK_STUCK. The menubar_clock clip block precedes the
record-flair target; recording/repro and operator acceptance are the PM's
post-merge work. No new file is shipped on the data volume.

Acceptance run in this lane: test-menubar-clock (14 checks, 1440 formatted
readings, both pixel depths, bound/unbound bars and clipping), its mutant twin,
test-flair-menubar-clock and its two-mutant twin all pass. The emu run logs
12:55 am then 12:56 am, exactly two clock paints, and matches all 1600 crop
pixels. CLOCK_WRONG_HOUR fails MC1 on host and MC5 pixel537,5 in QEMU;
CLOCK_STUCK fails MC4 on host and MC5 pixel563,5 in QEMU. make image passes.
test-chicago-art, test-chicago-metrics (934 checks) and test-font-styles
(4304 checks) pass. test-kernel-headroom measures165 kernels: flagship176456
loaded bytes /20152 free; largest177576 loaded /19032 free, above4096 floor.
Every QEMU invocation sets INITECH_QMP_STDIO=1. No Bochs or full test vector
was run. Clip recording and repro were intentionally left to the PM.
