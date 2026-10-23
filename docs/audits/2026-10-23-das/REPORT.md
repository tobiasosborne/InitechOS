# Desk accessories, small apps, Control Panels, TPS and film audit

Audit date: 2026-10-23. Supplied image commit: e67b8311092873fe16e6c6d62ad82f894bfb8d7d. This report contains 45 new findings: 9 P1, 23 P2 and 13 P3. New L numbers belong to this report; they are not the L numbers in the older full-drive audit.

The most urgent results are the standalone film panic, the Scrapbook Ctrl-C termination, destructive full-disk text replacement, and TPS claiming success for a truncated report. The normal TPS example is byte-identical to the shipped manual's golden. The ordinary calculator arithmetic tested here agrees with the supplied real-Mac vectors.

All emulators used copied images, QMP on stdio, -display none, -cpu 486 and -m 16. At most one emulator owned by this audit ran at a time. No owner's emulator, X display, source repository file, build, tracker or commit was modified. All guest screenshots are QMP screendumps. Files were extracted with mtools only after their emulator stopped.

## Evidence and replay

- [NITS.tsv](NITS.tsv) is the flat finding index; [findings.json](evidence/findings.json) contains the same structured details.
- [actions.jsonl](evidence/actions.jsonl) records actual guest input with timestamps and serial byte offsets. It includes nested method calls and must not be replayed blindly as a flat command list.
- [replay.py](evidence/replay.py) replays one session's low-level input at its measured timing: `python3 evidence/replay.py copybreak replay-copybreak`. Use a new output label. It starts only one QEMU and stops it in finally. Session JSON files identify the exact input images and emulator arguments.
- [drive.py](evidence/drive.py) is the extended interactive driver; [audit_driver-original.py](evidence/audit_driver-original.py) preserves the supplied driver. The latter still exists at the workspace root as audit_driver.py. The named JSON action suites, prepare scripts and retained prepared disk copies support shorter controlled replays.
- [image-manifest.json](evidence/image-manifest.json), per-session hashes, [cleanup.jsonl](evidence/cleanup.jsonl), and [final-validation.json](evidence/final-validation.json) identify the tested artifacts and cleanup. PIDs can repeat because commands use sandbox PID namespaces; session time and command identify ownership.
- [screenshot-verification.json](evidence/screenshot-verification.json) lists the visually checked finding screenshots. Contact sheets are inspection aids; the PNGs in shots/ retain native 640x480 pixels.
- [harness-notes.txt](evidence/harness-notes.txt) explains mistargeted, early and misnamed captures. Findings use their actual contents, not optimistic filenames.

Coordinates are native guest pixels, with origin at top left. Drag steps give start and delta. Unless otherwise stated, desktop entry is double-click disk (600,64), APPS (123,101), then the executable. Normal APPS launch points are CALC (150,172), TEXT (215,172), PAINT (285,172), CALCULAT (78,224). Apple menu rows are About (90,27), Calculator (90,49), Control Panels (90,65), Note Pad (90,81), Scrapbook (90,97), reached from (20,10). Ctrl is the guest's displayed command-key convention.

Reference paths beginning os/, spec/ or pubs/ are under /home/tobias/Projects/initech-os. Named System 7/Mac OS 8 references are under /home/tobias/Projects/system7-decomp; the Macintosh guide is /home/tobias/Projects/initech-manuals-ref/macos/Apple_Macintosh_System_Software_Users_Guide_V6.0.pdf. Reference copies and inspected manual pages/excerpts are in evidence/references/. Expectations based on memory or ordinary data/UI contracts are identified as such. Source diagnoses are guesses, not substitutes for observed evidence.

## Findings

### L001 - The required standalone film boot panics

Area: Film boot. Severity: P1.

Steps: Boot a copy of flair_live_interactive.img alone, -cpu 486 -m 16 -display none. Move to (470,228), wait 15 seconds, press Return, drag (320,210) by (100,100), Escape, Ctrl-Q, and try the upper Special menu. Session film.

Observed: The initial screen says PC LOAD LETTER (FLAIR heap exhausted). Serial records FAT mount failure, PANIC ... film CALC.EXE launch, then HALTED. None of those inputs changes it. Adding flair_data.img in the independent filmdata session permits boot.

Expected and basis: BRIEF.md explicitly requires this image alone. A missing secondary disk must not panic a supplied demonstration image; explain the dependency or include its assets. The deliberate panic wording is not the defect.

Evidence: [shots/180-film-rest.png](shots/180-film-rest.png), [shots/185-film-panic-unresponsive.png](shots/185-film-panic-unresponsive.png), [shots/186-film-data-rest.png](shots/186-film-data-rest.png), [evidence/film-serial.log](evidence/film-serial.log), [evidence/filmdata-serial.log](evidence/filmdata-serial.log).

Guess: os/milton/kmain.c:7086 routes failed film CALC launch through flair_desktop_oom even after the data mount failed.

### L002 - Copy can terminate the desktop through DOS Ctrl-C

Area: Scrapbook / DOS boundary. Severity: P1.

Steps: Boot tenants with images/main-data.img (one saved Scrapbook clip). Apple menu: drag (20,10) to (90,97). Wait for the clip, press Ctrl-C, wait two seconds. Then try Ctrl-V, Escape and a title drag (320,90) by (60,40). Sessions scraplimit and independent copybreak.

Observed: Immediately after Copy, ^C and Program terminated. overwrite the upper left of the GUI. Serial records the successful Copy menu dispatch and Scrap put, then INT23-BREAK and INT21-EXIT rc=0. The title drag and subsequent keys do not work. The failure already exists before Paste in the isolated replay.

Expected and basis: Copy must populate the clipboard while leaving the desktop usable. Macintosh System Software User's Guide, printed pp.125,152 (PDF pp.145,172), describes accessories operating alongside documents and copying clips. This is a crash/hang, not an intentional DOS prompt break.

Evidence: [shots/262-copybreak-before.png](shots/262-copybreak-before.png), [shots/263-copybreak-after-copy.png](shots/263-copybreak-after-copy.png), [shots/265-copybreak-unresponsive.png](shots/265-copybreak-unresponsive.png), [evidence/copybreak-serial.log](evidence/copybreak-serial.log), [evidence/scraplimit-serial.log](evidence/scraplimit-serial.log).

Guess: a GUI Ctrl-C reaches the pending DOS break path; os/milton/int21.c:5319-5343 prints ^C and terminates the current PSP.

### L003 - A failed full-disk replacement destroys the original file

Area: InitechText save. Severity: P1.

Steps: Use evidence/prepare.py's full-data.img, zero free bytes, KEEP.TXT containing 750 bytes. Launch TEXT, Ctrl-O, Down seven times, Return to load K4.TXT (4096 bytes). File > Save As at (50,30)->(100,121); Backspace six times, type KEEP.TXT, Return. Cancel replacement once, then Return twice to accept it. Session fulltest.

Observed: The editor says the document could not be saved. The stopped image contains KEEP.TXT with length zero; all 750 original bytes are gone. K4.TXT remains 4096 bytes. Cancellation before the second attempt returned to the chooser.

Expected and basis: Ordinary safe-save expectation: an unsuccessful replacement must retain the old file, or a recoverable backup. Reporting failure does not justify truncating the original.

Evidence: [shots/122-full-replace-prompt.png](shots/122-full-replace-prompt.png), [shots/123-full-replace-cancel.png](shots/123-full-replace-cancel.png), [shots/124-full-write-failed.png](shots/124-full-write-failed.png), [evidence/full-before.txt](evidence/full-before.txt), [evidence/extracted-full/comparison.json](evidence/extracted-full/comparison.json), [evidence/extracted-full/KEEP.TXT](evidence/extracted-full/KEEP.TXT), [evidence/fixtures/KEEP.TXT](evidence/fixtures/KEEP.TXT), [evidence/fulltest-serial.log](evidence/fulltest-serial.log).

Guess: os/apps/itext/itext.c:329 creates/truncates the target before the write is known to fit.

### L004 - TPS reports success for a truncated report on a full disk

Area: TPS output. Severity: P1.

Steps: Boot tracer_boot.img with tps-full-source.img from prepare_full_tps.py. CD APPS; TPSREP TPS.DBF; seven spaces and Return; 004417 twice; 88-114; Y. Session tpsfull.

Observed: The full report is shown on screen, followed by TPS REPORT WRITTEN TO TPSREPT.TXT and RETURN CODE 0. Extraction finds only 512 bytes of TPSREPT.TXT, ending within the cover sheet; the normal example is 2432 bytes. The previous TXT was retained as BAK.

Expected and basis: Shipped manual pubs/manuals/tps-report-generator/s1-reference.tex:184 defines return code 0 as "The report was written". A short or failed write must be an error and must not claim a complete report.

Evidence: [shots/161-tps-full-result.png](shots/161-tps-full-result.png), [evidence/tpsfull-serial.log](evidence/tpsfull-serial.log), [evidence/tps-full-before.txt](evidence/tps-full-before.txt), [evidence/extracted-tps-full/TPSREPT.TXT](evidence/extracted-tps-full/TPSREPT.TXT), [evidence/extracted-tps/comparison.json](evidence/extracted-tps/comparison.json).

Guess: os/milton/tpsrep_program.asm:822-831 discards the return status of FCB AH=15h in wrrec.

### L005 - LF-only blank lines disappear and are lost on Save

Area: InitechText line endings. Severity: P1.

Steps: Boot fixture-data.img; launch TEXT. Ctrl-O, Down four times, Return opens LF.TXT containing bytes ONE\n\nTHREE\n. Press Ctrl-S; stop and extract. Control: open CRLF.TXT with Down five times and save it. Session text.

Observed: LF.TXT displays ONE immediately above THREE. Saved bytes are ONE\rTHREE\r (10 bytes), losing the empty line. CRLF input retains its blank line and is correctly normalized to CR (11 bytes).

Expected and basis: A plain-text editor must preserve the number of lines when normalizing line endings. This follows ordinary text-file fidelity and the editor's documented CR/LF normalization in os/apps/itext/itext.c:33-38.

Evidence: [shots/076-text-lf.png](shots/076-text-lf.png), [shots/077-text-crlf.png](shots/077-text-crlf.png), [evidence/extracted-text/comparison.json](evidence/extracted-text/comparison.json), [evidence/fixtures/LF.TXT](evidence/fixtures/LF.TXT), [evidence/extracted-text/LF.TXT](evidence/extracted-text/LF.TXT), [evidence/text-serial.log](evidence/text-serial.log).

Guess: os/apps/itext/itext.c:308 checks the already-normalized output byte for CR, so a second LF is mistaken for the LF half of CRLF.

### L006 - Save does not save the editable worksheet

Area: InitechCalc save. Severity: P1.

Steps: Launch CALC.EXE at (150,172) in APPS. Type 123, press Down to commit into A1. Ctrl-S; wait six seconds for FILE COPY to finish; Ctrl-Q; relaunch. Session apps.

Observed: A1 visibly changes from Film Costs to 123. Save finishes successfully, but reopening restores Film Costs. Extracted APPS/TABLES.TXT is byte-for-byte the shipped LEDGER.TXT and contains no 123 edit.

Expected and basis: An enabled Save in an editable document must preserve its current content. This is an ordinary document contract; the film's caption and deliberately odd numbers do not exempt editable values from persistence.

Evidence: [shots/102-ledger-committed.png](shots/102-ledger-committed.png), [shots/104-ledger-save-finished.png](shots/104-ledger-save-finished.png), [shots/106-ledger-reopened.png](shots/106-ledger-reopened.png), [evidence/extracted-apps/TABLES.TXT](evidence/extracted-apps/TABLES.TXT), [evidence/extracted-apps/LEDGER.TXT](evidence/extracted-apps/LEDGER.TXT), [evidence/apps-serial.log](evidence/apps-serial.log).

Guess: os/apps/calc/calc.c:171-175 copies the fixed source path and ignores the in-memory sheet.

### L007 - Quit discards worksheet edits without an unsaved-work prompt

Area: InitechCalc quit. Severity: P1.

Steps: In filmdata dismiss the staged modal with Escape; activate the ledger at (100,90), type 77 and press Down. Ctrl-Q. The same Quit path is available in disk-launched CALC.

Observed: The edited 77 cell is visible; Ctrl-Q removes both ledger and chart without Save / Cancel / Don't Save. No file dialog or warning appears.

Expected and basis: My period document-application expectation: guard unsaved edits, as InitechText itself does. This is separate from L006: quitting without first asking for Save also loses edits.

Evidence: [shots/190-film-ledger-edit.png](shots/190-film-ledger-edit.png), [shots/193-film-quit.png](shots/193-film-quit.png), [evidence/filmdata-serial.log](evidence/filmdata-serial.log).

Guess: os/apps/calc/calc.c:170 exits directly on Quit or GOAWAY, with no dirty state.

### L008 - A bad checksum silently becomes empty pages and overwrites recoverable notes

Area: Note Pad persistence. Severity: P1.

Steps: Prepare corrupt-da-source.img by copying main-data.img and flipping only byte 12 of NOTEPAD.DAT (checksum); all page bytes remain intact. Launch Note Pad through Apple (20,10)->(90,81), close with Ctrl-W, reopen. Session dalife; fixture bytes retained.

Observed: It opens as empty page 1 of 8, without an alert. Serial alone says DA-CORRUPT ... empty pages. Closing saves empty pages; reopening loads that replacement. The prior First line, Page two sentinel and Last page payloads are lost. Later the new SHUTDOWN SENTINEL is saved in page 1.

Expected and basis: Ordinary recovery expectation: warn about unreadable storage and preserve it before replacing it. A one-byte checksum error must not silently destroy otherwise recoverable page content.

Evidence: [shots/233-note-corrupt-open.png](shots/233-note-corrupt-open.png), [shots/235-note-corrupt-reopened.png](shots/235-note-corrupt-reopened.png), [evidence/dalife-serial.log](evidence/dalife-serial.log), [evidence/fixtures/CORRUPT.DAT](evidence/fixtures/CORRUPT.DAT), [evidence/extracted-main/decoded.json](evidence/extracted-main/decoded.json), [evidence/extracted-dalife/NOTEPAD.DAT](evidence/extracted-dalife/NOTEPAD.DAT), [evidence/extracted-dalife/comparison.json](evidence/extracted-dalife/comparison.json).

Guess: os/apps/notepad/notepad.c:116-118 retains defaults after validation fails; :146 and close_app later recreate the same path unconditionally.

### L009 - Save replaces an existing TABLES.TXT without identifying it or asking

Area: InitechCalc file copy. Severity: P1.

Steps: Boot copy-hundred-source.img, containing an existing APPS/TABLES.TXT with ORIGINAL TABLES MUST SURVIVE FAILED COPY and a 1000-line LEDGER.TXT. Launch CALC and Ctrl-S. Session copy-hundred.

Observed: Only the Saving tables to disk... progress box appears. There is no destination or replacement confirmation. TABLES.TXT becomes the 17000-byte ledger; the original 42 bytes are gone.

Expected and basis: BRIEF.md explicitly requires that the report/ledger workflow not overwrite data without saying so. A destination different from the edited worksheet must be identified and existing unrelated content protected.

Evidence: [shots/172-copy-hundred-start.png](shots/172-copy-hundred-start.png), [shots/173-copy-hundred-end.png](shots/173-copy-hundred-end.png), [evidence/copy-fixtures/old-table.txt](evidence/copy-fixtures/old-table.txt), [evidence/extracted-copy/hundred-TABLES.TXT](evidence/extracted-copy/hundred-TABLES.TXT), [evidence/extracted-copy/summary.json](evidence/extracted-copy/summary.json), [evidence/copy-hundred-serial.log](evidence/copy-hundred-serial.log).

Guess: os/flair/tbxgate.c:1755-1768 opens the fixed destination with create/truncate and has no replacement question.

### L010 - Shift-arrow moves the caret instead of extending a selection

Area: InitechText keyboard. Severity: P2.

Steps: Type First line, Return, Second line, Return, Third line. Press Shift-Left twice. Repeat on the end of Note Pad's LAST text. Sessions main and text.

Observed: The caret moves two positions but no text is selected. Note Pad behaves the same way. Ctrl-A is a distinct working path.

Expected and basis: My period Macintosh text-editing expectation: Shift-arrow extends a selection. The brief explicitly requests this interaction.

Evidence: [shots/025-text-typed.png](shots/025-text-typed.png), [shots/026-text-shift-select.png](shots/026-text-shift-select.png), [shots/097-note-long.png](shots/097-note-long.png), [shots/098-note-shift.png](shots/098-note-shift.png), [evidence/main-serial.log](evidence/main-serial.log), [evidence/text-serial.log](evidence/text-serial.log).

Guess: os/apps/itext/itext.c key() passes arrow codes without using the Shift modifier; Note Pad uses the same pattern.

### L011 - Undo cannot recover a cut

Area: InitechText editing. Severity: P2.

Steps: Type the three-line document, Ctrl-A, Ctrl-C, Ctrl-X, then Ctrl-Z. Session main.

Observed: The document remains empty after Ctrl-Z; Paste restores it. The command has no undo effect.

Expected and basis: Named reference: goldens/captures/s8_simpletext_editmenu.png, copied to evidence/references, shows the period Undo command. Ordinary text editing requires recovering an accidental destructive edit.

Evidence: [shots/027-text-cut-undo.png](shots/027-text-cut-undo.png), [shots/028-text-save-dialog.png](shots/028-text-save-dialog.png), [evidence/references/s8_simpletext_editmenu.png](evidence/references/s8_simpletext_editmenu.png), [evidence/main-serial.log](evidence/main-serial.log).

Guess: os/apps/itext/itext.c declares Undo disabled and implements no undo action.

### L012 - Resizing does not rewrap text or make clipped text reachable

Area: InitechText geometry. Severity: P2.

Steps: Open fixture WRAP.TXT (Ctrl-O, Down ten, Return). Drag grow box (576,466) by (-250,-220). Session text.

Observed: The original long wrapped lines remain at their original width and are clipped at x=317. The right halves of lines are invisible, while the horizontal scrollbar remains disabled-looking. The smaller window has room for more wrapped lines but does not reflow them.

Expected and basis: Named reference: system7-decomp/specs/toolbox/textedit.md describes destination/view rectangles and layout. My GUI expectation is reflow or functional horizontal scrolling when the user changes document width.

Evidence: [shots/083-text-wrap-original.png](shots/083-text-wrap-original.png), [shots/087-text-wrap-shrunk.png](shots/087-text-wrap-shrunk.png), [shots/088-text-caret-offscreen.png](shots/088-text-caret-offscreen.png), [evidence/text-serial.log](evidence/text-serial.log).

Guess: os/apps/itext/itext.c uses compile-time CONT_W/CONT_H/WRAP_W rather than current window geometry.

### L013 - Scrolling during typing skips a visible line until a full repaint

Area: InitechText repaint. Severity: P2.

Steps: Open LINES25.TXT (Down eleven in Open), Ctrl-A, Right, Return, type LAST slowly. Capture, then drag title (300,54) by (30,0) and back. Session text.

Observed: Before the drag the visible sequence jumps from Line 23 to Line 25. After repaint Line 24 reappears in between; the document itself was not changed by moving the window.

Expected and basis: A scrolled editor must paint every visible line at the right offset. This is ordinary rendering correctness, not a claim of lost file bytes.

Evidence: [shots/093-text-scroll-before-refresh.png](shots/093-text-scroll-before-refresh.png), [shots/094-text-scroll-after-refresh.png](shots/094-text-scroll-after-refresh.png), [evidence/fixtures/LINES25.TXT](evidence/fixtures/LINES25.TXT), [evidence/text-serial.log](evidence/text-serial.log).

Guess: itext.c show_caret() changes g_scroll before comparing the old scroll value, allowing a partial rather than full repaint.

### L014 - Typing at the 4096-byte limit is silently discarded

Area: InitechText capacity. Severity: P2.

Steps: Open K4.TXT (Ctrl-O, Down seven, Return), exactly 4096 bytes. With caret at the start, type ! and Save. Stop and extract. Session text.

Observed: Nothing is inserted and no full-document alert is shown. The saved file is identical to the original 4096 bytes. 32 KB and 64 KB files, by contrast, get an explicit too-large alert.

Expected and basis: Ordinary capacity feedback: tell the user that an edit could not be accepted. The brief asks for boundary behavior; the limit itself should also be discoverable.

Evidence: [shots/081-text-4k.png](shots/081-text-4k.png), [shots/082-text-capacity-silent.png](shots/082-text-capacity-silent.png), [shots/079-text-32k.png](shots/079-text-32k.png), [evidence/extracted-text/comparison.json](evidence/extracted-text/comparison.json), [evidence/text-serial.log](evidence/text-serial.log).

Guess: itext.c replace_sel() returns immediately when FlairTECanReplace fails, without calling say().

### L015 - Every tested high-bit character is drawn as a box

Area: InitechText font coverage. Severity: P2.

Steps: Open EIGHT.TXT (Ctrl-O, Down six, Return), containing ASCII followed by bytes 80h through 9Fh. Save and extract. Session text.

Observed: The entire 32-character high-bit run renders as identical empty boxes. The bytes survive Save unchanged, so this is rendering loss rather than file-byte corruption.

Expected and basis: My classic Mac text expectation is a defined MacRoman glyph repertoire including accented letters. These bytes must be decoded and rendered, or the encoding limitation clearly identified.

Evidence: [shots/078-text-eightbit.png](shots/078-text-eightbit.png), [evidence/fixtures/EIGHT.TXT](evidence/fixtures/EIGHT.TXT), [evidence/extracted-text/comparison.json](evidence/extracted-text/comparison.json), [evidence/text-serial.log](evidence/text-serial.log).

### L016 - The filename field ignores selection and cursor-editing keys

Area: Standard File editing. Severity: P2.

Steps: In Text's first Save dialog with UNTITLED, press Ctrl-A and type AUDIT.TXT. Separately try Home, Shift-End and type again. Sessions main, shots 029/035/036.

Observed: Ctrl-A does not select the name: an A is appended, then the new filename is appended until the field limit. Home and Shift-End do not select or reposition editing. Backspacing the entire existing name is required.

Expected and basis: Named reference: s8_sf_saveas_default.png shows editable default-name selection; ordinary text-field commands should not insert a printable letter from a command chord.

Evidence: [shots/029-text-saved.png](shots/029-text-saved.png), [shots/035-save-name.png](shots/035-save-name.png), [shots/036-save-result.png](shots/036-save-result.png), [evidence/references/s8_sf_saveas_default.png](evidence/references/s8_sf_saveas_default.png), [evidence/main-serial.log](evidence/main-serial.log).

Guess: os/milton/kmain.c:5092-5109 appends printable key bytes without respecting modifiers or a selection/caret.

### L017 - An invalid 8.3 name makes Save silently do nothing

Area: Standard File validation. Severity: P2.

Steps: Reach the filename UNTITLEDAUDI in the prior test (12 letters, no dot). Click Save at (440,292) and press Return. Then clear the field and use valid AUDIT.TXT as control.

Observed: The invalid-name chooser remains open with an enabled Save button and no error text. The valid name succeeds. The field permits a name which its own confirmation refuses without explanation.

Expected and basis: Ordinary validation expectation: explain the 8.3 requirement or prevent invalid input visibly. A live default button should not silently ignore confirmation.

Evidence: [shots/036-save-result.png](shots/036-save-result.png), [shots/037-save-valid-name.png](shots/037-save-valid-name.png), [shots/038-saved-quit.png](shots/038-saved-quit.png), [evidence/main-serial.log](evidence/main-serial.log).

Guess: os/milton/kmain.c:5017-5028 and :5140 reject sf_name_ok without an alert or disabled-state update.

### L018 - Return does not commit an entered cell

Area: InitechCalc keyboard. Severity: P2.

Steps: Launch CALC; slowly type 123; press Return; then press Down. Session apps.

Observed: Return leaves the cell as Film Costs and 123 in the entry area. Down commits 123 into A1. This was slow input, not an unsettled fast-typing screenshot.

Expected and basis: Ordinary worksheet entry expectation: Return commits the entry. The source itself intends a Return branch, and the brief asks for keyboard behavior.

Evidence: [shots/101-ledger-return.png](shots/101-ledger-return.png), [shots/102-ledger-committed.png](shots/102-ledger-committed.png), [evidence/apps-serial.log](evidence/apps-serial.log).

Guess: os/apps/calc/calc.c:186 recognizes CR only, while this guest cooks Return to LF (handled explicitly by InitechText).

### L019 - Save failures caused by configuration or missing input are invisible

Area: InitechCalc error feedback. Severity: P2.

Steps: Run copy-zero, copy-bad, copy-empty, copy-missing and missingledger sessions. Their prepared disks respectively use rate 0, banana, empty MICHAEL.CFG, absent MICHAEL.CFG, and valid config with absent LEDGER.TXT. Launch CALC and Ctrl-S; wait seven seconds.

Observed: The worksheet stays on screen with no error or notice. Serial says TENANT-FILECOPY-REFUSED (for example -3 or -258). Extracted prior TABLES.TXT remains intact in the config-error cases.

Expected and basis: A failed Save must give visible failure and identify the affected input or configuration. This is an ordinary named-command contract. Invalid inputs need not be accepted.

Evidence: [shots/171-copy-zero-end.png](shots/171-copy-zero-end.png), [shots/175-copy-bad-end.png](shots/175-copy-bad-end.png), [shots/177-copy-empty-end.png](shots/177-copy-empty-end.png), [shots/179-copy-missing-end.png](shots/179-copy-missing-end.png), [shots/251-copy-missingledger.png](shots/251-copy-missingledger.png), [evidence/extracted-copy/summary.json](evidence/extracted-copy/summary.json), [evidence/copy-zero-serial.log](evidence/copy-zero-serial.log), [evidence/missingledger-serial.log](evidence/missingledger-serial.log).

Guess: os/apps/calc/calc.c:175 ignores the FILECOPY completion/error event.

### L020 - The real copy operation cannot be cancelled from its progress dialog

Area: InitechCalc file copy. Severity: P2.

Steps: In apps, invoke Ctrl-S a second time and immediately press Escape. Try Ctrl-Q and background clicks while it runs.

Observed: Escape leaves FILE COPY running. No Cancel button is present. Inputs remain blocked until it completes; serial records a second successful 156-byte copy. This concerns the real copy, not the film's staged saving prop.

Expected and basis: My period long-operation expectation: a visible Cancel path for a modal copy. BRIEF.md explicitly asks to test Cancel.

Evidence: [shots/107-ledger-copy-cancel.png](shots/107-ledger-copy-cancel.png), [shots/108-paint.png](shots/108-paint.png), [shots/109-paint-draw.png](shots/109-paint-draw.png), [evidence/apps-serial.log](evidence/apps-serial.log).

### L021 - Calculator has no usable Copy or Paste

Area: Calculator clipboard. Severity: P2.

Steps: Launch CALCULAT, enter a result, pull Edit at (95,30). Session main.

Observed: Copy and Paste are gray along with every other Edit row. The result cannot be transferred through the expected accessory commands.

Expected and basis: Named reference: Macintosh System Software User's Guide V6.0, printed p.127 / PDF p.147: "You can copy the calculation results" and paste numbers into the display. The inspected page is preserved as a PNG.

Evidence: [shots/024-calc-edit-settled.png](shots/024-calc-edit-settled.png), [evidence/references/mac-calc-147.png](evidence/references/mac-calc-147.png), [evidence/main-serial.log](evidence/main-serial.log).

Guess: os/apps/calcda/calcda.c declares every Edit item disabled.

### L022 - An open Calculator silently prevents Shut Down

Area: Calculator lifecycle. Severity: P2.

Steps: With CALCULAT open, click the disk at (600,64) to make Finder foreground; Special > Shut Down at (195,30)->(215,123). Wait. Session calcdeep.

Observed: The desktop remains running without an explanatory alert. Serial says FINDER-POWER-CANCEL app=CALCULAT.EXE cannot-ask. Paint, Note Pad and Text provide working shutdown paths in controls.

Expected and basis: Shut Down should close a calculator with no document to save, or tell the user which accessory must be closed. Source comments at kmain.c:5700 promise cancellation loudly, but the only explanation is serial.

Evidence: [shots/232-calc-shutdown.png](shots/232-calc-shutdown.png), [evidence/calcdeep-serial.log](evidence/calcdeep-serial.log), [shots/260-paint-shutdown-refusal.png](shots/260-paint-shutdown-refusal.png), [shots/243-text-shutdown-completed.png](shots/243-text-shutdown-completed.png).

Guess: os/milton/kmain.c:5716-5723 relies on tbx_ask_quit; Calculator supplies Close W but no Quit Q or ask-close handler.

### L023 - The paint application cannot draw

Area: InitechPaint tools. Severity: P2.

Steps: Launch PAINT.EXE at (285,172). Drag across the white canvas from (400,220) by (150,100). Inspect the window and all eight menus. Session painttps.

Observed: The canvas remains blank. There is no toolbox, brush, eraser, fill, selection tool or pattern palette; the corresponding menu commands are all gray.

Expected and basis: My period paint-program expectation is that a program presented as InitechPaint can create an image. The source calls it a deliberate bar carrier, but BRIEF.md only exempts the stacked bars and specified scene props, not an unusable paint app.

Evidence: [shots/126-paint-actual.png](shots/126-paint-actual.png), [shots/127-paint-draw-actual.png](shots/127-paint-draw-actual.png), [shots/129-paint-edit-actual.png](shots/129-paint-edit-actual.png), [shots/132-paint-select-actual.png](shots/132-paint-select-actual.png), [shots/134-paint-window-actual.png](shots/134-paint-window-actual.png), [evidence/painttps-serial.log](evidence/painttps-serial.log).

Guess: os/apps/paint/paint.c implements only white-fill update and Quit.

### L024 - Paint cannot open an image

Area: InitechPaint files. Severity: P2.

Steps: Launch PAINT and pull File at (20,30) and inspect the Open row. Session painttps.

Observed: Open is disabled; no image chooser can be reached. New and Close in the same menu are also gray (the window close box does work).

Expected and basis: My period paint-program expectation: opening an existing picture is a basic file operation. This is separate from the absent drawing tools.

Evidence: [shots/128-paint-file-actual.png](shots/128-paint-file-actual.png), [evidence/painttps-serial.log](evidence/painttps-serial.log).

### L025 - Paint provides no image save operation

Area: InitechPaint files. Severity: P2.

Steps: In PAINT pull File and inspect Save. Session painttps.

Observed: Save is disabled and no save chooser or output is available. The canvas has no persistence route.

Expected and basis: My period paint-program expectation: preserve an image to disk. This is a distinct missing file operation from Open.

Evidence: [shots/128-paint-file-actual.png](shots/128-paint-file-actual.png), [evidence/painttps-serial.log](evidence/painttps-serial.log).

### L026 - Long clip text cannot be scrolled with the visible scrollbar

Area: Scrapbook long clips. Severity: P2.

Steps: Boot text-data.img and open Scrapbook, which holds one 27-line TEXT clip. Click the down arrow at (466,331) three times, then press Down fifteen times. Session lastui.

Observed: Mouse arrow clicks leave the view at Line 01 through Line 18. The same clip scrolls to its LAST line using Down keys. Its visible scrollbar is disabled-looking because it reflects clip count, not hidden text height.

Expected and basis: My GUI expectation: content below the visible region must have discoverable mouse scrolling. An available keyboard-only escape does not make the disabled-looking scrollbar truthful.

Evidence: [shots/257-scrap-long-before-mouse.png](shots/257-scrap-long-before-mouse.png), [shots/258-scrap-long-mouse-no-scroll.png](shots/258-scrap-long-mouse-no-scroll.png), [shots/259-scrap-long-key-scroll.png](shots/259-scrap-long-key-scroll.png), [evidence/lastui-serial.log](evidence/lastui-serial.log).

Guess: os/apps/notepad/notepad.c bar() uses g_count*DA_VIEW_H in SCRAPBOOK mode while Down changes a separate text scroll value.

### L027 - Applying preferences erases the upper menu bar

Area: Control Panels repaint. Severity: P2.

Steps: Open Control Panels, Desktop Patterns, choose Diagonal at (276,252), OK (465,342). Also change Menu Bar Clock and Mouse settings in cpdeep. Capture immediately after apply and after exiting the panel.

Observed: The upper strip becomes the desktop pattern, except for the clock; later menu activity restores only pieces. After Mouse/Clock changes the same strip is teal with just the clock. Restart restores the entire bar.

Expected and basis: Visible system menus must survive applying settings. This is ordinary UI correctness and is separate from the intentional existence of two bars.

Evidence: [shots/062-pattern-applied.png](shots/062-pattern-applied.png), [shots/069-special-menu.png](shots/069-special-menu.png), [shots/071-after-restart.png](shots/071-after-restart.png), [shots/249-mouse-fast-motion.png](shots/249-mouse-fast-motion.png), [evidence/main-serial.log](evidence/main-serial.log), [evidence/cpdeep-serial.log](evidence/cpdeep-serial.log).

### L028 - A nonnumeric page length is silently treated as one

Area: TPS numeric parameters. Severity: P2.

Steps: Run the BADNUM fixture: TPSPARM.DAT contains LINES=banana with all other shipped values. Complete the normal seven-space / 004417 twice / 88-114 / Y ritual. Session tpscases.

Observed: It prints successfully with 13 one-detail-line pages and return code 0. The output matches the explicit LINES=0 pagination control.

Expected and basis: Shipped manual s1-reference.tex:122-123 says "The number of detail lines on each page of the report" and only documents 0 as 1. My validation expectation is to reject a nonnumeric page length, not silently reinterpret it.

Evidence: [shots/154-tps-badnum.png](shots/154-tps-badnum.png), [evidence/tps-fixtures/BADNUM.PAR](evidence/tps-fixtures/BADNUM.PAR), [evidence/extracted-tps-cases/BADNUM.TXT](evidence/extracted-tps-cases/BADNUM.TXT), [evidence/extracted-tps-cases/summary.json](evidence/extracted-tps-cases/summary.json), [evidence/tpscases-serial.log](evidence/tpscases-serial.log).

Guess: tpsrep_program.asm:133-139 atoi() returns zero for no digits, then substitutes one.

### L029 - Malformed numeric PAGES data becomes zero without a warning

Area: TPS numeric data. Severity: P2.

Steps: Run BADPAGE, whose first record PAGES field is XYZ instead of 12; all field descriptors and other records are unchanged. Complete the normal ritual. Session tpscases.

Observed: The report accepts the record and reports 103 pages instead of the normal 115, with return code 0 and no malformed-number warning.

Expected and basis: The manual identifies PAGES as a numeric field (s1-reference.tex:140, "PAGES ... Numeric ... 3"). My data-quality expectation is a clear refusal for invalid numeric bytes, so an apparently authoritative total cannot conceal damaged input.

Evidence: [shots/157-tps-badpage.png](shots/157-tps-badpage.png), [evidence/tps-fixtures/BADPAGE.DBF](evidence/tps-fixtures/BADPAGE.DBF), [evidence/extracted-tps-cases/BADPAGE.TXT](evidence/extracted-tps-cases/BADPAGE.TXT), [evidence/extracted-tps-cases/summary.json](evidence/extracted-tps-cases/summary.json), [evidence/tpscases-serial.log](evidence/tpscases-serial.log).

### L030 - Reopening the running Note Pad produces a generic launch error

Area: Accessory launch feedback. Severity: P2.

Steps: Open Note Pad through the Apple menu; choose the same Note Pad item again. Session dalife.

Observed: Instead of activating the existing Note Pad, an alert says The application could not be opened. The same vague message appears when trying Scrapbook while Note Pad is open. It does not explain the one-disk-app limit.

Expected and basis: Named reference: Macintosh System Software User's Guide, printed p.125 / PDF p.145, says choosing the accessory again makes its window active. Explain the capacity limit for different apps.

Evidence: [shots/234-note-duplicate-launch.png](shots/234-note-duplicate-launch.png), [shots/044-scrapbook-second-app.png](shots/044-scrapbook-second-app.png), [evidence/dalife-serial.log](evidence/dalife-serial.log), [evidence/main-serial.log](evidence/main-serial.log).

### L031 - The default film ledger window clips its own total and has no scroll route

Area: Film document layout. Severity: P2.

Steps: Boot film image with flair_data.img (the standalone boot is L001). Escape dismisses the staged modal. Activate the ledger and move the chart aside. Inspect the ledger lower edge at y=279. Session filmdata.

Observed: The ledger retains a small (60,80)-(360,300) frame while its app content uses the larger CALC layout. Its total is absent below the content bottom and the scrollbar is disabled-looking. The normal disk CALC window visibly shows $112,276.87.

Expected and basis: A supplied rest-state frame must fit its important content. The photo reference shows the total at x=950..1065,y=627..646; the deliberate amount itself is not challenged.

Evidence: [shots/187-film-data-escape.png](shots/187-film-data-escape.png), [shots/190-film-ledger-edit.png](shots/190-film-ledger-edit.png), [shots/100-ledger-launch.png](shots/100-ledger-launch.png), [evidence/references/film-preview.webp](evidence/references/film-preview.webp), [evidence/filmdata-serial.log](evidence/filmdata-serial.log).

Guess: kmain.c film scene repositions/resizes the CALC windows without adapting calc.c compile-time content layout.

### L032 - Holding a calculator key gives no pressed-state feedback

Area: Calculator feedback. Severity: P3.

Steps: Press and hold the 7 key at (285,175) for half a second, capture before release. Session main.

Observed: 7 is entered, but the key face stays white and raised while held.

Expected and basis: Named reference: system7-decomp/goldens/captures/s8_calc_key7_pressed.png shows the real Calculator key inverted during the hold.

Evidence: [shots/023-calc-held-seven-settled.png](shots/023-calc-held-seven-settled.png), [evidence/references/s8_calc_key7_pressed.png](evidence/references/s8_calc_key7_pressed.png), [evidence/main-serial.log](evidence/main-serial.log).

Guess: calcda.c command() repaints only the readout and has no key-up/pressed-state path.

### L033 - The text area keeps the arrow instead of an I-beam

Area: InitechText cursor. Severity: P3.

Steps: Open WRAP.TXT, move pointer to (70,110) inside text and wait. Session text.

Observed: The arrow remains over the text editing surface.

Expected and basis: My period TeachText/SimpleText expectation, also explicit in BRIEF.md: use an I-beam over editable text.

Evidence: [shots/084-text-pointer.png](shots/084-text-pointer.png), [evidence/text-serial.log](evidence/text-serial.log).

### L034 - The insertion caret remains static during idle samples

Area: InitechText caret. Severity: P3.

Steps: With WRAP.TXT open and caret at the start, capture, wait one second, capture, wait another second and capture. Session text.

Observed: The black caret at x=14,y=70..85 is unchanged across the sampled idle frames. This is a bounded observation, supported by the source's explicit no-blink limitation.

Expected and basis: My period Macintosh text-editor expectation: a blinking insertion point communicates focus. The source explicitly acknowledges no idle blink events; this is an observed polish gap, not an inferred implementation-only defect.

Evidence: [shots/084-text-pointer.png](shots/084-text-pointer.png), [shots/085-text-caret1.png](shots/085-text-caret1.png), [shots/086-text-caret2.png](shots/086-text-caret2.png), [evidence/text-serial.log](evidence/text-serial.log).

### L035 - The default filename is not selected for replacement

Area: Standard File default name. Severity: P3.

Steps: Open a new Text document, type any text, Ctrl-S. Inspect the default UNTITLED name before typing.

Observed: The name has an insertion caret at its end rather than selected text. Ordinary typing appends to UNTITLED.

Expected and basis: Named reference: s8_sf_saveas_default.png shows the default name selected. This visual/default-state nit is separate from L016's failure to support selection commands.

Evidence: [shots/028-text-save-dialog.png](shots/028-text-save-dialog.png), [evidence/references/s8_sf_saveas_default.png](evidence/references/s8_sf_saveas_default.png).

### L036 - The displayed time freezes while the panel stays open

Area: Date & Time display. Severity: P3.

Steps: Open Date & Time in main at 12:37. Leave it open through the year/month/day tests until the menu clock advances to 12:38. Cancel and reopen.

Observed: The panel still says Time: 12:37 while the menu clock says 12:38. Reopening updates it to 12:38.

Expected and basis: My clock-panel expectation: show the advancing clock, or clearly label a paused value as an edit snapshot. The current label simply says Time.

Evidence: [shots/055-date-time.png](shots/055-date-time.png), [shots/056-date-leap2000.png](shots/056-date-leap2000.png), [shots/059-date-cancel.png](shots/059-date-cancel.png), [evidence/main-serial.log](evidence/main-serial.log).

### L037 - The shipped exit procedure promises startup prompts that never appear

Area: TPS manual. Severity: P3.

Steps: From a pristine desktop choose Finder Special > Exit to InitechDOS and confirm. Inspect the first DOS screen and extract AUTOEXEC.BAT. Session painttps.

Observed: The system goes directly to A:\>. The shipped data image has AUTOEXEC.BAT containing @ECHO OFF and PATH=A:\DOS; no date or time question is presented.

Expected and basis: Manual pubs/manuals/tps-report-generator/s2-leaving-flair.tex:71-73 says "Because the data volume has no" AUTOEXEC.BAT, DOS asks for date/time; the numbered procedure repeats those prompts. Update the procedure to the shipped media.

Evidence: [shots/139-dos-start.png](shots/139-dos-start.png), [evidence/extracted-tps/AUTOEXEC.BAT](evidence/extracted-tps/AUTOEXEC.BAT), [evidence/painttps-serial.log](evidence/painttps-serial.log).

### L038 - The scene pie is flat and contiguous instead of an exploded perspective chart

Area: Film chart geometry. Severity: P3.

Steps: Boot filmdata and Escape; inspect chart x=328..425,y=170..267. Compare the photo chart x=824..1128,y=211..457.

Observed: The guest draws a circular flat disk with adjacent wedges. The photo has separated wedges, an elliptical top and a substantial extruded lower face.

Expected and basis: Named reference: spec/assets/preview.webp, those regions. This is a presence/shape comparison, not an uncalibrated photograph-to-framebuffer pixel error count. The 116% sum is intentional.

Evidence: [shots/187-film-data-escape.png](shots/187-film-data-escape.png), [evidence/references/film-preview.webp](evidence/references/film-preview.webp).

### L039 - The chart uses black and gray wedges on white instead of blue on dark

Area: Film chart color. Severity: P3.

Steps: Inspect the same guest and photo chart regions as L038.

Observed: The guest has one dark-blue wedge plus black and gray wedges on white. The photograph shows blue faces and sides against a dark background.

Expected and basis: Named reference: preview.webp x=824..1128,y=211..457. Relative color roles are visibly different; no claim is made about exact CRT RGB values.

Evidence: [shots/187-film-data-escape.png](shots/187-film-data-escape.png), [evidence/references/film-preview.webp](evidence/references/film-preview.webp).

### L040 - Percentage labels are detached in a right-hand list

Area: Film chart labels. Severity: P3.

Steps: Inspect filmdata after Escape: guest labels x=426..449,y=162..286; photo labels are within the wedges.

Observed: 40%,35%,18%,14%,9% appear as a vertical list, and an added 116% total appears below. The photo labels are placed on their corresponding slices.

Expected and basis: Named reference: preview.webp chart region x=824..1128,y=211..457. The numbers and their intentional total are preserved; placement is the nit.

Evidence: [shots/187-film-data-escape.png](shots/187-film-data-escape.png), [evidence/references/film-preview.webp](evidence/references/film-preview.webp).

### L041 - The Drive A Files window and its folder icons are absent

Area: Film composition. Severity: P3.

Steps: Inspect filmdata rest state, especially the area behind FILE COPY. Compare preview.webp x=356..826,y=228..397.

Observed: The guest has only a ledger, chart and saving modal. The photo also has a Drive A Files window with two folder icons between the worksheet and modal.

Expected and basis: Named reference: preview.webp, stated rectangle. This is missing scene content rather than criticism of the deliberate teal desktop.

Evidence: [shots/186-film-data-rest.png](shots/186-film-data-rest.png), [evidence/references/film-preview.webp](evidence/references/film-preview.webp).

### L042 - The progress fill has the wrong visual treatment

Area: Film progress appearance. Severity: P3.

Steps: Inspect filmdata FILE COPY bar x=155..485,y=250..262. Compare photo x=516..884,y=388..415.

Observed: The guest fill is a thin pale-cyan beveled strip in a recessed gray trough. The reference is a broad solid blue fill.

Expected and basis: Named reference: preview.webp, stated regions. Geometry is compared by shape and relative prominence, not direct raw pixels from different resolutions.

Evidence: [shots/186-film-data-rest.png](shots/186-film-data-rest.png), [evidence/references/film-preview.webp](evidence/references/film-preview.webp).

### L043 - FILE COPY omits the reference window controls and scroll gutters

Area: Film dialog chrome. Severity: P3.

Steps: Inspect guest modal (140,200)-(500,280) and photo FILE COPY (502,307)-(967,443).

Observed: The guest is a plain title-and-progress box. The reference has a close box, upper-right window widgets, right scrollbar and bottom horizontal scrollbar/gutter.

Expected and basis: Named reference: preview.webp, stated rectangle. The Saving tables to disk... wording is deliberate and correct; the missing visible furniture is the nit.

Evidence: [shots/186-film-data-rest.png](shots/186-film-data-rest.png), [evidence/references/film-preview.webp](evidence/references/film-preview.webp).

### L044 - The worksheet menu and tool strips visible in the frame are missing

Area: Film worksheet chrome. Severity: P3.

Steps: Inspect the upper worksheet area of filmdata and photo preview.webp x=319..1183,y=114..212.

Observed: The reference has worksheet menus (File/Edit/View/Insert/Format/Tools/Data/Window/Help), icon tools, font/size controls and a cell-name/formula strip. The guest ledger begins directly with A/B labels and three rows.

Expected and basis: Named reference: preview.webp, stated area. This is additional application furniture, separate from the deliberate stacked desktop bars and known M025 worksheet-body gap.

Evidence: [shots/186-film-data-rest.png](shots/186-film-data-rest.png), [shots/187-film-data-escape.png](shots/187-film-data-escape.png), [evidence/references/film-preview.webp](evidence/references/film-preview.webp).

### L045 - The 33rd clip is refused without its intended full notice

Area: Scrapbook capacity. Severity: P2.

Steps: Boot main-data.img, open Scrapbook (one saved clip). Choose Edit > Copy by dragging (95,30) to (110,90), then press Ctrl-V 31 times to reach 32 clips. Press Ctrl-V once more. Session scrapcapacity; mouse Copy avoids L002.

Observed: The display remains 32 of 32 with no explanation. The extra Paste dispatches and reads the clipboard, but the alert gate returns -3. The stopped file retains exactly 32 clips; this refusal does not corrupt them.

Expected and basis: Ordinary capacity feedback: explain that the Scrapbook is full and how to make space. Silent ignored Paste is misleading.

Evidence: [shots/272-scrap-32-real.png](shots/272-scrap-32-real.png), [shots/273-scrap-full-real.png](shots/273-scrap-full-real.png), [evidence/scrapcapacity-serial.log](evidence/scrapcapacity-serial.log), [evidence/extracted-scrapcapacity/SCRAPBK.DAT](evidence/extracted-scrapcapacity/SCRAPBK.DAT).

Guess: os/apps/notepad/notepad.c passes the full/too-large notice to tbx_alert without line wrapping; the gate rejects the request. The -3 return is observed; the exact violated argument limit is not proven.


## Still present

These are freshly observed portions of known findings, not new numbers.

| Prior ID | Result in this session | Evidence |
| --- | --- | --- |
| F15, first GUI audit | The upper Special menu still exposes About/Quit instead of Special operations. About is now gray, so the older enabled-About no-op is not claimed here. The film therefore has no reached normal Shut Down path in this menu. | [192](shots/192-film-upper-special-menu.png), filmdata serial |
| M003, look pass 6 | Command equivalents still use a caret, for example ^Q and ^W, rather than the Mac Command glyph. | [128](shots/128-paint-file-actual.png), [024](shots/024-calc-edit-settled.png) |
| M025, look pass 6, partially present | The film worksheet is no longer completely blank, but it still does not reproduce the reference expense table: only three labels, two columns and TODO_FRAME values appear. The photographic expense grid and actual entries remain absent. This residual content gap is not assigned a duplicate new ID. | [186](shots/186-film-data-rest.png), [187](shots/187-film-data-escape.png), [photo](evidence/references/film-preview.webp) |

The one-disk-tenant limit also remains, with a visible generic refusal now. Earlier reports described this limit in coverage, rather than a dedicated finding. L030 covers the new reached same-accessory activation/error behavior. G09's deliberately malformed app/crash fixtures were not rerun, so that ID is not declared fixed on the basis of a different error path.

## Now fixed

| Prior ID | What no longer reproduces | Evidence |
| --- | --- | --- |
| F13, Apple-menu portion only | Both menu display and the listed accessory launches work. About This Computer shows InitechDOS 3.30 and memory figures. Finder Help was not re-audited, so the whole composite F13 is not certified. | [019](shots/019-apple-held.png), [039](shots/039-about.png), main serial |
| F05, Restart/Shut Down no-op baseline | Restart actually reboots, and Shut Down reaches the safe-to-power-off screen with Note Pad, Scrapbook, Text, Paint and Control Panels. Calculator is the new L022 exception. | [071](shots/071-after-restart.png), [237](shots/237-note-shutdown.png), [243](shots/243-text-shutdown-completed.png), [260](shots/260-paint-shutdown-refusal.png), [274](shots/274-scrap-shutdown-real.png), [276](shots/276-cpanels-shutdown-real.png) |
| M024 | With the data disk attached, the film chart is populated rather than white. Its appearance still differs as L038-L040 describe. | [187](shots/187-film-data-escape.png) |
| M026 | The film saving state has the intended hourglass after pointer movement. | [186](shots/186-film-data-rest.png) |

These are bounded retests against the seven previous REPORT/TRIAGE pairs. Other older findings are neither silently refiled nor declared fixed.

## Coverage and limits

| Area | Driven and verified | Limits or unreachable work |
| --- | --- | --- |
| Calculator, CALCULAT | All 18 face keys by mouse; keyboard digits/operators/C; 2+3*4= gives 20; 5== gives 5; 1/0 gives infinity and 0/0 not a number; negative zero gives 0; -5, decimal duplicates/overflow, long digits, 9999999999 squared gives 1E20, 1/3*3 gives 1; Backspace and Escape; held key; Edit menu; Close W; repeated launches; shutdown refusal. Readouts are collected in calculator-results.png. | No CE, percent, square-root or paper-tape control exists on this four-function face. These are not invented defects: the named real Mac reference is also four-function, and Backspace's no-op matches its vectors. No claim of arbitrary-precision certification. |
| Ledger, CALC | Distinguished from the actual Calculator; edit/commit, Return/Down, pie/116%, trailing 570-, Save completion, real bytes copied, second Save, Escape during copy, Quit/reopen, film resize/movement. Rate 0, 100, bad, empty and missing config; empty/missing ledger; a 1000-line ledger; existing target. | It is not a second scientific calculator. LEDGER.TXT and MICHAEL.CFG belong to CALC's copy operation, not TPSREP's table report. The source and live behavior establish that distinction. No invented financial rounding repair or normalization of the canon numbers is proposed. |
| InitechText | Three-line typing; wrapping; 4096-byte document; 32 KB and 64 KB rejection; LF and CRLF, blank lines, high-bit bytes; Ctrl-A/C/X/V/Z; mouse-drag selection and Cut/Paste; Shift-arrows; resize; caret/pointer; 25-line scroll boundary; title repaint; Open, Save, Save As; valid/invalid names; replacement Cancel/confirm; full disk; Quit Save/Cancel/Don't Save; dirty Open Don't Save; New Save/Cancel/Don't Save; shutdown Save and chooser Cancel; exact saved bytes. | Shift-click selection, deleting selected text with every key, read-only permissions, unusual path names, every chooser navigation path, and all possible line/byte limits were not exhaustively exercised. The shared caret/line renderer is not certified pixel-perfect. |
| InitechPaint | Finder launch twice; canvas drag, grow, close box, Ctrl-Q, shutdown. Every File/Edit/Image/Layer/Select/View/Window/Help menu was opened and inspected. | Every command except Quit is gray. No actual tool, pattern, fill, selection, layer, image-resize, open, save or undo workflow could be reached; consequently off-canvas painting, leaky fills, brush edges and image-file roundtrips cannot be graded. No separate bug is invented for each unavailable gray row. |
| Note Pad | Cross-tenant paste from Text; pages 1,2,8; last-page boundary; contents retained through Close/reopen and Restart; long text; Shift-arrows; explicit full-page alert; corrupt checksum; same-app relaunch; Shutdown and reboot persistence. Extracted NPAD file decoded. | Each page's exact 1024-byte boundary was not independently enumerated; the observed over-capacity paste did show a full notice. Cursor animation and every page-corner cancellation gesture were not exhaustively covered. |
| Scrapbook | Text copied between tenants; two clips, first/last boundaries, Cut removing a clip, close/reopen and Restart; long clip keyboard vs mouse scroll; Ctrl-C crash isolated twice; mouse Copy workaround; 32 clips and 33rd refusal; shutdown persistence, SBK1 bytes extracted. | PICT and sound clips could not be produced from the reached apps; Paint has no drawing or Copy path. The app advertises TEXT only. No claim that graphics/audio conversion was tested. Earlier failed capacity attempts are explicitly excluded. |
| Control Panels | All five reached panels: Date & Time, Desktop Patterns, Mouse, Sound, Memory. Every pattern choice; all tracking/double-click radio choices; all sound/length choices and Test; Cancel vs OK; 2000-02-29, 1999 clamp to Feb 28, return to leap day; year cap 2099; clock hide/show; clock advancement in menu; fast mouse produces approximately doubled motion; restart persistence; panel shutdown. CPANELS.DAT extracted and decoded. | No General panel or Font Manager install/remove/list interface exists in the reached panel chooser, Apple menu, or APPS roster. Font installation/removal therefore could not be driven. No seconds field is offered, so exact 23:59:59 entry was unreachable. Date limits at 1980 and all month/hour/minute wrap combinations were not exhaustively enumerated. Sound controls were exercised, but audible output was not captured; no audio-quality verdict is made. Memory figures were inspected, not independently stress-certified. |
| TPS | Real GUI Exit to DOS route; shipped parameters/table; exact golden report and backup; six/eight alignment presses rejected, seven accepted; mismatched report IDs; overlong ID input (documented truncation); wrong memo; cover N; missing table; absent/empty/over-1-KB parameter file; empty/truncated DBF; bad LINES; LINES 0/100; bad numeric PAGES; 1000 records; zero-free-space report. | The 1000-record fixture is an explicit table stress control, separate from the 1000-line ledger copy. Arbitrary malformed DBF layouts, every DOS device failure, every non-ASCII parameter and all full-disk backup permutations remain outside this finite run. Oversize parameters were refused with missing-key feedback; no hang is claimed. |
| Film | Required no-data boot and subsequent controls document a panic. Data-attached control reaches the scene, hourglass, Escape dismissal, editable ledger, chart move/grow, upper Special and lower-bar attempts, Quit. Compared with preview.webp at named regions. | The no-data image cannot reach any interactive controls after panic. The data-attached saving tableau is a staged prop, not a file copy; its static bar is not called a hang or a failed disk write. No usable normal Special Shut Down was reached on that film desktop. Photo/CRT geometry is not an RGB oracle; no misleading native-pixel mismatch percentage is asserted. |
| Cross-app/lifecycle | All assigned small GUI apps launched, accessories from Apple and primary apps from Finder; multiple attempts and close/reopen paths; same-app and second-app refusals; shared TEXT Scrap; settings and page files across Restart; active-app shutdown controls, including edited CALC in the final finderaccess session. | A final finderaccess session launched Note Pad, Scrapbook and Control Panels twice from the scrolled APPS window; the other small apps already had repeated Finder launches. Only one disk app can run at once, blocking simultaneous Text-to-Text windows. No multi-hour soak, real printer, non-16-MB machine or second emulator backend was used. |

Important controls that held up:

- The normal TPS report is exactly 2432 bytes and equals the manual's golden SHA-256 b81305271c56b62f4dcf69e9049da1c9da2b81c9b6631fd6bf2f9c9bb4abf934. The prior report is retained as BAK on a successful second run. See [comparison](evidence/extracted-tps/comparison.json).
- The 1000-record report lists 1000 records, totals 12000 pages, and produces a 67968-byte report. LINES=0 gives 13 detail pages; LINES=100 gives one for the shipped 13 undeleted records. See [case summary](evidence/extracted-tps-cases/summary.json).
- The 1000-line ledger copy at rate 100 writes all 17000 bytes and finishes at 102 ticks. Invalid config preserves the pre-existing TABLES.TXT, despite the missing visible error. A valid config with empty ledger completes with a zero-byte output; a missing ledger refuses.
- CPANELS.DAT is a 16-byte CP01 record in the volume root. After main it encodes pattern 7, normal tracking, slow double-click, Click sound, Full length, clock shown; it survives Restart. Note Pad stores 8224 bytes and Scrapbook 32848 bytes in root files, with checksummed page records. The original files, decoded pages and checks are retained.
- Text shutdown chooser cancellation leaves the desktop running; subsequent Save writes the exact 21-byte UNSAVED SHUTDOWN TEXT and reaches the safe-shutdown screen. Note Pad's SHUTDOWN SENTINEL survives reboot.

## The fifteen fixes I would do first

1. Keep GUI Copy from entering DOS Control-Break (L002).
2. Preserve original text files until replacement succeeds (L003).
3. Check every TPS write and report a nonzero failure status (L004).
4. Make the specified standalone film image boot without panic (L001).
5. Preserve LF-only empty lines (L005).
6. Make ledger Save serialize the document being edited (L006).
7. Guard ledger Quit and close against unsaved changes (L007).
8. Preserve and explain corrupt Note Pad storage (L008).
9. Identify and confirm the ledger copy destination before replacement (L009).
10. Deliver visible Save and capacity errors (L019, L045, L014).
11. Give InitechPaint working tools and picture file operations (L023-L025).
12. Repair Text resizing and scrolling paint correctness (L012, L013).
13. Provide Undo and Shift-arrow selection (L010, L011).
14. Finish the filename field's editing and validation behavior (L016, L017, L035).
15. Make the accessories integrate: Calculator clipboard/shutdown, Scrapbook mouse scrolling, and stable menu bars after preference changes (L021, L022, L026, L027).

The [native scene/reference crop pairs](evidence/film-comparison.json) retain region coordinates without rescaling or pretending the photograph is a framebuffer golden.

The film's chart/body fidelity should then receive a focused pass using L031 and L038-L044 plus the residual M025. These are deliberate-number-preserving appearance repairs, not corrections to 116%, 570-, the teal desktop, the two bars or the hourglass convention.
