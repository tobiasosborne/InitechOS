# Note Pad and Scrapbook

NOTEPAD.EXE and SCRAPBK.EXE are two variants of one source (SCRAPBOOK is the
only semantic define). Dependencies: SDK, FLAIR public TextEdit/font headers,
libflair.a and the public region object. No FLAIR target source is recompiled.
The host tests temporarily compile exactly flair/textedit.c and flair/scrap.c;
FLAIR publishing a hosted TextEdit library removes that marked Makefile block.
The shared fake Toolbox is sdk/host. Its TEXT-only scrap cannot seed PICT;
the test models a PICT-only get as absent TEXT, preserving the error assertion.

Frozen legacy-final inventory: Note Pad has eight 1,024-byte editable pages,
selection, typing, deletion, arrows, Cut/Copy/Paste/Clear/Select All, corner
page turns, scroll, and save on turn/close. Scrapbook holds 32 TEXT clips,
appends clipboard clips, turns/removes pages, copies/cuts, scrolls and saves
on close. Both reopen their stores and retain a damaged store unchanged with
a notice. A store that exists but cannot be read (a read error, also after
partial reads, or damaged bytes) is never written, so that session cannot
save: it opens as a locked document does (System Software User's Guide v6.0
p. 87, "Locking a document also prevents saving any changes to it. You can,
however, open and print locked documents."). The notice appears when the
window first does, before any edit; typing, Backspace, Paste and Clear are
refused with SysBeep and Cut/Paste/Clear are gray; Copy and page turns work;
Close then has nothing to lose and ends at once (initech-gmyxm). PICT/sound, animations and native Note Pad pixel fidelity are absent.
The composed Platinum/Geneva9 windows follow the claimed store contract and
SimpleText geometry. Both implement one-level Undo, including Undo of Undo.
Note Pad restores page text, selection and dirty state; contiguous typing
and Backspace form one run, interrupted by menus/cursor movement/clicks.
Repaint preserves history; successful page turns and reload clear it.
Scrapbook restores the clip store, order and current index before append,
Cut or Clear, even when removal changed pages. Pure navigation keeps history.
Undo never changes the shared clipboard; rejected edits keep the last step.
These are authored adaptations of Macintosh System Software User's Guide v6
p.118 and the existing InitechText contract, not native Scrapbook captures.

Frozen omissions (approved phase-2 GM4 split): Note Pad has none beyond the
frozen legacy inventory. Scrapbook omits Edit Select All and Cmd-A: its
commands operate on whole TEXT clips and it has no partial-selection editor.
Note Pad retains its working Select All. PICT/sound remain outside the
existing store contract. The exact menu/shortcut inventory and semantic
dead-command sweep are in test_notepad_route (both builds); tests/COMMANDS.md
records the audited routes and independently authored outcomes.

make test runs two unit/regression programs and the two command sweeps (about
a second),
retaining the legacy store, corruption, clipboard and page-boundary assertions.
A store is read the way INT 21h AH=3Fh answers: every positive count is kept
until end of file, so legally short reads of the pinned N01/S01 fixtures show
the stored page (decoded by the test from the bytes) and N02/S02 stay damaged.
A valid edit keeps every other page (initech-stc9m, cases N01/S01): page 8's
first byte becomes Z, or clip 32 is cleared and Z pasted; the saved store is
decoded record by record and the other 7 (31) records equal the fixture's,
byte for byte (red when save drops page 1).
These are independent authored contract expectations, not real-Mac file
format differentials. make mutants kills twelve patches, including a live Undo
no-op in both builds, a single-read loader (initech-l3ahn), and a grayed
Select All and a dead Scrapbook Page Down that the command sweep catches,
the overlong capacity notice the gate refuses (initech-yhqb), and edits
accepted after a failed read (notepad_mut_unsavable_edit, initech-gmyxm). Every
refusal notice is shown: each alert line fits 47 bytes and 285 Chicago 12
pixels (toolbox_gate.h Sec 11, applied by sdk/host); a refused alert beeps. No mutation switches remain in the product source.
make smoke launches both on QEMU, copies alpha from Note Pad into Scrapbook,
closes both, extracts both stores with mtools and checks lengths, headers,
page bytes and an independently implemented checksum, then runs fsck.fat.
Screendumps: build/smoke-desktop/{notepad,scrapbook}.ppm.

Both apps reach the OS only through the Toolbox gate (INT 81h). The inherited
DOS AUX diagnostics (initech-q9p80) were removed rather than rerouted: nothing
read them, and an unreadable store is reported to the user when the window
first appears.
Remaining legacy composed-chrome emulator gates belong to the factory owner.

Initial windows meet the single menu bar (operator ruling, CLAUDE.md
"Hazards that are still true"). Host routing checks their structure top
against the public FLAIR_CHROME_MENUBAR_H metric; content sizes are preserved.

GM4 dead-command sweep (test_notepad_commands, test_scrapbook_commands): the
blocks below are the frozen inventory as data. The sweep checks the installed
bar against them in every context, chooses each row where it must be enabled
and requires its effect, sends its command key the way the shell does, and
sends every character byte and key code under each modifier, so a key that
changes anything must be listed here and every listed key must work. Effects
are judged only from what the host Toolbox records (paint, scrap, volume,
exit, alerts). Shift and Option do not change the meaning of a key in either
program; Control is Command and reaches only the menus. Tab, Delete and the
bytes from 7F up are not typed in Note Pad (Paste keeps them).

<!-- command-inventory NOTEPAD.EXE -->
Context: typed = type ab\rcd
Context: selected = type alpha; menu Edit/Select All
Context: clip = scrap alpha
Context: middle = type ab\rcd\ref; key v48; key v4B
Title: File | 2
Menu: File | Close | W | base | exit+file
Menu: File | Quit | Q | base | exit+file
Title: Edit | 7
Menu: Edit | Undo | Z | typed | screen
Menu: Edit | -
Menu: Edit | Cut | X | selected | screen+scrap
Menu: Edit | Copy | C | selected | scrap
Menu: Edit | Paste | V | clip | screen
Menu: Edit | Clear | - | selected | screen
Menu: Edit | Select All | A | typed | screen
Key: none | c0A c0D c20-7E | base | screen
Key: none | c08 v48 v4B v4D v50 | middle | screen
Key: shift | c0A c0D c20-7E | base | screen
Key: shift | c08 v48 v4B v4D v50 | middle | screen
Key: option | c0A c0D c20-7E | base | screen
Key: option | c08 v48 v4B v4D v50 | middle | screen
<!-- end-command-inventory -->

<!-- command-inventory SCRAPBK.EXE -->
Context: clip = scrap alpha
Context: one = scrap alpha; menu Edit/Paste
Context: three = scrap alpha; menu Edit/Paste; scrap bravo; menu Edit/Paste; scrap charlie; menu Edit/Paste; key v4B
Context: long = scraplines 40; menu Edit/Paste
Context: longdown = scraplines 40; menu Edit/Paste; key v50
Title: File | 2
Menu: File | Close | W | base | exit+file
Menu: File | Quit | Q | base | exit+file
Title: Edit | 6
Menu: Edit | Undo | Z | one | screen
Menu: Edit | -
Menu: Edit | Cut | X | one | screen+scrap
Menu: Edit | Copy | C | one | scrap
Menu: Edit | Paste | V | clip | screen
Menu: Edit | Clear | - | one | screen
Absent: Edit | Select All
Key: none | v49 v4B v4D v51 | three | screen
Key: none | v50 | long | screen
Key: none | v48 | longdown | screen
Key: shift | v49 v4B v4D v51 | three | screen
Key: shift | v50 | long | screen
Key: shift | v48 | longdown | screen
Key: option | v49 v4B v4D v51 | three | screen
Key: option | v50 | long | screen
Key: option | v48 | longdown | screen
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
<!-- end-interface-declarations -->
