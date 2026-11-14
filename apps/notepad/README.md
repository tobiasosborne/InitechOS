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

make test runs two unit/regression programs (<1 second),
retaining the legacy store, corruption, clipboard and page-boundary assertions.
These are independent authored contract expectations, not real-Mac file
format differentials. make mutants kills six patches, including a live Undo
no-op in both builds. No mutation switches remain in the product source.
make smoke launches both on QEMU, copies alpha from Note Pad into Scrapbook,
closes both, extracts both stores with mtools and checks lengths, headers,
page bytes and an independently implemented checksum, then runs fsck.fat.
Screendumps: build/smoke-desktop/{notepad,scrapbook}.ppm.

Inherited diagnostics use DOS AUX INT 21h in addition to the Toolbox gate;
the SDK lacks a diagnostic-output binding. This is retained behavior, not a
new app interface. The desktop/SDK owners must resolve that boundary debt.
Remaining legacy composed-chrome emulator gates belong to the factory owner.

Initial windows meet the single menu bar (operator ruling, CLAUDE.md
"Hazards that are still true"). Host routing checks their structure top
against the public FLAIR_CHROME_MENUBAR_H metric; content sizes are preserved.

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: desktop
Interface migration: notes-diagnostics21 initech-q9p80
The shared Note Pad/Scrapbook diagnostic AUX site alone awaits a Toolbox route.
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
