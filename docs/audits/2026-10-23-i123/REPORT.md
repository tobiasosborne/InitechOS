# Initech 123 audit - 2026-10-23

Scope: APPS\123.EXE and its interactions with Finder, copied FAT volumes, and
real Lotus 1-2-3 Release 2.2 worksheet files. Image/source commit:
`e67b8311092873fe16e6c6d62ad82f894bfb8d7d` (images/COMMIT and read-only
repository HEAD agreed). No repository edits, builds, make, tracker actions,
or input/capture on the owner's X display were performed.

The two P1 findings are actual data loss: a failed full-disk replacement
zeroes the previous saved worksheet; an unchanged F2/Return edit truncates a
240-character label. The large finding count includes 224 individually
exercised menu leaves which visibly refuse with "Command not implemented".
Those are acknowledged implementation gaps, not claims of 224 new regressions.
Other findings cover implemented behavior, formula results, storage recovery,
keyboard handling, shutdown integration, and presentation.

All new L-numbers below belong to THIS report. Identical-looking L-numbers
from 2026-10-05 are explicitly qualified in the retest tables at the end.
Severity: P1 data loss/corruption/crash/hang/blocked core task; P2 broken or
missing behavior; P3 presentation or polish. No guest panic or confirmed
hang was observed. One host-runner interruption is documented separately;
it is not counted as a guest defect.

## How to replay and read the evidence

The standard launch used copied `flair_tenants_interactive.img` plus copied
`flair_data.img`, QEMU `-cpu 486 -m 16 -display none -qmp stdio`, with serial
output to a file. Unless a finding specifies otherwise, start in READY after
these double-clicks in guest pixels: disk (600,64), APPS (123,101), 123.EXE
(211,124). The driver moves the pointer clear between opening disk and APPS.
A fresh-sheet replay does not depend on any files from other findings.

`F5 A1 Return` means press F5, type A1, and press Return. Slash paths use the
period product's initial-letter commands without Return between menu letters;
Return accepts typed arguments. The menu inventory additionally lists exact
Home/Right-count/Return sequences, so every deep or punctuation item can be
reached without guessing initials. Coordinates are measured from the guest's
upper-left corner. Screenshots are unmodified QMP captures converted to PNG;
contact sheets and comparison crops live in evidence/, not shots/.

Replay in a copy of this audit directory to retain the original captures:

1. `python3 evidence/prepare_fixtures.py base`
2. `python3 evidence/phases.py long`
3. `python3 evidence/prepare_fixtures.py full`
4. Run individual phases with `python3 evidence/phases.py PHASE`.
   Available phases: menus, hotkeys, files, capacity, transfers, functions,
   function_errors, formats, formats_wide, full2, full_repeat, keyslook,
   quits, power, typeerrors, pool, edge_more, heavy, operations.
   function_errors uses the stopped functions-data.img; the other ordinary
   phases start from fixture.img. full2/full_repeat use full-fixture2.img.
   Each phase starts one emulator and stops it in a finally block.
5. After a phase stops: `python3 evidence/extract.py PHASE` extracts and
   independently parses WK1 records. `python3 evidence/replay_entry.py`
   reproduces the valid entry/edit cases from the first interactive session.

The original supplied driver is copied as evidence/audit_driver_original.py;
evidence/drive123.py extends it. evidence/actions.jsonl records actual
keystrokes, coordinates, timing, session, and serial byte offsets. It contains
both high-level operations and their constituent events, so it is an audit
log, not a sequence to execute naively twice. Phase scripts are the executable
replays. Each *-session.json records image hashes and the exact QEMU command.

Menu verification: evidence/menu-plan.json, menu-results.json, menu-verified.json,
and hotkey-verified.json cover 381 entries in 82 menus. All 300 leaves were
reached: 299 were additionally invoked with their unique first-letter key;
Quit/Yes was exercised separately in the quit matrix. Every selected third-line
render was compared with the installed real 123.RI bytes at named offsets.
Every cited unavailable-command screenshot was checked against the inspected
pixel-identical ERROR panel. Three descriptions are clipped, reported below.

Byte evidence: evidence/extracted/ contains stopped-image mtools extractions,
directory listings, original WK1 bytes, record hex, cell types, values, formula
bytecode, and string records. extract.py is an independent Python record parser,
not a call into the Initech engine. Positive infinity is the WK1 ERR sentinel;
negative infinity is NA; the string-result NaN sentinel has a following STRING
record and is not by itself a numerical corruption. The C:TEST save was also
extracted from the boot partition at byte offset 32256 into
`evidence/extracted/files-boot/`.

## Reference basis and limitations of the comparison

- "Lotus Reference" means
  `/home/tobias/Projects/initech-manuals-ref/lotus123/Lotus 1-2-3 2.2 Manuals (1989)/Lotus 1-2-3 Release 2.2 - Reference Manual.pdf`.
  Both printed and PDF page numbers are supplied. Rendered pages and OCR
  transcriptions are in evidence/references/manual-pdf-NNN.*; the image is
  authoritative when OCR loses a symbol or exponent.
- Real files/screens/commands are under
  `/home/tobias/Projects/lotus123-decomp/goldens/minted/` and `mint/`.
  The original installed menu resource is
  `goldens/lotus-123-2.2-installed-ref/extracted/installed/123.RI`.
  Its SHA256 is saved in evidence/references/123.RI.sha256. Menu reference
  offsets were independently read from that binary, not merely trusted from
  the native menu source.
- Mac alert comparisons use
  `/home/tobias/Projects/system7-decomp/goldens/captures/s8_alert_modal.png`
  and the previous look audit's measured regions. Native white/navy coloring,
  the two menu bars, and the resulting 19-row viewport are accepted adaptations.
- Project manuals under pubs/manuals were searched. No Initech 123 user manual
  or operational promise for these spreadsheet commands was found; the
  InitechWord frontmatter merely names Initech 123. No shipped-manual violation
  is invented. The named Lotus manual supplies the period-product expectations.
- Where a rule is an auditor usability/data-integrity expectation rather than
  a named reference, the finding explicitly says so. Source explanations are
  guesses and are not substitutes for observed screen/byte evidence. Source
  snapshots for the main guesses are in evidence/source-context/.


## New findings

### L001 - Failed replacement on a full disk destroys the previously saved worksheet

Area: Files/Save atomicity. Severity: P1.

Steps: Boot full-fixture2.img (created from the stopped long image with existing Finder metadata; FULL.WK1 is real MINT01.WK1; FILLER.BIN consumes every free cluster). Retrieve FULL. Change A1 to replacement; enter 220 x characters plus the row number in A2 through A13. /File Save NEWFULL fails. Escape; /File Save FULL Return; choose Replace. Escape; /File Retrieve FULL Return.

Observed: Replace reports File I/O error. The original 1609-byte FULL.WK1 becomes a zero-byte file. Retrieve then says Not a valid WK1 file. NEWFULL is removed after its failure. The in-memory sheet remains dirty and Quit asks about saving. A second trial replacing the same original file after entering three 240-character labels also leaves a zero-byte FULL.WK1.

Expected: A failed save must preserve the last successful file, leaving the edited worksheet open for recovery.

Basis: Own data-integrity expectation. Lotus Reference p. 5-22 defines Replace as writing the current worksheet over the old file; an I/O failure must not silently destroy the only saved copy.

Guess: os/i123/app/face_a.c:646 calls tbx_file_create on the existing path before writes succeed; lines 657-659 do not roll back a replacement on failure.

Evidence: [shots/full2-full-replace-failed.png](shots/full2-full-replace-failed.png), [shots/full2-full-retrieve-after-fail.png](shots/full2-full-retrieve-after-fail.png), [shots/full2-full-quit-protection.png](shots/full2-full-quit-protection.png), [shots/repeat-full-replace.png](shots/repeat-full-replace.png), [evidence/full2-serial.log](evidence/full2-serial.log), [evidence/full2-before.txt](evidence/full2-before.txt), [evidence/extracted/full2/FULL.WK1](evidence/extracted/full2/FULL.WK1), [evidence/extracted/full2/directory.txt](evidence/extracted/full2/directory.txt), [evidence/full-replace-byte-proof.json](evidence/full-replace-byte-proof.json), [evidence/extracted/full_repeat/FULL.WK1](evidence/extracted/full_repeat/FULL.WK1), [evidence/full_repeat-serial.log](evidence/full_repeat-serial.log).

### L002 - F2 followed by Return truncates a full-length label

Area: Entry/edit. Severity: P1.

Steps: At A1 type 239 lowercase x characters followed by Z (240 characters), Return. /File Save LONG0. Press F2, Return without changing anything. /File Save LONGEDIT.

Observed: LONG0.WK1 stores a label prefix plus all 240 characters. LONGEDIT.WK1 stores the prefix plus only 239 x characters; the final Z is gone. A no-op edit changes the data.

Expected: Preserve the complete accepted label through an unmodified edit.

Basis: Lotus R2.2 Reference pp. 1-16, 1-17, 1-22 (PDF 32, 33, 38); byte equality is the audit data-integrity expectation.

Guess: os/i123/app/face_a.c:1064 passes the 241-byte g_edit buffer to i123_cell_content; os/i123/core/fmt.c:614 copies the stored prefix into that same capacity, leaving room for only 239 label characters.

Evidence: [shots/long-edit-data.png](shots/long-edit-data.png), [evidence/files-serial.log](evidence/files-serial.log), [evidence/extracted/files/LONG0.WK1](evidence/extracted/files/LONG0.WK1), [evidence/extracted/files/LONGEDIT.WK1](evidence/extracted/files/LONGEDIT.WK1), [evidence/long-edit-byte-proof.json](evidence/long-edit-byte-proof.json).

### L003 - Shutdown Save closes 123 but does not finish shutting down

Area: Quit/Finder integration. Severity: P2.

Steps: Make an unsaved worksheet. Shrink 123 by dragging (632,472) by (-288,-194). Activate the exposed APPS title at (380,90). Choose Finder Special > Shut Down with menu gesture (170,30) to (170,125). In the 123 save question choose Save (Return). Activate the worksheet title (150,50), enter SD0, Return. Repeat on a fresh app with SD1.

Observed: Both saves succeed and 123 exits, but the desktop remains running. The serial log says FINDER-POWER-CANCEL app=123.EXE declined before filename entry completes. No safe-shutdown marker occurs in either Save trial.

Expected: After the Save flow succeeds, resume the pending shutdown. Cancel should cancel it, as it does in the separate Cancel trials.

Basis: Own native application shutdown workflow expectation; os/milton/kmain.c:5690 documents that shutdown asks each app through its normal save/quit routine and only user cancellation cancels shutdown, citing Inside Macintosh Vol VI PDF 345.

Guess: os/milton/kmain.c:5719 checks whether the application is still resident immediately after the alert callback. Save has only started an asynchronous filename prompt, so the resident app is misclassified as a cancellation.

Evidence: [shots/power-save-0-prompt.png](shots/power-save-0-prompt.png), [shots/power-save-0-result.png](shots/power-save-0-result.png), [shots/power-save-1-result.png](shots/power-save-1-result.png), [evidence/power-serial.log](evidence/power-serial.log).

### L004 - A leading dollar sign is rejected in numeric entry

Area: Entry/numbers. Severity: P2.

Steps: F5, B4, Return; type $12.34, Return.

Observed: The cell stays blank and the rejected $12.34 remains in EDIT.

Expected: Accept numeric 12.34; the dollar sign need not display until a currency format is chosen.

Basis: Lotus Reference p. 1-19 (PDF 35), Numbers: "If you begin a number with a $ (dollar sign), 1-2-3 will enter the number".

Guess: os/i123/core/fmt.c:638 accepts a sign but does not consume a leading dollar sign.

Evidence: [shots/011-currency.png](shots/011-currency.png), [evidence/entry-serial.log](evidence/entry-serial.log).

### L005 - F2 cannot move the insertion point into existing text

Area: Entry/edit. Severity: P2.

Steps: Enter abcdef in A6. F2, Left, X; then Home, Y.

Observed: The edit buffer becomes apostrophe-abcdefX and then apostrophe-abcdefXY. Both keys leave insertion at the end.

Expected: Left moves one character left and Home moves to the first character, allowing edits inside the entry.

Basis: Lotus Reference pp. 1-22/1-23 (PDF 38/39), editing-key table.

Guess: os/i123/app/face_a.c:1106 returns for EDIT Left/Right/Tab; no edit-cursor index is maintained.

Evidence: [shots/017-edit-left.png](shots/017-edit-left.png), [shots/018-edit-home.png](shots/018-edit-home.png), [evidence/entry-serial.log](evidence/entry-serial.log).

### L006 - Copy refuses a multi-cell destination range

Area: Worksheet/Copy. Severity: P2.

Steps: Populate A1 with 1. /Copy; FROM A1, Return; TO D1..D3, Return.

Observed: ERROR says "Specify the upper left destination cell"; the target is not filled.

Expected: Copy a single source cell over the specified destination range.

Basis: Lotus Reference chapter 3, /Copy pp. 3-4/3-5; local spec/i123/era_shortfalls.md I123-P3-07 also records the missing tiling behavior.

Guess: os/i123/app/face_a.c:742 explicitly rejects destinations with different corners.

Evidence: [shots/copy-target-range-refused.png](shots/copy-target-range-refused.png), [evidence/transfers-serial.log](evidence/transfers-serial.log).

### L007 - Period does not anchor an interactively pointed range

Area: Range picking. Severity: P2.

Steps: F5 A1 Return; /Copy; press period, Down, Right, Return at the FROM prompt.

Observed: The pointer reaches B2 but the prompt contains a literal period. Return produces Invalid range.

Expected: Period anchors the starting corner so arrows extend a range; Escape or Backspace removes the anchor.

Basis: Lotus Reference chapter 1, Specifying a Range (PDF 40/41); this is the period anchor operation called out in the brief.

Guess: os/i123/app/face_a.c:873 treats period as ordinary prompt text and arrows only move the cell cursor.

Evidence: [shots/point-dot-range.png](shots/point-dot-range.png), [shots/point-dot-result.png](shots/point-dot-result.png), [evidence/transfers-serial.log](evidence/transfers-serial.log).

### L008 - A permitted 240-character column width makes its cell disappear

Area: Worksheet/width. Severity: P2.

Steps: Put 1 in A1. At A1 choose /Worksheet Column Set-Width, enter 240. Then press Right.

Observed: At A1 the A heading, value, and cell pointer disappear; the worksheet body is blank. Right reveals B and later columns again.

Expected: Show the visible portion of the wide active column and its pointer, or refuse a width the viewport cannot render.

Basis: Own native-spreadsheet usability expectation; the application explicitly accepts widths 1..240.

Guess: os/i123/app/face_a.c:341 uses whole-column i123_cols_fitting; col_x:351 then rejects the active column when none fits.

Evidence: [shots/width-240.png](shots/width-240.png), [shots/width-240-right.png](shots/width-240-right.png), [evidence/transfers-serial.log](evidence/transfers-serial.log).

### L009 - Range names cannot be redefined while formulas exist

Area: Range names. Severity: P2.

Steps: Put 5 in B1. /Range Name Create rates Return B1 Return. In C1 enter +rates*2. Repeat Name Create rates with range B2.

Observed: The redefinition fails with the nonspecific "Command refused". The original binding remains B1.

Expected: Permit a range name to be reassigned through Name Create without refusing the operation merely because formulas exist.

Basis: Lotus Reference chapter 10, /Range Name Create pp. 10-14..10-18; restriction independently listed in spec/i123/era_shortfalls.md I123-P3-06.

Evidence: [shots/name-redefine.png](shots/name-redefine.png), [evidence/transfers-serial.log](evidence/transfers-serial.log).

### L010 - Erasing an ordinary cell does not recover capacity

Area: Capacity/recovery. Severity: P2.

Steps: Retrieve supplied CAP2047.WK1. Enter 8 in A2048. At capacity, /Range Erase A1 Return; then enter 9 in A2049.

Observed: A2049 still fails in EDIT. CAPERA.WK1 still has 2048 cell records, including a default-format BLANK at A1.

Expected: Erasing an unformatted cell should free a slot, or offer a reachable way to release it without erasing the entire sheet.

Basis: Own capacity-recovery expectation; distinct from earlier L080, which covers the unexplained full-store error itself.

Guess: os/i123/app/face_a.c:752 erases the cell and then recreates its default format as an allocated BLANK.

Evidence: [shots/capacity-after-erase.png](shots/capacity-after-erase.png), [evidence/capacity-serial.log](evidence/capacity-serial.log), [evidence/extracted/capacity/CAPERA.json](evidence/extracted/capacity/CAPERA.json), [evidence/extracted/capacity/CAPERA.WK1](evidence/extracted/capacity/CAPERA.WK1).

### L011 - @NOW returns ERR

Area: Formula/date-time. Severity: P2.

Steps: F5 A25 Return; type @NOW, Return. The function sweep then makes further edits, triggering automatic recalculation.

Observed: The formula evaluates to ERR. The saved result is positive infinity, the WK1 ERR sentinel; the desktop clock remains visible.

Expected: Return the current date-and-time serial number and update it on recalculation.

Basis: Lotus Reference p. 13-46 (PDF 334), @NOW.

Guess: os/i123/core/eval.c:916 returns ERR when have_now is unset; the native application does not supply a clock value.

Evidence: [shots/func-25.png](shots/func-25.png), [evidence/functions-serial.log](evidence/functions-serial.log), [evidence/extracted/functions/FUNCS.json](evidence/extracted/functions/FUNCS.json).

### L012 - Save Backup is offered but cannot create a backup

Area: Files/Save. Severity: P2.

Steps: Save A1=original as SAVE. Change A1 to changed. /File Save SAVE Return, choose Backup.

Observed: ERROR says Command not implemented. No SAVE.BAK is created.

Expected: Copy the previous SAVE.WK1 to SAVE.BAK and write the new worksheet to SAVE.WK1.

Basis: Lotus Reference p. 5-22 (PDF 154), Backup.

Guess: os/i123/app/face_a.c:780 routes the third replace-menu choice to unsupported().

Evidence: [shots/backup-refused.png](shots/backup-refused.png), [evidence/files-serial.log](evidence/files-serial.log), [evidence/extracted/files/directory.txt](evidence/extracted/files/directory.txt).

### L013 - The documented password-save syntax is rejected

Area: Files/password. Severity: P2.

Steps: Enter a value, then /File Save; type PASS p, Return.

Observed: ERROR says Invalid file name. No password prompt appears.

Expected: A space followed by p requests password entry and confirmation.

Basis: Lotus Reference pp. 5-22/5-23 (PDF 154/155), Saving a File with a Password.

Guess: os/i123/app/face_a.c:603 rejects every space before recognizing a password suffix.

Evidence: [shots/password-refused.png](shots/password-refused.png), [evidence/files-serial.log](evidence/files-serial.log).

### L014 - Invalid 8.3 names are diagnosed as generic I/O failures

Area: Files/errors. Severity: P3.

Steps: Attempt /File Save with TOOLONGNAME, then 12345678.1234. Clear each notice with Escape.

Observed: Both malformed DOS names produce File I/O error; the notice gives no name or length correction.

Expected: Identify invalid filename syntax or the 8.3 length limit, as the separate space/wildcard cases already identify invalid file names.

Basis: Own error-message usability expectation, supported by the contrasting observed Invalid file name response to A B and BAD?.

Evidence: [shots/filename-0.png](shots/filename-0.png), [shots/filename-5.png](shots/filename-5.png), [shots/filename-1.png](shots/filename-1.png), [evidence/files-serial.log](evidence/files-serial.log).

### L015 - Three menu descriptions are clipped at the right edge

Area: Menu/polish. Severity: P3.

Steps: Press slash, highlight Range (Right once). Also navigate /Worksheet Global Default Other International Punctuation and highlight B (,..) and D (,;.).

Observed: The Range description ends with Sear instead of Search. Each indicated punctuation description loses its last character. All other 378 displayed descriptions match their full reference text.

Expected: Keep the full selected-item description readable, for example by scrolling or wrapping it.

Basis: Real Lotus installed 123.RI strings at the offsets in evidence/menu-verified.json; exact screen crops y=94..109. Native geometry is four characters narrower than the 80-column reference.

Evidence: [shots/menu-143-selected.png](shots/menu-143-selected.png), [shots/menu-062-selected.png](shots/menu-062-selected.png), [shots/menu-064-selected.png](shots/menu-064-selected.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L016 - Named ranges are displayed as raw cell addresses in formulas

Area: Range names/display. Severity: P3.

Steps: Name B1 rates using /Range Name Create. Enter +rates*2 in C1 and inspect its control-panel contents before deleting the name.

Observed: C1 displays +B1*2 while rates is still defined.

Expected: Substitute a matching range name into the displayed formula.

Basis: Lotus Reference p. 10-15 (PDF 233): the first tip describes automatic substitution of an address by its range name.

Guess: name resolution is compiled to coordinates, but i123_cell_content/decompilation does not substitute the name table.

Evidence: [shots/name-redefine.png](shots/name-redefine.png), [evidence/transfers-serial.log](evidence/transfers-serial.log).

### L017 - A full disk is reported only as File I/O error

Area: Files/errors. Severity: P3.

Steps: On full-fixture2.img retrieve FULL, make the edits above, and attempt /File Save NEWFULL.

Observed: ERROR reports only File I/O error. It does not identify exhaustion of disk space.

Expected: Identify disk full so the user can free space or choose another volume.

Basis: Own error-message usability expectation. Finder already gives a specific disk-full alert on the uninitialized full fixture; the 123 file layer obscures the same cause.

Guess: os/i123/app/face_a.c:585 maps every file error except missing file/path to the same generic message.

Evidence: [shots/full2-full-new-refused.png](shots/full2-full-new-refused.png), [evidence/full2-serial.log](evidence/full2-serial.log), [evidence/full2-before.txt](evidence/full2-before.txt).

### L018 - Escape from GOTO enters the slash menu

Area: Navigation/GOTO. Severity: P2.

Steps: From READY press F5, then Escape.

Observed: The Worksheet/Range/... menu appears instead of READY.

Expected: Cancel the GOTO prompt and return to the unchanged worksheet.

Basis: Lotus Reference chapter 1 cancellation convention; own expectation that cancelling a prompt restores its caller.

Guess: os/i123/app/face_a.c:861 calls menu_mode for every range prompt, including the non-menu A_GOTO action.

Evidence: [shots/goto-escape-menu.png](shots/goto-escape-menu.png), [evidence/keyslook-serial.log](evidence/keyslook-serial.log).

### L019 - @CELL filename is blank even after saving

Area: Formula/file attributes. Severity: P2.

Steps: Save the fresh sheet as LOOK. In C2 enter @CELL("filename",A1).

Observed: The formula returns an empty string despite Saved LOOK.wk1 in the status line.

Expected: Return the current file name including its path.

Basis: Lotus Reference p. 13-17 (PDF 305), filename attribute.

Guess: os/i123/core/eval.c:854 unconditionally returns an empty string for filename.

Evidence: [shots/cell-filename-saved.png](shots/cell-filename-saved.png), [evidence/keyslook-serial.log](evidence/keyslook-serial.log).

### L020 - F3 cannot list defined names in a range prompt

Area: Range names/keys. Severity: P2.

Steps: Create name rates for B1. Press F5, then F3.

Observed: The POINT prompt remains; no list containing rates is displayed.

Expected: Display the named-range menu and allow selection.

Basis: Lotus Reference pp. 1-24 and 10-15 (PDF 40, 233) explicitly specify NAME (F3) in a range prompt.

Evidence: [shots/f3-named-range.png](shots/f3-named-range.png), [evidence/keyslook-serial.log](evidence/keyslook-serial.log).

### L021 - End plus an arrow does not perform a block jump

Area: Navigation/End. Severity: P2.

Steps: Populate consecutive A1..F1 cells as in keyslook. F5 A1 Return; End, Right.

Observed: No END indicator appears; Right moves just to B1.

Expected: End followed by Right moves to the end of the contiguous block, F1 in this fixture.

Basis: Lotus Reference pp. 1-9/1-10 (PDF 25/26), End movement; named real screen status reference PDF 22.

Evidence: [shots/end-indicator.png](shots/end-indicator.png), [shots/end-right.png](shots/end-right.png), [evidence/keyslook-serial.log](evidence/keyslook-serial.log).

### L022 - Caps Lock has no CAPS status indicator

Area: Status line. Severity: P3.

Steps: In READY press Caps Lock once.

Observed: No CAPS indicator appears along the worksheet status line.

Expected: Display CAPS while Caps Lock is enabled.

Basis: Lotus Reference p. 1-6 (PDF 22), status indicator table.

Evidence: [shots/caps-indicator.png](shots/caps-indicator.png), [evidence/keyslook-serial.log](evidence/keyslook-serial.log).

### L023 - Num Lock has no NUM status indicator

Area: Status line. Severity: P3.

Steps: In READY press Num Lock once.

Observed: No NUM indicator appears along the worksheet status line.

Expected: Display NUM while Num Lock is enabled.

Basis: Lotus Reference p. 1-5 figure and status indicator table, PDF 21/23.

Evidence: [shots/num-indicator.png](shots/num-indicator.png), [evidence/keyslook-serial.log](evidence/keyslook-serial.log).

### L024 - Alt-F4 does not undo the last cell change

Area: Recovery/Undo. Severity: P2.

Steps: Set A1 to a, then overwrite it with undo. Press Alt-F4.

Observed: A1 remains undo, with no UNDO indicator or restoration.

Expected: Restore the prior edit when the undo feature is enabled; provide the documented undo workflow.

Basis: Lotus Reference p. 1-5 (PDF 21) defines UNDO (Alt-F4); real MINT01 initial screen has UNDO enabled. The menu to enable Undo also refuses in this build.

Evidence: [shots/undo-absent.png](shots/undo-absent.png), [evidence/keyslook-serial.log](evidence/keyslook-serial.log).

### L025 - F4 does not toggle an entered reference to absolute

Area: Formula/keys. Severity: P2.

Steps: At B3 type +A1 without Return; press F4.

Observed: The entry remains +A1; no dollar signs appear.

Expected: Cycle the cell reference through absolute/mixed/relative forms.

Basis: Lotus Reference chapter 1 function-key description for ABS (F4), PDF 27.

Evidence: [shots/f4-absolute.png](shots/f4-absolute.png), [evidence/keyslook-serial.log](evidence/keyslook-serial.log).

### L026 - Return does not dismiss ordinary ERROR notices

Area: Errors/navigation. Severity: P2.

Steps: Choose /System to get Command not implemented; press Return.

Observed: The same ERROR notice remains. Escape does dismiss it.

Expected: Return and Escape should both clear an error notice.

Basis: Lotus Reference p. 1-5 (PDF 21): press ESC or ENTER to clear an error and continue.

Guess: os/i123/app/face_a.c:870 only lets Return clear M_NOTICE when g_error is set; ordinary notice() uses g_notice instead.

Evidence: [shots/unsupported-before-enter.png](shots/unsupported-before-enter.png), [shots/unsupported-after-enter.png](shots/unsupported-after-enter.png), [evidence/keyslook-serial.log](evidence/keyslook-serial.log).

### L027 - Native worksheet scroll controls do nothing

Area: Window/scrolling. Severity: P2.

Steps: F5 A100 Return. Click vertical up arrow (627,69). Drag the vertical track from (627,120) to (627,270).

Observed: The top row remains 100 and no scrolling occurs. The track has no useful thumb reflecting the viewport.

Expected: Make the native scroll controls move the worksheet view.

Basis: Own native-window usability expectation, as distinct from DOS Lotus keyboard navigation.

Evidence: [shots/scroll-before.png](shots/scroll-before.png), [shots/scroll-arrow.png](shots/scroll-arrow.png), [shots/scroll-thumb.png](shots/scroll-thumb.png), [evidence/keyslook-serial.log](evidence/keyslook-serial.log).

### L028 - Plain Scroll Lock cancels a pending cell entry

Area: Entry/keyboard routing. Severity: P2.

Steps: With an existing cell selected, type pending without Return, then press unmodified Scroll Lock.

Observed: READY returns and the pending text disappears, leaving the old cell value.

Expected: A plain lock key should not silently execute Ctrl-Break cancellation.

Basis: Own keyboard safety expectation; the app source labels this handler Ctrl-Break/Ctrl-C, so the unmodified key is outside its stated intent.

Guess: os/i123/app/face_a.c:1076 tests vk==0x46 without checking modifiers or the extended Break key identity.

Evidence: [shots/scroll-lock-cancels.png](shots/scroll-lock-cancels.png), [evidence/keyslook-serial.log](evidence/keyslook-serial.log).

### L029 - Retrieve provides no file-name list

Area: Files/browsing. Severity: P2.

Steps: Boot the fixture containing MINT01.WK1 and the other sample files. Choose /File Retrieve.

Observed: Only a text name prompt and Type a file name guidance are shown. The prompt has no selectable file list; File List itself is separately unavailable.

Expected: Show matching .WKS/.WK1 files in the current directory and permit file-name selection.

Basis: Lotus Reference p. 5-19 (PDF 151), File Retrieve procedure step 2; p. 1-6 FILES description (PDF 22).

Guess: os/i123/app/face_a.c:328-333 paints a typed-name-only prompt and the M_GET handler has no directory enumeration.

Evidence: [shots/menu-196-result.png](shots/menu-196-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L030 - High decimal counts display overflow stars despite ample column width

Area: Range Format/numeric display. Severity: P2.

Steps: Set A to width 30 with /Worksheet Column Set-Width. Enter 1234.5678912345 in A1. Apply Fixed decimals 12 then 13; Currency and Comma decimals 12 then 13; Percent decimals 10 then 11. The complete sweep also tries through 15.

Observed: The lower precision displays the number. Fixed/Currency/Comma 13..15 and Percent 11..15 produce 30 asterisks although their formatted text would fit within the 30-character column. Scientific format at 15 decimals still displays a number.

Expected: Display the selected supported decimal precision whenever the formatted number plus its reserved trailing character fits the column.

Basis: Lotus Reference pp. 10-4..10-6 (PDF 222..224) supports up to 15 decimal places and explains that widening the column removes width-related stars.

Guess: os/i123/core/fmt.c:245-246 scales the value by 10^decimals then refuses values >=9e15, independently of display width.

Evidence: [shots/wide-format-fixed-12.png](shots/wide-format-fixed-12.png), [shots/wide-format-fixed-13.png](shots/wide-format-fixed-13.png), [shots/wide-format-currency-13.png](shots/wide-format-currency-13.png), [shots/wide-format-comma-13.png](shots/wide-format-comma-13.png), [shots/wide-format-percent-11.png](shots/wide-format-percent-11.png), [shots/wide-format-sci-15.png](shots/wide-format-sci-15.png), [evidence/formats_wide-serial.log](evidence/formats_wide-serial.log), [evidence/rendered-case-lines.json](evidence/rendered-case-lines.json).

### L031 - The control panel omits the active cell format code

Area: Control panel/polish. Severity: P3.

Steps: At A1=1234.5678912345 apply /Range Format Fixed 0 to A1, then Currency 2. Inspect the first control-panel row.

Observed: It continues to show A1: 1234.5678912345 with no (F0) or (C2) descriptor, even though the cell display changes.

Expected: Show the selected format and decimal count next to the current-cell contents.

Basis: Lotus Reference p. 10-4 (PDF 222) explicitly describes the (C0)/(P2) control-panel notations.

Guess: os/i123/app/face_a.c:402-421 paints address and content but no format descriptor.

Evidence: [shots/wide-format-fixed-0.png](shots/wide-format-fixed-0.png), [shots/wide-format-currency-2.png](shots/wide-format-currency-2.png), [evidence/formats_wide-serial.log](evidence/formats_wide-serial.log).

### L032 - A same-length label replacement fails near the string-pool limit

Area: Capacity/edit allocation. Severity: P2.

Steps: Retrieve POOL135.WK1: A1..A135 each contain a 240-character label, using 32535 bytes including prefixes. Try to replace A1 with 240 y characters and Return. Cancel with Ctrl-Break, then replace A1 with one y.

Observed: The same-length replacement fails into unexplained EDIT; the existing label remains. A one-character replacement succeeds. The sheet has only 135 populated cells, far below its 2048-cell count limit.

Expected: Reuse the old label storage, or transact the replacement without requiring a second complete label allocation when the final live-data size is unchanged.

Basis: Own storage and edit correctness expectation; the local implementation documents a 32768-byte cell pool. The lack of a useful exhaustion message is already earlier L080, not renumbered here.

Guess: os/i123/core/sheet.c:250 allocates the new blob before the old blob is released at line 259.

Evidence: [shots/pool-replace-full.png](shots/pool-replace-full.png), [shots/pool-old-preserved.png](shots/pool-old-preserved.png), [shots/pool-small-replace.png](shots/pool-small-replace.png), [evidence/pool-serial.log](evidence/pool-serial.log), [evidence/POOL135.WK1](evidence/POOL135.WK1), [evidence/extracted/pool/POOLOUT.json](evidence/extracted/pool/POOLOUT.json).

### L033 - @STRING returns ERR for a valid high-precision conversion

Area: Formula/string conversion. Severity: P2.

Steps: Enter @STRING(1234.5678912345,15) in A1. As a control enter @STRING(0,15) in A2. Save EMPTY.

Observed: A1 is ERR; A2 produces the expected string with fifteen decimal zeros. The A1 saved result is the positive-infinity ERR sentinel.

Expected: Return the decimal text of the supplied number with fifteen decimal places.

Basis: Lotus Reference p. 13-56 (PDF 344) says x can be any value and n can be any integer 0..15.

Guess: os/i123/core/fmt.c:784-785 rejects the scaled value in i123_fixed_text; the @STRING evaluator then converts failure to ERR. This is a formula-result failure as well as the separately observed cell-format display failure.

Evidence: [shots/empty-extra-00.png](shots/empty-extra-00.png), [shots/empty-extra-01.png](shots/empty-extra-01.png), [evidence/edge_more-serial.log](evidence/edge_more-serial.log), [evidence/extracted/edge_more/EMPTY.json](evidence/extracted/edge_more/EMPTY.json).

### L034 - Ctrl-C appends a literal c to a pending entry

Area: Entry/keyboard routing. Severity: P2.

Steps: At D1=123.5, type pending without Return. Press Ctrl-C.

Observed: The input line becomes pendingc and remains in LABEL mode. The first entry session also reproduced this while in EDIT.

Expected: Honor the application cancellation handler for Ctrl-C, or at minimum do not insert its unmodified character.

Basis: Own modified-key expectation and the explicit Ctrl-C/Ctrl-Break cancellation intent in os/i123/app/face_a.c:1076; this is not claimed as a Lotus-manual Ctrl-C shortcut.

Guess: the event reaches the application as printable c plus modifier flags, while key() checks ch==3 and ignores mods for this shortcut.

Evidence: [shots/ctrl-c-literal.png](shots/ctrl-c-literal.png), [evidence/edge_more-serial.log](evidence/edge_more-serial.log), [evidence/entry-serial.log](evidence/entry-serial.log).

### L035 - Move leaves WK1 per-column metadata unlike the real-Lotus result

Area: Files/Move metadata. Severity: P2.

Steps: Retrieve real MOVE01A. Move A1 to D1 and save MOVEB. Move B2 to F5 and save MOVEC. Move A2..C2 to A8 and save MOVED. Compare record 0x0096 with real MOVE01B/C/D from the same recorded operations.

Observed: All cell records match the real files byte-for-byte. Record 0x0096 does not. MOVED retains the A-column triple (0,0,1), while real MOVE01D records (0,7,7); A8 is the remaining A-column value. MOVEC also retains the old B-column last row. Screen values remain correct.

Expected: Update the saved per-column metadata consistently with the move, as the command-minted real-Lotus files do.

Basis: Named originals goldens/minted/MOVE01B.WK1, MOVE01C.WK1, MOVE01D.WK1 and mint/MOVE01.steps. lotus123-decomp/specs/file-formats/wk1-format.md section 3.6 verifies the triples but labels their extent purpose inferred. Native-file loading by real Lotus was not performed, so no resulting compatibility failure or data loss is claimed.

Guess: the transfer erases/moves cells without rebuilding hid_first/hid_last; os/i123/fs/wk1.c:326-334 writes those stored arrays unchanged.

Evidence: [shots/MOVED.png](shots/MOVED.png), [evidence/transfers-serial.log](evidence/transfers-serial.log), [evidence/move-metadata-comparison.json](evidence/move-metadata-comparison.json), [evidence/extracted/transfers/MOVED.WK1](evidence/extracted/transfers/MOVED.WK1).

### L036 - /Worksheet/Global/Format/Date/4 (Long Intn'l) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Format/Date/4 (Long Intn'l). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, (no Right), Return, Home, Right x7, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Currently configured: MM/DD/YY".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0794, description at 0x07a4; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-014-selected.png](shots/menu-014-selected.png), [shots/menu-014-result.png](shots/menu-014-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L037 - /Worksheet/Global/Format/Date/5 (Short Intn'l) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Format/Date/5 (Short Intn'l). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, (no Right), Return, Home, Right x7, Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Currently configured: MM/DD".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0820, description at 0x0831; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-015-selected.png](shots/menu-015-selected.png), [shots/menu-015-result.png](shots/menu-015-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L038 - /Worksheet/Global/Format/Date/Time/1 (HH:MM:SS AM/PM) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Format/Date/Time/1 (HH:MM:SS AM/PM). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, (no Right), Return, Home, Right x7, Return, Home, Right x5, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x08b3, description at 0x08c5; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-017-selected.png](shots/menu-017-selected.png), [shots/menu-017-result.png](shots/menu-017-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L039 - /Worksheet/Global/Format/Date/Time/2 (HH:MM AM/PM) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Format/Date/Time/2 (HH:MM AM/PM). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, (no Right), Return, Home, Right x7, Return, Home, Right x5, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x08c6, description at 0x08d5; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-018-selected.png](shots/menu-018-selected.png), [shots/menu-018-result.png](shots/menu-018-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L040 - /Worksheet/Global/Format/Date/Time/3 (Long Intn'l) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Format/Date/Time/3 (Long Intn'l). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, (no Right), Return, Home, Right x7, Return, Home, Right x5, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Currently configured: HH:MM:SS (24 hour)".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x08d6, description at 0x08e6; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-019-selected.png](shots/menu-019-selected.png), [shots/menu-019-result.png](shots/menu-019-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L041 - /Worksheet/Global/Format/Date/Time/4 (Short Intn'l) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Format/Date/Time/4 (Short Intn'l). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, (no Right), Return, Home, Right x7, Return, Home, Right x5, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Currently configured: HH:MM (24 hour)".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x098b, description at 0x099c; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-020-selected.png](shots/menu-020-selected.png), [shots/menu-020-result.png](shots/menu-020-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L042 - /Worksheet/Global/Default/Printer/Interface is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Printer/Interface. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, (no Right), Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify printer interface".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x17e4, description at 0x17ee; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-040-selected.png](shots/menu-040-selected.png), [shots/menu-040-result.png](shots/menu-040-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L043 - /Worksheet/Global/Default/Printer/AutoLF/Yes is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Printer/AutoLF/Yes. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, (no Right), Return, Home, Right x1, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Printer automatically issues a line feed after a carriage return".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1ada, description at 0x1ade; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-042-selected.png](shots/menu-042-selected.png), [shots/menu-042-result.png](shots/menu-042-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L044 - /Worksheet/Global/Default/Printer/AutoLF/No is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Printer/AutoLF/No. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, (no Right), Return, Home, Right x1, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Printer does not automatically issue a line feed after a carriage return".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1b1f, description at 0x1b22; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-043-selected.png](shots/menu-043-selected.png), [shots/menu-043-result.png](shots/menu-043-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L045 - /Worksheet/Global/Default/Printer/Left is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Printer/Left. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, (no Right), Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set default left margin".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1831, description at 0x1836; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-044-selected.png](shots/menu-044-selected.png), [shots/menu-044-result.png](shots/menu-044-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L046 - /Worksheet/Global/Default/Printer/Right is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Printer/Right. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, (no Right), Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set default right margin".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x187f, description at 0x1885; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-045-selected.png](shots/menu-045-selected.png), [shots/menu-045-result.png](shots/menu-045-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L047 - /Worksheet/Global/Default/Printer/Top is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Printer/Top. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, (no Right), Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set default top margin".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x18d1, description at 0x18d5; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-046-selected.png](shots/menu-046-selected.png), [shots/menu-046-result.png](shots/menu-046-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L048 - /Worksheet/Global/Default/Printer/Bot is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Printer/Bot. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, (no Right), Return, Home, Right x5, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set default bottom margin".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x191a, description at 0x191e; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-047-selected.png](shots/menu-047-selected.png), [shots/menu-047-result.png](shots/menu-047-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L049 - /Worksheet/Global/Default/Printer/Pg-Length is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Printer/Pg-Length. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, (no Right), Return, Home, Right x6, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set default number of lines per page".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x196c, description at 0x1976; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-048-selected.png](shots/menu-048-selected.png), [shots/menu-048-result.png](shots/menu-048-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L050 - /Worksheet/Global/Default/Printer/Wait/No is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Printer/Wait/No. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, (no Right), Return, Home, Right x7, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Do not wait at the end of a page for paper change".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1b1f, description at 0x1b6b; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-050-selected.png](shots/menu-050-selected.png), [shots/menu-050-result.png](shots/menu-050-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L051 - /Worksheet/Global/Default/Printer/Wait/Yes is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Printer/Wait/Yes. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, (no Right), Return, Home, Right x7, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Wait at the end of a page for paper change".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1ada, description at 0x1b9d; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-051-selected.png](shots/menu-051-selected.png), [shots/menu-051-result.png](shots/menu-051-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L052 - /Worksheet/Global/Default/Printer/Setup is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Printer/Setup. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, (no Right), Return, Home, Right x8, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify default setup string".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x19fe, description at 0x1a04; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-052-selected.png](shots/menu-052-selected.png), [shots/menu-052-result.png](shots/menu-052-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L053 - /Worksheet/Global/Default/Printer/Name is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Printer/Name. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, (no Right), Return, Home, Right x9, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Change current printer device".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1a36, description at 0x1a3b; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-053-selected.png](shots/menu-053-selected.png), [shots/menu-053-result.png](shots/menu-053-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L054 - /Worksheet/Global/Default/Directory is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Directory. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify default directory".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x16c3, description at 0x16cd; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-055-selected.png](shots/menu-055-selected.png), [shots/menu-055-result.png](shots/menu-055-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L055 - /Worksheet/Global/Default/Status is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Status. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Display all default settings".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1701, description at 0x1708; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-056-selected.png](shots/menu-056-selected.png), [shots/menu-056-result.png](shots/menu-056-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L056 - /Worksheet/Global/Default/Update is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Update. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Save new default settings in configuration file".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1725, description at 0x172c; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-057-selected.png](shots/menu-057-selected.png), [shots/menu-057-result.png](shots/menu-057-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L057 - /Worksheet/Global/Default/Other/International/Punctuation/A (.,,) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Punctuation/A (.,,). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, (no Right), Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Decimal: Period   Argument separator: Comma       Thousands separator: Comma".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1e22, description at 0x1e2a; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-061-selected.png](shots/menu-061-selected.png), [shots/menu-061-result.png](shots/menu-061-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L058 - /Worksheet/Global/Default/Other/International/Punctuation/B (,..) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Punctuation/B (,..). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, (no Right), Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Decimal: Comma    Argument separator: Period      Thousands separator: Period".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1e77, description at 0x1e7f; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-062-selected.png](shots/menu-062-selected.png), [shots/menu-062-result.png](shots/menu-062-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L059 - /Worksheet/Global/Default/Other/International/Punctuation/C (.;,) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Punctuation/C (.;,). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, (no Right), Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Decimal: Period   Argument separator: Semicolon   Thousands separator: Comma".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1ecd, description at 0x1ed5; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-063-selected.png](shots/menu-063-selected.png), [shots/menu-063-result.png](shots/menu-063-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L060 - /Worksheet/Global/Default/Other/International/Punctuation/D (,;.) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Punctuation/D (,;.). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, (no Right), Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Decimal: Comma    Argument separator: Semicolon   Thousands separator: Period".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1f22, description at 0x1f2a; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-064-selected.png](shots/menu-064-selected.png), [shots/menu-064-result.png](shots/menu-064-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L061 - /Worksheet/Global/Default/Other/International/Punctuation/E (., ) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Punctuation/E (., ). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, (no Right), Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Decimal: Period   Argument separator: Comma       Thousands separator: Space".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1f78, description at 0x1f80; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-065-selected.png](shots/menu-065-selected.png), [shots/menu-065-result.png](shots/menu-065-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L062 - /Worksheet/Global/Default/Other/International/Punctuation/F (,. ) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Punctuation/F (,. ). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Decimal: Comma    Argument separator: Period      Thousands separator: Space".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1fcd, description at 0x1fd5; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-066-selected.png](shots/menu-066-selected.png), [shots/menu-066-result.png](shots/menu-066-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L063 - /Worksheet/Global/Default/Other/International/Punctuation/G (.; ) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Punctuation/G (.; ). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, (no Right), Return, Home, Right x6, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Decimal: Period   Argument separator: Semicolon   Thousands separator: Space".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x2022, description at 0x202a; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-067-selected.png](shots/menu-067-selected.png), [shots/menu-067-result.png](shots/menu-067-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L064 - /Worksheet/Global/Default/Other/International/Punctuation/H (,; ) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Punctuation/H (,; ). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, (no Right), Return, Home, Right x7, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Decimal: Comma    Argument separator: Semicolon   Thousands separator: Space".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x2077, description at 0x207f; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-068-selected.png](shots/menu-068-selected.png), [shots/menu-068-result.png](shots/menu-068-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L065 - /Worksheet/Global/Default/Other/International/Currency/Prefix is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Currency/Prefix. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, Right x1, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Currency sign precedes value".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x20cc, description at 0x20d3; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-070-selected.png](shots/menu-070-selected.png), [shots/menu-070-result.png](shots/menu-070-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L066 - /Worksheet/Global/Default/Other/International/Currency/Suffix is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Currency/Suffix. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, Right x1, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Currency sign follows value".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x20f0, description at 0x20f7; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-071-selected.png](shots/menu-071-selected.png), [shots/menu-071-result.png](shots/menu-071-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L067 - /Worksheet/Global/Default/Other/International/Date/A (MM/DD/YY) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Date/A (MM/DD/YY). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, Right x2, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Format D4 will be MM/DD/YY     Format D5 will be MM/DD".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x2113, description at 0x2120; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-073-selected.png](shots/menu-073-selected.png), [shots/menu-073-result.png](shots/menu-073-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L068 - /Worksheet/Global/Default/Other/International/Date/B (DD/MM/YY) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Date/B (DD/MM/YY). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, Right x2, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Format D4 will be DD/MM/YY     Format D5 will be DD/MM".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x2157, description at 0x2164; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-074-selected.png](shots/menu-074-selected.png), [shots/menu-074-result.png](shots/menu-074-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L069 - /Worksheet/Global/Default/Other/International/Date/C (DD.MM.YY) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Date/C (DD.MM.YY). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, Right x2, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Format D4 will be DD.MM.YY     Format D5 will be DD.MM".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x219b, description at 0x21a8; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-075-selected.png](shots/menu-075-selected.png), [shots/menu-075-result.png](shots/menu-075-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L070 - /Worksheet/Global/Default/Other/International/Date/D (YY-MM-DD) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Date/D (YY-MM-DD). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, Right x2, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Format D4 will be YY-MM-DD     Format D5 will be MM-DD".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x21df, description at 0x21ec; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-076-selected.png](shots/menu-076-selected.png), [shots/menu-076-result.png](shots/menu-076-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L071 - /Worksheet/Global/Default/Other/International/Time/A (HH:MM:SS) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Time/A (HH:MM:SS). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, Right x3, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Format D8 will be HH:MM:SS     Format D9 will be HH:MM".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x2223, description at 0x2230; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-078-selected.png](shots/menu-078-selected.png), [shots/menu-078-result.png](shots/menu-078-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L072 - /Worksheet/Global/Default/Other/International/Time/B (HH.MM.SS) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Time/B (HH.MM.SS). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, Right x3, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Format D8 will be HH.MM.SS     Format D9 will be HH.MM".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x2267, description at 0x2274; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-079-selected.png](shots/menu-079-selected.png), [shots/menu-079-result.png](shots/menu-079-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L073 - /Worksheet/Global/Default/Other/International/Time/C (HH,MM,SS) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Time/C (HH,MM,SS). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, Right x3, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Format D8 will be HH,MM,SS     Format D9 will be HH,MM".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x22ab, description at 0x22b8; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-080-selected.png](shots/menu-080-selected.png), [shots/menu-080-result.png](shots/menu-080-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L074 - /Worksheet/Global/Default/Other/International/Time/D (HHhMMmSSs) is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Time/D (HHhMMmSSs). Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, Right x3, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Format D8 will be HHhMMmSSs    Format D9 will be HHhMMm".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x22ef, description at 0x22fd; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-081-selected.png](shots/menu-081-selected.png), [shots/menu-081-result.png](shots/menu-081-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L075 - /Worksheet/Global/Default/Other/International/Negative/Parentheses is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Negative/Parentheses. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, Right x4, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Display parentheses around negative numbers".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x2335, description at 0x2341; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-083-selected.png](shots/menu-083-selected.png), [shots/menu-083-result.png](shots/menu-083-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L076 - /Worksheet/Global/Default/Other/International/Negative/Sign is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/International/Negative/Sign. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, (no Right), Return, Home, Right x4, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Display minus sign in front of negative numbers".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x236d, description at 0x2372; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-084-selected.png](shots/menu-084-selected.png), [shots/menu-084-result.png](shots/menu-084-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L077 - /Worksheet/Global/Default/Other/Help/Instant is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/Help/Instant. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, Right x1, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Instant access; do not remove Help Disk".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x23a2, description at 0x23aa; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-087-selected.png](shots/menu-087-selected.png), [shots/menu-087-result.png](shots/menu-087-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L078 - /Worksheet/Global/Default/Other/Help/Removable is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/Help/Removable. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, Right x1, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Help Disk can be removed when Help is not in use".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x23d2, description at 0x23dc; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-088-selected.png](shots/menu-088-selected.png), [shots/menu-088-result.png](shots/menu-088-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L079 - /Worksheet/Global/Default/Other/Clock/Standard is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/Clock/Standard. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, Right x2, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Use Lotus standard date and time: DD-MMM-YY  HH:MM AM/PM".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x240d, description at 0x2416; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-090-selected.png](shots/menu-090-selected.png), [shots/menu-090-result.png](shots/menu-090-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L080 - /Worksheet/Global/Default/Other/Clock/International is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/Clock/International. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, Right x2, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Use current International settings for date (D4) and time (D9)".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1bc8, description at 0x244f; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-091-selected.png](shots/menu-091-selected.png), [shots/menu-091-result.png](shots/menu-091-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L081 - /Worksheet/Global/Default/Other/Clock/None is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/Clock/None. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, Right x2, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Hide date-and-time indicator".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x248e, description at 0x2493; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-092-selected.png](shots/menu-092-selected.png), [shots/menu-092-result.png](shots/menu-092-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L082 - /Worksheet/Global/Default/Other/Clock/Clock is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/Clock/Clock. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, Right x2, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Display the date-and-time indicator".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1c27, description at 0x24b0; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-093-selected.png](shots/menu-093-selected.png), [shots/menu-093-result.png](shots/menu-093-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L083 - /Worksheet/Global/Default/Other/Clock/Filename is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/Clock/Filename. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, Right x2, Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Display current file name instead of date-and-time indicator".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x24d4, description at 0x24dd; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-094-selected.png](shots/menu-094-selected.png), [shots/menu-094-result.png](shots/menu-094-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L084 - /Worksheet/Global/Default/Other/Undo/Enable is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/Undo/Enable. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, Right x3, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Turn undo on".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1642, description at 0x251a; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-096-selected.png](shots/menu-096-selected.png), [shots/menu-096-result.png](shots/menu-096-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L085 - /Worksheet/Global/Default/Other/Undo/Disable is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/Undo/Disable. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, Right x3, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Turn undo off".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1666, description at 0x2527; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-097-selected.png](shots/menu-097-selected.png), [shots/menu-097-result.png](shots/menu-097-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L086 - /Worksheet/Global/Default/Other/Beep/Enable is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/Beep/Enable. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, Right x4, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Computer bell will sound when an error occurs".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1642, description at 0x2535; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-099-selected.png](shots/menu-099-selected.png), [shots/menu-099-result.png](shots/menu-099-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L087 - /Worksheet/Global/Default/Other/Beep/Disable is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/Beep/Disable. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, Right x4, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Computer bell will not sound when an error occurs".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1666, description at 0x2563; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-100-selected.png](shots/menu-100-selected.png), [shots/menu-100-result.png](shots/menu-100-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L088 - /Worksheet/Global/Default/Other/Add-In/Set is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/Add-In/Set. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, Right x5, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify an add-in to be attached automatically, and attach it".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x2595, description at 0x2599; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-102-selected.png](shots/menu-102-selected.png), [shots/menu-102-result.png](shots/menu-102-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L089 - /Worksheet/Global/Default/Other/Add-In/Cancel is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Other/Add-In/Cancel. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x4, Return, Home, Right x5, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Cancel setting for an auto-attach add-in, and detach it".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x25d7, description at 0x25de; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-103-selected.png](shots/menu-103-selected.png), [shots/menu-103-result.png](shots/menu-103-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L090 - /Worksheet/Global/Default/Autoexec/Enable is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Autoexec/Enable. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x5, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Run autoexecute macro when file is retrieved".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1642, description at 0x273d; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-105-selected.png](shots/menu-105-selected.png), [shots/menu-105-result.png](shots/menu-105-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L091 - /Worksheet/Global/Default/Autoexec/Disable is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Default/Autoexec/Disable. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x5, Return, Home, Right x5, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Do not run autoexecute macro when file is retrieved".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1666, description at 0x276a; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-106-selected.png](shots/menu-106-selected.png), [shots/menu-106-result.png](shots/menu-106-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L092 - /Worksheet/Global/Zero is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Global/Zero. Exact keys: /, Home, (no Right), Return, Home, (no Right), Return, Home, Right x6, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Change the way cells with a value of zero appear on the screen".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x14ca, description at 0x14cf; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-108-selected.png](shots/menu-108-selected.png), [shots/menu-108-result.png](shots/menu-108-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L093 - /Worksheet/Insert/Column is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Insert/Column. Exact keys: /, Home, (no Right), Return, Home, Right x1, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Insert one or more blank columns to the left of the cell pointer".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0c46, description at 0x0d9d; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-110-selected.png](shots/menu-110-selected.png), [shots/menu-110-result.png](shots/menu-110-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L094 - /Worksheet/Insert/Row is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Insert/Row. Exact keys: /, Home, (no Right), Return, Home, Right x1, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Insert one or more blank rows above the cell pointer".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0dfa, description at 0x0dfe; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-111-selected.png](shots/menu-111-selected.png), [shots/menu-111-result.png](shots/menu-111-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L095 - /Worksheet/Delete/Column is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Delete/Column. Exact keys: /, Home, (no Right), Return, Home, Right x2, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Delete one or more columns".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0c46, description at 0x0e4c; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-113-selected.png](shots/menu-113-selected.png), [shots/menu-113-result.png](shots/menu-113-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L096 - /Worksheet/Delete/Row is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Delete/Row. Exact keys: /, Home, (no Right), Return, Home, Right x2, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Delete one or more rows".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0dfa, description at 0x0e8a; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-114-selected.png](shots/menu-114-selected.png), [shots/menu-114-result.png](shots/menu-114-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L097 - /Worksheet/Column/Hide is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Column/Hide. Exact keys: /, Home, (no Right), Return, Home, Right x3, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Hide a range of columns".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0f44, description at 0x0f49; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-118-selected.png](shots/menu-118-selected.png), [shots/menu-118-result.png](shots/menu-118-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L098 - /Worksheet/Column/Display is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Column/Display. Exact keys: /, Home, (no Right), Return, Home, Right x3, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Display a range of hidden columns".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0f7a, description at 0x0f82; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-119-selected.png](shots/menu-119-selected.png), [shots/menu-119-result.png](shots/menu-119-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L099 - /Worksheet/Column/Column-Range/Set-Width is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Column/Column-Range/Set-Width. Exact keys: /, Home, (no Right), Return, Home, Right x3, Return, Home, Right x4, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify the width of a range of columns".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0ec2, description at 0x0ffe; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-121-selected.png](shots/menu-121-selected.png), [shots/menu-121-result.png](shots/menu-121-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L100 - /Worksheet/Column/Column-Range/Reset-Width is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Column/Column-Range/Reset-Width. Exact keys: /, Home, (no Right), Return, Home, Right x3, Return, Home, Right x4, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Return a range of columns to global column width".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0f0b, description at 0x104c; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-122-selected.png](shots/menu-122-selected.png), [shots/menu-122-result.png](shots/menu-122-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L101 - /Worksheet/Titles/Both is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Titles/Both. Exact keys: /, Home, (no Right), Return, Home, Right x5, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Freeze all rows and columns above and to the left of the cell pointer".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x10d7, description at 0x10dc; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-127-selected.png](shots/menu-127-selected.png), [shots/menu-127-result.png](shots/menu-127-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L102 - /Worksheet/Titles/Horizontal is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Titles/Horizontal. Exact keys: /, Home, (no Right), Return, Home, Right x5, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Freeze all rows above the cell pointer".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1122, description at 0x112d; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-128-selected.png](shots/menu-128-selected.png), [shots/menu-128-result.png](shots/menu-128-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L103 - /Worksheet/Titles/Vertical is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Titles/Vertical. Exact keys: /, Home, (no Right), Return, Home, Right x5, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Freeze all columns to the left of the cell pointer".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1154, description at 0x115d; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-129-selected.png](shots/menu-129-selected.png), [shots/menu-129-result.png](shots/menu-129-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L104 - /Worksheet/Titles/Clear is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Titles/Clear. Exact keys: /, Home, (no Right), Return, Home, Right x5, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Unfreeze all title columns and rows".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1190, description at 0x1196; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-130-selected.png](shots/menu-130-selected.png), [shots/menu-130-result.png](shots/menu-130-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L105 - /Worksheet/Window/Horizontal is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Window/Horizontal. Exact keys: /, Home, (no Right), Return, Home, Right x6, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Split the screen horizontally at the current row".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1122, description at 0x11ba; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-132-selected.png](shots/menu-132-selected.png), [shots/menu-132-result.png](shots/menu-132-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L106 - /Worksheet/Window/Vertical is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Window/Vertical. Exact keys: /, Home, (no Right), Return, Home, Right x6, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Split the screen vertically at the current column".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1154, description at 0x11eb; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-133-selected.png](shots/menu-133-selected.png), [shots/menu-133-result.png](shots/menu-133-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L107 - /Worksheet/Window/Sync is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Window/Sync. Exact keys: /, Home, (no Right), Return, Home, Right x6, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Synchronize scrolling in windows".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x121d, description at 0x1222; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-134-selected.png](shots/menu-134-selected.png), [shots/menu-134-result.png](shots/menu-134-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L108 - /Worksheet/Window/Unsync is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Window/Unsync. Exact keys: /, Home, (no Right), Return, Home, Right x6, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Scroll windows independently".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1243, description at 0x124a; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-135-selected.png](shots/menu-135-selected.png), [shots/menu-135-result.png](shots/menu-135-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L109 - /Worksheet/Window/Clear is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Window/Clear. Exact keys: /, Home, (no Right), Return, Home, Right x6, Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Return to full-screen display".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1190, description at 0x1267; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-136-selected.png](shots/menu-136-selected.png), [shots/menu-136-result.png](shots/menu-136-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L110 - /Worksheet/Status is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Status. Exact keys: /, Home, (no Right), Return, Home, Right x7, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Display worksheet settings".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0d0a, description at 0x0d11; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-137-selected.png](shots/menu-137-selected.png), [shots/menu-137-result.png](shots/menu-137-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L111 - /Worksheet/Page is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Page. Exact keys: /, Home, (no Right), Return, Home, Right x8, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Insert a row containing a page-break symbol above the cell pointer".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0d2c, description at 0x0d31; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-138-selected.png](shots/menu-138-selected.png), [shots/menu-138-result.png](shots/menu-138-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L112 - /Worksheet/Learn/Range is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Learn/Range. Exact keys: /, Home, (no Right), Return, Home, Right x9, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify a learn range in which to store keystrokes".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x1285, description at 0x128b; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-140-selected.png](shots/menu-140-selected.png), [shots/menu-140-result.png](shots/menu-140-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L113 - /Worksheet/Learn/Cancel is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Learn/Cancel. Exact keys: /, Home, (no Right), Return, Home, Right x9, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Cancel the currently specified learn range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x12be, description at 0x12c5; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-141-selected.png](shots/menu-141-selected.png), [shots/menu-141-result.png](shots/menu-141-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L114 - /Worksheet/Learn/Erase is unavailable

Area: Menu/Worksheet. Severity: P2.

Steps: On the empty worksheet choose /Worksheet/Learn/Erase. Exact keys: /, Home, (no Right), Return, Home, Right x9, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Erase the contents of the learn range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0c81, description at 0x12f0; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-142-selected.png](shots/menu-142-selected.png), [shots/menu-142-result.png](shots/menu-142-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L115 - /Range/Format/Date/4 (Long Intn'l) is unavailable

Area: Menu/Range. Severity: P2.

Steps: On the empty worksheet choose /Range/Format/Date/4 (Long Intn'l). Exact keys: /, Home, Right x1, Return, Home, (no Right), Return, Home, Right x7, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Currently configured: MM/DD/YY".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0794, description at 0x07a4; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-156-selected.png](shots/menu-156-selected.png), [shots/menu-156-result.png](shots/menu-156-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L116 - /Range/Format/Date/5 (Short Intn'l) is unavailable

Area: Menu/Range. Severity: P2.

Steps: On the empty worksheet choose /Range/Format/Date/5 (Short Intn'l). Exact keys: /, Home, Right x1, Return, Home, (no Right), Return, Home, Right x7, Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Currently configured: MM/DD".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0820, description at 0x0831; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-157-selected.png](shots/menu-157-selected.png), [shots/menu-157-result.png](shots/menu-157-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L117 - /Range/Format/Date/Time/1 (HH:MM:SS AM/PM) is unavailable

Area: Menu/Range. Severity: P2.

Steps: On the empty worksheet choose /Range/Format/Date/Time/1 (HH:MM:SS AM/PM). Exact keys: /, Home, Right x1, Return, Home, (no Right), Return, Home, Right x7, Return, Home, Right x5, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x08b3, description at 0x08c5; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-159-selected.png](shots/menu-159-selected.png), [shots/menu-159-result.png](shots/menu-159-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L118 - /Range/Format/Date/Time/2 (HH:MM AM/PM) is unavailable

Area: Menu/Range. Severity: P2.

Steps: On the empty worksheet choose /Range/Format/Date/Time/2 (HH:MM AM/PM). Exact keys: /, Home, Right x1, Return, Home, (no Right), Return, Home, Right x7, Return, Home, Right x5, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x08c6, description at 0x08d5; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-160-selected.png](shots/menu-160-selected.png), [shots/menu-160-result.png](shots/menu-160-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L119 - /Range/Format/Date/Time/3 (Long Intn'l) is unavailable

Area: Menu/Range. Severity: P2.

Steps: On the empty worksheet choose /Range/Format/Date/Time/3 (Long Intn'l). Exact keys: /, Home, Right x1, Return, Home, (no Right), Return, Home, Right x7, Return, Home, Right x5, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Currently configured: HH:MM:SS (24 hour)".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x08d6, description at 0x08e6; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-161-selected.png](shots/menu-161-selected.png), [shots/menu-161-result.png](shots/menu-161-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L120 - /Range/Format/Date/Time/4 (Short Intn'l) is unavailable

Area: Menu/Range. Severity: P2.

Steps: On the empty worksheet choose /Range/Format/Date/Time/4 (Short Intn'l). Exact keys: /, Home, Right x1, Return, Home, (no Right), Return, Home, Right x7, Return, Home, Right x5, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Currently configured: HH:MM (24 hour)".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x098b, description at 0x099c; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-162-selected.png](shots/menu-162-selected.png), [shots/menu-162-result.png](shots/menu-162-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L121 - /Range/Justify is unavailable

Area: Menu/Range. Severity: P2.

Steps: On the empty worksheet choose /Range/Justify. Exact keys: /, Home, Right x1, Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Adjust a column of labels to a specified width".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x28bb, description at 0x28c3; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-181-selected.png](shots/menu-181-selected.png), [shots/menu-181-result.png](shots/menu-181-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L122 - /Range/Input is unavailable

Area: Menu/Range. Severity: P2.

Steps: On the empty worksheet choose /Range/Input. Exact keys: /, Home, Right x1, Return, Home, Right x7, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Restrict data entry to unprotected cells".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x29b3, description at 0x29b9; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-184-selected.png](shots/menu-184-selected.png), [shots/menu-184-result.png](shots/menu-184-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L123 - /Range/Search/Formulas/Find is unavailable

Area: Menu/Range. Severity: P2.

Steps: On the empty worksheet choose /Range/Search/Formulas/Find. Exact keys: /, Home, Right x1, Return, Home, Right x10, Return, Home, (no Right), Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Highlight search string in the search range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x2d53, description at 0x2d58; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-189-selected.png](shots/menu-189-selected.png), [shots/menu-189-result.png](shots/menu-189-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L124 - /Range/Search/Formulas/Replace is unavailable

Area: Menu/Range. Severity: P2.

Steps: On the empty worksheet choose /Range/Search/Formulas/Replace. Exact keys: /, Home, Right x1, Return, Home, Right x10, Return, Home, (no Right), Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Replace each occurrence of search string with specified text".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x2d84, description at 0x2d8c; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-190-selected.png](shots/menu-190-selected.png), [shots/menu-190-result.png](shots/menu-190-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L125 - /Range/Search/Label is unavailable

Area: Menu/Range. Severity: P2.

Steps: On the empty worksheet choose /Range/Search/Label. Exact keys: /, Home, Right x1, Return, Home, Right x10, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Search for string in labels only".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x2807, description at 0x2d04; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-191-selected.png](shots/menu-191-selected.png), [shots/menu-191-result.png](shots/menu-191-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L126 - /Range/Search/Both is unavailable

Area: Menu/Range. Severity: P2.

Steps: On the empty worksheet choose /Range/Search/Both. Exact keys: /, Home, Right x1, Return, Home, Right x10, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Search for string in formulas and labels".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x2d25, description at 0x2d2a; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-192-selected.png](shots/menu-192-selected.png), [shots/menu-192-result.png](shots/menu-192-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L127 - /File/Combine/Copy/Entire-File is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Combine/Copy/Entire-File. Exact keys: /, Home, Right x4, Return, Home, Right x2, Return, Home, (no Right), Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Incorporate entire file into worksheet".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3342, description at 0x334e; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-200-selected.png](shots/menu-200-selected.png), [shots/menu-200-result.png](shots/menu-200-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L128 - /File/Combine/Copy/Named/Specified-Range is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Combine/Copy/Named/Specified-Range. Exact keys: /, Home, Right x4, Return, Home, Right x2, Return, Home, (no Right), Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Incorporate a range from a file into the worksheet".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3375, description at 0x338b; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-201-selected.png](shots/menu-201-selected.png), [shots/menu-201-result.png](shots/menu-201-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L129 - /File/Combine/Add/Entire-File is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Combine/Add/Entire-File. Exact keys: /, Home, Right x4, Return, Home, Right x2, Return, Home, Right x1, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Incorporate entire file into worksheet".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3342, description at 0x334e; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-203-selected.png](shots/menu-203-selected.png), [shots/menu-203-result.png](shots/menu-203-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L130 - /File/Combine/Add/Named/Specified-Range is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Combine/Add/Named/Specified-Range. Exact keys: /, Home, Right x4, Return, Home, Right x2, Return, Home, Right x1, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Incorporate a range from a file into the worksheet".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3375, description at 0x338b; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-204-selected.png](shots/menu-204-selected.png), [shots/menu-204-result.png](shots/menu-204-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L131 - /File/Combine/Subtract/Entire-File is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Combine/Subtract/Entire-File. Exact keys: /, Home, Right x4, Return, Home, Right x2, Return, Home, Right x2, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Incorporate entire file into worksheet".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3342, description at 0x334e; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-206-selected.png](shots/menu-206-selected.png), [shots/menu-206-result.png](shots/menu-206-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L132 - /File/Combine/Subtract/Named/Specified-Range is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Combine/Subtract/Named/Specified-Range. Exact keys: /, Home, Right x4, Return, Home, Right x2, Return, Home, Right x2, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Incorporate a range from a file into the worksheet".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3375, description at 0x338b; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-207-selected.png](shots/menu-207-selected.png), [shots/menu-207-result.png](shots/menu-207-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L133 - /File/Xtract/Formulas is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Xtract/Formulas. Exact keys: /, Home, Right x4, Return, Home, Right x3, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Save data including formulas".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x33dc, description at 0x33e5; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-209-selected.png](shots/menu-209-selected.png), [shots/menu-209-result.png](shots/menu-209-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L134 - /File/Xtract/Values is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Xtract/Values. Exact keys: /, Home, Right x4, Return, Home, Right x3, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Save current values and labels".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3402, description at 0x3409; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-210-selected.png](shots/menu-210-selected.png), [shots/menu-210-result.png](shots/menu-210-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L135 - /File/Erase/Worksheet is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Erase/Worksheet. Exact keys: /, Home, Right x4, Return, Home, Right x4, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Erase a worksheet file".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3428, description at 0x3432; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-212-selected.png](shots/menu-212-selected.png), [shots/menu-212-result.png](shots/menu-212-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L136 - /File/Erase/Print is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Erase/Print. Exact keys: /, Home, Right x4, Return, Home, Right x4, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Erase a print file".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3449, description at 0x344f; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-213-selected.png](shots/menu-213-selected.png), [shots/menu-213-result.png](shots/menu-213-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L137 - /File/Erase/Graph is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Erase/Graph. Exact keys: /, Home, Right x4, Return, Home, Right x4, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Erase a graph file".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3462, description at 0x3468; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-214-selected.png](shots/menu-214-selected.png), [shots/menu-214-result.png](shots/menu-214-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L138 - /File/Erase/Other is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Erase/Other. Exact keys: /, Home, Right x4, Return, Home, Right x4, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Erase any file".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x347b, description at 0x3481; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-215-selected.png](shots/menu-215-selected.png), [shots/menu-215-result.png](shots/menu-215-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L139 - /File/List/Worksheet is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/List/Worksheet. Exact keys: /, Home, Right x4, Return, Home, Right x5, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "List worksheet files".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3428, description at 0x34c7; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-217-selected.png](shots/menu-217-selected.png), [shots/menu-217-result.png](shots/menu-217-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L140 - /File/List/Print is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/List/Print. Exact keys: /, Home, Right x4, Return, Home, Right x5, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "List print files".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3449, description at 0x34dc; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-218-selected.png](shots/menu-218-selected.png), [shots/menu-218-result.png](shots/menu-218-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L141 - /File/List/Graph is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/List/Graph. Exact keys: /, Home, Right x4, Return, Home, Right x5, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "List graph files".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3462, description at 0x34ed; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-219-selected.png](shots/menu-219-selected.png), [shots/menu-219-result.png](shots/menu-219-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L142 - /File/List/Other is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/List/Other. Exact keys: /, Home, Right x4, Return, Home, Right x5, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "List all files".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x347b, description at 0x34fe; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-220-selected.png](shots/menu-220-selected.png), [shots/menu-220-result.png](shots/menu-220-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L143 - /File/List/Linked is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/List/Linked. Exact keys: /, Home, Right x4, Return, Home, Right x5, Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "List all files linked to worksheet".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x350d, description at 0x3514; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-221-selected.png](shots/menu-221-selected.png), [shots/menu-221-result.png](shots/menu-221-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L144 - /File/Import/Text is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Import/Text. Exact keys: /, Home, Right x4, Return, Home, Right x6, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Import each line of data as a single label".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3537, description at 0x353c; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-223-selected.png](shots/menu-223-selected.png), [shots/menu-223-result.png](shots/menu-223-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L145 - /File/Import/Numbers is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Import/Numbers. Exact keys: /, Home, Right x4, Return, Home, Right x6, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Import numbers and quoted text into separate columns".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3567, description at 0x356f; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-224-selected.png](shots/menu-224-selected.png), [shots/menu-224-result.png](shots/menu-224-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L146 - /File/Directory is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Directory. Exact keys: /, Home, Right x4, Return, Home, Right x7, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Display and/or change the current directory".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x31b1, description at 0x31bb; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-225-selected.png](shots/menu-225-selected.png), [shots/menu-225-result.png](shots/menu-225-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L147 - /File/Admin/Reservation/Get is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Admin/Reservation/Get. Exact keys: /, Home, Right x4, Return, Home, Right x8, Return, Home, (no Right), Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Secure a reservation for saving the file".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3638, description at 0x363c; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-228-selected.png](shots/menu-228-selected.png), [shots/menu-228-result.png](shots/menu-228-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L148 - /File/Admin/Reservation/Release is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Admin/Reservation/Release. Exact keys: /, Home, Right x4, Return, Home, Right x8, Return, Home, (no Right), Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Release a reservation for saving the file".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3665, description at 0x366d; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-229-selected.png](shots/menu-229-selected.png), [shots/menu-229-result.png](shots/menu-229-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L149 - /File/Admin/Table is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Admin/Table. Exact keys: /, Home, Right x4, Return, Home, Right x8, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Enter a table of file information in the worksheet".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x35de, description at 0x35e4; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-230-selected.png](shots/menu-230-selected.png), [shots/menu-230-result.png](shots/menu-230-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L150 - /File/Admin/Link-Refresh is unavailable

Area: Menu/File. Severity: P2.

Steps: On the empty worksheet choose /File/Admin/Link-Refresh. Exact keys: /, Home, Right x4, Return, Home, Right x8, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Update linked cells".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3617, description at 0x3624; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-231-selected.png](shots/menu-231-selected.png), [shots/menu-231-result.png](shots/menu-231-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L151 - /Print/Printer/Range is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Range. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify a range to print".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3754, description at 0x375a; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-234-selected.png](shots/menu-234-selected.png), [shots/menu-234-result.png](shots/menu-234-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L152 - /Print/Printer/Line is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Line. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Advance paper one line".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3787, description at 0x378c; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-235-selected.png](shots/menu-235-selected.png), [shots/menu-235-result.png](shots/menu-235-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L153 - /Print/Printer/Page is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Page. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Advance paper to top of next page".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x37a3, description at 0x37a8; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-236-selected.png](shots/menu-236-selected.png), [shots/menu-236-result.png](shots/menu-236-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L154 - /Print/Printer/Options/Header is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Options/Header. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x3, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Create a header".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x389e, description at 0x38a5; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-238-selected.png](shots/menu-238-selected.png), [shots/menu-238-result.png](shots/menu-238-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L155 - /Print/Printer/Options/Footer is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Options/Footer. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x3, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Create a footer".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x38c4, description at 0x38cb; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-239-selected.png](shots/menu-239-selected.png), [shots/menu-239-result.png](shots/menu-239-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L156 - /Print/Printer/Options/Margins/Left is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Options/Margins/Left. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x3, Return, Home, Right x2, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set left margin".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3a21, description at 0x3a26; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-241-selected.png](shots/menu-241-selected.png), [shots/menu-241-result.png](shots/menu-241-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L157 - /Print/Printer/Options/Margins/Right is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Options/Margins/Right. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x3, Return, Home, Right x2, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set right margin".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3a67, description at 0x3a6d; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-242-selected.png](shots/menu-242-selected.png), [shots/menu-242-result.png](shots/menu-242-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L158 - /Print/Printer/Options/Margins/Top is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Options/Margins/Top. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x3, Return, Home, Right x2, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set top margin".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3ab1, description at 0x3ab5; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-243-selected.png](shots/menu-243-selected.png), [shots/menu-243-result.png](shots/menu-243-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L159 - /Print/Printer/Options/Margins/Bottom is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Options/Margins/Bottom. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x3, Return, Home, Right x2, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set bottom margin".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3af2, description at 0x3af9; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-244-selected.png](shots/menu-244-selected.png), [shots/menu-244-result.png](shots/menu-244-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L160 - /Print/Printer/Options/Margins/None is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Options/Margins/None. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x3, Return, Home, Right x2, Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Clear all margin settings".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3b3f, description at 0x3b44; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-245-selected.png](shots/menu-245-selected.png), [shots/menu-245-result.png](shots/menu-245-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L161 - /Print/Printer/Options/Borders/Columns is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Options/Borders/Columns. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x3, Return, Home, Right x3, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Print border columns to the left of each print range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3b5e, description at 0x3b66; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-247-selected.png](shots/menu-247-selected.png), [shots/menu-247-result.png](shots/menu-247-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L162 - /Print/Printer/Options/Borders/Rows is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Options/Borders/Rows. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x3, Return, Home, Right x3, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Print border rows above each print range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3bbc, description at 0x3bc1; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-248-selected.png](shots/menu-248-selected.png), [shots/menu-248-result.png](shots/menu-248-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L163 - /Print/Printer/Options/Setup is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Options/Setup. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x3, Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Enter printer setup string".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x393a, description at 0x3940; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-249-selected.png](shots/menu-249-selected.png), [shots/menu-249-result.png](shots/menu-249-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L164 - /Print/Printer/Options/Pg-Length is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Options/Pg-Length. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x3, Return, Home, Right x5, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify number of lines per page".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3970, description at 0x397a; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-250-selected.png](shots/menu-250-selected.png), [shots/menu-250-result.png](shots/menu-250-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L165 - /Print/Printer/Options/Other/As-Displayed is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Options/Other/As-Displayed. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x3, Return, Home, Right x6, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Print range as displayed".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3c08, description at 0x3c15; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-252-selected.png](shots/menu-252-selected.png), [shots/menu-252-result.png](shots/menu-252-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L166 - /Print/Printer/Options/Other/Cell-Formulas is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Options/Other/Cell-Formulas. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x3, Return, Home, Right x6, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "List entries, one per line".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3c2e, description at 0x3c3c; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-253-selected.png](shots/menu-253-selected.png), [shots/menu-253-result.png](shots/menu-253-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L167 - /Print/Printer/Options/Other/Formatted is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Options/Other/Formatted. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x3, Return, Home, Right x6, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Print headers, footers, and page breaks".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3c57, description at 0x3c61; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-254-selected.png](shots/menu-254-selected.png), [shots/menu-254-result.png](shots/menu-254-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L168 - /Print/Printer/Options/Other/Unformatted is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Options/Other/Unformatted. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x3, Return, Home, Right x6, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Do not print headers, footers, and page breaks".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3c89, description at 0x3c95; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-255-selected.png](shots/menu-255-selected.png), [shots/menu-255-result.png](shots/menu-255-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L169 - /Print/Printer/Clear/All is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Clear/All. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x4, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Return all print settings to defaults".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3cc4, description at 0x3cc8; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-258-selected.png](shots/menu-258-selected.png), [shots/menu-258-result.png](shots/menu-258-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L170 - /Print/Printer/Clear/Range is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Clear/Range. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x4, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Clear current print range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3754, description at 0x3cee; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-259-selected.png](shots/menu-259-selected.png), [shots/menu-259-result.png](shots/menu-259-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L171 - /Print/Printer/Clear/Borders is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Clear/Borders. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x4, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Clear border column and row ranges".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3911, description at 0x3d08; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-260-selected.png](shots/menu-260-selected.png), [shots/menu-260-result.png](shots/menu-260-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L172 - /Print/Printer/Clear/Format is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Clear/Format. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x4, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Return margins, page length, and setup string to defaults".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3d2b, description at 0x3d32; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-261-selected.png](shots/menu-261-selected.png), [shots/menu-261-result.png](shots/menu-261-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L173 - /Print/Printer/Align is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Align. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x5, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Reset to top of page (after adjusting paper)".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3834, description at 0x383a; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-262-selected.png](shots/menu-262-selected.png), [shots/menu-262-result.png](shots/menu-262-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L174 - /Print/Printer/Go is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/Printer/Go. Exact keys: /, Home, Right x5, Return, Home, (no Right), Return, Home, Right x6, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Print the specified range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3867, description at 0x386a; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-263-selected.png](shots/menu-263-selected.png), [shots/menu-263-result.png](shots/menu-263-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L175 - /Print/File/Range is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Range. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify a range to print".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3754, description at 0x375a; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-266-selected.png](shots/menu-266-selected.png), [shots/menu-266-result.png](shots/menu-266-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L176 - /Print/File/Line is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Line. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Advance paper one line".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3787, description at 0x378c; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-267-selected.png](shots/menu-267-selected.png), [shots/menu-267-result.png](shots/menu-267-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L177 - /Print/File/Page is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Page. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Advance paper to top of next page".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x37a3, description at 0x37a8; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-268-selected.png](shots/menu-268-selected.png), [shots/menu-268-result.png](shots/menu-268-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L178 - /Print/File/Options/Header is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Options/Header. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x3, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Create a header".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x389e, description at 0x38a5; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-270-selected.png](shots/menu-270-selected.png), [shots/menu-270-result.png](shots/menu-270-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L179 - /Print/File/Options/Footer is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Options/Footer. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x3, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Create a footer".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x38c4, description at 0x38cb; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-271-selected.png](shots/menu-271-selected.png), [shots/menu-271-result.png](shots/menu-271-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L180 - /Print/File/Options/Margins/Left is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Options/Margins/Left. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x3, Return, Home, Right x2, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set left margin".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3a21, description at 0x3a26; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-273-selected.png](shots/menu-273-selected.png), [shots/menu-273-result.png](shots/menu-273-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L181 - /Print/File/Options/Margins/Right is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Options/Margins/Right. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x3, Return, Home, Right x2, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set right margin".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3a67, description at 0x3a6d; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-274-selected.png](shots/menu-274-selected.png), [shots/menu-274-result.png](shots/menu-274-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L182 - /Print/File/Options/Margins/Top is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Options/Margins/Top. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x3, Return, Home, Right x2, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set top margin".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3ab1, description at 0x3ab5; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-275-selected.png](shots/menu-275-selected.png), [shots/menu-275-result.png](shots/menu-275-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L183 - /Print/File/Options/Margins/Bottom is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Options/Margins/Bottom. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x3, Return, Home, Right x2, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set bottom margin".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3af2, description at 0x3af9; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-276-selected.png](shots/menu-276-selected.png), [shots/menu-276-result.png](shots/menu-276-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L184 - /Print/File/Options/Margins/None is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Options/Margins/None. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x3, Return, Home, Right x2, Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Clear all margin settings".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3b3f, description at 0x3b44; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-277-selected.png](shots/menu-277-selected.png), [shots/menu-277-result.png](shots/menu-277-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L185 - /Print/File/Options/Borders/Columns is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Options/Borders/Columns. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x3, Return, Home, Right x3, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Print border columns to the left of each print range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3b5e, description at 0x3b66; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-279-selected.png](shots/menu-279-selected.png), [shots/menu-279-result.png](shots/menu-279-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L186 - /Print/File/Options/Borders/Rows is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Options/Borders/Rows. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x3, Return, Home, Right x3, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Print border rows above each print range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3bbc, description at 0x3bc1; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-280-selected.png](shots/menu-280-selected.png), [shots/menu-280-result.png](shots/menu-280-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L187 - /Print/File/Options/Setup is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Options/Setup. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x3, Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Enter printer setup string".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x393a, description at 0x3940; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-281-selected.png](shots/menu-281-selected.png), [shots/menu-281-result.png](shots/menu-281-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L188 - /Print/File/Options/Pg-Length is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Options/Pg-Length. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x3, Return, Home, Right x5, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify number of lines per page".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3970, description at 0x397a; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-282-selected.png](shots/menu-282-selected.png), [shots/menu-282-result.png](shots/menu-282-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L189 - /Print/File/Options/Other/As-Displayed is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Options/Other/As-Displayed. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x3, Return, Home, Right x6, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Print range as displayed".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3c08, description at 0x3c15; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-284-selected.png](shots/menu-284-selected.png), [shots/menu-284-result.png](shots/menu-284-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L190 - /Print/File/Options/Other/Cell-Formulas is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Options/Other/Cell-Formulas. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x3, Return, Home, Right x6, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "List entries, one per line".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3c2e, description at 0x3c3c; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-285-selected.png](shots/menu-285-selected.png), [shots/menu-285-result.png](shots/menu-285-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L191 - /Print/File/Options/Other/Formatted is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Options/Other/Formatted. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x3, Return, Home, Right x6, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Print headers, footers, and page breaks".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3c57, description at 0x3c61; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-286-selected.png](shots/menu-286-selected.png), [shots/menu-286-result.png](shots/menu-286-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L192 - /Print/File/Options/Other/Unformatted is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Options/Other/Unformatted. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x3, Return, Home, Right x6, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Do not print headers, footers, and page breaks".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3c89, description at 0x3c95; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-287-selected.png](shots/menu-287-selected.png), [shots/menu-287-result.png](shots/menu-287-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L193 - /Print/File/Clear/All is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Clear/All. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x4, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Return all print settings to defaults".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3cc4, description at 0x3cc8; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-290-selected.png](shots/menu-290-selected.png), [shots/menu-290-result.png](shots/menu-290-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L194 - /Print/File/Clear/Range is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Clear/Range. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x4, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Clear current print range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3754, description at 0x3cee; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-291-selected.png](shots/menu-291-selected.png), [shots/menu-291-result.png](shots/menu-291-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L195 - /Print/File/Clear/Borders is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Clear/Borders. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x4, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Clear border column and row ranges".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3911, description at 0x3d08; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-292-selected.png](shots/menu-292-selected.png), [shots/menu-292-result.png](shots/menu-292-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L196 - /Print/File/Clear/Format is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Clear/Format. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x4, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Return margins, page length, and setup string to defaults".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3d2b, description at 0x3d32; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-293-selected.png](shots/menu-293-selected.png), [shots/menu-293-result.png](shots/menu-293-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L197 - /Print/File/Align is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Align. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x5, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Reset to top of page (after adjusting paper)".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3834, description at 0x383a; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-294-selected.png](shots/menu-294-selected.png), [shots/menu-294-result.png](shots/menu-294-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L198 - /Print/File/Go is unavailable

Area: Menu/Print. Severity: P2.

Steps: On the empty worksheet choose /Print/File/Go. Exact keys: /, Home, Right x5, Return, Home, Right x1, Return, Home, Right x6, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Print the specified range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3867, description at 0x386a; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-295-selected.png](shots/menu-295-selected.png), [shots/menu-295-result.png](shots/menu-295-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L199 - /Graph/Type/Line is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/Type/Line. Exact keys: /, Home, Right x6, Return, Home, (no Right), Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Line graph".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x403c, description at 0x4041; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-299-selected.png](shots/menu-299-selected.png), [shots/menu-299-result.png](shots/menu-299-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L200 - /Graph/Type/Bar is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/Type/Bar. Exact keys: /, Home, Right x6, Return, Home, (no Right), Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Bar graph".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x404c, description at 0x4050; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-300-selected.png](shots/menu-300-selected.png), [shots/menu-300-result.png](shots/menu-300-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L201 - /Graph/Type/XY is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/Type/XY. Exact keys: /, Home, Right x6, Return, Home, (no Right), Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "XY graph".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x405a, description at 0x405d; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-301-selected.png](shots/menu-301-selected.png), [shots/menu-301-result.png](shots/menu-301-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L202 - /Graph/Type/Stack-Bar is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/Type/Stack-Bar. Exact keys: /, Home, Right x6, Return, Home, (no Right), Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Stacked bar graph".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x4066, description at 0x4070; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-302-selected.png](shots/menu-302-selected.png), [shots/menu-302-result.png](shots/menu-302-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L203 - /Graph/Type/Pie is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/Type/Pie. Exact keys: /, Home, Right x6, Return, Home, (no Right), Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Pie chart".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x4082, description at 0x4086; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-303-selected.png](shots/menu-303-selected.png), [shots/menu-303-result.png](shots/menu-303-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L204 - /Graph/X is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/X. Exact keys: /, Home, Right x6, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set X data range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3d8f, description at 0x3d91; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-304-selected.png](shots/menu-304-selected.png), [shots/menu-304-result.png](shots/menu-304-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L205 - /Graph/A is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/A. Exact keys: /, Home, Right x6, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set first data range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3db7, description at 0x3db9; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-305-selected.png](shots/menu-305-selected.png), [shots/menu-305-result.png](shots/menu-305-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L206 - /Graph/B is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/B. Exact keys: /, Home, Right x6, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set second data range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3de7, description at 0x3de9; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-306-selected.png](shots/menu-306-selected.png), [shots/menu-306-result.png](shots/menu-306-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L207 - /Graph/C is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/C. Exact keys: /, Home, Right x6, Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set third data range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3e19, description at 0x3e1b; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-307-selected.png](shots/menu-307-selected.png), [shots/menu-307-result.png](shots/menu-307-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L208 - /Graph/D is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/D. Exact keys: /, Home, Right x6, Return, Home, Right x5, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set fourth data range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3e49, description at 0x3e4b; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-308-selected.png](shots/menu-308-selected.png), [shots/menu-308-result.png](shots/menu-308-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L209 - /Graph/E is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/E. Exact keys: /, Home, Right x6, Return, Home, Right x6, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set fifth data range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3e7b, description at 0x3e7d; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-309-selected.png](shots/menu-309-selected.png), [shots/menu-309-result.png](shots/menu-309-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L210 - /Graph/F is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/F. Exact keys: /, Home, Right x6, Return, Home, Right x7, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Set sixth data range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3eab, description at 0x3ead; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-310-selected.png](shots/menu-310-selected.png), [shots/menu-310-result.png](shots/menu-310-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L211 - /Graph/Reset is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/Reset. Exact keys: /, Home, Right x6, Return, Home, Right x8, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Graph  X  A  B  C  D  E  F  Ranges  Options  Quit".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3edb, description at 0x3ee1; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-311-selected.png](shots/menu-311-selected.png), [shots/menu-311-result.png](shots/menu-311-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L212 - /Graph/View is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/View. Exact keys: /, Home, Right x6, Return, Home, Right x9, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "View the current graph".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3f13, description at 0x3f18; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-312-selected.png](shots/menu-312-selected.png), [shots/menu-312-result.png](shots/menu-312-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L213 - /Graph/Save is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/Save. Exact keys: /, Home, Right x6, Return, Home, Right x10, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Save the current graph in a file for printing".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3f2f, description at 0x3f34; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-313-selected.png](shots/menu-313-selected.png), [shots/menu-313-result.png](shots/menu-313-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L214 - /Graph/Options is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/Options. Exact keys: /, Home, Right x6, Return, Home, Right x11, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Legend  Format  Titles  Grid  Scale  Color  B&W  Data-Labels  Quit".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3f7a, description at 0x3f82; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-314-selected.png](shots/menu-314-selected.png), [shots/menu-314-result.png](shots/menu-314-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L215 - /Graph/Name/Use is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/Name/Use. Exact keys: /, Home, Right x6, Return, Home, Right x12, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Make a named graph current".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x4d17, description at 0x4d1b; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-316-selected.png](shots/menu-316-selected.png), [shots/menu-316-result.png](shots/menu-316-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L216 - /Graph/Name/Create is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/Name/Create. Exact keys: /, Home, Right x6, Return, Home, Right x12, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Name the current graph".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x4d5c, description at 0x4d63; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-317-selected.png](shots/menu-317-selected.png), [shots/menu-317-result.png](shots/menu-317-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L217 - /Graph/Name/Delete is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/Name/Delete. Exact keys: /, Home, Right x6, Return, Home, Right x12, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Delete a named graph".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x4d8d, description at 0x4d94; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-318-selected.png](shots/menu-318-selected.png), [shots/menu-318-result.png](shots/menu-318-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L218 - /Graph/Name/Reset is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/Name/Reset. Exact keys: /, Home, Right x6, Return, Home, Right x12, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Delete all named graphs".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3edb, description at 0x4dc9; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-319-selected.png](shots/menu-319-selected.png), [shots/menu-319-result.png](shots/menu-319-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L219 - /Graph/Name/Table is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/Name/Table. Exact keys: /, Home, Right x6, Return, Home, Right x12, Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Create a table of named graphs".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x4de1, description at 0x4de7; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-320-selected.png](shots/menu-320-selected.png), [shots/menu-320-result.png](shots/menu-320-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L220 - /Graph/Group/Columnwise is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/Group/Columnwise. Exact keys: /, Home, Right x6, Return, Home, Right x13, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Use columns as data ranges".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x4e1e, description at 0x4e29; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-322-selected.png](shots/menu-322-selected.png), [shots/menu-322-result.png](shots/menu-322-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L221 - /Graph/Group/Rowwise is unavailable

Area: Menu/Graph. Severity: P2.

Steps: On the empty worksheet choose /Graph/Group/Rowwise. Exact keys: /, Home, Right x6, Return, Home, Right x13, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Use rows as data ranges".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x4e44, description at 0x4e4c; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-323-selected.png](shots/menu-323-selected.png), [shots/menu-323-result.png](shots/menu-323-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L222 - /Data/Fill is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Fill. Exact keys: /, Home, Right x7, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Fill a range with a sequence of values".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x4e64, description at 0x4e69; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-326-selected.png](shots/menu-326-selected.png), [shots/menu-326-result.png](shots/menu-326-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L223 - /Data/Table/1 is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Table/1. Exact keys: /, Home, Right x7, Return, Home, Right x1, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "One input cell, one or more dependent formulas".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x503c, description at 0x503e; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-328-selected.png](shots/menu-328-selected.png), [shots/menu-328-result.png](shots/menu-328-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L224 - /Data/Table/2 is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Table/2. Exact keys: /, Home, Right x7, Return, Home, Right x1, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Two input cells, one dependent formula".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x5096, description at 0x5098; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-329-selected.png](shots/menu-329-selected.png), [shots/menu-329-result.png](shots/menu-329-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L225 - /Data/Table/Reset is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Table/Reset. Exact keys: /, Home, Right x7, Return, Home, Right x1, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Clear table ranges and input cells for all data tables".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x50d4, description at 0x50da; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-330-selected.png](shots/menu-330-selected.png), [shots/menu-330-result.png](shots/menu-330-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L226 - /Data/Sort/Data-Range is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Sort/Data-Range. Exact keys: /, Home, Right x7, Return, Home, Right x2, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Select records to be sorted".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x5111, description at 0x511c; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-332-selected.png](shots/menu-332-selected.png), [shots/menu-332-result.png](shots/menu-332-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L227 - /Data/Sort/Primary-Key is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Sort/Primary-Key. Exact keys: /, Home, Right x7, Return, Home, Right x2, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify primary order for records".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x514b, description at 0x5157; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-333-selected.png](shots/menu-333-selected.png), [shots/menu-333-result.png](shots/menu-333-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L228 - /Data/Sort/Secondary-Key is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Sort/Secondary-Key. Exact keys: /, Home, Right x7, Return, Home, Right x2, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify order for records with same primary key".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x51a6, description at 0x51b4; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-334-selected.png](shots/menu-334-selected.png), [shots/menu-334-result.png](shots/menu-334-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L229 - /Data/Sort/Reset is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Sort/Reset. Exact keys: /, Home, Right x7, Return, Home, Right x2, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Clear data range and sort keys".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x50d4, description at 0x51f9; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-335-selected.png](shots/menu-335-selected.png), [shots/menu-335-result.png](shots/menu-335-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L230 - /Data/Sort/Go is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Sort/Go. Exact keys: /, Home, Right x7, Return, Home, Right x2, Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Sort data and return to READY mode".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3867, description at 0x521b; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-336-selected.png](shots/menu-336-selected.png), [shots/menu-336-result.png](shots/menu-336-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L231 - /Data/Query/Input is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Query/Input. Exact keys: /, Home, Right x7, Return, Home, Right x3, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify range that contains records to search".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x5258, description at 0x525e; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-339-selected.png](shots/menu-339-selected.png), [shots/menu-339-result.png](shots/menu-339-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L232 - /Data/Query/Criteria is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Query/Criteria. Exact keys: /, Home, Right x7, Return, Home, Right x3, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify the range that contains criteria".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x52a0, description at 0x52a9; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-340-selected.png](shots/menu-340-selected.png), [shots/menu-340-result.png](shots/menu-340-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L233 - /Data/Query/Output is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Query/Output. Exact keys: /, Home, Right x7, Return, Home, Right x3, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify the range to which extracted records are copied".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x52e9, description at 0x52f0; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-341-selected.png](shots/menu-341-selected.png), [shots/menu-341-result.png](shots/menu-341-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L234 - /Data/Query/Find is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Query/Find. Exact keys: /, Home, Right x7, Return, Home, Right x3, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Highlight each record that matches criteria".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x5328, description at 0x532d; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-342-selected.png](shots/menu-342-selected.png), [shots/menu-342-result.png](shots/menu-342-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L235 - /Data/Query/Extract is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Query/Extract. Exact keys: /, Home, Right x7, Return, Home, Right x3, Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Copy all records that match criteria to output range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x5359, description at 0x5361; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-343-selected.png](shots/menu-343-selected.png), [shots/menu-343-result.png](shots/menu-343-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L236 - /Data/Query/Unique is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Query/Unique. Exact keys: /, Home, Right x7, Return, Home, Right x3, Return, Home, Right x5, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Copy records that match criteria to output range, eliminating duplicates".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x5396, description at 0x539d; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-344-selected.png](shots/menu-344-selected.png), [shots/menu-344-result.png](shots/menu-344-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L237 - /Data/Query/Delete is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Query/Delete. Exact keys: /, Home, Right x7, Return, Home, Right x3, Return, Home, Right x6, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Delete all records that match criteria".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x53e6, description at 0x53ed; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-345-selected.png](shots/menu-345-selected.png), [shots/menu-345-result.png](shots/menu-345-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L238 - /Data/Query/Reset is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Query/Reset. Exact keys: /, Home, Right x7, Return, Home, Right x3, Return, Home, Right x7, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Clear input, criteria, and output ranges".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x50d4, description at 0x5414; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-346-selected.png](shots/menu-346-selected.png), [shots/menu-346-result.png](shots/menu-346-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L239 - /Data/Distribution is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Distribution. Exact keys: /, Home, Right x7, Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Calculate frequency distribution of the values in a range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x4f26, description at 0x4f33; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-348-selected.png](shots/menu-348-selected.png), [shots/menu-348-result.png](shots/menu-348-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L240 - /Data/Matrix/Invert is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Matrix/Invert. Exact keys: /, Home, Right x7, Return, Home, Right x5, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Create the inverse of a matrix".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x54a9, description at 0x54b0; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-350-selected.png](shots/menu-350-selected.png), [shots/menu-350-result.png](shots/menu-350-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L241 - /Data/Matrix/Multiply is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Matrix/Multiply. Exact keys: /, Home, Right x7, Return, Home, Right x5, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Multiply two ranges as matrices".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x54e7, description at 0x54f0; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-351-selected.png](shots/menu-351-selected.png), [shots/menu-351-result.png](shots/menu-351-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L242 - /Data/Regression/X-Range is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Regression/X-Range. Exact keys: /, Home, Right x7, Return, Home, Right x6, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify independent variables (X range)".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x5551, description at 0x5559; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-353-selected.png](shots/menu-353-selected.png), [shots/menu-353-result.png](shots/menu-353-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L243 - /Data/Regression/Y-Range is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Regression/Y-Range. Exact keys: /, Home, Right x7, Return, Home, Right x6, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify dependent variable (Y range)".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x55ab, description at 0x55b3; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-354-selected.png](shots/menu-354-selected.png), [shots/menu-354-result.png](shots/menu-354-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L244 - /Data/Regression/Output-Range is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Regression/Output-Range. Exact keys: /, Home, Right x7, Return, Home, Right x6, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify the output range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x55ff, description at 0x560c; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-355-selected.png](shots/menu-355-selected.png), [shots/menu-355-result.png](shots/menu-355-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L245 - /Data/Regression/Intercept/Compute is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Regression/Intercept/Compute. Exact keys: /, Home, Right x7, Return, Home, Right x6, Return, Home, Right x3, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Calculate the y-axis intercept automatically".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x56b6, description at 0x56be; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-357-selected.png](shots/menu-357-selected.png), [shots/menu-357-result.png](shots/menu-357-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L246 - /Data/Regression/Intercept/Zero is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Regression/Intercept/Zero. Exact keys: /, Home, Right x7, Return, Home, Right x6, Return, Home, Right x3, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Use zero as the y-axis intercept".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x56eb, description at 0x56f0; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-358-selected.png](shots/menu-358-selected.png), [shots/menu-358-result.png](shots/menu-358-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L247 - /Data/Regression/Reset is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Regression/Reset. Exact keys: /, Home, Right x7, Return, Home, Right x6, Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Clear the X range, Y range, output range, and reset Intercept to Compute".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x50d4, description at 0x563d; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-359-selected.png](shots/menu-359-selected.png), [shots/menu-359-result.png](shots/menu-359-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L248 - /Data/Regression/Go is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Regression/Go. Exact keys: /, Home, Right x7, Return, Home, Right x6, Return, Home, Right x5, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Calculate a data regression on specified ranges".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3867, description at 0x5686; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-360-selected.png](shots/menu-360-selected.png), [shots/menu-360-result.png](shots/menu-360-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L249 - /Data/Parse/Format-Line/Create is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Parse/Format-Line/Create. Exact keys: /, Home, Right x7, Return, Home, Right x7, Return, Home, (no Right), Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Create a format line at the current cell".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x583a, description at 0x5841; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-364-selected.png](shots/menu-364-selected.png), [shots/menu-364-result.png](shots/menu-364-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L250 - /Data/Parse/Format-Line/Edit is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Parse/Format-Line/Edit. Exact keys: /, Home, Right x7, Return, Home, Right x7, Return, Home, (no Right), Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Edit a format line at the current cell".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x586a, description at 0x586f; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-365-selected.png](shots/menu-365-selected.png), [shots/menu-365-result.png](shots/menu-365-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L251 - /Data/Parse/Input-Column is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Parse/Input-Column. Exact keys: /, Home, Right x7, Return, Home, Right x7, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify a column of labels to parse".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x574e, description at 0x575b; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-366-selected.png](shots/menu-366-selected.png), [shots/menu-366-result.png](shots/menu-366-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L252 - /Data/Parse/Output-Range is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Parse/Output-Range. Exact keys: /, Home, Right x7, Return, Home, Right x7, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Specify the range in which parsed data is placed".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x55ff, description at 0x57a1; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-367-selected.png](shots/menu-367-selected.png), [shots/menu-367-result.png](shots/menu-367-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L253 - /Data/Parse/Reset is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Parse/Reset. Exact keys: /, Home, Right x7, Return, Home, Right x7, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Clear input column and output range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x50d4, description at 0x57d2; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-368-selected.png](shots/menu-368-selected.png), [shots/menu-368-result.png](shots/menu-368-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L254 - /Data/Parse/Go is unavailable

Area: Menu/Data. Severity: P2.

Steps: On the empty worksheet choose /Data/Parse/Go. Exact keys: /, Home, Right x7, Return, Home, Right x7, Return, Home, Right x4, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Parse labels in the input column and place them in the output range".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x3867, description at 0x57f6; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-369-selected.png](shots/menu-369-selected.png), [shots/menu-369-result.png](shots/menu-369-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L255 - /System is unavailable

Area: Menu/System. Severity: P2.

Steps: On the empty worksheet choose /System. Exact keys: /, Home, Right x8, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Leave 1-2-3 temporarily and use operating system".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0270, description at 0x0277; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-371-selected.png](shots/menu-371-selected.png), [shots/menu-371-result.png](shots/menu-371-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L256 - /Add-In/Attach is unavailable

Area: Menu/Add-In. Severity: P2.

Steps: On the empty worksheet choose /Add-In/Attach. Exact keys: /, Home, Right x9, Return, Home, (no Right), Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Load an add-in program into memory".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x03a0, description at 0x03a7; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-373-selected.png](shots/menu-373-selected.png), [shots/menu-373-result.png](shots/menu-373-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L257 - /Add-In/Detach is unavailable

Area: Menu/Add-In. Severity: P2.

Steps: On the empty worksheet choose /Add-In/Detach. Exact keys: /, Home, Right x9, Return, Home, Right x1, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Remove an attached add-in program from memory".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x03e3, description at 0x03ea; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-374-selected.png](shots/menu-374-selected.png), [shots/menu-374-result.png](shots/menu-374-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L258 - /Add-In/Invoke is unavailable

Area: Menu/Add-In. Severity: P2.

Steps: On the empty worksheet choose /Add-In/Invoke. Exact keys: /, Home, Right x9, Return, Home, Right x2, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Activate an attached add-in program".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0431, description at 0x0438; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-375-selected.png](shots/menu-375-selected.png), [shots/menu-375-result.png](shots/menu-375-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

### L259 - /Add-In/Clear is unavailable

Area: Menu/Add-In. Severity: P2.

Steps: On the empty worksheet choose /Add-In/Clear. Exact keys: /, Home, Right x9, Return, Home, Right x3, Return. Press Escape to dismiss the refusal.

Observed: The command opens ERROR with "Command not implemented". Its selected-item description was "Remove all attached add-in programs from memory".

Expected: Perform the specific operation advertised by this menu item, or provide its working settings/input workflow.

Basis: Real Lotus R2.2 installed 123.RI: item at 0x0475, description at 0x047b; the strings were independently read from that binary and compared with the actual serial-rendered description.

Guess: os/i123/app/face_a.c:850 falls through to unsupported() for this leaf; spec/i123/era_shortfalls.md acknowledges deferred command execution. This is not exempted by the brief.

Evidence: [shots/menu-376-selected.png](shots/menu-376-selected.png), [shots/menu-376-result.png](shots/menu-376-result.png), [evidence/menus-serial.log](evidence/menus-serial.log), [evidence/menu-verified.json](evidence/menu-verified.json).

## Still present

These refer to the 2026-10-05-astra-full-drive audit, not this report's new IDs.

| Earlier ID | Observed retest |
| --- | --- |
| L012 | F1 still does not open help. shots/027-f1.png and shots/key-f1.png; entry and keyslook serial logs. |
| L021 | The period date/time status display is still absent. shots/001-launch.png. CALC and CIRC now exist, so the earlier wholly blank-status observation does not apply to those states. |
| L080 | Entry of the 2049th populated cell still produces unexplained EDIT. Replacing an existing numeric cell works. shots/capacity-2048.png, capacity-2049.png, capacity-replace.png; CAPOUT.WK1 contains 2048 cells. The string-pool exhaustion path also lacks a specific explanation. |
| L022, shared alert appearance | The reserved caution-icon area is blank in the reached 123 save question, shots/quit-ctrlq-0-guard.png. This is the same appearance omission seen in the earlier Empty Trash dialog; the Empty Trash action itself was outside this subsystem and was not replayed. |

## Now fixed

"Fixed" here means the earlier exact symptom did not reproduce in these tests;
it does not certify the entire feature. The new findings above qualify current
editor, file, range, and quit behavior.

| Earlier audit ID | Positive evidence |
| --- | --- |
| 2026-10-05 L008 | Ctrl-Q, File > Quit, close box, and /Quit all guard unsaved work. Save, Cancel, Don't Save, and cancellation of the Save filename prompt ran twice per route. Eight saved files match the intended labels; pending valid input survives Cancel and saves, while invalid pending input leaves EDIT. See evidence/quit-roundtrip-check.json and shots/quit-*.png. |
| 2026-10-05 L009 | Ordinary formulas, arithmetic, ranges, and most listed functions calculate. The real MINT01, MINT03 and MINT05 controls round-trip byte-for-byte; function-specific failures have new findings. |
| 2026-10-05 L010 | Slash menus now work; 381 items were traversed, with all 300 leaves reached. Execution gaps are individually listed above. |
| 2026-10-05 L011 | /File Save and Retrieve persist and restore worksheets. The original "no reached Save" symptom is fixed; full-disk replacement safety is not. |
| 2026-10-05 L013 | F2 loads existing contents and Backspace edits them. The missing F2 entry point is fixed; cursor movement and full-length no-op edits remain defective. |
| 2026-10-05 L014 | F5 accepts valid addresses and names, including IV8192. Invalid IV8193 and BAD! are refused without moving the pointer. GOTO cancellation has a new finding. |
| 2026-10-05 L015 | PgDn moves A1 to A20 in the 19-row native view, and PgUp returns. shots/024-page-down.png. |
| 2026-10-05 L016 | Tab moves A1 to I1; Shift-Tab returns to the previous horizontal page. shots/025-tab.png and entry serial. |
| 2026-10-05 L017 | Clicking selects the visible cell, including after resizing. shots/cell-click.png, resize-hit.png; keyslook serial. |
| 2026-10-05 L018 | After shrinking by 288x194, GOTO E1 retains a visible pointer, and a click maps to F3. shots/resize-pointer.png and resize-hit.png. The separate 240-column-width problem remains. |
| 2026-10-05 L019 | A typed 240-character label is accepted intact; the 241st character is not stored. LONG.WK1 contains the 240 characters plus the implicit label prefix. A subsequent F2 edit is a separate new data-loss defect. |
| 2026-10-05 L020 | Unmodified backslash reaches the label editor; backslash-xy repeats across D1 instead of suspending the desktop. shots/002-prefix-values.png and entry serial. |
| 2026-10-15 M011 | The Cancel button's upper bevel is 54 uninterrupted white pixels, and its lower bevel has the expected 53 shadow pixels. The prior label overpaint is absent. |
| 2026-10-15 M012 | The two previously wrong outer corner pixels now match the dialog background, RGB (231,231,231). The inspected prior corner-fringe symptom is gone. See evidence/known-alert-pixels.json and shots/quit-dialog-look.png. |

## Coverage and things not reached

| Area | What was actually exercised |
| --- | --- |
| Entry | All four requested label prefixes, implicit labels, signed/decimal/exponent values, percent, comma and dollar entry, 1e308/1e-308, long digits, 0.1+0.2, A1 as label versus +A1 as formula; Return and movement commits; F2, Backspace, Escape, attempted cursor keys; 240/241 characters with byte extraction. Commas correctly refuse under the manual's numeric-entry rules; 1e308 is outside the documented +/-99 exponent range. |
| Navigation | F5 valid/bad/named/max addresses, boundary arrows, Home, PgUp/PgDn, Tab/Shift-Tab, End/arrow, F1/F3/F4/F6/F7/F8/F10 attempts, clicks, native scroll controls, grow/shrink/zoom and current-cell visibility. |
| Functions | All 90 names from the engine token table entered; all 90 wrong argument counts refused without committing cells. 100 additional type/domain cases cover all 83 functions with arguments, empty ranges, NA/ERR, huge arguments, date boundaries, and a valid @@ indirection. Additional empty-range database/lookup/financial probes are saved in EMPTY.WK1. This is broad execution coverage, not an independent numerical proof for every function and input. |
| Recalculation | Forward chain, automatic update, Manual stale result plus CALC, F9 update, Natural/Columnwise/Rowwise, circular self-reference and CIRC, iteration counts including 0/51 refusals and 1/50 acceptance. A 2048-formula fixture loaded, calculated, received Ctrl-Break 30 ms after F9, and saved. The screenshot was taken about 1.7 seconds after F9; this does not establish that Break interrupted rather than followed completion. |
| Menus | 381 entries/82 menus; arrow navigation, unique initial-letter navigation for 299 leaves, Escape at every visited level; Quit/Yes separately repeated. All third-line descriptions checked against 123.RI. 224 visible refusals are individual findings, not claims that their downstream workflows ran. |
| Worksheet | Real-Lotus Copy and Move sequences, overlap, mixed and absolute references, refused replicated destination, typed and attempted anchored pointing ranges, widths 1/240 and invalid 0/241, all format families and decimal counts 0..15, invalid decimals -1/16, narrow and 30-character-wide views, erase Y/N, Range Erase, names create/delete/redefine/labels/table/reset, value conversion, transpose, label alignment, global format/width, and protection/unprotection. |
| Files | Save/Retrieve; byte-identical MINT01/03/05 round trips; six Copy/Move cell-record comparisons; malformed, missing, and too-large-cell-count retrieval preserving the current sheet; Cancel/Replace/Backup; long names, bad names, space/wildcard/path failures; C: save; password syntax; two full-disk replacement failures with extracted zero-byte files. |
| Capacity/recovery | Imported 2047 cells then entered cells 2048 and 2049; existing numeric replacement; erase/retry; 2049-cell retrieval refusal; 135 maximum-length labels near the 32 KiB pool boundary, a 136th-label attempt, same-length replacement, shorter replacement; Undo attempt and its unavailable enabling command. |
| Quit/power | Every application exit route, each choice twice, pending input and invalid input, Save filename cancellation, full-disk Save cancellation. Finder shutdown Cancel/Save/Don't Save each ran twice; Discard reached the safe screen, Cancel retained the sheet, Save exposed the continuation defect. |
| Look | Native control panel, modes READY/LABEL/VALUE/EDIT/POINT/MENU/ERROR/FILES, headings, borders, pointer, status indicators, resize geometry, and save-question buttons. Normalized real VGA ninth columns for pixel comparisons: column A heading 0/1152 differing ink pixels; READY 12/640; row border 20/9728. The small inherited glyph differences are documented in evidence/look-metrics.json, not newly recast as rendering regressions. |

The unavailable Insert/Delete, titles, split/synchronized windows, hide/display,
printing, graph rendering/options, data processing, external System programs,
add-ins, and many global defaults cannot be tested beyond their visible refusals.
No physical printer, real-Lotus execution of native-produced files, original
Lotus command replay in a second emulator, or exhaustive arithmetic oracle for
all possible values was performed. The raw Move metadata mismatch is qualified
accordingly. HELP is unavailable; a distinct WAIT frame was not captured. No
audible-beep assessment, video-rate flicker measurement, or overnight soak is
claimed. No general "all bugs found" or production-readiness claim is made.

`M4ROUND.WK1` was explicitly recalculated after retrieval, whereas the original
MINT04 capture was saved before that F9. Its three circular cached results change
from 1 to 8; this is not treated as a like-for-like round-trip failure. Copy
outputs differ only in WINDOW1 from the corresponding real files. Move outputs
have the separately reported record-0x0096 differences, and some WINDOW1 changes.
All six Copy/Move cell record sets are byte-identical to their real references.

Excluded attempts: entry shots 004..009 were contaminated by the discovery that
Ctrl-C inserted c. Initial shots 029..033 were taken while fast sendkey input had
not drained; they do not establish a hang, truncation limit, or file operation.
The synchronized long phase supersedes them. The first full phase never reached
123 because Finder's initial full-disk alert intercepted launch input; full2,
using preexisting Finder metadata, is the valid full-disk test. The host runner
ended with status 143 after the functions Save screenshot; its image lock was
verified released before extraction, and function_errors resumed from the saved
file. No guest finding is based on that host interruption.


## The fifteen things to fix first

This ranking is the auditor's judgment about data safety and period-workstation credibility.

1. Make failed Save/Replace preserve the last good file (L001).
2. Make an unchanged F2 edit preserve every accepted character (L002).
3. Complete the in-line editor and reference-key behavior (L005, L025).
4. Provide working Undo and a visible undo state (L024).
5. Implement insertion/deletion of rows and columns, including reference adjustment.
6. Support Copy to destination ranges and anchored range selection (L006, L007).
7. Make high-precision numeric formats and @STRING work across their documented ranges (L030, L033).
8. Make Backup and file selection usable before users entrust a workbook to the disk (L012, L029).
9. Continue Finder shutdown after a successful asynchronous Save (L003).
10. Keep wide columns visible and make the native scroll controls operate (L008, L027).
11. Recover storage after erasure and permit same-size replacements at capacity (L010, L032; earlier L080).
12. Finish range-name reassignment, display, and name selection (L009, L016, L020).
13. Complete the remaining basic value/attribute behavior: leading $, @NOW, and current filename (L004, L011, L019).
14. Deliver worksheet titles and split/synchronized windows, which the menus already promise.
15. Make the advertised Print and Graph workflows produce real output, then tackle the Data command backlog.

## Deliverable and cleanup checks

This report contains 259 new findings: 2 P1, 250 P2, and 7 P3. NITS.tsv has one matching row per finding. Of these, 224 are the individually observed unavailable menu leaves. The remaining 35 findings concern behavior or presentation beyond those refusals.

22 emulator image sessions were recorded. All normal sessions have explicit QMP quit/exit records. The host-interrupted functions session is separately documented, and all copied-image locks are released. No other emulator or process was killed. See evidence/cleanup-audit.json, cleanup.jsonl, and the per-session manifests.

Evidence validation includes PNG decoding and SHA256 for every cited screenshot, observed pixel-panel matching for every menu refusal, serial-render checks for every selected menu description, independent WK1 parsing, and raw-file comparisons. evidence/validation.json summarizes link, row-count, and ASCII checks.
