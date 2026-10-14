# WL-0108 -- LOOK pass 6 menu batch

## Context

2026-10-15; lane-menus from BASE_COMMIT 882ac7d. Scope: initech-tdnl.93
(M002), .94 (M004), .95 (M006), .70 (M001/M005). Read CLAUDE.md, PRD6.3/6.4,
LOOK pass6 REPORT/TRIAGE batch3, system7-decomp sys8/menus.md and dropped
capture, locked metrics/goldens, font resources, menu/live drawers, existing
oracles and WL-0093. bd is unavailable; no tracker operation or push was run.
The operator owns tdnl.69: Charcoal and the caret command substitution remain
unchanged. Git metadata was copied once to build/lane.git; original .git was
not edited. No other project directory was written.

## What changed

- The existing single HiliteMenu/DrawMenuBar drawer now resolves light top,
  dark face, darker bottom, black baseline. The live Finder already calls it
  from os/milton/kmain.c over shell.c's offset view; no duplicate live drawer
  was introduced. Direct colors are teal; indexed colors use existing slots.
- Chicago glyph0x12 was added to chicago12.art, the C transcriber, and the
  generated header; Finder View uses checkMark18. The typed NFNT metrics
  golden gains aw11/lb1 and test-chicago-metrics grades it, including a new
  CHECK_ADVANCE mutant. No other glyph art changed.
- Deliberately re-keyed locked menu geometry, ink-side highlight bounds,
  affected host/pixel goldens and injected File/crossdrag/chicago_menus traces.
  Menu marks/items/titles continue to honor the effective clip.
- New independent test-menu-look and ppm_flair_menu_look_check grade original
  Mac/NFNT crops; new QEMU test-flair-menu-look captures held File and View.
  Added both gates and mutants to TEST_UNIT_GATES/TEST_EMU_GATES and registered
  clip menu_look in RECORD_SCRIPTS. No clip was recorded.

## Why

Derivation, never coordinates harvested from our renderer:

| Constant | Independent capture -> value |
| --- | --- |
| Apple slot | s8_menu_dropped panel/highlight starts x33 ->33px |
| Apple cell origin | captured ink starts17 - retained authored art bearing2 ->15 |
| Title cell pad | File ink43 - block33 =10 ink pad; Chicago bearing1 ->9px |
| Highlight ink pad | File43 - left33 =10; right exclusive73 - last ink62 -1 =10 |
| Item LPAD | N first ink53 - panel33 =20; Chicago bearing1 ->19px cell pad |
| Command RPAD | frame230 - last shortcut ink216 =14; N last ink is cell end-2 and frame is panel.right-1 ->13px cell pad |
| First row | rect21 - frame19 ->2px, retained |
| Item cap top | cap23 - rect21 ->2px, retained; panel cap offset4 |
| Item pitch / separator | 39-23=16; 109-87-16=6, retained |

Chicago File advances23px with two blank outer columns, so PAD9 yields41px
versus the captured Charcoal40px. Only the origins/ink-side pads are matched;
this lane does not promise Charcoal letter shapes. No-command RPAD12 and
command gap8 remain explicit authored policy: one inventory-dependent panel
capture cannot uniquely determine them. menus.md3 expressly lacks captured
checkmarked items, so mark cell panel+4 (NFNT ink+5) remains existing policy
inside the measured20px leading margin. No fabricated Mac mark coordinate.

Palette: s8_menu_dropped x34 y0 / y1..17 / y18 / y19 is
#8787DA / #5454B3 / #0000A5 / black. Authored teal mapping is
#8DDCDC / #27666C / #003F45 / black. Indexed fallbacks are existing
idx2 / idx63 (#3F3F3F) / idx0 / idx0. This reuses the scroll lane's
no-new-index reasoning (abd7679); two dark direct-color derived canon rows
preserve hue and contrast without adding indexed slots or changing neutral
grays. White/face sRGB contrast: historical6.3713:1, direct6.5556:1,
indexed10.5309:1 (desktop teal stays unchanged).

NFNT check provenance: NFNT_5478.bin size3098, bitImage@26, rowWords70,
fRectHeight15, locTable@2126, loc[18..19]=18/27; ow@2648=0x010B. Ink9x8 on
cell rows4..11,18 pixels, bearing1, advance11. This extraction was explicitly
requested by the operator; the prior ASCII strike stays hand-authored.
Independent white File ink crop is NFNT F, loc[70..71]=231/236, lb1/aw7,
rows3..11. Expected pixels do not come from the artifact's strike or renderer.

Locked spec/golden acts, dated2026-10-15:

- chrome_metrics.json/.h: Apple33/origin15, title9, item19, command13;
  retained row2/cap2/height16/separator6 and authored gaps documented.
- chrome_fidelity_golden.h: independent menu crop extension via new
  menu_look_golden.h; existing window-chrome expectations unchanged.
- color_canon.json/.h and color_canon_extract.c: two new direct-color
  derivatives; regenerate header from JSON (no hand-edited generated drift).
- chicago12.art/.h and chicago12_nfnt_golden.h: only glyph0x12 added; existing
  advances and other art unchanged.
- test_menu.c: item ink and command probes, actual ink-side title pads and
  distinct accent classes; test_finder_menu.c: slots33/74/117/167/231, widths
 41/43/50/64/46, end277 (NFNT23/25/32/46/28 +18).
- ppm_flair_menu_check: File L33, width19+26+8+16+13=82, exclusive R115;
  ppm_flair_menu_crossdrag_check: Edit L74/R156 by the same82px width.
- ppm_flair_disk_windows_check: Finder cells42/83/126/176/240 and end277;
  ppm_flair_app_launch_check: Apple33/PAD9, existing sweep retained and moved
  Apple art now graded at independent origin15 (no columns/classes dropped).
- ppm_flair_fonts_check: Style L74, cell LPAD19; widest styled NFNT62 plus
  retained12px no-command pad -> exclusive R167, frame166.
- ppm_flair_check: original20px Apple crop translates+13 to[13,33), threshold65
  unchanged; independent NFNT File contributes64px there, locked Apple art94.
- Makefile File trace now50,10; crossdrag50->100 (File[33,74), Edit[74,117)).
  chicago_menus clip points110/170/230/330 follow no-Apple slots0/41/84/144/
  199/257/307/377. Fonts trace bytes still hit File38 and Style80; dated note.

## Frictions

- Requested s8_menu_apple.png is absent from the corpus. The available
  s8_menu_dropped.png and original NFNT provided all graded measurements.
- The DROP-capture harness waits up to750ms after every preceding mouseDown,
  including disk double-clicks. RED and isolated repeat showed two singles,
  not an opened root. The new gate swaps only the existing
  finder_desktop_recorddbl.o timing fixture; production menu objects and
  behavior are identical. Both traces quit on the completed menu cancel.
- Pointer50,30 obscured the graded first row/white title. The isolated repeat
  confirmed it. The locked trace now holds File70,21 (padding, beyond ink63,
  above item41), then crosses to View140,21. Historical source crop similarly
  excludes its pointer on rows21/22, retaining the measured cap band23..31 and
  its clear above-cap column53. No expectation/tolerance was loosened.
- The desktop gate's canon regeneration dependency caught a manually edited
  generated header; isolated repeat confirmed it. Extend the C generator to
  four derived rows and regenerate. The later desktop pixel RED was the old
  Apple crop; isolated repeat confirmed it before its capture-derived re-key.
- test-makefile-vars found inherited BASE_COMMIT missing fourth argument in
  flair-data-rules for the i123-menu-stale image (undefined4, three sites).
  Restored standard IWORD_EXE; no application source was changed.

## Acceptance

All make invocations used -j2 or less and explicit SYSTEM7_DECOMP,
WIN31_DECOMP, DBASE3_DECOMP, LOTUS123_DECOMP, WORDPERFECT51_DECOMP roots.
Every emulator invocation used INITECH_QMP_STDIO=1, one emulator gate at a
time. No missing-corpus opt-out. No broad make test/test-unit/test-emu.
Every red emulator gate was repeated alone before interpreting the result.

RED before repair: test-menu-look originally17 checks/14 failures, including
ML1/ML2/ML3/ML4/ML5/ML6/ML7 at both depths; test-chicago-metrics H check had
measure10/lb0/bbox7x9 versus NFNTaw11/lb1. The comparable new emulator gate
failed twice alone on original title/geometry pixels. These logs are under
build/menu-look-red.log, chicago-check-red.log and menu-look-emu-red*.log.

Final new gates and mutant evidence:

| Target | Final evidence line |
| --- | --- |
| test-menu-look | test-menu-look:19 checks,0 failures; green (independent title band, item20/4, shortcut14, NFNT check18; both depths; clipping) |
| test-menu-look-mutant | MENU_LOOK_MUT_FLAT_HILITE correctly RED (ML1) |
| test-menu-look-mutant | MENU_LOOK_MUT_WRONG_RPAD correctly RED (ML4) |
| test-menu-look-mutant | MENU_LOOK_MUT_V_MARK correctly RED (ML5) |
| test-flair-menu-look | green (File dark band/white ink/item20/4/shortcut14; View NFNT check18) |
| test-flair-menu-look-mutant | MENU_LOOK_MUT_FLAT_HILITE correctly RED (ML1 title band) |
| test-chicago-metrics-mutant | CHECK_ADVANCE correctly RED (0x12 advance12 vs11) |

Final host logs: build/menu-final-look-host.log and menu-final-host.log.
Final new QEMU logs: build/test-flair-menu-look.final.log,
test-flair-menu-look-mutant.final.log; captured File/View PPMs and lossless
PNG copies in build/menu_look_{file,view}.{ppm,png}, visually inspected.

Targeted regression evidence (all run green):

- test-menu:83 checks,0 failures; test-menu-mutant: all ten named mutants
  correctly RED (fixed width, disabled selection, crossdrag, Apple square,
  flat bar, bevel, separator, disabled ink, title state, capped track).
- test-menu-handlers:50 checks,0 failures; test-tbx-menu:23 checks,0 failures.
- test-chicago-metrics: all934 checks green; test-chicago-metrics-mutant:
  ONE_ADVANCE correctly RED, plus CHECK_ADVANCE above.
- test-chicago-art: committed header == transcribe(chicago12.art).
- test-chrome-fidelity:28 checks,0 failures; test-chrome-fidelity-mutant: all
  existing named mutants correctly RED, including the NFNT T advance mutant.
- test-spec: VERDICT PASS (ADR-0003 data parses, sizes/banner/INT25/26 locked).
- test-makefile-vars: green (no rule reads a variable before definition).
- test-flair-menu: VERDICT PASS -- booted desktop has WORKING MENUS.
- test-flair-menu-crossdrag: VERDICT PASS -- live drop FOLLOWS cross-menu drag.
- test-flair-held-gestures: green (held drag and menu release choose final target).
- test-flair-tenant-menu: green (all four legs; no sweep/identity assertion lost).
- test-flair-desktop: VERDICT PASS -- LIVE FLAIR chimera desktop frame.
- test-flair-fonts: green; Style frame166 =74+19+NFNT62+12-1, styled items exact.
- test-kernel-repro: green -- kernel.bin reproducible;
  sha256 acaca40fdaed0cada0b80212de5fe1d1f9804617ff9b956362d619b04c31daea.
- test-kernel-headroom:152 kernels, window196608B/384 sectors, floor4096B;
  flagship174280B loaded/22328B headroom; largest
  kernel_flairtenants_mut_no_raise_on_title.elf175400B loaded/21208B binding.
  Flagship before174152B, after174280B: growth128B, below2048B constraint.

ASCII source check PASS; git diff --check clean. test_finder_menu and the
Finder-window grader were re-keyed by derivation but their separate gates
were NOT RUN (outside the permitted list). No test-chrome, Bochs, broad vector
or recording gate was run. Existing other emulator mutants were NOT RUN
unless specifically listed above. New menus are mechanically accepted;
Rule14 video/operator acceptance remains pending, deliberately delegated to
PM by the explicit instruction not to run record-flair. bd status is unchanged.

## Pointers

- docs/audits/2026-10-15-look-pass6/REPORT.md M001-M006 and TRIAGE.md batch3.
- system7-decomp/specs/sys8/menus.md1.3/1.4/2.2/3 and
  goldens/captures/s8_menu_dropped.png; goldens/resources/NFNT_5478.bin.
- spec/chrome_metrics.json platinum_menu_layout; spec/menu_look_golden.h;
  harness/proptest/test_menu_look.c; tools/ppm_flair_menu_look_check.c.
- spec/flair_menu_look_traces.mk and Makefile RECORD_SPEC_menu_look;
  PM should record menu_look and re-record chicago_menus after integration.
- docs/worklog/WL-0093; commits618211a (typed NFNT extension pattern) and
  abd7679 (existing indexed fallback reasoning).
- Deliverable: build/lane.bundle, containing BASE_COMMIT..lane-menus;
  verified by git bundle verify before handoff. No push was requested/run.
