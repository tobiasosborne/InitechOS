# WL-0096 -- Initech 123 retrieves and saves (initech-tdnl.91, slice 2)

## Context and reference

Slice 1 was completed and committed as 46dd682 before this slice began.
123.EXE had ignored slash and never linked its already graded WK1 codec.
The UI now uses the R2.2 Reference Manual (local in initech-manuals-ref/
lotus123/Lotus 1-2-3 2.2 Manuals (1989)) pp. 1-13/14 (control-panel menus,
first-letter selection and Escape), 5-1 (menu order), 5-19/20 (Retrieve
replaces data/settings), 5-21 (Save/default extension/current name), 5-22
(Cancel/Replace/Backup). Manual PDF pages 29/30, 133, 151..154 were rendered
and read; intermediates are at build/reference/pdfs.

Exact resource strings: lotus123-decomp/goldens/lotus-123-2.2-installed-ref/
extracted/installed/123.RI, offsets 0x15f (File menu), 0x2ef8/0x2f43
(Retrieve description/prompt), 0x2f63/0x2f92 (Save), 0xa9d (Replace),
0xad2 (Cancel). Save placement/mode also comes from goldens/minted/
MINT01.shots/03_save_prompt.png. Rows come from 02_entered.png and the
independent reader, not the artifact renderer.

## Changes

face_a.c handles /File Retrieve and Save, typed names, default .wk1, menu
highlight/first letters, filename backspace and Escape. The full reference
menu line is shown; other commands and Backup answer visibly with Command
not implemented. File errors also answer visibly. Those messages, typed-file
guidance and A: substitution for the reference's C: are authored, no local
reference. Quit follows the existing application Quit path.

Retrieve decodes into an alternate bank and swaps only after successful I/O,
close and validation. Save preflights the codec before CREATE, asks before
replacement, checks write/close results and removes incomplete new files.
Those transaction policies and capacity limits are authored, no local reference.
File transfers use static BSS, within the slice-1 gate's ownership bounds.
Makefile links wk1.c into 123.EXE; the core/codec itself is unchanged.
spec/flair_i123_file_traces.mk is a deliberate Rule-8 addition for this bead.

## Acceptance and frictions

Before code, the file trace failed Save did not create X.WK1, and the UI trace
failed menu/prompt/visible refusal. Final QEMU oracles passed:
- Locked Save/Quit/relaunch/Retrieve; stopped-disk mcopy + wk1_ref.py verifies
  Hello, 123, 1.5, -7 and complete record consumption; final rows/pixels match.
- Real MINT01.WK1 retrieves both rows, including formula caches, and pointer D2
  as its screenshot. Retrieval preserves the original disk bytes.
- Reference menu/prompt, visible unsupported command, missing-file refusal,
  malformed-file refusal followed by forced repaint retaining the old cells.
- Cancel leaves real MINT01 byte-identical; Replace changes A1 to Changed;
  mtools/independent reader/fsck.fat judge the disk.
- SAVE_EMPTY writes a valid empty file: independent cells and retrieved row RED.
  COMMAND_SILENT omits the refusal on panel line 2: RED, prompt still works.
  GET_CLOBBER decodes malformed data into the active sheet: old cells RED after
  repaint while the format refusal and desktop remain alive.
- Existing i123 + ARROWS_DEAD gates green; full host vector 378 gates green,
  with the verified local dbase golden copy from slice 1. makefile-vars green.

The functional trace uses the production Finder click timing and a 500-tick
pump lifetime; 53 inputs leave too little margin in the default 250 ticks.
The initial recording's 3000 ticks stopped at input 40; its completion marker
caught the failure. The final script reuses RECORDLONGDBL (6000 ticks), with
recording click timing already used by existing clips. A briefly duplicated
record variant was removed after cmp proved its kernel byte-identical to that
existing image. Record fields precede the Makefile rule because prerequisites
expand at read time. The acceptance clip has 55 frames; two captures are
byte-identical. Screen and prompt frames were visually inspected.

Logs: build/i123-files-{final-emu,unit,repro,headroom}.log;
clips: build/clips/i123_files.{gif,mp4}. Source is ASCII; diff --check clean.
Bochs i123 leg not run in this sandbox, explicitly per operator.

## Limits and handoff

Only this File path is implemented. No file-list chooser, Backup, password,
network reservation, Undo or remaining slash tree. Formula caches display;
formula editing/recalculation remains the existing later phase. Typed names
are capped at 48 characters, WK1 transfer at 64 KiB, cells at the existing
2048-cell capacity; errors refuse visibly and preserve the worksheet.
Replace uses DOS CREATE/truncate then writes: replacement is not atomic on
I/O failure or power loss. Incomplete NEW files are removed. No atomic rename
verb was added in this slice. These are explicit limits, not tested guarantees.

No bd or push. Writable Git metadata and verified branch bundle remain under
build. The real WK1 placed on the gate disk is a cmp-verified local copy;
reference corpus files are untouched.

Final gate/device-name correction is folded into the first slice commit;
46dd682 above names the original checkpoint. This slice is committed only
after the corrected gate's full vector and all touched QEMU families pass.

Corrected final gate: 186792-byte flagship, 187496-byte largest; slice 2 adds
zero shipped kernel bytes. test-unit: ALL GREEN (378 host gates), all i123
QEMU gates/mutants green, makefile-vars/headroom green, and final clip repro
PASS -- two captures byte-identical after the device-name correction.
