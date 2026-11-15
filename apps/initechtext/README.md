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
Frozen omissions (approved phase-2 GM4 split): File Page Setup..., Print...
(Cmd-P), Print One Copy. Their rows/shortcut and redundant divider are absent;
there is no printer backend. File retains New/Open/Close/Save/Save As/Quit
in the original order. The route suite sweeps the exact ordered native menu
and shortcut manifest, including disabled contextual rows, and checks all
retained commands against literal text, selection, file and exit outcomes.
See tests/COMMANDS.md for the audited mode/input-route coverage and shared
shell-dialog boundary.

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

make test runs the 127-check routing regression on sdk/host, a
disk-backed load/conservation regression and the GM4 command sweep, in <1s.
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
make mutants kills fifteen patches, including partial-input publication,
bypassing the final EOF check, reinserting a disabled Print command and a
dead Save As that the command sweep catches.
make smoke launches
from the Finder on QEMU and Bochs, edits alpha CR bravo, saves UNTITLED on
QEMU and Bochs, checks exact bytes with mtools/cmp and checks the volume with fsck.fat.
Screendump: build/smoke-desktop/text.ppm.
Remaining legacy SimpleText pixel/scroll emulator gates belong to factory.

Initial windows meet the single menu bar (operator ruling, CLAUDE.md
"Hazards that are still true"). Host routing checks their structure top
against the public FLAIR_CHROME_MENUBAR_H metric; content sizes are preserved.

GM4 dead-command sweep (test_itext_commands, tests/command_sweep.h): the
block below is the frozen inventory as data. The sweep checks the installed
bar in every context, chooses each row where it must work, sends its command
key as the shell does, and sends every character byte and key code under each
modifier, so any key that changes anything must be listed and every listed key
must work, judged only from the host Toolbox record. Shift and Option do not
change a key; Control is Command and reaches only the menus. Tab, Delete and
the bytes from 7F up are not typed (Paste keeps them).

<!-- command-inventory TEXT.EXE -->
Context: typed = type ab\rcd
Context: selected = type alpha; menu Edit/Select All
Context: clip = scrap alpha
Context: middle = type ab\rcd\ref; key v48; key v4B
Context: opened = file A.TXT alpha; menu File/Open...; sfreply A.TXT
Context: openedtyped = file A.TXT alpha; menu File/Open...; sfreply A.TXT; type x
Title: File | 8
Menu: File | New | N | opened | screen
Menu: File | Open... | O | base | dialog
Menu: File | -
Menu: File | Close | W | base | exit
Menu: File | Save | S | openedtyped | file
Menu: File | Save As... | - | base | dialog
Menu: File | -
Menu: File | Quit | Q | base | exit
Title: Edit | 7
Menu: Edit | Undo | Z | typed | screen
Menu: Edit | -
Menu: Edit | Cut | X | selected | screen+scrap
Menu: Edit | Copy | C | selected | scrap
Menu: Edit | Paste | V | clip | screen
Menu: Edit | Clear | - | selected | screen
Menu: Edit | Select All | A | typed | screen
Absent: File | Page Setup...
Absent: File | Print...
Absent: File | Print One Copy
Key: none | c0A c0D c20-7E | base | screen
Key: none | c08 v48 v4B v4D v50 | middle | screen
Key: shift | c0A c0D c20-7E | base | screen
Key: shift | c08 v48 v4B v4D v50 | middle | screen
Key: option | c0A c0D c20-7E | base | screen
Key: option | c08 v48 v4B v4D v50 | middle | screen
<!-- end-command-inventory -->

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: desktop
Interface use-gate: sdk/include/tbx.h
Interface use-pure: flair/include/scrap.h
Interface use-pure: flair/include/surface.h
Interface use-pure: flair/include/text.h
Interface use-pure: flair/include/textedit.h
Interface use-pure: flair/region/include/region_algebra.h
Interface use-records: build/flair/generated/asset_lz_data.h
Interface use-records: build/flair/generated/boot_logo.h
Interface use-records: build/flair/generated/chicago12.h
Interface use-records: build/flair/generated/chicago_packed_data.h
Interface use-records: build/flair/generated/color_canon.h
Interface use-records: build/flair/generated/flair_dac.h
Interface use-records: build/flair/generated/geneva12.h
Interface use-records: build/flair/generated/geneva12_packed_data.h
Interface use-records: build/flair/generated/palette.h
Interface use-records: flair/include/assets/font_linkage.h
Interface use-pure: flair/include/assets/geneva9.h
Interface use-records: flair/include/ostype.h
Interface use-pure: flair/include/assets/geneva12.h
<!-- end-interface-declarations -->
