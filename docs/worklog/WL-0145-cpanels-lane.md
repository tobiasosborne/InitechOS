# WL-0145 - R4.5 control panels lane (initech-tdnl.19)

## Context and implementation

One disk tenant, APPS/CPANELS.EXE, launched through the existing installed-item
Apple menu table and the single tenant launch verb. The chooser and all five
panels use the existing DrawDialog / DialogHandleEvent / Control Manager
through a validated, synchronous flat-item gate. No retained application
pointers, second event loop, control renderer, or launch mechanism.

Date & Time reads and sets through AH=2A/2B/2C/2D. Gregorian date buttons and
a Menu Bar Clock page stage changes until OK; Cancel restores the snapshot.
Patterns offers the locked teal default and the first eight real PAT#_0 tiles.
Mouse stages tracking and double-click radio groups; OK changes the existing
raw-delta and Finder classifier parameters. Sound selects three PIT channel-2
tones, with silence/short/full durations (no analog-volume claim). Memory
reads total RAM, system usage and largest unused block through the live MCB
walk; the current implementation reserves no disk cache and has VM off.

CPANELS.DAT is a fixed 16-byte versioned record, specified in JSON and C.
The application writes through the existing Toolbox-to-INT21 file adapter
only on OK. The shell reads it through the same DOS adapter at boot; invalid
length, magic, version, ranges, reserved bytes or checksum fail loud to
serial and use defaults. CMOS owns the date/time; stale disk time is never
replayed. The desktop's existing full/damage painter applies the pattern.

## Independent sources and gaps

Pixel oracle: ../system7-decomp/goldens/captures/s8_alert_modal.png, whole
65x26 OK/default-ring crop. PRD6.4 substitutes Chicago for Charcoal, so the
label expectation is painted from the independent original
../system7-decomp/goldens/resources/NFNT_5478.bin. The existing clock oracle's
NFNT reader was factored for reuse; no shipped font data enters expectations.
The radio-label ambient face is checked against the captured dialog face.
The pattern oracle reads ../system7-decomp/goldens/resources/PAT#_0.bin
independently of the shipped table, including a painted-desktop restart leg.

Also inspected: captures/s8_controls_dialog.png,
s8_controls_appearance_colour.png, s7_753_control_root.png,
s7_701hd_cpanels.png; the corpus INDEX.md and GAPS.md; local fixtures README,
spec/chrome_metrics.json and spec/assets/color_canon.json. The corpus has no
screenshots/ directory in this checkout.

No native chooser, Date & Time, Desktop Patterns, Mouse, Sound or Memory
panel-layout capture exists in the supplied capture directory. As authorized,
these layouts compose existing Dialog/Control Manager furniture and locked
chrome metrics; they are not claimed native replicas. The inherited radio
SHAPE remains the Control Manager's recorded unminted gap. Its teal ambient
background was a real embedding bug and now follows the sampled dialog face.
Hardware sourcing is cached in docs/research/cpanels-hardware.md.

## Red / green and mutations

The host routing oracle first failed 22 chooser/transaction checks against the
stub. It also exposed Cancel retaining a staged RTC value. The first emu
chrome comparison exposed the shared default ring's radius-3 corners and the
button erasing its corner moat. The shared manager now paints the sampled
radius-4 corner crops after the button, without erasing its centre. The oracle
still compares every pixel. A subsequent radio-face probe failed both label
and cleared-corner samples before the ambient-face fix.

Host named mutants: CP_MUT_OK_NOTHING -> OK_WRITES;
CP_MUT_CANCEL_WRITES -> CANCEL_WRITES;
CP_MUT_CLOCK_NOT_APPLIED -> CLOCK_APPLIED.
Emu twins use CTRL_MUT_RING_OLD_CORNERS for each panel; the three transaction
mutants are real replacement CPANELS.EXE binaries on isolated volumes.
CP_MUT_MOUSE_INERT proves that a successful settings call must reach the real
pump. CTRL_MUT_RADIO_DESKTOP_BG proves the dialog's ambient face.

Persistence executes Special > Restart with --allow-reboot, checks the second
FLAIR-LIVE-READY and CPANELS-LOAD, independently reads the exact saved bytes
with mtools, then grades a fully painted boot frame against PAT#_0.bin.
The early-console screendump was rejected; the final pixel leg waits for
FLAIR-LIVE-OK. Corrupt input checks the serial warning and 512 locked-teal
pixels. Kernel headroom runs separately from other Make processes after an
initial concurrent build produced a GCC LTO bus error.

## Volume and Rule 14 handoff

FLAIR_FIXTURES includes CPANELS.EXE. The damaged fixture already rebuilds APPS
with stranded TRASH at entry 0, so its locked coordinates remain unchanged.
Locked purge is re-keyed to 21 removed / 2 refused; the Empty Trash recording
marker is re-keyed to 23 (README + APPS + 21 leaves). The launch trace's first
entry and heap base are unchanged. Gate-local mutant volumes have explicit
stock fixture contracts.

The control_panels RECORD_SCRIPTS / RECORD_SPEC / RECORD_IMAGE / RECORD_DATA
block is before record-flair, as required by parse-time prerequisites. Its
locked trace shows the chooser and five panels. The PM records/reproves the
Rule-14 clip on main after merge. No clip was recorded, no full test vector or
Bochs was run, and no git or beads mutation was performed in this lane.

## File inventory

New: os/apps/cpanels/{cpanels.c,core.c,cpanels.h};
os/flair/cp_service.h; spec/{cp_dialog.h,cpanels_record.h,
flair_cpanels_traces.mk}; spec/cpanels/record.json;
harness/proptest/test_cpanels.c; tools/ppm_flair_cpanels_check.c;
docs/research/cpanels-hardware.md; this worklog.

Modified: Makefile; os/flair/{tbxgate.c,tbxgate.h,desktop.c,desktop.h,event.c,
event.h,menu.c,menu.h,control.c,control.h,dialog.c,dialog.h,finder_desktop.c};
os/milton/{kmain.c,int21.c,int21.h}; spec/{apple_menu_items.h,toolbox_gate.h};
harness/{menubar_clock_oracle.c,menubar_clock_oracle.h,fixture_volumes.mk};
harness/proptest/test_finder_desktop.c; harness/diff/fat_diff/locked_trash.sh.

A compatibility compile also caught the mouse-parameter edit moving the existing
unused annotation away from clamp_coord: EVENT_MUTATE_STALE_WHERE deliberately
removes its calls. The annotation is restored; the existing Event and Finder
host mutants pass. The rebuilt production event.o is byte-identical to its
pre-annotation-fix copy, so the measured kernel bytes/headroom are unchanged.
The Apple-menu installed-row check is deliberately re-keyed from 3 to 4
(About, divider, Calculator, Control Panels), not relaxed to a wildcard.

## Final acceptance

All requested lane gates and mutation twins passed. The complete output is
build/cpanels_lane_full.log; concise exact tails are in
build/cpanels_lane_results.log. Added public targets:

- test-cpanels; test-cpanels-mutant
- test-flair-cpanels (five-panel aggregate)
- test-flair-cpanels-date; test-flair-cpanels-date-mutant
- test-flair-cpanels-patterns; test-flair-cpanels-patterns-mutant
- test-flair-cpanels-mouse; test-flair-cpanels-mouse-mutant
- test-flair-cpanels-sound; test-flair-cpanels-sound-mutant
- test-flair-cpanels-memory; test-flair-cpanels-memory-mutant
- test-flair-cpanels-settings; test-flair-cpanels-settings-mutant
- test-flair-cpanels-persist; test-flair-cpanels-corrupt
- test-flair-cpanels-mouse-apply-mutant; test-flair-cpanels-radio-mutant
- test-flair-cpanels-mutant (all mutant families aggregate)

All leaf host gates are in TEST_UNIT_GATES and all leaf emu gates are in
TEST_EMU_GATES. Each panel matched all 1690 independent button/ring pixels;
the three radio panels additionally matched their two ambient-face probes.
Each old-ring twin failed at 51/1690 pixels. The radio-background twin failed
at (170,164) and (150,166). Transaction RED lines: CP_OK_WRITE,
CP_CANCEL_WRITE, CP_CLOCK_APPLY; pump RED line: CP_MOUSE_APPLY cursor421
expected422. Host mutation lines are OK_WRITES, CANCEL_WRITES, CLOCK_APPLIED.

make image passed. Volume-growth checks passed: test-flair-locked-purge,
test-flair-finder-service, test-flair-app-launch (all ten legs),
test-flair-app-launch-mutant (all six named regressions), test-flair-empty-trash,
and test-flair-apple-menu. The first launch trace and heap-base golden stayed
byte-identical. The final flagship binary also stayed byte-identical after
the annotation/documentation-only cleanup.

Headroom: 207 kernels measured, 196608-byte window, 4096-byte floor.
Flagship 184040 bytes, 12568 bytes free; binding largest
kernel_flairtenants_db_write_skip.elf 184616 bytes, 11992 bytes free.

Shared-manager checks passed: test-control (170), test-dialog (81),
test-finder-desktop (104), test-event (322), test-apple-menu (26),
test-menubar-clock (14), and the Event/Finder/clock mutant families.
test-makefile-vars passed. New source/oracle files passed the ASCII and
no-getenv/no-host-path scan. The pixel-oracle diagnostic cleanup was rechecked
against both polarities of every saved panel dump and both boot-pattern dumps.
