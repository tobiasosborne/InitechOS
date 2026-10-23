# InitechWord audit - 2026-10-23

Audited supplied image commit e67b8311092873fe16e6c6d62ad82f894bfb8d7d.
91 findings: 1 P1, 87 P2, 3 P3.
The catalog separates 30 observed defects from 61 documented feature/compatibility gaps.
The critical result is repeatable loss of the previous file on a full-disk overwrite (L001).
The basic editor is substantially more dependable than its feature coverage: tested code
nesting, blocks, undo, ordinary saving, and a real-file round trip passed. Many findings
below are explicitly documented omissions, retained because the brief asks for a complete
1990s workstation, not just conformance to the reduced implementation.

## 1. Method and replay conventions

Only copied disks were booted. Five QEMU sessions (S01-S05) used -display none,
-cpu 486, -m 16 and QMP on stdio. One emulator at a time; no host X input or
capture, repository writes, builds, commits, tracker operations or other users' processes.
All five QEMUs were stopped through their own QMP connections. See evidence/cleanup.jsonl
and each session JSON for PIDs, times, image hashes and complete command lines.

S01 used images/prepared.img; S02 and S04 used images/full-input.img; S03 used
images/stress-input.img; S05 used prepared.img. Preparation scripts and source fixtures
are in evidence/. All filesystem extraction happened after the corresponding QEMU stopped.

Common launch: double-click disk (599,65), APPS (125,104), WORD.EXE (284,124).
On the full disk a startup disk-full alert must first be dismissed with Enter.
"New/clear document" means, from the editing screen, F7, N, N. It deliberately
abandons the preceding test document. F1 then Esc can dismiss a modal before that sequence.
Home Home Up and Down are consecutive key presses, not held chords. Plain filenames
are in the data disk root unless a path is written. Text is typed without a final Enter
unless Enter is explicitly specified. Screen coordinates are in the 640x480 QEMU framebuffer.

Each finding's shots are unmodified screendumps converted to PNG. Contacts/crops in evidence/
were used only to inspect them. The authoritative input stream is evidence/actions.jsonl;
primitive per-session streams and evidence/replay.py preserve the recorded timing and avoid double-running
composite type/click records. Example: `python3 evidence/replay.py s04 /tmp/iword-replay-s04`
requires a NEW output directory and runs only one emulator. Preparation scripts may be rerun
before replay if the prepared input disks are unavailable. Source snapshots in
evidence/source-contracts/ include numbered lines; all relative source references below
are under /home/tobias/Projects/initech-os. Real WP corpus paths are under
/home/tobias/Projects/wordperfect51-decomp; the original PDF is
/home/tobias/Projects/initech-manuals-ref/wordperfect51/WPDOS51Reference.pdf.

A "Documented feature gap" is observed behavior that the project's manual already admits;
it is not presented as an accidental regression. Source causes are guesses unless stated
as direct serial/byte evidence. Exact real-WP behavior at the long-word Tab/Indent edge
remains unminted; those two expectations are explicitly consistency judgments.

## 2. Findings

### L001 - P1 - Full-disk overwrite destroys the previous document

Area: Save. Classification: Defect.

Replay: S02: retrieve LONG.WP5; F10, ORIGINAL.WP5, Enter, Y. Repeat on fresh full-input.img in S04 via F7, Y, ORIGINAL.WP5, Enter, Y.

Observed: Both attempts report Disk Full. ORIGINAL.WP5 changes from 373 bytes to ZERO bytes. The new 8,727-byte document remains in memory, but the previously saved version is gone. Serial: create succeeds, write(n=8727) returns 0, close returns 0.

Expected and basis: A failed save must preserve the last saved version (data-integrity expectation, not a claimed historical WP atomic-save measurement).

Evidence: [shots/s02-real-full-original-error.png](shots/s02-real-full-original-error.png), [shots/s04-exit-save-full.png](shots/s04-exit-save-full.png), [shots/s04-cancel-retains-long.png](shots/s04-cancel-retains-long.png), [evidence/fixtures/ORIGINAL.WP5](evidence/fixtures/ORIGINAL.WP5), [evidence/s02-extracted/ORIGINAL.WP5](evidence/s02-extracted/ORIGINAL.WP5), [evidence/s04-extracted/ORIGINAL.WP5](evidence/s04-extracted/ORIGINAL.WP5), [evidence/s02-bytes.json](evidence/s02-bytes.json), [evidence/s04-bytes.json](evidence/s04-bytes.json), [evidence/prepare_full.py](evidence/prepare_full.py), [evidence/s02-serial.log](evidence/s02-serial.log), [evidence/s04-serial.log](evidence/s04-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: os/iword/app/face_a.c:1247-1258 creates/truncates the destination before the failing write; no rollback is visible (guess).

### L002 - P2 - Failed new saves leave zero-byte document files

Area: Save. Classification: Defect.

Replay: S02, after L001: F1 twice; F10 NEW.WP5 Enter. Later fill the released cluster by changing Setup, then F10 SMALL.WP5 Enter.

Observed: Both saves fail and leave named zero-byte files. NEW.WP5 and SMALL.WP5 are not usable WP documents.

Expected and basis: A failed new save should remove its incomplete destination or clearly identify it as incomplete (auditor expectation).

Evidence: [shots/s02-real-full-new-error.png](shots/s02-real-full-new-error.png), [shots/s02-full-small.png](shots/s02-full-small.png), [evidence/s02-extracted/NEW.WP5](evidence/s02-extracted/NEW.WP5), [evidence/s02-extracted/SMALL.WP5](evidence/s02-extracted/SMALL.WP5), [evidence/s02-directory.txt](evidence/s02-directory.txt), [evidence/s02-serial.log](evidence/s02-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L003 - P2 - Resizing hides the status line and every prompt

Area: Window. Classification: Defect.

Replay: S03: type Visible prompt test. Drag grow box (632,472) by (-160,-130). Press F10. Grow back from (472,342) by (160,130).

Observed: The smaller window has no status line. F10 enters a name prompt that cannot be seen. Enlarging reveals the pending prompt.

Expected and basis: An enabled grow control must preserve access to document status and prompts; classic Mac window behavior from memory.

Evidence: [shots/s03-resize.png](shots/s03-resize.png), [shots/s03-resize-save-prompt.png](shots/s03-resize-save-prompt.png), [shots/s03-resize-prompt-restored.png](shots/s03-resize-prompt-restored.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: os/iword/app/face_a.c:843 uses fixed editing geometry and the event handler at 2412 has no resize adaptation (guess).

### L004 - P2 - Directories silently lose all entries after file 254

Area: List Files. Classification: Defect.

Replay: S03 input contains MANY/F000.WP5 through F269.WP5. F5 Enter, Down Right Right Right Enter Enter. Down 140 times, Right, Down. F1; Shift-F10 MANY\F269.WP5 Enter.

Observed: List Files reports 254 files and 86,868 used bytes; the list ends at F253. F269 retrieves successfully by name. Sixteen existing documents are omitted without a warning.

Expected and basis: Project contract pubs/manuals/initechword-reference/ref-l.tex:73: "List Files shows the documents and other files on the data diskette". The fixture actually contains 270 files, 92,340 bytes.

Evidence: [shots/s03-many-first.png](shots/s03-many-first.png), [shots/s03-many-last.png](shots/s03-many-last.png), [shots/s03-many-hidden-retrieve.png](shots/s03-many-hidden-retrieve.png), [evidence/stress-before-directory.txt](evidence/stress-before-directory.txt), [evidence/prepare_stress.py](evidence/prepare_stress.py), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: os/iword/app/face_a.c:1369 LF_MAX=256 and :1412 bounded find; dot entries consume two slots (guess).

### L005 - P2 - Shut Down silently cancels while Word has unsaved work

Area: Quit / Finder. Classification: Defect.

Replay: S01: append unsaved shutdown to EXITTEST.WP5. Drag Word title (320,50) by (0,150). Activate APPS at (210,90). Hold Special (201,30), release (216,123).

Observed: No save question or explanation appears; the desktop remains running. Serial says FINDER-POWER-CANCEL app=WORD.EXE cannot-ask. The unsaved document is preserved.

Expected and basis: Finder should bring the document forward for its save decision, or visibly explain the refusal (classic Mac shutdown expectation from memory). This is a blocked shutdown, not demonstrated data loss.

Evidence: [shots/s01-before-shutdown.png](shots/s01-before-shutdown.png), [shots/s01-finder-special.png](shots/s01-finder-special.png), [shots/s01-shutdown-unsaved.png](shots/s01-shutdown-unsaved.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L006 - P2 - Real WP soft-page documents cannot be retrieved

Area: WP5 import. Classification: Documented compatibility gap.

Replay: S01, clear screen separately for each file: Shift-F10 MINT39.WP5 Enter; repeat with MINT44.WP5.

Observed: Both real WP 5.1 files are rejected as Incompatible File Format. MINT39 has a multi-page line list; MINT44 wraps a paragraph across a page break.

Expected and basis: The supplied real files and their captures are valid WP documents: /home/tobias/Projects/wordperfect51-decomp/mint/wpMINT39.steps, wpMINT44.steps and corresponding 01_typed.png. Refusing an ordinary multi-page document is an interoperability gap. The project documents refusal of unknown codes in ref-w.tex:48.

Evidence: [shots/s01-import-mint39.png](shots/s01-import-mint39.png), [shots/s01-import-mint44.png](shots/s01-import-mint44.png), [evidence/fixtures/MINT39.WP5](evidence/fixtures/MINT39.WP5), [evidence/fixtures/MINT44.WP5](evidence/fixtures/MINT44.WP5), [evidence/references/real-mint39.png](evidence/references/real-mint39.png), [evidence/references/real-mint44.png](evidence/references/real-mint44.png), [evidence/wp5-softpage-inputs-decoder.txt](evidence/wp5-softpage-inputs-decoder.txt), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L007 - P2 - Table cells render as one concatenated line

Area: WP5 import. Classification: Defect.

Replay: S01: clear document; Shift-F10 MINT38.WP5 Enter; F11.

Observed: A1 B1 C1 / A2 B2 C2 render as A1B1C1A2B2C2After. with no table grid or cell separation.

Expected and basis: Real capture /home/tobias/Projects/wordperfect51-decomp/mint/shots_wpMINT38/06_cells_typed.png, region (0,0)-(584,73), shows three columns and two rows. The import should preserve readable cell structure.

Evidence: [shots/s01-import-mint38.png](shots/s01-import-mint38.png), [shots/s01-import-mint38-reveal.png](shots/s01-import-mint38-reveal.png), [evidence/fixtures/MINT38.WP5](evidence/fixtures/MINT38.WP5), [evidence/references/real-mint38.png](evidence/references/real-mint38.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L008 - P2 - Imported centered lines appear at the left margin

Area: WP5 import. Classification: Documented compatibility gap.

Replay: S01: clear document; Shift-F10 MINT01.WP5 Enter.

Observed: The centered line starts at x=7, regardless of its alignment code.

Expected and basis: Real MINT01.shots/02_typed.png in the supplied corpus shows the intended alignment. Project ref-c.tex:118-124 explicitly acknowledges the limitation.

Evidence: [shots/s01-real-mint01.png](shots/s01-real-mint01.png), [evidence/references/real-mint01.png](evidence/references/real-mint01.png), [evidence/fixtures/MINT01.WP5](evidence/fixtures/MINT01.WP5), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L009 - P2 - Imported flush-right lines appear at the left margin

Area: WP5 import. Classification: Documented compatibility gap.

Replay: S01: clear document; Shift-F10 MINT03.WP5 Enter.

Observed: The flush-right line starts at x=7, regardless of its alignment code.

Expected and basis: Real MINT03.shots/02_typed.png in the supplied corpus shows the intended alignment. Project ref-c.tex:118-124 explicitly acknowledges the limitation.

Evidence: [shots/s01-import-mint03.png](shots/s01-import-mint03.png), [evidence/references/real-mint03.png](evidence/references/real-mint03.png), [evidence/fixtures/MINT03.WP5](evidence/fixtures/MINT03.WP5), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L010 - P2 - Extended characters disappear from imported text

Area: WP5 import. Classification: Defect.

Replay: S03: clear document; Shift-F10 MINT05.WP5 Enter; F11.

Observed: The accented e in Cafe is missing: the line reads Caf and ...; Reveal substitutes [?].

Expected and basis: Real MINT05.shots/02_typed.png shows the accented e; the bytes are C0 29 01 C0, documented in specs/file-formats/wp5-body-codes.md section 3. A character must remain visible.

Evidence: [shots/s03-mint05.png](shots/s03-mint05.png), [shots/s03-mint05-reveal.png](shots/s03-mint05-reveal.png), [evidence/stress-fixtures/MINT05.WP5](evidence/stress-fixtures/MINT05.WP5), [evidence/references/real-mint05.png](evidence/references/real-mint05.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L011 - P2 - Imported footnote references are invisible

Area: WP5 import. Classification: Defect.

Replay: S03: retrieve MINT05.WP5 on a clear screen.

Observed: The superscript/reference 1 after Footnote host. is absent. Reveal shows [?] for the footnote group.

Expected and basis: Real MINT05.shots/02_typed.png, first text line, has a highlighted 1 at that position. The user needs to see that a note exists.

Evidence: [shots/s03-mint05.png](shots/s03-mint05.png), [shots/s03-mint05-reveal.png](shots/s03-mint05-reveal.png), [evidence/references/real-mint05.png](evidence/references/real-mint05.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L012 - P2 - Ctrl-M inserts m instead of a searchable Soft Return

Area: Search. Classification: Defect.

Replay: S03: type ten copies of "alpha " then "target phrase tail". Home Home Up, F2, target, Ctrl-M, phrase, F2.

Observed: The query is targetmphrase, not target[SRt]phrase, and fails. A literal-space query also failed in S01.

Expected and basis: WP Reference printed p.573 explicitly says Ctrl-M includes a Soft Return in a search string. See evidence/references/wp-search.txt.

Evidence: [shots/s03-search-srt-query.png](shots/s03-search-srt-query.png), [shots/s03-search-srt-result.png](shots/s03-search-srt-result.png), [shots/s01-search-wrap-fails.png](shots/s01-search-wrap-fails.png), [evidence/s01-serial.log](evidence/s01-serial.log), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: os/iword/app/face_a.c:2313 expects ch=13, but the guest delivers message 0000326D for Ctrl-M; control translation/dispatch mismatch (guess).

### L013 - P2 - Indent at the right edge produces a vertical column outside the margin

Area: Wrap / Indent. Classification: Defect.

Replay: S03: new document; type 66 x characters, F4, "ABCDEFGH IJKL MNOP". F11; save INDENT.WP5.

Observed: Letters appear one per line at approximately x=570, beyond the default right margin. The status reaches Pos 8.1 inches although the right margin position is 7.5 inches.

Expected and basis: The project promises paragraph wrapping at the indent (ref-i.tex:9-11) and lines ending at their margins (ref-w.tex:86-91). Expected refusal or a usable next-line indent is auditor judgment; no real-WP edge mint establishes the exact desired rule.

Evidence: [shots/s03-indent-edge-long.png](shots/s03-indent-edge-long.png), [shots/s03-indent-edge-reveal.png](shots/s03-indent-edge-reveal.png), [evidence/s03-extracted/INDENT.WP5](evidence/s03-extracted/INDENT.WP5), [evidence/s03-bytes.json](evidence/s03-bytes.json), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L014 - P2 - Tab at the long-word boundary disagrees between screen and saved coordinates

Area: Wrap / Tab. Classification: Defect.

Replay: S01: new document; type 66 x characters, Tab, Z. Save as part of EDGE.WP5.

Observed: Z displays at the next line left edge (Pos 1.1 after it), but the Tab payload stores 9600 WPU, 8 inches (C1 02 80 25 80 25 50 00 C1).

Expected and basis: spec/iword/wp5_format.h: tabs payload is the tab-stop position. The displayed and saved destination should agree (auditor consistency expectation); exact WP boundary behavior remains unminted.

Evidence: [shots/s01-66x-tab.png](shots/s01-66x-tab.png), [shots/s01-66x-reveal.png](shots/s01-66x-reveal.png), [evidence/s01-extracted/EDGE.WP5](evidence/s01-extracted/EDGE.WP5), [evidence/wp5-independent-decoder.txt](evidence/wp5-independent-decoder.txt), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L015 - P2 - Spaces in a filename are silently removed

Area: Filenames. Classification: Defect.

Replay: S01: F10; type BAD NAME.WP5; Enter.

Observed: The prompt silently becomes BADNAME.WP5 and that file is saved, without invalid-name feedback.

Expected and basis: Project contract pubs/manuals/initechword-reference/ref-f.tex:34: "a name with any other character". is refused, according to lines 34-37. Spaces are not among the allowed characters in lines 17-21.

Evidence: [shots/s01-space-name.png](shots/s01-space-name.png), [shots/s01-space-name-saved.png](shots/s01-space-name-saved.png), [evidence/s01-extracted/BADNAME.WP5](evidence/s01-extracted/BADNAME.WP5), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: os/iword/app/face_a.c:1725 accepts only ch > space, silently discarding spaces before validation (guess).

### L016 - P2 - Esc does not leave List Files as the manual promises

Area: Manual / List Files. Classification: Defect.

Replay: S01: F5 Enter Esc.

Observed: The directory stays open; F1/F7 work.

Expected and basis: Project contract pubs/manuals/initechword-reference/ref-l.tex:87: "F7 or Esc to leave the list". NOTE: the real WP mint LF03_16 also ignores Esc, so correcting the manual is a valid fix.

Evidence: [shots/s01-list-root.png](shots/s01-list-root.png), [shots/s01-list-esc.png](shots/s01-list-esc.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L017 - P2 - F1 does not cancel the Look menu

Area: Cancel / Look. Classification: Defect.

Replay: S03: Look at TEXTLONG.TXT; press F1.

Observed: The Look screen remains. Esc exits.

Expected and basis: Project contract pubs/manuals/initechword-reference/ref-c.tex:12: "Cancel backs out of a menu, prompt or question without doing anything". Lines 13-14 say it can be used whenever the status line shows a menu; Look has a menu.

Evidence: [shots/s03-look-cancel-f1.png](shots/s03-look-cancel-f1.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L018 - P2 - Text-file Look cannot scroll past its first 20 lines

Area: Look. Classification: Defect.

Replay: S03: F5 Enter; select TEXTLONG.TXT (Down 17, Right), 6. Down 25 times; PgDn.

Observed: Only Text line 001 through 020 remain visible; lines 021-080 cannot be read. The same Down test scrolls a WP file successfully.

Expected and basis: Look should allow reading the selected file. Project ref-l.tex:85-87 offers Look to read a document; real WP Reference Look, printed p.363 (PDF p.381), permits cursor movement.

Evidence: [shots/s03-look-text-top.png](shots/s03-look-text-top.png), [shots/s03-look-text-stuck.png](shots/s03-look-text-stuck.png), [shots/s03-look-wp-scroll.png](shots/s03-look-wp-scroll.png), [evidence/stress-fixtures/TEXTLONG.TXT](evidence/stress-fixtures/TEXTLONG.TXT), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: os/iword/app/face_a.c:1504 gates scrolling on g_look_wp; :1614 always starts text rendering at byte zero (guess).

### L019 - P2 - Next Doc and Prev Doc are inert enabled choices

Area: Look. Classification: Defect.

Replay: S03: late in session F5 Enter Down x6, 6 opens CROSS.WP5; press 1, then 2.

Observed: Both keep CROSS.WP5 on screen, although COPY.WP5 and other files exist. The menu offers 1 Next Doc; 2 Prev Doc in normal bright text.

Expected and basis: The actual WP Look menu in spec/iword/listfiles.h:154-158 names document navigation. Either implement these choices or accurately disable them.

Evidence: [shots/s03-look-long-before-next.png](shots/s03-look-long-before-next.png), [shots/s03-look-long-next.png](shots/s03-look-long-next.png), [shots/s03-look-long-prev.png](shots/s03-look-long-prev.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: os/iword/app/face_a.c:1502 M_LOOK does not dispatch 1 or 2 (guess).

### L020 - P2 - Directory prompt ignores typed paths

Area: List Files. Classification: Documented feature gap.

Replay: S01: F5; type APPS.

Observed: Dir A:\*.* remains unchanged and Enter lists the root.

Expected and basis: The editable-looking path should accept a directory; real WP List Files, Reference PDF pp.364-370. This is an acknowledged source limitation (face_a.c:1347-1350).

Evidence: [shots/s01-list-path-typing.png](shots/s01-list-path-typing.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L021 - P3 - Change-default-directory hint advertises a dead key

Area: List Files. Classification: Defect.

Replay: S01: F5; press =.

Observed: The hint "Type = to change default Dir" remains, but no default-directory prompt opens.

Expected and basis: The displayed instruction itself promises an action.

Evidence: [shots/s01-list-default-dir.png](shots/s01-list-default-dir.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L022 - P2 - Clicking document text does not position the insertion point

Area: Mouse. Classification: Defect.

Replay: S01: retrieve LONG.WP5; Home Down puts cursor on line 23. Click (85,70), in line 1; type MOUSE.

Observed: MOUSE is inserted at line 23, not the clicked line.

Expected and basis: WP Reference, Mouse Support, and ordinary native document interaction; location selection is expected from memory as well.

Evidence: [shots/s01-home-down.png](shots/s01-home-down.png), [shots/s01-mouse-hit.png](shots/s01-mouse-hit.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: os/iword/app/face_a.c:2412 dispatches keys/menu/update/close but not content mouse positioning (guess).

### L023 - P2 - Visible scroll arrows do not scroll the document

Area: Window. Classification: Defect.

Replay: S01: LONG.WP5 at first screen; click down arrow (627,451) three times.

Observed: No movement. Serial logs FLAIR-SCROLL-IGNORED win 0 v each time.

Expected and basis: An enabled-looking scrollbar should scroll its document (classic Mac convention, memory). This is Word with no scroll behavior, not the earlier Finder post-resize J01 case.

Evidence: [shots/s01-home-down.png](shots/s01-home-down.png), [shots/s01-scroll-arrow.png](shots/s01-scroll-arrow.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L024 - P2 - Page Up and Page Down do nothing

Area: Navigation. Classification: Defect.

Replay: S01: retrieve LONG.WP5; PgDn at top. Home Home Down; PgUp at end.

Observed: Both leave cursor and viewport unchanged. Home Home Up/Down do work.

Expected and basis: WP Reference printed p.104: PgDn/PgUp go to first line of next/previous page.

Evidence: [shots/s01-long-open.png](shots/s01-long-open.png), [shots/s01-pgdn.png](shots/s01-pgdn.png), [shots/s01-long-end.png](shots/s01-long-end.png), [shots/s01-pgup.png](shots/s01-pgup.png), [evidence/references/wp-cursor.txt](evidence/references/wp-cursor.txt), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L025 - P2 - Go To becomes ordinary document typing

Area: Navigation. Classification: Defect.

Replay: S01: retrieve LONG.WP5, Ctrl-Home, 6.

Observed: A literal 6 is inserted before Line 001; no Go To prompt/state is entered.

Expected and basis: WP Reference p.224: Ctrl-Home, page number, Enter navigates to a page.

Evidence: [shots/s01-goto-six.png](shots/s01-goto-six.png), [evidence/references/wp-goto.txt](evidence/references/wp-goto.txt), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L026 - P2 - Ctrl-Right moves a character instead of a word

Area: Navigation. Classification: Documented feature gap.

Replay: S01: type one two three; Home Home Up; Ctrl-Right.

Observed: Cursor advances to Pos 1.1, one character.

Expected and basis: WP Reference p.104 specifies Word Right; project ref-c.tex:241 explicitly documents its reduced modifier behavior.

Evidence: [shots/s01-ctrl-right.png](shots/s01-ctrl-right.png), [evidence/references/wp-cursor.txt](evidence/references/wp-cursor.txt), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L027 - P2 - Delete Word deletes only one character

Area: Deletion. Classification: Documented feature gap.

Replay: S03: type one two three; Home Home Up; Ctrl-Delete.

Observed: Only o is removed, leaving ne two three.

Expected and basis: WP Reference p.116: Ctrl-Del deletes the word at the cursor. Project ref-d.tex:57-59 documents character-only behavior.

Evidence: [shots/s03-delete-word.png](shots/s03-delete-word.png), [evidence/references/wp-delete.txt](evidence/references/wp-delete.txt), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L028 - P2 - Delete to End of Line only moves the cursor

Area: Deletion. Classification: Defect.

Replay: S03, after Ctrl-Delete above: Ctrl-End.

Observed: ne two three stays intact; cursor moves to its end.

Expected and basis: WP Reference p.116 defines Ctrl-End as deletion, not End navigation.

Evidence: [shots/s03-delete-end-line.png](shots/s03-delete-end-line.png), [evidence/references/wp-delete.txt](evidence/references/wp-delete.txt), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L029 - P2 - Delete to End of Page never asks or deletes

Area: Deletion. Classification: Defect.

Replay: S05: new document one two three; Home Home Up; Ctrl-PgDn.

Observed: The whole line remains with cursor at its start and no confirmation prompt.

Expected and basis: WP Reference p.116: Ctrl-PgDn then Y deletes to end of page.

Evidence: [shots/s05-delete-page-at-start.png](shots/s05-delete-page-at-start.png), [evidence/references/wp-delete.txt](evidence/references/wp-delete.txt), [evidence/s05-serial.log](evidence/s05-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L030 - P2 - Insert cannot switch to typeover

Area: Typing. Classification: Documented feature gap.

Replay: S03: after word-deletion test, Home Home Up, Insert, Z.

Observed: Z is inserted before ne two three; n is not overwritten.

Expected and basis: WP 5.1 Insert/typeover convention (memory); the project explicitly states "Ins key does nothing" in ref-i.tex:73-74.

Evidence: [shots/s03-typeover.png](shots/s03-typeover.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L031 - P2 - Repeat Value inserts its count into the document

Area: Keyboard. Classification: Defect.

Replay: S03: at position after Z, press Esc, 3, Right.

Observed: A literal 3 is inserted; Right moves only once.

Expected and basis: WP Reference p.105: Escape, number, movement key repeats cursor movement.

Evidence: [shots/s03-repeat-value.png](shots/s03-repeat-value.png), [evidence/references/wp-cursor.txt](evidence/references/wp-cursor.txt), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L032 - P2 - Enter cannot extend a block through the paragraph

Area: Block. Classification: Defect.

Replay: S03: new word word; Home Home Up, F12, Enter.

Observed: Block stays zero length at the first character.

Expected and basis: WP Reference p.50: Enter blocks to the next Hard Return.

Evidence: [shots/s03-block-enter-at-start.png](shots/s03-block-enter-at-start.png), [evidence/references/wp-block.txt](evidence/references/wp-block.txt), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L033 - P2 - A block cannot be saved as a file

Area: Block. Classification: Documented feature gap.

Replay: S03: type lower Case; Home Home Up, F12, End, F10.

Observed: The selected text and Block on status remain; the command never opens.

Expected and basis: WP Reference pp.49-51 explicitly permits Save on a block. Project ref-b.tex:50-60 acknowledges the limited command set.

Evidence: [shots/s03-block-save.png](shots/s03-block-save.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L034 - P2 - Case conversion is absent for selected text

Area: Block. Classification: Documented feature gap.

Replay: S03: type lower Case; Home Home Up, F12, End, Shift-F3.

Observed: The selected text and Block on status remain; the command never opens.

Expected and basis: WP Reference p.57: block, Switch, Uppercase/Lowercase. Project ref-b.tex:50-60 acknowledges the limited command set.

Evidence: [shots/s03-block-case.png](shots/s03-block-case.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L035 - P2 - Search cannot extend or contract a block

Area: Block. Classification: Documented feature gap.

Replay: S03: type lower Case; Home Home Up, F12, End, F2.

Observed: The selected text and Block on status remain; the command never opens.

Expected and basis: WP Reference p.573 describes searching with Block on. Project ref-b.tex:50-60 acknowledges the limited command set.

Evidence: [shots/s03-block-search.png](shots/s03-block-search.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L036 - P2 - Append returns without offering a file

Area: Block. Classification: Documented feature gap.

Replay: S01: block text; Ctrl-F4, 1, 4.

Observed: The Append menu vanishes; Block stays on.

Expected and basis: WP Reference Block p.49 includes Append. Project ref-m.tex:138-139 documents this omission.

Evidence: [shots/s01-block-move-menu.png](shots/s01-block-move-menu.png), [shots/s01-block-append.png](shots/s01-block-append.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L037 - P2 - Tabular Column movement is absent

Area: Block. Classification: Documented feature gap.

Replay: S05: block one two three; Ctrl-F4, 2.

Observed: The Move menu returns to the document without performing the chosen operation.

Expected and basis: Real WP Block/Move menu includes this operation; project ref-m.tex:138-139 explicitly lists it as unimplemented.

Evidence: [shots/s05-block-move-menu.png](shots/s05-block-move-menu.png), [shots/s05-block-tabular.png](shots/s05-block-tabular.png), [evidence/s05-serial.log](evidence/s05-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L038 - P2 - Rectangle movement is absent

Area: Block. Classification: Documented feature gap.

Replay: S05: block one two three; Ctrl-F4, 3.

Observed: The Move menu returns to the document without performing the chosen operation.

Expected and basis: Real WP Block/Move menu includes this operation; project ref-m.tex:138-139 explicitly lists it as unimplemented.

Evidence: [shots/s05-block-move-menu.png](shots/s05-block-move-menu.png), [shots/s05-block-rectangle.png](shots/s05-block-rectangle.png), [evidence/s05-serial.log](evidence/s05-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L039 - P2 - Home-Space inserts an ordinary breakable space

Area: Codes. Classification: Defect.

Replay: S03: new document; a, Home, Space, b; later save CHARS.WP5.

Observed: Reveal shows an ordinary blank. Saved body starts 61 20 62, not 61 A0 62.

Expected and basis: Real MINT05 was typed with Home-Space and stores A0; specs/file-formats/wp5-body-codes.md section 2.

Evidence: [shots/s03-hardspace-compose.png](shots/s03-hardspace-compose.png), [evidence/s03-extracted/CHARS.WP5](evidence/s03-extracted/CHARS.WP5), [evidence/wp5-independent-decoder.txt](evidence/wp5-independent-decoder.txt), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L040 - P2 - Compose inserts literal keystrokes instead of an extended character

Area: Codes. Classification: Defect.

Replay: S03: after a b, Ctrl-2, e, apostrophe. Save CHARS.WP5.

Observed: The document contains a b2e' (hex 61 20 62 32 65 27). No Compose prompt appears.

Expected and basis: Real mint/wpMINT05.steps uses Ctrl-2, e, apostrophe to create C0 29 01 C0 (accented e).

Evidence: [shots/s03-hardspace-compose.png](shots/s03-hardspace-compose.png), [evidence/s03-extracted/CHARS.WP5](evidence/s03-extracted/CHARS.WP5), [evidence/references/real-mint05.png](evidence/references/real-mint05.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L041 - P2 - Chosen measurement units do not affect measurements

Area: Setup. Classification: Documented feature gap.

Replay: S01: Shift-F1, 3, 8, 2, c; F1; type Inches?.

Observed: Setup says Centimeters, but the status still uses inch marks and values. S03 also cycles p/w/u/c/i for entry and status preferences.

Expected and basis: A selected unit should govern its labeled field. Project ref-s.tex:360-362 explicitly acknowledges that all measurements remain inches.

Evidence: [shots/s01-units-cm.png](shots/s01-units-cm.png), [shots/s01-units-still-inches.png](shots/s01-units-still-inches.png), [shots/s03-units-cycle.png](shots/s03-units-cycle.png), [evidence/s01-extracted/IWORD.SET](evidence/s01-extracted/IWORD.SET), [evidence/s01-serial.log](evidence/s01-serial.log), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L042 - P2 - Original Document Backup Yes never creates a backup

Area: Setup / Save. Classification: Documented feature gap.

Replay: S01: Shift-F1,3,1,2,Y,F1. Save Original version as BACKUP.WP5. Append changed; F10 Enter Y.

Observed: Setup shows Yes; the directory has only the new BACKUP.WP5, with no backup of Original version.

Expected and basis: The setting should protect the old version. Project ref-s.tex:314-315 explicitly admits the answer is recorded but no copy is kept.

Evidence: [shots/s01-backup-yes.png](shots/s01-backup-yes.png), [shots/s01-backup-replaced.png](shots/s01-backup-replaced.png), [evidence/s01-directory.txt](evidence/s01-directory.txt), [evidence/s01-extracted/BACKUP.WP5](evidence/s01-extracted/BACKUP.WP5), [evidence/s01-extracted/IWORD.SET](evidence/s01-extracted/IWORD.SET), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L043 - P2 - Timed backup is unavailable

Area: Setup. Classification: Documented feature gap.

Replay: S03: Shift-F1,3,1,1.

Observed: The gray Timed Document Backup choice does nothing.

Expected and basis: A dependable period word processor should offer timed recovery (memory); the omission is explicit in ref-s.tex:308-310.

Evidence: [shots/s03-timed-backup.png](shots/s03-timed-backup.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L044 - P2 - Base Font selection immediately returns to editing

Area: Font. Classification: Documented feature gap.

Replay: S01: Ctrl-F8, 4.

Observed: No selection screen or change appears.

Expected and basis: Real mint/shots_wpMINT28/25_base_font.png shows the screen. Project ref-f.tex:69-71 documents the omission.

Evidence: [shots/s01-font-menu.png](shots/s01-font-menu.png), [shots/s01-base-font.png](shots/s01-base-font.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L045 - P2 - Print Color selection immediately returns to editing

Area: Font. Classification: Documented feature gap.

Replay: S01: Ctrl-F8, 5.

Observed: No selection screen or change appears.

Expected and basis: Real mint/shots_wpMINT28/27_print_color.png shows the screen. Project ref-f.tex:69-71 documents the omission.

Evidence: [shots/s01-font-menu.png](shots/s01-font-menu.png), [shots/s01-print-color.png](shots/s01-print-color.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L046 - P3 - Unimplemented Font choices look enabled

Area: Menu polish. Classification: Defect.

Replay: S01: Ctrl-F8.

Observed: Base Font and Print Color are shown in normal white even though selecting them does nothing.

Expected and basis: Use the same gray convention already used for unavailable List Files and Setup choices (internal UI consistency expectation).

Evidence: [shots/s01-font-menu.png](shots/s01-font-menu.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L047 - P3 - Unavailable Format choices look enabled

Area: Menu polish. Classification: Defect.

Replay: S03: Shift-F8, then 1.

Observed: Page/Document/Other and the six inactive Line settings look like working white choices. Their numbers leave the menu unchanged.

Expected and basis: Inactive options should be gray as they are in List Files and Setup (internal consistency expectation).

Evidence: [shots/s03-format-main.png](shots/s03-format-main.png), [shots/s03-format-line-1.png](shots/s03-format-line-1.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L048 - P2 - Display setup is unavailable

Area: Setup. Classification: Documented feature gap.

Replay: S03: Shift-F1, 2.

Observed: The gray Display row does not open.

Expected and basis: The named WP setup capability is unavailable. Project ref-s.tex:295-297 documents the gray options; expectation is the requested full period-workstation feature set, not a claim that the project promised implementation.

Evidence: [shots/s03-setup-2.png](shots/s03-setup-2.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L049 - P2 - Initial Settings setup is unavailable

Area: Setup. Classification: Documented feature gap.

Replay: S03: Shift-F1, 4.

Observed: The gray Initial Settings row does not open.

Expected and basis: The named WP setup capability is unavailable. Project ref-s.tex:295-297 documents the gray options; expectation is the requested full period-workstation feature set, not a claim that the project promised implementation.

Evidence: [shots/s03-setup-4.png](shots/s03-setup-4.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L050 - P2 - Keyboard Layout setup is unavailable

Area: Setup. Classification: Documented feature gap.

Replay: S03: Shift-F1, 5.

Observed: The gray Keyboard Layout row does not open.

Expected and basis: The named WP setup capability is unavailable. Project ref-s.tex:295-297 documents the gray options; expectation is the requested full period-workstation feature set, not a claim that the project promised implementation.

Evidence: [shots/s03-setup-5.png](shots/s03-setup-5.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L051 - P2 - Location of Files setup is unavailable

Area: Setup. Classification: Documented feature gap.

Replay: S03: Shift-F1, 6.

Observed: The gray Location of Files row does not open.

Expected and basis: The named WP setup capability is unavailable. Project ref-s.tex:295-297 documents the gray options; expectation is the requested full period-workstation feature set, not a claim that the project promised implementation.

Evidence: [shots/s03-setup-6.png](shots/s03-setup-6.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L052 - P2 - Format Page group cannot be entered

Area: Format. Classification: Documented feature gap.

Replay: S03: Shift-F8, 2.

Observed: The Format menu remains; none of this group can be reached.

Expected and basis: Real mint/shots_wpMINT21/01_format_menu.png names the group. Project ref-f.tex:130-141 documents its absence. Child settings were not independently tested because this entry point is blocked.

Evidence: [shots/s03-format-main-2.png](shots/s03-format-main-2.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L053 - P2 - Format Document group cannot be entered

Area: Format. Classification: Documented feature gap.

Replay: S03: Shift-F8, 3.

Observed: The Format menu remains; none of this group can be reached.

Expected and basis: Real mint/shots_wpMINT21/01_format_menu.png names the group. Project ref-f.tex:130-141 documents its absence. Child settings were not independently tested because this entry point is blocked.

Evidence: [shots/s03-format-main-3.png](shots/s03-format-main-3.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L054 - P2 - Format Other group cannot be entered

Area: Format. Classification: Documented feature gap.

Replay: S03: Shift-F8, 4.

Observed: The Format menu remains; none of this group can be reached.

Expected and basis: Real mint/shots_wpMINT21/01_format_menu.png names the group. Project ref-f.tex:130-141 documents its absence. Child settings were not independently tested because this entry point is blocked.

Evidence: [shots/s03-format-main-4.png](shots/s03-format-main-4.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L055 - P2 - Hyphenation cannot be configured

Area: Format. Classification: Documented feature gap.

Replay: S03: Shift-F8,1,1.

Observed: The Line menu stays unchanged at its default value.

Expected and basis: Real mint/shots_wpMINT21/02_format_line.png shows this setting. Project ref-f.tex:135-141 acknowledges it is inactive.

Evidence: [shots/s03-format-line-1.png](shots/s03-format-line-1.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L056 - P2 - Hyphenation Zone cannot be configured

Area: Format. Classification: Documented feature gap.

Replay: S03: Shift-F8,1,2.

Observed: The Line menu stays unchanged at its default value.

Expected and basis: Real mint/shots_wpMINT21/02_format_line.png shows this setting. Project ref-f.tex:135-141 acknowledges it is inactive.

Evidence: [shots/s03-format-line-2.png](shots/s03-format-line-2.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L057 - P2 - Line Height cannot be configured

Area: Format. Classification: Documented feature gap.

Replay: S03: Shift-F8,1,4.

Observed: The Line menu stays unchanged at its default value.

Expected and basis: Real mint/shots_wpMINT21/02_format_line.png shows this setting. Project ref-f.tex:135-141 acknowledges it is inactive.

Evidence: [shots/s03-format-line-4.png](shots/s03-format-line-4.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L058 - P2 - Line Numbering cannot be configured

Area: Format. Classification: Documented feature gap.

Replay: S03: Shift-F8,1,5.

Observed: The Line menu stays unchanged at its default value.

Expected and basis: Real mint/shots_wpMINT21/02_format_line.png shows this setting. Project ref-f.tex:135-141 acknowledges it is inactive.

Evidence: [shots/s03-format-line-5.png](shots/s03-format-line-5.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L059 - P2 - Tab Set cannot be configured

Area: Format. Classification: Documented feature gap.

Replay: S03: Shift-F8,1,8.

Observed: The Line menu stays unchanged at its default value.

Expected and basis: Real mint/shots_wpMINT21/02_format_line.png shows this setting. Project ref-f.tex:135-141 acknowledges it is inactive.

Evidence: [shots/s03-format-line-8.png](shots/s03-format-line-8.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L060 - P2 - Widow/Orphan Protection cannot be configured

Area: Format. Classification: Documented feature gap.

Replay: S03: Shift-F8,1,9.

Observed: The Line menu stays unchanged at its default value.

Expected and basis: Real mint/shots_wpMINT21/02_format_line.png shows this setting. Project ref-f.tex:135-141 acknowledges it is inactive.

Evidence: [shots/s03-format-line-9.png](shots/s03-format-line-9.png), [evidence/s03-serial.log](evidence/s03-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L061 - P2 - Zero-inch margins are silently ignored

Area: Margins. Classification: Defect.

Replay: S01: Shift-F8,1,7; type 0 Enter for left, 0 Enter for right.

Observed: The menu still shows 1 inch for each margin, without explaining a refusal.

Expected and basis: Project contract pubs/manuals/initechword-reference/ref-m.tex:64: "Type a margin in inches". Lines 65-66 only require it to be less than 9 inches. Zero is a valid numeric input under that stated rule.

Evidence: [shots/s01-zero-margins.png](shots/s01-zero-margins.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: os/iword/app/face_a.c:1924 uses if(v), conflating zero with invalid input (guess).

### L062 - P2 - Help is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press f3.

Observed: Only the gray command name appears on the status line. No Help operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-help.png](shots/s01-key-help.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L063 - P2 - End Field is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press f9.

Observed: Only the gray command name appears on the status line. No End Field operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-end-field.png](shots/s01-key-end-field.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L064 - P2 - Switch is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press shift-f3.

Observed: Only the gray command name appears on the status line. No Switch operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-switch.png](shots/s01-key-switch.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L065 - P2 - reverse Indent is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press shift-f4.

Observed: Only the gray command name appears on the status line. No reverse Indent operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-left-indent.png](shots/s01-key-left-indent.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L066 - P2 - Date/Outline is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press shift-f5.

Observed: Only the gray command name appears on the status line. No Date/Outline operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-date.png](shots/s01-key-date.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L067 - P2 - Center is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press shift-f6.

Observed: Only the gray command name appears on the status line. No Center operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-center.png](shots/s01-key-center.png), [shots/s01-block-center.png](shots/s01-block-center.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L068 - P2 - Print is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press shift-f7.

Observed: Only the gray command name appears on the status line. No Print operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-print.png](shots/s01-key-print.png), [shots/s05-list-file-print.png](shots/s05-list-file-print.png), [evidence/s01-serial.log](evidence/s01-serial.log), [evidence/s05-serial.log](evidence/s05-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L069 - P2 - Merge Codes is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press shift-f9.

Observed: Only the gray command name appears on the status line. No Merge Codes operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-merge-codes.png](shots/s01-key-merge-codes.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L070 - P2 - Thesaurus is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press alt-f1.

Observed: Only the gray command name appears on the status line. No Thesaurus operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-thesaurus.png](shots/s01-key-thesaurus.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L071 - P2 - Mark Text is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press alt-f5.

Observed: Only the gray command name appears on the status line. No Mark Text operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-mark-text.png](shots/s01-key-mark-text.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L072 - P2 - Flush Right is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press alt-f6.

Observed: Only the gray command name appears on the status line. No Flush Right operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-flush-right.png](shots/s01-key-flush-right.png), [shots/s01-block-flush.png](shots/s01-block-flush.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L073 - P2 - Columns/Table is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press alt-f7.

Observed: Only the gray command name appears on the status line. No Columns/Table operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-columns-table.png](shots/s01-key-columns-table.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L074 - P2 - Style is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press alt-f8.

Observed: Only the gray command name appears on the status line. No Style operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-style.png](shots/s01-key-style.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L075 - P2 - Graphics is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press alt-f9.

Observed: Only the gray command name appears on the status line. No Graphics operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-graphics.png](shots/s01-key-graphics.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L076 - P2 - Macro playback is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press alt-f10.

Observed: Only the gray command name appears on the status line. No Macro playback operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-macro.png](shots/s01-key-macro.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L077 - P2 - Shell is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press ctrl-f1.

Observed: Only the gray command name appears on the status line. No Shell operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-shell.png](shots/s01-key-shell.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L078 - P2 - Spell is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press ctrl-f2.

Observed: Only the gray command name appears on the status line. No Spell operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-spell.png](shots/s01-key-spell.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L079 - P2 - Screen rewrite/split is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press ctrl-f3.

Observed: Only the gray command name appears on the status line. No Screen rewrite/split operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-screen.png](shots/s01-key-screen.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L080 - P2 - Text In/Out is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press ctrl-f5.

Observed: Only the gray command name appears on the status line. No Text In/Out operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-text-in-out.png](shots/s01-key-text-in-out.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L081 - P2 - Tab Align is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press ctrl-f6.

Observed: Only the gray command name appears on the status line. No Tab Align operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-tab-align.png](shots/s01-key-tab-align.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L082 - P2 - Footnote is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press ctrl-f7.

Observed: Only the gray command name appears on the status line. No Footnote operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-footnote.png](shots/s01-key-footnote.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L083 - P2 - Merge/Sort is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press ctrl-f9.

Observed: Only the gray command name appears on the status line. No Merge/Sort operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-merge-sort.png](shots/s01-key-merge-sort.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L084 - P2 - Macro Define is only a placeholder

Area: WP command. Classification: Documented feature gap.

Replay: S01: on a clear editing screen press ctrl-f10.

Observed: Only the gray command name appears on the status line. No Macro Define operation or menu opens.

Expected and basis: WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.

Evidence: [shots/s01-key-macro-define.png](shots/s01-key-macro-define.png), [evidence/s01-serial.log](evidence/s01-serial.log). The matching session serial log and primitive input stream are in evidence/.

Source hypothesis: spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).

### L085 - P2 - Delete is unavailable in the file list

Area: List Files. Classification: Documented feature gap.

Replay: S05: F5 Enter, Down three times, Right selects CODES.WP5; press 2.

Observed: The selected file and listing stay unchanged; no operation or prompt opens.

Expected and basis: Real WP List Files menu in spec/iword/listfiles.h:128-143 includes Delete. Project ref-l.tex:103-107 explicitly acknowledges it is disabled.

Evidence: [shots/s05-list-file-selected.png](shots/s05-list-file-selected.png), [shots/s05-list-file-delete.png](shots/s05-list-file-delete.png), [evidence/s05-file-ops.json](evidence/s05-file-ops.json), [evidence/s05-extracted/CODES.WP5](evidence/s05-extracted/CODES.WP5), [evidence/s05-serial.log](evidence/s05-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L086 - P2 - Move/Rename is unavailable in the file list

Area: List Files. Classification: Documented feature gap.

Replay: S05: F5 Enter, Down three times, Right selects CODES.WP5; press 3.

Observed: The selected file and listing stay unchanged; no operation or prompt opens.

Expected and basis: Real WP List Files menu in spec/iword/listfiles.h:128-143 includes Move/Rename. Project ref-l.tex:103-107 explicitly acknowledges it is disabled.

Evidence: [shots/s05-list-file-selected.png](shots/s05-list-file-selected.png), [shots/s05-list-file-rename.png](shots/s05-list-file-rename.png), [evidence/s05-file-ops.json](evidence/s05-file-ops.json), [evidence/s05-extracted/CODES.WP5](evidence/s05-extracted/CODES.WP5), [evidence/s05-serial.log](evidence/s05-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L087 - P2 - Short/Long Display is unavailable in the file list

Area: List Files. Classification: Documented feature gap.

Replay: S05: F5 Enter, Down three times, Right selects CODES.WP5; press 5.

Observed: The selected file and listing stay unchanged; no operation or prompt opens.

Expected and basis: Real WP List Files menu in spec/iword/listfiles.h:128-143 includes Short/Long Display. Project ref-l.tex:103-107 explicitly acknowledges it is disabled.

Evidence: [shots/s05-list-file-selected.png](shots/s05-list-file-selected.png), [shots/s05-list-file-display.png](shots/s05-list-file-display.png), [evidence/s05-file-ops.json](evidence/s05-file-ops.json), [evidence/s05-extracted/CODES.WP5](evidence/s05-extracted/CODES.WP5), [evidence/s05-serial.log](evidence/s05-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L088 - P2 - Other Directory is unavailable in the file list

Area: List Files. Classification: Documented feature gap.

Replay: S05: F5 Enter, Down three times, Right selects CODES.WP5; press 7.

Observed: The selected file and listing stay unchanged; no operation or prompt opens.

Expected and basis: Real WP List Files menu in spec/iword/listfiles.h:128-143 includes Other Directory. Project ref-l.tex:103-107 explicitly acknowledges it is disabled.

Evidence: [shots/s05-list-file-selected.png](shots/s05-list-file-selected.png), [shots/s05-list-file-other-dir.png](shots/s05-list-file-other-dir.png), [evidence/s05-file-ops.json](evidence/s05-file-ops.json), [evidence/s05-extracted/CODES.WP5](evidence/s05-extracted/CODES.WP5), [evidence/s05-serial.log](evidence/s05-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L089 - P2 - Copy is unavailable in the file list

Area: List Files. Classification: Documented feature gap.

Replay: S05: F5 Enter, Down three times, Right selects CODES.WP5; press 8.

Observed: The selected file and listing stay unchanged; no operation or prompt opens.

Expected and basis: Real WP List Files menu in spec/iword/listfiles.h:128-143 includes Copy. Project ref-l.tex:103-107 explicitly acknowledges it is disabled.

Evidence: [shots/s05-list-file-selected.png](shots/s05-list-file-selected.png), [shots/s05-list-file-copy.png](shots/s05-list-file-copy.png), [evidence/s05-file-ops.json](evidence/s05-file-ops.json), [evidence/s05-extracted/CODES.WP5](evidence/s05-extracted/CODES.WP5), [evidence/s05-serial.log](evidence/s05-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L090 - P2 - Find is unavailable in the file list

Area: List Files. Classification: Documented feature gap.

Replay: S05: F5 Enter, Down three times, Right selects CODES.WP5; press 9.

Observed: The selected file and listing stay unchanged; no operation or prompt opens.

Expected and basis: Real WP List Files menu in spec/iword/listfiles.h:128-143 includes Find. Project ref-l.tex:103-107 explicitly acknowledges it is disabled.

Evidence: [shots/s05-list-file-selected.png](shots/s05-list-file-selected.png), [shots/s05-list-file-find.png](shots/s05-list-file-find.png), [evidence/s05-file-ops.json](evidence/s05-file-ops.json), [evidence/s05-extracted/CODES.WP5](evidence/s05-extracted/CODES.WP5), [evidence/s05-serial.log](evidence/s05-serial.log). The matching session serial log and primitive input stream are in evidence/.

### L091 - P2 - Name Search is unavailable in the file list

Area: List Files. Classification: Documented feature gap.

Replay: S05: F5 Enter, Down three times, Right selects CODES.WP5; press n.

Observed: The selected file and listing stay unchanged; no operation or prompt opens.

Expected and basis: Real WP List Files menu in spec/iword/listfiles.h:128-143 includes Name Search. Project ref-l.tex:103-107 explicitly acknowledges it is disabled.

Evidence: [shots/s05-list-file-selected.png](shots/s05-list-file-selected.png), [shots/s05-list-file-name-search.png](shots/s05-list-file-name-search.png), [evidence/s05-file-ops.json](evidence/s05-file-ops.json), [evidence/s05-extracted/CODES.WP5](evidence/s05-extracted/CODES.WP5), [evidence/s05-serial.log](evidence/s05-serial.log). The matching session serial log and primitive input stream are in evidence/.

## 3. Still present / Now fixed

Still present: no earlier InitechWord IDs exist, as the brief states. The earlier audit
reports were searched for InitechWord/iword/WordPerfect and relevant desktop exit/scroll
entries were inspected. This report does not relabel old Finder J01 or tenant G03 as Word defects.

Now fixed: none claimed. This was the first Word drive. In particular, the observed Finder
shutdown refusal does not prove the older general shutdown F05 fixed or still present.

## 4. Coverage, passes and limits

Covered:

- Ordinary and rapid typing (45 ms between QMP characters), 66/67-character unbroken words,
  wrap, spaces, hard returns, line joins by Backspace/Delete, Tab and Indent at the edge.
  A 350-line document and a 65,536-byte document were retrieved and navigated.
- All 48 function-key slots in the shipped map, including six unmapped modified F11/F12
  slots, both Reveal and Block aliases, all seven Font sizes and nine appearances, Normal,
  nested/crossing attributes, empty pairs, code-deletion prompts and direct Reveal deletion.
- Forward/backward selection, bold/underline on blocks, copy/move/retrieve, deletion No/Yes,
  three undo levels, crossing an attribute and a hard page, cancel; the unsupported block
  operations have separate findings. Ctrl-F4 sentence selection was exercised. A few screenshot
  names say copy in S01 although the recorded sequence actually selected Move; the passing
  copy proof is S03 COPY.WP5, not those early names.
- Forward/backward text search, case matching, literal attribute search, code deletion by
  replacement, adjacent matches, empty replacement, confirmation Y/N and cancel, backwards
  replacement, not-found and Soft Return entry. Search with a literal space across a wrapped
  line failed; that alone is not claimed to differ from WP, which documents explicit SRt entry.
- Save/new name/replace No and Yes, invalid long and punctuation names, spaces, save on exit,
  F7 save/exit/name cancel, save-then-clear, save-then-exit, discard-and-exit, clean Ctrl-Q,
  dirty Ctrl-Q/close-box/File Exit, File Save/Retrieve, full-disk failures and read-only refusal.
- List Files root/subdirectories/parent, all offered operation keys, directory capacity,
  WP and text Look, Look arrows/next/previous/cancel, retrieval onto empty and occupied screens
  with No/Yes; real WP imports, empty/bad-header/absent/plain-text/oversize/code-only files.
- Setup's displayed choices, all five unit letters for both preference fields, backup Yes,
  settings persistence and full-disk rollback; Format's three parent groups and all nine Line
  choices, center/right justification, line spacing 2, margins and cancellation; window resize,
  document click, scrollbar, Finder shutdown with an unsaved document.

Byte-verified passes (evidence/byte-validation.json and wp5-independent-decoder.txt):

- ROUND01.WP5 is byte-for-byte identical to the real MINT01.WP5, all 495 bytes.
- ADJ.WP5 body is bbbb after replacing every a in aaaa; zero adjacent matches were skipped.
- BLOCK.WP5 has bold around AB and underline around DE. NEST.WP5 has the expected crossing
  bold/underline sequence. COPY.WP5 contains COPY ME.COPY ME. CROSS.WP5 keeps CD plain.
- CODEOUT.WP5 preserves a document containing only an empty bold pair.
- PAGES.WP5 restores its hard-page D4 group and 0C after block deletion/Undelete.
- MAXOUT.WP5 remains 65,536 bytes and differs from its input only in the permitted cursor
  flag at 0x103. Extra text and Bold were refused without damaging its contents.
- Read-only READONLY.WP5 survived the failed overwrite unchanged. Settings failed on the
  full disk with an error and retained Inches in memory. The failed new IWORD.SET is zero bytes.
- The independent corpus decoder found zero trailer errors in the 13 selected output files.
  This is structural validation, not proof of complete WP semantic or printer compatibility.

Limits / unreached:

- No actual printing, spell check, help topic, split-window operation, macro, table creation,
  date dialog, style/graphics editor, secondary document, mail merge, or inactive Format/Setup
  child dialog can be driven through the observed disabled entry points. No invented findings
  are assigned to unobserved child screens.
- No real WP executable was run during this session; comparisons use its supplied captures,
  minted documents, format notes and original manual. The long-word boundary behavior needs
  an additional real-WP mint before prescribing exact compatibility bytes.
- Not every numeric margin/spacing value, every possible block boundary or every possible
  keyboard chord was enumerated. Tests were repeated where useful, especially full-disk loss,
  directory no-ops, save flows and aliases; this is not a mathematical exhaustive audit.
- Several hundred lines were supplied as a generated WP fixture rather than typed by hand.
  No CPU panic or emulator hang was observed; no intentional host power-cut test was performed.
- The S02 screenshots without "real" in their initial full-disk attempt show the desktop:
  a startup disk-full alert consumed the first launch attempt. Those are not evidence of a
  Word save failure. s02-settings-full-error.png actually shows a successful setting change
  after the failed overwrite freed a cluster; the valid failed-setting evidence is S04.
- The <6 year display is a reproduced real-WP Y2K behavior (LISTF mint ledger lines 91-92),
  not a newly claimed defect. Blue editing background, gray adaptation bands, the two menu
  bars, and the other deliberate brief exceptions are not reported as defects. The original
  WP and the project's ref-s.tex:91 specify blue, despite the brief's shorthand "black".
- Some unsupported attributes draw as ordinary text, as the manual says. All sixteen code
  pairs were exercised and saved; no unverified assertion about their exact historical screen
  colors or printer output is made here. Imported header/footer text was not judged against
  ordinary editing view because that view may intentionally omit it.

## 5. Fifteen fixes to prioritize

1. L001: Preserve the previous version with a transactional save.
2. L068: Make an office document printable from the workstation.
3. L006: Accept ordinary multi-page WordPerfect files.
4. L003: Keep every prompt visible in supported window sizes.
5. L005: Connect the application save guard to Finder shutdown.
6. L042: Make the advertised backup protect user work.
7. L004: Enumerate complete directories or clearly report limits.
8. L007: Preserve readable structure when importing tables.
9. L062: Provide usable in-application help and key discovery.
10. L024: Restore the expected navigation speed for long documents.
11. L025: Stop navigation commands from altering the document.
12. L012: Make searches across layout boundaries usable.
13. L013: Give margin-edge indent behavior a real-WP oracle and implement it.
14. L015: Reject invalid names without silently changing the requested destination.
15. L010: Render common accented text before expanding exotic formatting.

All retained screenshots, serial logs, extracted bytes, preparation scripts, replay scripts and verification results are local to this audit directory.
