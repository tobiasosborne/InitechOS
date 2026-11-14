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

Open measures the raw file length and reads until that length is complete;
positive short reads continue. A further one-byte read must return EOF before
the document is published, including empty and full-capacity inputs. Growth
during loading refuses the open rather than adopting the initial prefix.
Seek/read/early-EOF/close failures and files
over capacity produce an OK alert before changing the document, name or Undo.
The previously open document remains editable and Save still uses its name.
This implements GM4 and the operator's phase-2 original-name preservation
ruling for failed opens; it does not extend the shared save transaction's
rollback guarantees (sdk/include/tbx_save.h remains owned by SDK).

make test runs the claimed 98-check routing regression on sdk/host and a
disk-backed load/conservation regression, both in <1s.
Expectations are authored from the real-Mac SimpleText mint-results-007 and
Inside Macintosh contracts cited in the source; this is a unit/regression
suite, not a binary or pixel differential. The load suite uses pinned T01..T05
fixtures from the desktop reader study (4fb96275), plus literal prior-document
bytes. An independent stdio reader checks real host files after saves and
failed opens; the app's loader never supplies expected output. It covers
one-/three-/four-byte reads, line-end normalization across chunks, empty and
capacity inputs, seek/read/close failures, early EOF after a prefix, and real
growth/shrinkage through a separate host descriptor during the load (including
empty-to-nonempty and growth past capacity). The EOF probe must also succeed.
make mutants kills twelve patches, including partial-input publication and
bypassing the final EOF check.
make smoke launches
from the Finder on QEMU and Bochs, edits alpha CR bravo, saves UNTITLED on
QEMU and Bochs, checks exact bytes with mtools/cmp and checks the volume with fsck.fat.
Screendump: build/smoke-desktop/text.ppm.
Remaining legacy SimpleText pixel/scroll emulator gates belong to factory.
