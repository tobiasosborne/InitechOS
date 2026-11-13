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
a notice. PICT/sound, animations and native Note Pad pixel fidelity are absent.
The composed Platinum/Geneva9 windows follow the claimed store contract and
SimpleText geometry. Undo is disabled in both apps; Scrapbook also disables
Select All (one visible dead command in Note Pad, two in Scrapbook).

make test runs two unit/regression programs (44 and 26 checks, <1 second),
retaining the legacy store, corruption, clipboard and page-boundary assertions.
These are independent authored contract expectations, not real-Mac file
format differentials. make mutants kills five patches; target EXEs before
and after removal are identical: Note Pad 27,456 bytes, Scrapbook 26,976.
make smoke launches both on QEMU, copies alpha from Note Pad into Scrapbook,
closes both, extracts both stores with mtools and checks lengths, headers,
page bytes and an independently implemented checksum, then runs fsck.fat.
Screendumps: build/smoke-desktop/{notepad,scrapbook}.ppm.

Inherited diagnostics use DOS AUX INT 21h in addition to the Toolbox gate;
the SDK lacks a diagnostic-output binding. This is retained behavior, not a
new app interface. The desktop/SDK owners must resolve that boundary debt.
Remaining legacy composed-chrome emulator gates belong to the factory owner.
