# Control Panels

CPANELS.EXE is the chooser plus Date & Time, Desktop Patterns, Mouse, Sound
and Memory panels. Target dependency: SDK only, through typed gate bindings
with plain mk/tenant.mk flags and no semantic switches. The SDK supplies the
settings record and dialog transport. There are no dead commands.

Frozen legacy-final inventory: stage changes, OK to persist/apply, Cancel to
discard, Gregorian date/time adjustment, menu clock visibility, nine patterns
(teal and eight System 7 PAT# tiles), tracking/double-click speeds, three
speaker sounds/lengths and preview, and read-only memory labels. RTC is CMOS;
CPANELS.DAT stores the other settings in the claimed record.json format.
Native panel layouts are not recovered: existing Dialog/Control furniture is
an authored composition, not a pixel replica of a period panel.

make test runs three suites, 144 core/dialog checks, four shared-sdk/host
transport checks and the GM4 command sweep, in about a second. The PAT# resource comparison is differential
against real System 7 bytes; routing/transactions/calendar/record assertions
are unit/regression. make mutants kills five patches, among them a dead
Sound Test button that only the command sweep catches. make smoke launches on QEMU, chooses
Stipple, saves, quits, then mtools/cmp checks a literal independent 16-byte
record and fsck checks the volume. Screendump:
build/smoke-desktop/patterns.ppm.

Host dependencies: SDK host transport; FLAIR's public include/ Dialog API and
cp_dialog_items.h adapter, region/include/ records, and the owner-built
build/flair/host/libdialog.a from make -C flair host-dialog. Factory's public
include/test_assert.h supplies the canonical host assertions. The app neither
compiles FLAIR sources nor reads its private renderer/adapter headers.
make test checks its include paths and all own-object header dependencies,
including host tests. The real Dialog Manager press/release assertions remain
unchanged and link the public hosted library. No target adds a FLAIR dependency.
Remaining legacy panel pixel/bar-composition smokes belong to factory.

Initial windows meet the single menu bar (operator ruling, CLAUDE.md
"Hazards that are still true"). Host routing checks their structure top
against the public FLAIR_CHROME_MENUBAR_H metric; content sizes are preserved.

GM4 dead-command sweep (test_cpanels_commands, tests/command_sweep.h): the
block below is the frozen inventory as data. Every clickable dialog item of
every panel is named by its text and item number; each is clicked where it
must change something (a radio button already chosen is shown in a context
where it is not), and every visible clickable item must be listed. Every key
under each modifier is also sent: keys reach only the Dialog Manager (Return
and Escape for OK and Cancel are its routes, flair/), and Control-Q is Quit.

<!-- command-inventory CPANELS.EXE -->
Context: dt = button Date & Time@3
Context: dtclock = button Date & Time@3; button Menu Bar Clock@15
Context: patterns = button Desktop Patterns@4
Context: patternsblack = button Desktop Patterns@4; button Black@4
Context: mouse = button Mouse@5
Context: mouseslow = button Mouse@5; button Slow@3; button Slow@6
Context: sound = button Sound@6
Context: memory = button Memory@7
Title: File | 1
Menu: File | Quit | Q | base | exit
Button: Close@1 | base | exit
Button: Cancel@2 | base | exit
Button: Date & Time@3 | base | screen
Button: Desktop Patterns@4 | base | screen
Button: Mouse@5 | base | screen
Button: Sound@6 | base | screen
Button: Memory@7 | base | screen
Button: OK@1 | dt | screen+file
Button: Cancel@2 | dt | screen
Button: Hour -@3 | dt | screen
Button: Hour +@4 | dt | screen
Button: Min -@5 | dt | screen
Button: Min +@6 | dt | screen
Button: Menu Bar Clock@7 | dtclock | screen
Button: Day -@8 | dt | screen
Button: Day +@9 | dt | screen
Button: Mon -@10 | dt | screen
Button: Mon +@11 | dt | screen
Button: Year -@12 | dt | screen
Button: Year +@13 | dt | screen
Button: Date & Time@14 | dtclock | screen
Button: Menu Bar Clock@15 | dt | screen
Button: OK@1 | patternsblack | screen+file+settings
Button: Cancel@2 | patterns | screen
Button: Teal solid@3 | patternsblack | screen
Button: Black@4 | patterns | screen
Button: Dark stipple@5 | patterns | screen
Button: Stipple@6 | patterns | screen
Button: Gray@7 | patterns | screen
Button: Rows@8 | patterns | screen
Button: Columns@9 | patterns | screen
Button: Diagonal@10 | patterns | screen
Button: Lines@11 | patterns | screen
Button: OK@1 | mouseslow | screen+file+settings
Button: Cancel@2 | mouse | screen
Button: Slow@3 | mouse | screen
Button: Normal@4 | mouseslow | screen
Button: Fast@5 | mouse | screen
Button: Slow@6 | mouse | screen
Button: Normal@7 | mouseslow | screen
Button: Fast@8 | mouse | screen
Button: OK@1 | sound | screen+file
Button: Cancel@2 | sound | screen
Button: Beep@3 | sound | beep
Button: Bell@4 | sound | screen+beep
Button: Click@5 | sound | screen+beep
Button: Silent@6 | sound | screen+beep
Button: Short@7 | sound | screen+beep
Button: Full@8 | sound | beep
Button: Test@9 | sound | beep
Button: OK@1 | memory | screen
Button: Cancel@2 | memory | screen
<!-- end-command-inventory -->

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: desktop
Interface use-gate: sdk/include/tbx.h
<!-- end-interface-declarations -->
