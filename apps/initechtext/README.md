# InitechText

TEXT.EXE edits one plain-text document in a Geneva 12 Platinum window.
Dependencies: SDK and FLAIR public TextEdit/font headers, libflair.a and
region object. The target consumes those libraries with plain flags and no
semantic defines. te_fonts.c is retired: the library owns the metric tables.
The only temporary block compiles flair/textedit.c and flair/scrap.c for the
host tests, removed when FLAIR publishes its hosted TextEdit library.

Frozen legacy-final inventory: New/Open/Close/Save/Save As/Quit, unsaved-work
and replace guards, typing/backspace/arrows, selection/drag, wrapping,
scrolling, Cut/Copy/Paste/Clear/Select All and single-level Undo with typing
runs. Up to 4,096 bytes; opening normalizes LF/CRLF to CR, saving preserves
single-byte TEXT. Close exits the app. No resize reflow or caret blinking.
Three visible commands are disabled: Page Setup, Print, Print One Copy.
GOLDEN-MASTER item 4's dead-command sweep is still outstanding.

make test runs the claimed 98-check routing regression on sdk/host in <1s.
Expectations are authored from the real-Mac SimpleText mint-results-007 and
Inside Macintosh contracts cited in the source; this is a unit/regression
suite, not a binary or pixel differential. make mutants kills ten patches;
EXE bytes stay identical after hook removal (28,720 bytes). make smoke launches
from the Finder on QEMU and Bochs, edits alpha CR bravo, saves MEMO.TXT on
QEMU and UNTITLED on Bochs, checks exact bytes with mtools/cmp and checks the volume with fsck.fat.
Screendump: build/apps/initechtext/smoke/text.ppm.
Remaining legacy SimpleText pixel/scroll emulator gates belong to factory.
