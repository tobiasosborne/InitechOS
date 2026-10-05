# InitechOS hands-on audit - supplied 7030e2f images

Guest audit: 2026-10-05, Europe/Berlin. This report concerns the copied image bytes, not a later moving repository HEAD. No source changes, builds, make commands, commits, tracker writes, or external messages were made.

Scope: about 50 minutes of guest auditing, 35 copied-image QEMU sessions, 331 guest screenshots, 975 typed input sequences, a 210-launch mixed-app soak, and a 2050-entry worksheet stress. There are 118 new findings: 10 P1, 102 P2 and 6 P3.

The recent fixes are real, but the system is still a development desktop rather than a dependable period workstation. The most consequential new results are a corrupt DBF after disk-full appends, writes through a purged Finder directory window, a reproducible database-exit hang, and destructive confirmation on mouse-down. The native spreadsheet is a usable entry demonstration with substantial missing worksheet workflows. The report deliberately includes small appearance defects and separately numbered missing commands; the count is not a claim that each needs an independent implementation project.

## 1. Verdict per recent fix

| Recent change | Verdict | Evidence and boundary |
| --- | --- | --- |
| Trash opens as a window | holds | Empty and nonempty Trash opened; its folder children opened; moving the view retained its contents; dragging README back into root restored it; duplicate basenames survived as SAME.TXT and SAME001.TXT. See shots/002-empty-trash.png, shots/099-trash-open-child.png, shots/103-trash-restore-before.png, shots/104-trash-restore-after.png, shots/130-trash-two-same-names.png and evidence/trash-collision-after.txt. The previously known window-slot limit still applies. |
| Special > Empty Trash with confirmation | partly | A visible count/space alert appeared, Escape/Cancel preserved items, background input was blocked, Return purged, and recursive removal/empty-icon changes occurred. See evidence/trash-slot-repro-serial.log and evidence/trash-edges-serial.log. L002 exposes stale writable descendants, L004 premature button commitment, and L118 the locked-file path. L022-L024 are appearance/summary nits. |
| Disk apps receive their own menu choices | holds | TenantFix's About drew its version and added the Info title; File > Quit and Ctrl-Q exited. CTENANT and 123 both exited through their own File > Quit; the traces have TENANT-MENU for the correct app route. See shots/020-tenant-about.png, shots/022-tenant-quit-menu.png, shots/ab-ctenant-menu-quit.png, shots/ab-123-menu-quit.png and evidence/app-boundaries-serial.log. The built-in HELLO/NOTES issue is the already-known K07. |
| CTENANT.EXE exists and runs | holds | It loaded from APPS, displayed keyboard and click feedback, and quit; 70 additional lifecycle controls completed in the soak. See shots/023-ctenant.png, shots/024-ctenant-input.png and evidence/soak-verification.json. L117 records its repaint nit. |
| 123.EXE exists, shows a worksheet and accepts labels/numbers | holds for that stated slice | Real disk launch, labels, integers, decimal/negative numbers, prefix alignment, and full-size arrow following to A23/M1 worked. See shots/027-123-row-entry.png, shots/058-123-vertical-follow-ready.png, shots/059-123-horizontal-follow-ready.png and evidence/cell-stress-serial.log. This is not a verdict of Lotus feature completeness; L008-L021 and L080 detail the reached boundaries. |
| COPY onto itself is safe | holds | An 8192-byte pattern survived identical-object spellings .\SELF.BIN, A:\SELF.BIN, \SELF.BIN and OTHER\..\SELF.BIN; FILLED\..\SELF.BIN was also refused. All refusals reported zero copied. The extracted 8192-byte source matched its original SHA-256. See evidence/dos-corrected-serial.log, evidence/dos-edge-serial.log and evidence/dos-corrected-verification.json. |
| COPY to a directory preserves that directory | holds | COPY README.TXT FILLED and COPY SELF.BIN FILLED\ placed files inside the existing directory. SAVE.TXT retained its sentinel; the copied binary was byte-identical. See shots/e003-copy-directory.png and evidence/dos-corrected-verification.json. Switch parsing and read-only protection have separate new failures. |
| DEL honors its path | holds | Relative DELTEST\Z*.TXT and absolute A:\DELTEST\*.TXT acted in the requested child directory. The root sentinel still printed after deletion; the first narrower pass retained the child KEEP.TXT. See shots/e004-del-path-40.png, evidence/dos-corrected-serial.log and evidence/dos-edge-serial.log. The absolute-path preservation result is established by the complete raw transcript, not the early screenshot. |
| DEL handles more than sixteen matches | holds | A 40-file Z00.TXT-Z39.TXT set was completely removed in one operation. The child directory retained only its nonmatching KEEP.TXT and dot entries. See evidence/dos-fixture.img-before.txt, evidence/dos-corrected-DELTEST-after.txt and evidence/dos-corrected-serial.log. |
| DEL *.* asks for confirmation | holds | Both bare and path-qualified all-files forms prompted. N retained A0.TXT; Y removed all three fixture files. No deletion occurred merely on issuing the command. See shots/139-del-bare-confirm.png, shots/140-del-bare-decline.png, shots/141-del-bare-confirmed.png and evidence/del-confirm-exact-serial.log; qualified controls are in evidence/dos-corrected-serial.log. |

### Replay, evidence, and expectations

All guest input used QEMU emulated PS/2 devices through QMP; all guest screenshots came from QMP screendump with -display none, -cpu 486 and 16 MiB RAM. QMP used stdio. At most two audit emulators ran concurrently. Display :0 was neither addressed nor captured. Every boot used a copied boot image and a copied data image. All mutations stayed here. Original supplied-image hashes and per-session pre/post hashes are in evidence/image-manifest.json and the *-session.json files.

Replay in a new writable copy of this directory, because scripts reuse output names. Run python3 -u audit_driver.py and enter JSON lines to replay individual gestures. Example: ["start","replay","images/flair_tenants_interactive.img","images/flair_data.img"], then ["click",600,64,2], then ["quit"]. Coordinates are guest pixels. drag arguments are x,y,delta-x,delta-y. A menu operation holds its title, moves to a row and releases. The standard fresh root opens at (20,60), APPS at (40,80); its application icons are (75,124), (143,124), (211,124). For SAMIR use the data disk specified by the finding and press backslash. For DOS use tracer_boot and the specified second disk. Labels and scripts are exact identifiers, not assertions inferred from screenshot filenames.

prepare_fixtures.py and evidence/*preparation*.json describe the valid FAT fixtures. The full DBF, Trash, redirected-input and program fixtures have their exact preparation in the evidence and driver inputs; the supplied and derived images are retained for direct replay. Text suites replay with python3 run_text_suite.py NAME.json. The main GUI journal is evidence/actions.jsonl, including nested helper calls and serial offsets: do not blindly replay every nested record, which would duplicate keystrokes. Use the provided scripts or the explicit finding steps. evidence/command-journal.tsv is a compact inventory of typed inputs. Screenshots cited by findings were visually inspected; serial events and stopped-image extraction supplement them where a still cannot establish event order or stored bytes.

Reference abbreviations used below:

- DOS-manual: original MS-DOS 3.3 User's Guide (July 1987), supplied under /home/tobias/Projects/initech-manuals-ref/dos33/; searchable extraction in evidence/references/dos33-manual.txt. Printed page numbers differ from PDF page numbers.
- DB-manual: original Using dBASE III Plus, in the supplied dBASE III Plus 1.1 (1986) Manuals; extraction evidence/references/dbase3-using.txt. Additional III+ HELP-derived specs and actual minted interpreter results are copied under evidence/references/dbase-*.md. Later-only functions ALIAS() and FCOUNT() were probed but are not filed as missing III+ functions because the supplied III+ references did not establish them.
- L22-manual: original Lotus 1-2-3 Release 2.2 Reference Manual (1989). It is image-only; selected Chapter 1 pages were rendered and read, with OCR kept as an aid in evidence/references/lotus-ch1-* and lotus-entry-*. Actual real-Lotus MINT01 entry/save captures are lotus22-entered.png and lotus22-save.png. The 240-character limit and the F-key/navigation rules come from the original pages, not a modern spreadsheet convention.
- MAC6: original Macintosh System Software User's Guide v6.0, extraction evidence/references/mac6-manual.txt; used for classic Finder behavior. Platinum appearance uses the actual Mac OS 8.1 captures mac81-window.png and mac81-alert.png plus the local sys8 controls specification. System 7 and Mac OS 8 are not treated as pixel-identical eras. The earlier general Chicago/Charcoal/frame/icon observations remain F16, not new duplicate findings.
- Film comparison: the repository's spec/assets/preview.webp, copied as evidence/references/film-preview.webp, was inspected only as a reference image. No live desktop capture was taken.

Every behavioral expectation below is attributed to a named reference or explicitly to my memory/usability judgment. Source reads were used to find launch paths and suitable inputs, never as a substitute for a guest observation. The two stacked bars, teal desktop, busy-hourglass convention, 116% pie total, 570- figure, saving-dialog wording, panic text and year-00-as-1900 convention are accepted choices. No finding attacks those choices.

## 2. New findings

P1 = data loss/corruption, crash/hang, or blocked core task. P2 = broken or missing feature. P3 = look/polish nit. Each numbered entry is one defect; related symptoms and shared old diagnostics are identified explicitly.

### L001 - Disk-full APPEND corrupts the DBF record count and prevents reopening

Area: database. Severity: P1.

Steps: Run prepare_fixtures.py and the documented db-full preparation; replay db-full.json with run_text_suite.py. Boot tenants + db-full-fixture.img; backslash; USE CLIENTS.DBF. Repeat APPEND BLANK and REPLACE NAME WITH "Rnn",BAL WITH n for n=0..17. Then USE, USE CLIENTS.DBF, LIST.

Observed: The first nine appends worked. Later appends failed. RECCOUNT() still reported 12. After closing, USE could not reopen the table. Independent extraction found a 502-byte DBF with header count 21, header length 129 and record length 31: only 12 complete records exist, whereas the header requires 781 bytes including EOF. The original three rows remain in the bytes but the table is unusable through SAMIR.

Expected and basis: A refused append must preserve a self-consistent, reopenable table and the previous records. This is my data-integrity expectation; DBF header/count meaning is documented in dbase3-decomp/specs/file-formats/dbf.md. Disk-full is a normal floppy workload.

Evidence: [db-full.json](db-full.json), [evidence/db-full-serial.log](evidence/db-full-serial.log), [evidence/db-full-verification.json](evidence/db-full-verification.json), [evidence/db-full-CLIENTS.DBF](evidence/db-full-CLIENTS.DBF), [shots/120-db-full-appended.png](shots/120-db-full-appended.png), [shots/121-db-full-reopen.png](shots/121-db-full-reopen.png).

### L002 - An open purged folder remains writable and leaks an orphan cluster

Area: Trash. Severity: P1.

Steps: Replay reproduce_trash_deleted_window.py. Fresh tenants + trash-fixture.img: open INITECH (600,64); drag A (192,101) to Trash (600,422); open Trash and its A at (75,124); Special > Empty Trash, Return. In the surviving A window press Ctrl-N.

Observed: A still displayed CHILD after both directories and KEEP.TXT had been purged. Ctrl-N reported NEWFOLD parent=36 and the view acquired bogus IDB1/document entries. The new folder was not reachable from root or Trash. fsck.fat -n found one unused allocated cluster. An independent fresh replay reproduced the same orphan.

Expected and basis: Close or invalidate every window whose directory is deleted, and reject writes to freed storage. This is my filesystem/window-lifetime expectation. MAC6 p.112 says emptying Trash permanently removes its contents; a surviving usable directory view is inconsistent with that operation.

Evidence: [reproduce_trash_deleted_window.py](reproduce_trash_deleted_window.py), [evidence/trash-edges-serial.log](evidence/trash-edges-serial.log), [evidence/trash-independent-serial.log](evidence/trash-independent-serial.log), [evidence/trash-independent-fsck.txt](evidence/trash-independent-fsck.txt), [shots/101-trash-purge-open-descendant.png](shots/101-trash-purge-open-descendant.png), [shots/102-newfolder-in-purged-child.png](shots/102-newfolder-in-purged-child.png).

### L003 - QUIT hangs after the same table is opened in two work areas

Area: database. Severity: P1.

Steps: Replay db_exit_probe.py, case db-exit-two. On tenants + db-fixture.img press backslash. SELECT 1; USE CLIENTS.DBF ALIAS ONE; SELECT 2; USE CLIENTS.DBF ALIAS TWO; QUIT. Wait 18 seconds; send Ctrl-C and Return.

Observed: No desktop return, prompt, or recovery followed. QMP still responded; the CPU snapshot had HLT=1 and interrupts disabled (EFL=0x93). The two-area mutation variant also hung. The single-area USE/LIST/QUIT control returned to FLAIR.

Expected and basis: QUIT must close the work areas and restore the suspended desktop, or reject an unsupported second open safely. DB-manual work-area/alias chapter documents independent selected areas; clean process teardown is my OS expectation. This is an observed guest hang, not the runner timeout itself.

Evidence: [db_exit_probe.py](db_exit_probe.py), [evidence/db-exit-two-serial.log](evidence/db-exit-two-serial.log), [evidence/db-exit-two-qmp-state.json](evidence/db-exit-two-qmp-state.json), [evidence/db-exit-control-serial.log](evidence/db-exit-control-serial.log), [shots/db-exit-two-after.png](shots/db-exit-two-after.png), [shots/db-exit-two-recovery.png](shots/db-exit-two-recovery.png).

### L004 - Empty Trash buttons commit on mouse-down, before release can cancel

Area: dialogs. Severity: P1.

Steps: Fresh tenants + flair_data: stage README.TXT in Trash. Choose Special > Empty Trash at (172,28)->(200,65). Press OK at (464,168), keep the button held for 0.5 s, then move to (414,218) and release outside. Cancel can be tested the same way at (391,168).

Observed: OK purged the file while the button was still held. The dialog and item were already gone in the held screenshot; release outside did not cancel. Cancel also dismissed immediately on mouse-down. Raw event ordering places EMPTIED between mouseDown and mouseUp.

Expected and basis: Track the button until release inside; release outside cancels the press. Named reference: local Control Manager, TrackControl button-like parts. This is distinct from the old fixed title-bar Close-box finding G02.

Evidence: [evidence/desktop-main-serial.log](evidence/desktop-main-serial.log), [evidence/actions.jsonl](evidence/actions.jsonl), [shots/006-empty-confirm.png](shots/006-empty-confirm.png), [shots/010-ok-button-held.png](shots/010-ok-button-held.png), [shots/011-ok-drag-out.png](shots/011-ok-drag-out.png), [evidence/references/control-manager.md](evidence/references/control-manager.md).

### L005 - COPY overwrites a read-only destination

Area: DOS. Severity: P1.

Steps: Boot tracer_boot + dos-fixture.img. READONLY.TXT is explicitly +r in prepare_fixtures.py. COPY README.TXT READONLY.TXT; TYPE READONLY.TXT.

Observed: COPY reported one copied and replaced READONLY-SENTINEL with the README text. It did not require removing the read-only attribute.

Expected and basis: Refuse modification of a read-only destination. DOS-manual, ATTRIB and access-denied diagnostics, documents protection against changes. This is separate from the repaired directory-destination COPY case.

Evidence: [prepare_fixtures.py](prepare_fixtures.py), [evidence/fixture-preparation.json](evidence/fixture-preparation.json), [evidence/dos-corrected-serial.log](evidence/dos-corrected-serial.log), [shots/e008-readonly.png](shots/e008-readonly.png).

### L006 - DEL removes a read-only file

Area: DOS. Severity: P1.

Steps: Fresh tracer_boot + dos-fixture.img: DEL READONLY.TXT, then TYPE READONLY.TXT, without first copying onto it.

Observed: DEL silently removed the protected file. This is not inferred from the earlier COPY, which could have changed attributes.

Expected and basis: Honor the read-only state. DOS-manual ATTRIB says read-only prevents accidental deletion.

Evidence: [evidence/dos-edge-serial.log](evidence/dos-edge-serial.log), [evidence/dos-edge-listing-root.txt](evidence/dos-edge-listing-root.txt), [shots/110-dos-root-path.png](shots/110-dos-root-path.png).

### L007 - SET SAFETY ON does not confirm ZAP

Area: database. Severity: P1.

Steps: Fresh tenants + db-fixture.img; backslash; USE CLIENTS.DBF; SET SAFETY ON; ZAP; ? RECCOUNT(). Replay db-final.json.

Observed: ZAP returned immediately with no Y/N question and RECCOUNT() became zero. No consent response was sent. The same behavior occurred in the broader and full work-area tests.

Expected and basis: DB-manual, ZAP, printed p.U5-284 explicitly specifies a ZAP filename? (Y/N) prompt when SAFETY is ON. Preserve the rows until an affirmative response.

Evidence: [db-final.json](db-final.json), [evidence/db-final-serial.log](evidence/db-final-serial.log), [shots/db-final-04.png](shots/db-final-04.png), [shots/db-final-05.png](shots/db-final-05.png), [evidence/references/dbase3-using.txt](evidence/references/dbase3-using.txt).

### L008 - Quit discards an edited worksheet without an unsaved-work warning

Area: Initech 123. Severity: P1.

Steps: Launch 123 from the normal APPS folder. Enter Hello, 123, 1.5 and -7 across row 1; add another label. Press Ctrl-Q, then launch 123 again.

Observed: The app immediately exited; there was no save/discard/cancel choice. Relaunch showed an empty A1 worksheet and the entered work was gone.

Expected and basis: Protect unsaved work on exit or clearly disclose a volatile demo before accepting it. My period spreadsheet/Mac-document expectation is a dirty-document warning; Lotus reference Chapter 9 supplies a distinct Quit workflow. The missing save route is recorded separately.

Evidence: [evidence/desktop-main-serial.log](evidence/desktop-main-serial.log), [shots/027-123-row-entry.png](shots/027-123-row-entry.png), [shots/044-123-long-label-stored.png](shots/044-123-long-label-stored.png), [shots/045-123-quit-unsaved.png](shots/045-123-quit-unsaved.png), [shots/046-123-relaunch-empty.png](shots/046-123-relaunch-empty.png).

### L009 - Ordinary formulas and @ functions cannot be committed

Area: Initech 123. Severity: P2.

Steps: In a fresh 123 sheet enter 123 in B1, 1.5 in C1 and -7 in D1. In A2 type +B1*2 and Return. Escape, then type @SUM(B1..D1) and Return.

Observed: Both valid entries remained uncommitted in EDIT mode; A2 had no calculated result. Number and label controls did commit.

Expected and basis: Compute 246 and 117.5. Named reference: the real Lotus R2.2 MINT01 capture and mint/mint01.steps contain these exact formulas. This is a missing core calculation feature, not a claim that every formula parser branch was tested.

Evidence: [shots/028-123-formula.png](shots/028-123-formula.png), [shots/041-123-function-rejected.png](shots/041-123-function-rejected.png), [evidence/desktop-main-serial.log](evidence/desktop-main-serial.log), [evidence/references/lotus22-entered.png](evidence/references/lotus22-entered.png).

### L010 - The slash command menu does not open

Area: Initech 123. Severity: P2.

Steps: In READY press slash and then f,s, without Return.

Observed: Slash produced no menu. The following fs became a LABEL entry in the current cell.

Expected and basis: Slash enters the Lotus command menu, with Worksheet/Range/Copy/Move/File and other choices. Named reference: L22-manual Chapter 1, Using 1-2-3 Menus; the real MINT01 /fs save capture. A Mac menu alternative would also need to expose these operations.

Evidence: [shots/030-123-file-save.png](shots/030-123-file-save.png), [evidence/references/lotus22-save.png](evidence/references/lotus22-save.png), [evidence/desktop-main-serial.log](evidence/desktop-main-serial.log).

### L011 - There is no reached worksheet Save operation

Area: Initech 123. Severity: P1.

Steps: Enter a worksheet, try /fs, and inspect the lower File menu (its only offered item is Quit). Quit and relaunch as in L008.

Observed: No filename prompt, Save item, saved worksheet, or recoverable work appeared. Only Quit was available in the app File menu.

Expected and basis: A productive worksheet must be saved to disk. Named reference: L22-manual Chapter 5 /File Save, and the actual Lotus save-prompt capture. This is the persistence gap beyond the slash menu interaction itself.

Evidence: [shots/030-123-file-save.png](shots/030-123-file-save.png), [shots/138-123-file-menu.png](shots/138-123-file-menu.png), [shots/046-123-relaunch-empty.png](shots/046-123-relaunch-empty.png), [evidence/app-boundaries-serial.log](evidence/app-boundaries-serial.log), [evidence/references/lotus22-save.png](evidence/references/lotus22-save.png).

### L012 - F1 provides no worksheet help

Area: Initech 123. Severity: P2.

Steps: In READY press F1.

Observed: The sheet remained in READY and no help text or Help screen appeared.

Expected and basis: HELP (F1), L22-manual p.1-11, displays context-sensitive help.

Evidence: [shots/029-123-f1-help.png](shots/029-123-f1-help.png), [evidence/desktop-main-serial.log](evidence/desktop-main-serial.log).

### L013 - F2 cannot edit an existing cell

Area: Initech 123. Severity: P2.

Steps: Select A1 containing Hello and press F2.

Observed: A1 remained selected in READY; the edit line stayed empty.

Expected and basis: EDIT (F2), L22-manual p.1-11, enters EDIT with the current cell contents.

Evidence: [shots/032-123-f2-edit.png](shots/032-123-f2-edit.png), [evidence/desktop-main-serial.log](evidence/desktop-main-serial.log).

### L014 - F5 does not start Goto

Area: Initech 123. Severity: P2.

Steps: In READY press F5, then type Z100.

Observed: No address prompt appeared; Z100 became a LABEL entry at the old cell.

Expected and basis: GOTO (F5), L22-manual p.1-11, moves directly to the specified cell/range.

Evidence: [shots/031-123-goto.png](shots/031-123-goto.png), [evidence/desktop-main-serial.log](evidence/desktop-main-serial.log).

### L015 - Page Down does not advance one worksheet screen

Area: Initech 123. Severity: P2.

Steps: In READY at A1 press Page Down.

Observed: The address and pointer remained A1. Arrow-based movement did work to A23.

Expected and basis: PGDN, L22-manual p.1-10, moves down one screen.

Evidence: [shots/061-123-pgdn-ready.png](shots/061-123-pgdn-ready.png), [evidence/desktop-main-serial.log](evidence/desktop-main-serial.log).

### L016 - Tab does not page horizontally

Area: Initech 123. Severity: P2.

Steps: Use Right twelve times to reach M1; press Tab.

Observed: M1 and the F..M column viewport remained unchanged.

Expected and basis: TAB in READY, L22-manual p.1-10, moves right one screen. This is not a claim that Tab should move one cell.

Evidence: [shots/060-123-tab.png](shots/060-123-tab.png), [evidence/desktop-main-serial.log](evidence/desktop-main-serial.log).

### L017 - Clicking a worksheet cell does not select it

Area: Initech 123. Severity: P2.

Steps: From A1 click the visible E4 area at (360,180).

Observed: The address and highlight remained A1.

Expected and basis: My expectation for a worksheet in a native mouse-driven document window is direct cell selection; this is a memory/usability expectation, not a claim about DOS Lotus mouse configuration.

Evidence: [shots/034-123-mouse-cell.png](shots/034-123-mouse-cell.png), [evidence/desktop-main-serial.log](evidence/desktop-main-serial.log).

### L018 - Shrinking the window lets the current cell disappear off screen

Area: Initech 123. Severity: P2.

Steps: Shrink the 640x440 123 window by about 288x194 with its grow box. Move Right to E1. Try the horizontal right arrow.

Observed: The address became E1, but the view showed only A..D and no cell pointer. The visible scrollbar did not reveal E1.

Expected and basis: Keep the current cell visible for the actual resized viewport. This is my native-window/worksheet visibility expectation; the normal full-size arrow-follow control worked.

Evidence: [shots/039-123-pointer-off-small-window.png](shots/039-123-pointer-off-small-window.png), [evidence/desktop-main-serial.log](evidence/desktop-main-serial.log).

### L019 - Entry silently stops at 75 characters instead of the documented 240

Area: Initech 123. Severity: P2.

Steps: At A1 type the alphabet four times (104 letters), without Return; then press Return.

Observed: The entry line stopped after the third alphabet minus its final three letters (75 characters); the remaining keystrokes did not extend it. No length error or scrolling edit line appeared.

Expected and basis: L22-manual pp.1-16/1-17 permits 240-character entries. Preserve all 104 characters or explicitly refuse them.

Evidence: [shots/048-123-long-label-input.png](shots/048-123-long-label-input.png), [evidence/desktop-main-serial.log](evidence/desktop-main-serial.log).

### L020 - The unmodified backslash hotkey steals the repeating-label prefix

Area: Initech 123. Severity: P2.

Steps: Boot tenants + db-fixture.img; open root APPS (192,101), then 123 (211,124). At B1 press backslash to begin a repeating label.

Observed: The entire desktop suspended into SAMIR. QUIT returned to the sheet, which had not received the prefix. On flair_data the same key attempted the unavailable text app and resumed.

Expected and basis: L22-manual p.1-17 defines backslash as the repeating-label prefix. Reserve a non-conflicting system shortcut while applications accept text. The existence of a system hotkey is deliberate; its collision is the defect.

Evidence: [shots/ab-backslash-launches-database.png](shots/ab-backslash-launches-database.png), [evidence/app-boundaries-serial.log](evidence/app-boundaries-serial.log).

### L021 - The worksheet status line omits the period clock/date

Area: Initech 123. Severity: P3.

Steps: Launch 123 and inspect the bottom status line at native size.

Observed: The status line was blank throughout entry and navigation.

Expected and basis: The real Lotus R2.2 MINT01 capture shows date/time at the lower left. This is a reference-backed presentation omission, with the intentional native white/navy adaptation accepted.

Evidence: [shots/026-123-launch.png](shots/026-123-launch.png), [evidence/desktop-main-serial.log](evidence/desktop-main-serial.log).

### L022 - Empty Trash leaves the caution-icon area completely blank

Area: look. Severity: P3.

Steps: Stage a file and open Special > Empty Trash. Compare the dialog with the local Mac OS 8.1 caution-alert capture.

Observed: The large left-hand icon space was empty; only the text and two buttons identified the destructive alert.

Expected and basis: The actual s8_alert_modal capture contains a yellow caution triangle in this same layout. This is a named-reference drawing omission, not a demand to change the intentional wording or two menu bars.

Evidence: [shots/006-empty-confirm.png](shots/006-empty-confirm.png), [evidence/references/mac81-alert.png](evidence/references/mac81-alert.png).

### L023 - The Trash alert has a heavy black slab border instead of the Platinum alert edge

Area: look. Severity: P3.

Steps: Open the same confirmation and compare the complete frame at native size.

Observed: A roughly seven-pixel black outer band dominated the dialog. The reference has a thin black edge with a subtle raised/tinted border.

Expected and basis: Named reference: system7-decomp/specs/sys8/controls.md section 6 and s8_alert_modal.png. The gap is the alert frame, a surface earlier audits had not reached with buttons; general window-frame criticism stays under F16.

Evidence: [shots/006-empty-confirm.png](shots/006-empty-confirm.png), [evidence/references/mac81-alert.png](evidence/references/mac81-alert.png), [evidence/references/mac81-controls.md](evidence/references/mac81-controls.md).

### L024 - The confirmation understates the disk space occupied by a folder tree

Area: Trash. Severity: P3.

Steps: On trash-fixture.img stage A, containing CHILD/KEEP.TXT, and open Empty Trash before purging.

Observed: The alert counted three items but said they use 1K. Independent FAT inspection found three one-cluster objects at 512 bytes/cluster, totaling 1536 bytes, which needs 2K when rounded upward.

Expected and basis: A figure labeled disk space should reflect allocated storage, including folder clusters, or identify itself as logical file bytes. This is my filesystem-feedback expectation; the Mac alert reference uses a disk-space figure.

Evidence: [shots/100-trash-tree-count.png](shots/100-trash-tree-count.png), [evidence/trash-tree-space.json](evidence/trash-tree-space.json), [images/trash-fixture.img](images/trash-fixture.img).

### L025 - CD backslash does not return to the root

Area: DOS. Severity: P2.

Steps: Fresh tracer_boot + dos-fixture: MD TEST; CD TEST; CD \; CD. Then CD A:\; CD as a control.

Observed: The unqualified root form left the location A:\TEST without an error. The drive-qualified form returned to A:\.

Expected and basis: DOS-manual CHDIR explicitly says CD \ returns to the root. Both root spellings should resolve consistently.

Evidence: [evidence/dos-edge-serial.log](evidence/dos-edge-serial.log), [shots/110-dos-root-path.png](shots/110-dos-root-path.png).

### L026 - MD accepts a wildcard character as a literal directory name

Area: DOS. Severity: P2.

Steps: MD BAD?NAME; CD BAD?NAME; CD; then inspect the stopped FAT disk with mdir.

Observed: The directory was created and the guest entered A:\BAD?NAME. mtools sanitized its display to BAD_NAME, exposing an invalid DOS name on disk.

Expected and basis: Reject wildcard characters in new directory names. This follows DOS-manual filename/wildcard rules and my DOS 8.3 naming expectation. Long over-8.3 names were correctly refused in the control tests.

Evidence: [evidence/dos-edge-serial.log](evidence/dos-edge-serial.log), [shots/111-dos-illegal-names.png](shots/111-dos-illegal-names.png), [evidence/dos-edge-listing-root.txt](evidence/dos-edge-listing-root.txt).

### L027 - MD accepts reserved device names as real directories

Area: DOS. Severity: P2.

Steps: On a fresh DOS fixture issue MD CON; CD CON; CD. MD AUX was also exercised in dos-corrected.

Observed: Both reserved names were accepted as directories; CD CON put the shell in A:\CON.

Expected and basis: Reject reserved DOS device names for filesystem objects. Expectation from my DOS 3.x memory; DOS-manual character-device sections define CON and AUX as devices.

Evidence: [evidence/dos-edge-serial.log](evidence/dos-edge-serial.log), [shots/111-dos-illegal-names.png](shots/111-dos-illegal-names.png), [evidence/dos-corrected-serial.log](evidence/dos-corrected-serial.log).

### L028 - COPY treats /A and /B switches as filenames and ignores the requested destination

Area: DOS. Severity: P2.

Steps: The fixture CTRLZ.TXT contains BEFORE, byte 0x1A, AFTER and CR/LF. COPY CTRLZ.TXT /A ASCII2.TXT; TYPE ASCII2.TXT; DIR. Then COPY /A CTRLZ.TXT ASCII3.TXT and COPY CTRLZ.TXT /B BIN2.TXT; TYPE each requested target.

Observed: Each copy reported success, but none of ASCII2.TXT, ASCII3.TXT or BIN2.TXT existed. DIR exposed a literal /A file; the /B variant likewise used the switch as its destination. The prefix /A run used the newly created switch-named file as its source and CTRLZ.TXT as its destination. A trailing /A was also ignored in the earlier test.

Expected and basis: DOS-manual COPY accepts /A and /B before or after filenames. Parse switches separately, honor the requested destination, and apply text/binary semantics. The earlier trailing-/A screenshot alone is not used to claim source EOF truncation, because that position applies to the destination.

Evidence: [shots/133-dos-source-ascii-switch.png](shots/133-dos-source-ascii-switch.png), [shots/134-dos-prefix-and-binary-switch.png](shots/134-dos-prefix-and-binary-switch.png), [evidence/dos-ascii-serial.log](evidence/dos-ascii-serial.log), [evidence/dos-ascii-root-raw.json](evidence/dos-ascii-root-raw.json).

### L029 - COPY cannot concatenate explicitly named files with plus

Area: DOS. Severity: P2.

Steps: On dos-fixture.img, COPY KEEP.TXT+CTRLZ.TXT MERGE.TXT; TYPE MERGE.TXT.

Observed: Both sources existed, but COPY said File not found and created no usable merged file.

Expected and basis: DOS-manual COPY documents concatenation with plus. This is the explicit-name concatenation grammar, separate from known K11 wildcard enumeration.

Evidence: [evidence/dos-corrected-serial.log](evidence/dos-corrected-serial.log).

### L030 - New filesystem entries receive an invalid zero month/day timestamp

Area: DOS. Severity: P2.

Steps: In dos-env: ECHO METADATA > STAMP.TXT; DATE; TIME. Stop QEMU and inspect STAMP.TXT with mdir -a. New directories and COPY destinations in the other DOS sessions provide controls.

Observed: The guest clock reported October 4, 2026, but new entries had 1980-00-00 00:00. mdir exposed the invalid month/day. This also appeared on Finder-created service entries.

Expected and basis: Store the actual last-modified date/time, or at minimum a valid FAT date. DOS-manual file/directory descriptions include modification timestamps. This is metadata correctness, not the already-known abbreviated DIR layout.

Evidence: [shots/118-dos-timestamp-clock.png](shots/118-dos-timestamp-clock.png), [evidence/dos-env-root-after.txt](evidence/dos-env-root-after.txt), [evidence/dos-corrected-root-after.txt](evidence/dos-corrected-root-after.txt).

### L031 - FOR works in a batch but is not recognized at the prompt

Area: DOS. Severity: P2.

Steps: SET AUDIT=VALUE; run ENV.BAT as a control. Then type FOR %f IN (ONE TWO) DO ECHO %f directly at the prompt.

Observed: The batch FOR printed ONE and TWO. The single-percent interactive form returned Bad command or file name.

Expected and basis: DOS-manual FOR, printed p.145, explicitly instructs using one percent sign outside a batch file.

Evidence: [shots/115-dos-batch-environment.png](shots/115-dos-batch-environment.png), [shots/116-dos-interactive-control.png](shots/116-dos-interactive-control.png), [evidence/dos-env-serial.log](evidence/dos-env-serial.log), [fixtures/ENV.BAT](fixtures/ENV.BAT).

### L032 - IF is not recognized at the interactive prompt

Area: DOS. Severity: P2.

Steps: With SELF.BIN present, type IF EXIST SELF.BIN ECHO YES. Compare the IF EXIST control in ENV.BAT.

Observed: The direct command returned Bad command or file name; the batch control printed BATCH-IF-YES.

Expected and basis: My DOS 3.x command-processor expectation is that IF can execute a conditional command at the prompt. The original manual documents IF syntax; the interactive applicability here is labeled memory-based.

Evidence: [shots/116-dos-interactive-control.png](shots/116-dos-interactive-control.png), [evidence/dos-env-serial.log](evidence/dos-env-serial.log), [fixtures/ENV.BAT](fixtures/ENV.BAT).

### L033 - F3 command recall does nothing

Area: DOS. Severity: P2.

Steps: Fresh DOS: ECHO REPEAT, Return. At the empty next prompt press F3, wait one second, then Return.

Observed: The command line stayed empty and Return produced another empty prompt. No previous command was copied.

Expected and basis: DOS-manual MS-DOS editing keys, printed pp.154-155, specifies F3 plus Return to repeat the previous command.

Evidence: [shots/126-dos-f3.png](shots/126-dos-f3.png), [evidence/dos-final-serial.log](evidence/dos-final-serial.log).

### L034 - COMMAND /C cannot launch a secondary command processor

Area: DOS. Severity: P2.

Steps: On the supplied tracer_boot + dos-fixture.img prompt, type COMMAND /C ECHO CHILD.

Observed: Bad command or file name; CHILD was never printed.

Expected and basis: DOS-manual COMMAND documents /C execution in a secondary processor. This finding is limited to the supplied runnable image and disk, not unbooted artifacts elsewhere in the repository.

Evidence: [shots/127-dos-secondary-shell.png](shots/127-dos-secondary-shell.png), [evidence/dos-final-serial.log](evidence/dos-final-serial.log).

### L035 - EXIT terminates the permanent primary shell and leaves no usable prompt

Area: DOS. Severity: P1.

Steps: Fresh tracer_boot + dos-fixture.img. The boot trace specifies SHELL=COMMAND.COM /P /E:512. Type EXIT, wait, then VER and Return.

Observed: The trace ended SHELL-EXIT / SHELL-DONE. VER produced no output and the screen stayed at the EXIT command. Only the external QMP controller could end the run.

Expected and basis: A permanent primary DOS shell must remain available; /P should prevent exiting it into a dead machine. The precise primary-shell EXIT rule is from my DOS memory, with /P permanence grounded in DOS-manual COMMAND.

Evidence: [shots/128-dos-permanent-exit.png](shots/128-dos-permanent-exit.png), [evidence/dos-final-serial.log](evidence/dos-final-serial.log), [evidence/actions.jsonl](evidence/actions.jsonl).

### L036 - CHKDSK is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img; type CHKDSK at the prompt, as in dos-corrected.json.

Observed: The shell returned Bad command or file name. No CHKDSK executable was available on the tested disk or reached through the configured path.

Expected and basis: DOS-manual has a named CHKDSK command section providing a way to inspect/check the filesystem. This is an installation/feature gap on these images, not a claim about host source files or an instruction to format the original images.

Evidence: [evidence/dos-corrected-serial.log](evidence/dos-corrected-serial.log), [dos-corrected.json](dos-corrected.json).

### L037 - FORMAT is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img; type FORMAT at the prompt, as in dos-corrected.json.

Observed: The shell returned Bad command or file name. No FORMAT executable was available on the tested disk or reached through the configured path.

Expected and basis: DOS-manual has a named FORMAT command section providing a way to initialize a disk. This is an installation/feature gap on these images, not a claim about host source files or an instruction to format the original images.

Evidence: [evidence/dos-corrected-serial.log](evidence/dos-corrected-serial.log), [dos-corrected.json](dos-corrected.json).

### L038 - DISKCOPY is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img; type DISKCOPY at the prompt, as in dos-corrected.json.

Observed: The shell returned Bad command or file name. No DISKCOPY executable was available on the tested disk or reached through the configured path.

Expected and basis: DOS-manual has a named DISKCOPY command section providing a way to copy a disk. This is an installation/feature gap on these images, not a claim about host source files or an instruction to format the original images.

Evidence: [evidence/dos-corrected-serial.log](evidence/dos-corrected-serial.log), [dos-corrected.json](dos-corrected.json).

### L039 - ATTRIB is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img; type ATTRIB at the prompt, as in dos-corrected.json.

Observed: The shell returned Bad command or file name. No ATTRIB executable was available on the tested disk or reached through the configured path.

Expected and basis: DOS-manual has a named ATTRIB command section providing a way to inspect or change read-only/archive attributes. This is an installation/feature gap on these images, not a claim about host source files or an instruction to format the original images.

Evidence: [evidence/dos-corrected-serial.log](evidence/dos-corrected-serial.log), [dos-corrected.json](dos-corrected.json).

### L040 - TREE is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img; type TREE at the prompt, as in dos-corrected.json.

Observed: The shell returned Bad command or file name. No TREE executable was available on the tested disk or reached through the configured path.

Expected and basis: DOS-manual has a named TREE command section providing a way to inspect the directory tree. This is an installation/feature gap on these images, not a claim about host source files or an instruction to format the original images.

Evidence: [evidence/dos-corrected-serial.log](evidence/dos-corrected-serial.log), [dos-corrected.json](dos-corrected.json).

### L041 - FIND is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img; type FIND at the prompt, as in dos-corrected.json.

Observed: The shell returned Bad command or file name. No FIND executable was available on the tested disk or reached through the configured path.

Expected and basis: DOS-manual has a named FIND command section providing a way to filter text for a string. This is an installation/feature gap on these images, not a claim about host source files or an instruction to format the original images.

Evidence: [evidence/dos-corrected-serial.log](evidence/dos-corrected-serial.log), [dos-corrected.json](dos-corrected.json).

### L042 - SORT is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img; type SORT at the prompt, as in dos-corrected.json.

Observed: The shell returned Bad command or file name. No SORT executable was available on the tested disk or reached through the configured path.

Expected and basis: DOS-manual has a named SORT command section providing a way to sort text input. This is an installation/feature gap on these images, not a claim about host source files or an instruction to format the original images.

Evidence: [evidence/dos-corrected-serial.log](evidence/dos-corrected-serial.log), [dos-corrected.json](dos-corrected.json).

### L043 - MORE is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img; type MORE at the prompt, as in dos-corrected.json.

Observed: The shell returned Bad command or file name. No MORE executable was available on the tested disk or reached through the configured path.

Expected and basis: DOS-manual has a named MORE command section providing a way to page text output. This is an installation/feature gap on these images, not a claim about host source files or an instruction to format the original images.

Evidence: [evidence/dos-corrected-serial.log](evidence/dos-corrected-serial.log), [dos-corrected.json](dos-corrected.json).

### L044 - DATE() returns the current year as 1926 instead of 2026

Area: database. Severity: P2.

Steps: Fresh SAMIR: SET CENTURY ON; ? DATE(); ? YEAR(DATE()). Compare the contemporaneous DOS DATE control.

Observed: SAMIR printed 10/04/1926 and 1926. DOS on these images reported 10/04/2026.

Expected and basis: DB-manual DATE obtains the current system date. This is not criticism of the deliberately retained CTOD year-00 -> 1900/accounting behavior; it is the wrong century for the live system clock.

Evidence: [shots/c002-date.png](shots/c002-date.png), [shots/118-dos-timestamp-clock.png](shots/118-dos-timestamp-clock.png), [evidence/db-confirm-serial.log](evidence/db-confirm-serial.log).

### L045 - Empty and invalid dates print malformed digits

Area: database. Severity: P2.

Steps: In a fresh SAMIR prompt: ? CTOD(""); ? CTOD("99/99/99"); ? CTOD("02/29/00"). Compare valid CTOD("02/29/04").

Observed: The three empty/invalid results displayed 11/24//- rather than a blank date or a relevant error. The valid leap-day control displayed normally.

Expected and basis: Named reference: local dates-and-century spec, CTOD and blank-date sections, backed by the III+ blank-date idiom. Represent an empty date without invented digits.

Evidence: [shots/c002-date.png](shots/c002-date.png), [shots/b010-dates.png](shots/b010-dates.png), [evidence/db-confirm-serial.log](evidence/db-confirm-serial.log), [evidence/references/dbase-dates.md](evidence/references/dbase-dates.md).

### L046 - The next dot prompt is glued to query output

Area: database. Severity: P3.

Steps: Type ? 2, then another query; also try a string expression or DBF().

Observed: The next . prompt appeared on the same line immediately after the value, e.g. 4. ? EXP(1) and CLIENTS. ? BOF(). Numeric values and the next command visually ran together.

Expected and basis: My memory of the dBASE dot-prompt interface is a fresh prompt line after a completed command. Keep output and the input prompt visually distinct; the manual examples also separate them.

Evidence: [shots/db-final-01.png](shots/db-final-01.png), [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [shots/c002-date.png](shots/c002-date.png).

### L047 - CLOSE FORMAT and CLOSE INDEX close the database itself

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF; CLOSE FORMAT; ? RECCOUNT(); LIST. Reopen and try CLOSE INDEX. The broader test also tried CLOSE PROCEDURE.

Observed: RECCOUNT became zero and LIST no longer listed the table. Reopening the DBF restored its rows. A command for a different file type closed the active database too.

Expected and basis: DB-manual CLOSE, printed p.U5-53, distinguishes FORMAT, INDEX, PROCEDURE and DATABASES. Only the requested file type should close.

Evidence: [shots/c004-close-indexes.png](shots/c004-close-indexes.png), [evidence/db-confirm-serial.log](evidence/db-confirm-serial.log), [evidence/db-functions-serial.log](evidence/db-functions-serial.log).

### L048 - A valid alias-qualified field cannot be read

Area: database. Severity: P2.

Steps: SELECT 1; USE CLIENTS.DBF ALIAS ONE; GO 1; SELECT 2; USE CLIENTS.DBF ALIAS TWO; GO 2; ? BAL; SELECT ONE; ? BAL; ? TWO->BAL; SELECT 2; ? BAL.

Observed: Direct queries returned 123.45 in ONE and -42 in TWO, but TWO->BAL returned 1 File does not exist. Both work areas were open and selectable.

Expected and basis: DB-manual work areas/aliases explicitly permits alias->field to read a field in an unselected open area. The alias is not a filename to reopen.

Evidence: [shots/c006-areas.png](shots/c006-areas.png), [evidence/db-confirm-serial.log](evidence/db-confirm-serial.log).

### L049 - TYPE reports its string argument type instead of the named expression type

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF; ? TYPE("BAL"),TYPE("1+1"),TYPE("MISSING"),TYPE("DATE()").

Observed: The output was C C C C, including for the numeric field, arithmetic and undefined variable.

Expected and basis: DB-manual TYPE(), printed p.U6-86, evaluates the expression named by its character argument. Expected N, N, U and D (the local function reference documents the date code).

Evidence: [shots/db-final-01.png](shots/db-final-01.png), [evidence/db-final-serial.log](evidence/db-final-serial.log), [evidence/references/dbase-functions.md](evidence/references/dbase-functions.md).

### L050 - Division by zero raises an error instead of the original overflow result

Area: database. Severity: P2.

Steps: At the dot prompt type ? 1/0.

Observed: SAMIR printed 39 Numeric overflow (data was lost).

Expected and basis: The real III+ C1/E2 captures, summarized in local re/mint-results-002.md, explicitly establish asterisk-filled overflow output and no trapped error for 1/0. This is a measured compatibility difference, not a mathematical preference.

Evidence: [shots/db-final-02.png](shots/db-final-02.png), [evidence/db-final-serial.log](evidence/db-final-serial.log), [evidence/references/dbase-minted-002.md](evidence/references/dbase-minted-002.md).

### L051 - INDEX ON cannot create an index

Area: database. Severity: P2.

Steps: Open CLIENTS.DBF in SAMIR, then issue INDEX ON NAME TO TESTIDX.

Observed: 16 Unrecognized command verb; the requested workflow did not start. For absent named input files, no file-specific diagnostic was reached.

Expected and basis: DB-manual has a named INDEX command section to create a persistent NDX for keyed navigation. Existing K08 covers other already-audited editor/aggregate/export gaps; this is an additional reached command.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L052 - REINDEX is unavailable

Area: database. Severity: P2.

Steps: Open CLIENTS.DBF in SAMIR, then issue REINDEX.

Observed: 16 Unrecognized command verb; the requested workflow did not start. For absent named input files, no file-specific diagnostic was reached.

Expected and basis: DB-manual has a named REINDEX command section to rebuild database indexes. Existing K08 covers other already-audited editor/aggregate/export gaps; this is an additional reached command.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L053 - APPEND FROM cannot import records

Area: database. Severity: P2.

Steps: Open CLIENTS.DBF in SAMIR, then issue APPEND FROM CLIENTS.DBF.

Observed: 7 File already exists; no import occurred.

Expected and basis: DB-manual has a named APPEND command section to append records from another DBF. Existing K08 covers other already-audited editor/aggregate/export gaps; this is an additional reached command.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L054 - SAVE TO cannot persist memory variables

Area: database. Severity: P2.

Steps: Open CLIENTS.DBF in SAMIR, then issue SAVE TO MEMSAVE.

Observed: 16 Unrecognized command verb; the requested workflow did not start. For absent named input files, no file-specific diagnostic was reached.

Expected and basis: DB-manual has a named SAVE command section to save variables in a MEM file. Existing K08 covers other already-audited editor/aggregate/export gaps; this is an additional reached command.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L055 - RESTORE FROM is unavailable

Area: database. Severity: P2.

Steps: Open CLIENTS.DBF in SAMIR, then issue RESTORE FROM MEMSAVE.

Observed: 16 Unrecognized command verb; the requested workflow did not start. For absent named input files, no file-specific diagnostic was reached.

Expected and basis: DB-manual has a named RESTORE command section to restore a saved variable set. Existing K08 covers other already-audited editor/aggregate/export gaps; this is an additional reached command.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L056 - REPORT FORM is unavailable

Area: database. Severity: P2.

Steps: Open CLIENTS.DBF in SAMIR, then issue REPORT FORM REPORT.

Observed: 16 Unrecognized command verb; the requested workflow did not start. For absent named input files, no file-specific diagnostic was reached.

Expected and basis: DB-manual has a named REPORT command section to run a report definition. Existing K08 covers other already-audited editor/aggregate/export gaps; this is an additional reached command.

Evidence: [evidence/db-suite-serial.log](evidence/db-suite-serial.log), [db-suite.json](db-suite.json).

### L057 - LABEL FORM is unavailable

Area: database. Severity: P2.

Steps: Open CLIENTS.DBF in SAMIR, then issue LABEL FORM LABEL.

Observed: 16 Unrecognized command verb; the requested workflow did not start. For absent named input files, no file-specific diagnostic was reached.

Expected and basis: DB-manual has a named LABEL command section to run a label definition. Existing K08 covers other already-audited editor/aggregate/export gaps; this is an additional reached command.

Evidence: [evidence/db-suite-serial.log](evidence/db-suite-serial.log), [db-suite.json](db-suite.json).

### L058 - SET ALTERNATE is unavailable

Area: database. Severity: P2.

Steps: At the dot prompt issue SET ALTERNATE TO ALT.TXT. ON/OFF pairs and bare TO were also used where listed in db-functions.json.

Observed: The option was refused with 7 File already exists; it did not take effect.

Expected and basis: DB-manual has a specific SET ALTERNATE section to capture commands/output to a text transcript. The misleading generic error-number translation is already known under K08; the new finding here is the missing setting/workflow.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L059 - SET DEFAULT is unavailable

Area: database. Severity: P2.

Steps: At the dot prompt issue SET DEFAULT TO A. ON/OFF pairs and bare TO were also used where listed in db-functions.json.

Observed: The option was refused with 7 File already exists; it did not take effect.

Expected and basis: DB-manual has a specific SET DEFAULT section to select the default drive for file searches. The misleading generic error-number translation is already known under K08; the new finding here is the missing setting/workflow.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L060 - SET PATH is unavailable

Area: database. Severity: P2.

Steps: At the dot prompt issue SET PATH TO A:\. ON/OFF pairs and bare TO were also used where listed in db-functions.json.

Observed: The option was refused with 7 File already exists; it did not take effect.

Expected and basis: DB-manual has a specific SET PATH section to set the database file search path. The misleading generic error-number translation is already known under K08; the new finding here is the missing setting/workflow.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L061 - SET ESCAPE is unavailable

Area: database. Severity: P2.

Steps: At the dot prompt issue SET ESCAPE ON. ON/OFF pairs and bare TO were also used where listed in db-functions.json.

Observed: The option was refused with 7 File already exists; it did not take effect.

Expected and basis: DB-manual has a specific SET ESCAPE section to control program interruption. The misleading generic error-number translation is already known under K08; the new finding here is the missing setting/workflow.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L062 - SET CONFIRM is unavailable

Area: database. Severity: P2.

Steps: At the dot prompt issue SET CONFIRM ON. ON/OFF pairs and bare TO were also used where listed in db-functions.json.

Observed: The option was refused with 7 File already exists; it did not take effect.

Expected and basis: DB-manual has a specific SET CONFIRM section to require Return to complete field entry. The misleading generic error-number translation is already known under K08; the new finding here is the missing setting/workflow.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L063 - SET BELL is unavailable

Area: database. Severity: P2.

Steps: At the dot prompt issue SET BELL OFF. ON/OFF pairs and bare TO were also used where listed in db-functions.json.

Observed: The option was refused with 7 File already exists; it did not take effect.

Expected and basis: DB-manual has a specific SET BELL section to control the entry/error bell. The misleading generic error-number translation is already known under K08; the new finding here is the missing setting/workflow.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L064 - SET CONSOLE is unavailable

Area: database. Severity: P2.

Steps: At the dot prompt issue SET CONSOLE OFF. ON/OFF pairs and bare TO were also used where listed in db-functions.json.

Observed: The option was refused with 7 File already exists; it did not take effect.

Expected and basis: DB-manual has a specific SET CONSOLE section to control screen output. The misleading generic error-number translation is already known under K08; the new finding here is the missing setting/workflow.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L065 - SET HEADING is unavailable

Area: database. Severity: P2.

Steps: At the dot prompt issue SET HEADING OFF. ON/OFF pairs and bare TO were also used where listed in db-functions.json.

Observed: The option was refused with 7 File already exists; it did not take effect.

Expected and basis: DB-manual has a specific SET HEADING section to control column headings. The misleading generic error-number translation is already known under K08; the new finding here is the missing setting/workflow.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L066 - SET PROCEDURE is unavailable

Area: database. Severity: P2.

Steps: At the dot prompt issue SET PROCEDURE TO SIMPLE. ON/OFF pairs and bare TO were also used where listed in db-functions.json.

Observed: The option was refused with 7 File already exists; it did not take effect.

Expected and basis: DB-manual has a specific SET PROCEDURE section to load a procedure library. The misleading generic error-number translation is already known under K08; the new finding here is the missing setting/workflow.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L067 - EXP() is absent from the function library

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF; ? EXP(1).

Observed: 31 Invalid function name, including with the valid ordinary argument shown here.

Expected and basis: DB-manual Chapter 6 documents this function; local function specs enumerate it from III+ HELP.DBS. Return approximately 2.71828. This is function availability, not a proposed exact OS/version literal or an untested printer driver.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L068 - LOG() is absent from the function library

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF; ? LOG(1).

Observed: 31 Invalid function name, including with the valid ordinary argument shown here.

Expected and basis: DB-manual Chapter 6 documents this function; local function specs enumerate it from III+ HELP.DBS. Return zero. This is function availability, not a proposed exact OS/version literal or an untested printer driver.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L069 - SQRT() is absent from the function library

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF; ? SQRT(4).

Observed: 31 Invalid function name, including with the valid ordinary argument shown here.

Expected and basis: DB-manual Chapter 6 documents this function; local function specs enumerate it from III+ HELP.DBS. Return two. This is function availability, not a proposed exact OS/version literal or an untested printer driver.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L070 - TIME() is absent from the function library

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF; ? TIME().

Observed: 31 Invalid function name, including with the valid ordinary argument shown here.

Expected and basis: DB-manual Chapter 6 documents this function; local function specs enumerate it from III+ HELP.DBS. Return the current time string. This is function availability, not a proposed exact OS/version literal or an untested printer driver.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L071 - RECSIZE() is absent from the function library

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF; ? RECSIZE().

Observed: 31 Invalid function name, including with the valid ordinary argument shown here.

Expected and basis: DB-manual Chapter 6 documents this function; local function specs enumerate it from III+ HELP.DBS. Return 31 for this fixture, including its deletion byte. This is function availability, not a proposed exact OS/version literal or an untested printer driver.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L072 - DISKSPACE() is absent from the function library

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF; ? DISKSPACE().

Observed: 31 Invalid function name, including with the valid ordinary argument shown here.

Expected and basis: DB-manual Chapter 6 documents this function; local function specs enumerate it from III+ HELP.DBS. Return the available disk byte count. This is function availability, not a proposed exact OS/version literal or an untested printer driver.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L073 - VERSION() is absent from the function library

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF; ? VERSION().

Observed: 31 Invalid function name, including with the valid ordinary argument shown here.

Expected and basis: DB-manual Chapter 6 documents this function; local function specs enumerate it from III+ HELP.DBS. Return a version identification string. This is function availability, not a proposed exact OS/version literal or an untested printer driver.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L074 - OS() is absent from the function library

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF; ? OS().

Observed: 31 Invalid function name, including with the valid ordinary argument shown here.

Expected and basis: DB-manual Chapter 6 documents this function; local function specs enumerate it from III+ HELP.DBS. Return an operating-system identification string. This is function availability, not a proposed exact OS/version literal or an untested printer driver.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L075 - PCOL() is absent from the function library

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF; ? PCOL().

Observed: 31 Invalid function name, including with the valid ordinary argument shown here.

Expected and basis: DB-manual Chapter 6 documents this function; local function specs enumerate it from III+ HELP.DBS. Return the printer column position. This is function availability, not a proposed exact OS/version literal or an untested printer driver.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L076 - PROW() is absent from the function library

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF; ? PROW().

Observed: 31 Invalid function name, including with the valid ordinary argument shown here.

Expected and basis: DB-manual Chapter 6 documents this function; local function specs enumerate it from III+ HELP.DBS. Return the printer row position. This is function availability, not a proposed exact OS/version literal or an untested printer driver.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L077 - ROW() is absent from the function library

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF; ? ROW().

Observed: 31 Invalid function name, including with the valid ordinary argument shown here.

Expected and basis: DB-manual Chapter 6 documents this function; local function specs enumerate it from III+ HELP.DBS. Return the screen row position. This is function availability, not a proposed exact OS/version literal or an untested printer driver.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L078 - COL() is absent from the function library

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF; ? COL().

Observed: 31 Invalid function name, including with the valid ordinary argument shown here.

Expected and basis: DB-manual Chapter 6 documents this function; local function specs enumerate it from III+ HELP.DBS. Return the screen column position. This is function availability, not a proposed exact OS/version literal or an untested printer driver.

Evidence: [evidence/db-functions-serial.log](evidence/db-functions-serial.log), [db-functions.json](db-functions.json).

### L079 - Disk full is misreported as a read-only file

Area: database. Severity: P2.

Steps: Repeat the full-disk APPEND workload from L001; watch the first failure after R08. The fixture DBF has no read-only attribute and earlier edits succeed.

Observed: Every failed APPEND/REPLACE said 111 Cannot write to a read-only file. The actual volume had zero free bytes; the file was writable. This sends the user to the wrong recovery action.

Expected and basis: Report disk full (the original III+ catalog includes a disk-full diagnostic), and identify the affected file. This is the distinct error-feedback defect in L001, not its corrupt record-count defect.

Evidence: [shots/120-db-full-appended.png](shots/120-db-full-appended.png), [evidence/db-full-root-before.txt](evidence/db-full-root-before.txt), [evidence/db-full-serial.log](evidence/db-full-serial.log).

### L080 - The 2049th occupied cell fails as unexplained EDIT mode

Area: Initech 123. Severity: P2.

Steps: Replay cell_stress.py: fresh 123, repeat typing 1 followed by Down 2050 times, starting at A1. Inspect attempts 2048 and 2049; then Escape, Home, replace existing A1 with 2.

Observed: 2048 cells committed successfully. The next valid 1 at A2049 did not commit or advance: the mode changed to EDIT with no memory/capacity explanation. Another attempt appended a second 1 to the stranded edit. Editing existing A1 still worked.

Expected and basis: Either support the next cell or visibly explain the capacity limit without presenting a valid number as an input-editing problem. This is my resource-failure feedback expectation, not a claim that a finite-memory worksheet must hold every possible cell.

Evidence: [cell_stress.py](cell_stress.py), [evidence/cell-stress-serial.log](evidence/cell-stress-serial.log), [shots/cell-2048.png](shots/cell-2048.png), [shots/cell-2049.png](shots/cell-2049.png), [shots/cell-2050.png](shots/cell-2050.png), [shots/cell-existing-edit.png](shots/cell-existing-edit.png).

### L081 - APPEND is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type APPEND as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents APPEND to search additional directories for data files. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L082 - ASSIGN is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type ASSIGN as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents ASSIGN to redirect drive references. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L083 - BACKUP is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type BACKUP as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents BACKUP to back up files. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L084 - CHCP is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type CHCP as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents CHCP to inspect/select the console code page. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L085 - COMP is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type COMP as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents COMP to compare files. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L086 - CTTY is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type CTTY as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents CTTY to select the controlling terminal. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L087 - DISKCOMP is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type DISKCOMP as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents DISKCOMP to compare disks. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L088 - EXE2BIN is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type EXE2BIN as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents EXE2BIN to convert executable file formats. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L089 - FASTOPEN is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type FASTOPEN as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents FASTOPEN to provide the documented file-open cache. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L090 - FC is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type FC as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents FC to compare file contents. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L091 - GRAPHICS is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type GRAPHICS as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents GRAPHICS to enable graphics-screen printing. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L092 - JOIN is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type JOIN as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents JOIN to join a drive into a directory. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L093 - KEYB is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type KEYB as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents KEYB to select a keyboard layout. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L094 - LABEL is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type LABEL as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents LABEL to change a disk volume label. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L095 - MODE is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type MODE as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents MODE to configure console/printer/serial modes. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L096 - NLSFUNC is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type NLSFUNC as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents NLSFUNC to load country/code-page support. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L097 - PRINT is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type PRINT as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents PRINT to queue print jobs. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L098 - RECOVER is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type RECOVER as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents RECOVER to attempt damaged-file recovery. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L099 - REPLACE is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type REPLACE as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents REPLACE to replace files as a batch operation. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L100 - RESTORE is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type RESTORE as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents RESTORE to restore backed-up files. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L101 - SELECT is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type SELECT as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents SELECT to install/configure a DOS disk. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L102 - SHARE is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type SHARE as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents SHARE to provide DOS file-sharing/locking support. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L103 - SUBST is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type SUBST as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents SUBST to substitute a drive for a path. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L104 - SYS is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type SYS as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents SYS to transfer system files to a disk. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L105 - XCOPY is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type XCOPY as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents XCOPY to copy directory trees. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L106 - DEBUG is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type DEBUG as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents DEBUG to inspect/debug DOS programs. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L107 - EDLIN is unavailable from the supplied DOS installation

Area: DOS. Severity: P2.

Steps: Boot tracer_boot + dos-fixture.img and type EDLIN as in dos-utilities.json.

Observed: Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.

Expected and basis: The original DOS-manual documents EDLIN to edit text files at the DOS prompt. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.

Evidence: [evidence/dos-utilities-serial.log](evidence/dos-utilities-serial.log), [dos-utilities.json](dos-utilities.json).

### L108 - A macro variable containing a command is not expanded for execution

Area: database. Severity: P2.

Steps: Run DO STRESS on db-program-fixture.img, prepared as documented in the journal. STRESS.PRG loops N to 1000, prints LOOP-PASS, stores "? N + 5" to CMD, then executes &CMD.

Observed: The loop and IF branch worked, then the macro statement returned 16 Unrecognized command verb instead of printing 1005. N remained 1000 at the prompt.

Expected and basis: DB-manual macro substitution and the local III+ language reference permit an ampersand memory-variable substitution in commands. This is independent of the old DO-file/USE integration issue K08: this program loaded and ran preceding statements.

Evidence: [fixtures/STRESS.PRG](fixtures/STRESS.PRG), [evidence/db-program-serial.log](evidence/db-program-serial.log), [shots/program-01.png](shots/program-01.png).

### L109 - SORT TO is unavailable

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF, then SORT TO SORTED ON NAME. The exact preceding file-operation attempts are in db-program.json.

Observed: 16 Unrecognized command verb. SORT and TOTAL had the correct existing fields. The TYPE/RENAME/ERASE files were absent after the refused COPY FILE control; no file-specific diagnostic was reached.

Expected and basis: DB-manual has the named SORT command to write a sorted copy of the active table. The finding is the unrecognized command entry point; successful missing-file handling is not claimed.

Evidence: [evidence/db-program-serial.log](evidence/db-program-serial.log), [db-program.json](db-program.json).

### L110 - TYPE is not recognized at the database prompt

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF, then TYPE COPY.PRG. The exact preceding file-operation attempts are in db-program.json.

Observed: 16 Unrecognized command verb. SORT and TOTAL had the correct existing fields. The TYPE/RENAME/ERASE files were absent after the refused COPY FILE control; no file-specific diagnostic was reached.

Expected and basis: DB-manual has the named TYPE command to view an ASCII file, or report that the requested file is absent. The finding is the unrecognized command entry point; successful missing-file handling is not claimed.

Evidence: [evidence/db-program-serial.log](evidence/db-program-serial.log), [db-program.json](db-program.json).

### L111 - RENAME is not recognized at the database prompt

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF, then RENAME COPY.PRG TO RENAMED.PRG. The exact preceding file-operation attempts are in db-program.json.

Observed: 16 Unrecognized command verb. SORT and TOTAL had the correct existing fields. The TYPE/RENAME/ERASE files were absent after the refused COPY FILE control; no file-specific diagnostic was reached.

Expected and basis: DB-manual has the named RENAME command to rename a file, or identify a missing source. The finding is the unrecognized command entry point; successful missing-file handling is not claimed.

Evidence: [evidence/db-program-serial.log](evidence/db-program-serial.log), [db-program.json](db-program.json).

### L112 - ERASE is not recognized at the database prompt

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF, then ERASE RENAMED.PRG. The exact preceding file-operation attempts are in db-program.json.

Observed: 16 Unrecognized command verb. SORT and TOTAL had the correct existing fields. The TYPE/RENAME/ERASE files were absent after the refused COPY FILE control; no file-specific diagnostic was reached.

Expected and basis: DB-manual has the named ERASE command to erase a file, or identify a missing source. The finding is the unrecognized command entry point; successful missing-file handling is not claimed.

Evidence: [evidence/db-program-serial.log](evidence/db-program-serial.log), [db-program.json](db-program.json).

### L113 - TOTAL ON is unavailable

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF, then TOTAL ON CITY TO TOTAL. The exact preceding file-operation attempts are in db-program.json.

Observed: 16 Unrecognized command verb. SORT and TOTAL had the correct existing fields. The TYPE/RENAME/ERASE files were absent after the refused COPY FILE control; no file-specific diagnostic was reached.

Expected and basis: DB-manual has the named TOTAL command to create grouped totals. The finding is the unrecognized command entry point; successful missing-file handling is not claimed.

Evidence: [evidence/db-program-serial.log](evidence/db-program-serial.log), [db-program.json](db-program.json).

### L114 - RUN cannot invoke a DOS command

Area: database. Severity: P2.

Steps: USE CLIENTS.DBF, then RUN VER. The exact preceding file-operation attempts are in db-program.json.

Observed: 16 Unrecognized command verb. SORT and TOTAL had the correct existing fields. The TYPE/RENAME/ERASE files were absent after the refused COPY FILE control; no file-specific diagnostic was reached.

Expected and basis: DB-manual has the named RUN command to run a DOS command and return to the database. The finding is the unrecognized command entry point; successful missing-file handling is not claimed.

Evidence: [evidence/db-program-serial.log](evidence/db-program-serial.log), [db-program.json](db-program.json).

### L115 - SET FUNCTION cannot redefine a function key

Area: database. Severity: P2.

Steps: SET FUNCTION 1 TO "LIST" at the SAMIR dot prompt.

Observed: 7 File already exists; no key definition was accepted.

Expected and basis: DB-manual SET FUNCTION documents this key-definition command. The generic error translation is already known; this is the absent setting.

Evidence: [evidence/db-program-serial.log](evidence/db-program-serial.log), [shots/program-16.png](shots/program-16.png).

### L116 - CLEAR MEMORY cannot clear variables

Area: database. Severity: P2.

Steps: After DO STRESS sets N to 1000, issue CLEAR MEMORY and ? N.

Observed: CLEAR MEMORY was unrecognized and N remained 1000. RELEASE of a named variable worked in a separate control.

Expected and basis: DB-manual CLEAR MEMORY specifies removing memory variables. Recognize the documented operation or provide a truthful unsupported response.

Evidence: [evidence/db-program-serial.log](evidence/db-program-serial.log), [shots/program-18.png](shots/program-18.png).

### L117 - CTENANT loses its input readouts when its window repaints

Area: apps. Severity: P3.

Steps: Fresh normal APPS: launch CTENANT at (143,124); press Q; click content (250,250). Capture, then drag its title (300,180) by (+50,+50).

Observed: Both Key q lines and the Click readout vanished after the move, leaving only the startup sentence. The window was not closed or relaunched.

Expected and basis: My ordinary GUI expectation is that moving/revealing a window preserves its displayed state. CTENANT is a test application, so this is recorded as a polish/readout defect rather than loss of a user document.

Evidence: [shots/135-ctenant-before-move.png](shots/135-ctenant-before-move.png), [shots/136-ctenant-after-move.png](shots/136-ctenant-after-move.png), [evidence/gui-polish-serial.log](evidence/gui-polish-serial.log).

### L118 - Trash stages and purges a locked file without a protection-specific refusal

Area: Trash. Severity: P2.

Steps: Boot tenants + trash-fixture.img, whose LOCKED.TXT is explicitly +r. Open root; drag LOCKED.TXT (123,153) to Trash; choose Empty Trash and press Return.

Observed: The protected file staged and purged with the ordinary confirmation. There was no locked-file explanation or explicit unlock step.

Expected and basis: MAC6 pp.88 and 112 says an individually locked document cannot be discarded until unlocked; DOS-manual ATTRIB also documents the underlying protection. If the GUI intentionally permits an override, it must disclose that choice. This is the GUI protection affordance, separate from DOS DEL in L006.

Evidence: [evidence/trash-fixture-preparation.json](evidence/trash-fixture-preparation.json), [evidence/trash-restore-serial.log](evidence/trash-restore-serial.log), [shots/105-trash-locked-confirm.png](shots/105-trash-locked-confirm.png), [shots/106-trash-locked-refused.png](shots/106-trash-locked-refused.png).

## 3. Still present

F07, F08, F09, F10, F11, F12, F13, F14, F15, F16, F17, F18.

G04, G05, G06, G07, G08, G09.

H02, H07.

J01, J02, J03.

K05, K06, K07, K08, K09, K10, K11, K12, K13, K14.

Only the portions actually reproduced are included by those IDs; this does not revive fixed subclaims inside old grouped findings. The five REPORT.md/TRIAGE.md pairs supplied the exclusion catalog (evidence/known-finding-catalog.json). No new L number is assigned to an already-described defect. Known IDs are grounded by the following evidence sets without repeating their descriptions:

| IDs | Current-session evidence |
| --- | --- |
| F07-F11 | gf-readme-open.png; 090-group-drag.png; 091-placement-reset.png; gf-size-before-close.png / gf-size-reopen.png; new-folder paths in desktop-main, finder-stress and gui-final logs |
| F12-F18 | menu-sweep-results.json and menu-sweep-serial.log; m-finder-*.png / m-system-*.png; ab-alt-tab.png; current Mac-reference comparison; 074-many-root.png |
| G04-G07 | finder-stress-serial.log, desktop-full-serial.log; 074/075/087/089 and 107/108 screenshots |
| G08, G09 | app-boundaries-serial.log / ab-inactive-click-exit.png; gui-final-serial.log / gf-second-disk-app-refused.png |
| H02, H07 | trash-collision-serial.log / 129-name-collision-refused.png / 132-cycle-refused.png; 074-many-root.png |
| J01-J03 | 080-many-shrink.png / 081-many-horizontal.png / 083-many-thumb.png / 088-multi-open-one.png; finder-stress-serial.log |
| K05-K10 | db-suite-serial.log, db-confirm-serial.log, gui-final-serial.log, dos-corrected-serial.log |
| K11-K14 | dos-last-serial.log and 123-dos-samir-redirect.png / 124-dos-wildcard-known.png; dos-edge-serial.log / 114-dos-console-recovery.png; b001-unlabeled-desktop.png and ab-123-menu-quit.png |

Paths in this table are under shots/ or evidence/ as their extensions/context indicate. Older F01/F02/F06 and G01/G02/G10/G11 behavior was not re-established by the exercised normal paths. K01-K04 and K15 have affirmative fix verdicts above. J04/J05 were not specifically re-reproduced, and no current verdict is claimed for them.

## 4. Coverage and reachability

This was a finite hands-on audit, not an exhaustive proof over every possible key sequence, file count, or hardware configuration. The command journal, suite JSON and raw traces are the exact coverage boundary. Bare missing-utility probes establish an unavailable entry point on these disks, not the behavior of an unseen implementation. Repeated missing commands are individually listed because the requested period-completeness bar includes each workflow.

| Area | Exercised | Could not reach / limits |
| --- | --- | --- |
| Desktop and Finder | Normal/unlabeled/full disks; volume, Trash and all supplied application icons, plus file/folder icon types; real file and folder movement; duplicate names; rejected cycles; restoration from Trash; single/multiple selection; Ctrl-A/O/N/W and disabled shortcuts; direct document Open; keyboard selection attempts; right/Control-click; placement and size reopening; an 81-entry user root with 17 dropped by the 64-entry ceiling; four/fifth folder windows | No usable rename, Get Info, Duplicate, list views, file associations, clipboard, search, aliases or preferences through the tested UI. Those previously known routes are not re-filed. Directory limits prevented accessing every fixture icon through Finder. |
| Menus | All eight HELLO menus and both rows in each; all NOTES File/Edit/About rows; all nonseparator Finder File/Edit/View/Special/Help rows; both Apple symbols; all four upper-bar About/Quit pairs; TenantFix File/Fixture/dynamically added Info; CTENANT and 123 File menus; keyboard equivalents; 45-second held menu with release at the highlighted row | Disabled rows stayed unavailable. The Info fixture's disabled item was reached and did not dispatch. Apparent gf-ctenant-menu-quit / gf-123-menu-quit frames were mislabeled attempted actions while Finder was frontmost; the actual menu-quit controls are ab-ctenant-menu-quit / ab-123-menu-quit. |
| Windows and controls | Activation, occlusion, title movement, grow, zoom/restore, collapse/expand, partial screen edges; held Close cancelled outside; Finder arrow clicks/repeat/leave, track and thumb paths; scrolled/moved views; active/inactive changes; last Finder closure | No real Open/Save chooser, editable modal, checkboxes, radios, sliders or preferences panel was reachable. The film modal blocked its background controls. Minimum-size invisible controls and exact no-motion thumb quantization from J04/J05 were not retested. |
| Trash/dialogs | Empty/nonempty/singleton views, file/folder trees, two colliding names, cancel/Escape, Return/OK, background blocking, button drag-out, full-disk refusal, read-only file, restoration, recursive deletion, live deleted descendants, count/space/icon changes | Put Away remained unavailable. A tentative early blank-repaint suspicion was discarded: final inspection of saved frames 008/009 shows README retained, and fresh cancellation replays also retained it. No finding relies on that mistaken initial reading. |
| HELLO/NOTES/TenantFix/CTENANT | Every offered built-in menu row; typing and content clicks; foreground changes; real disk-app registration/menus/exit; refusal when a second disk tenant was requested; click-through control; C tenant text/cell output, movement, collapse/expand and zoom | HELLO and NOTES remain demo tenants. No productive editor appeared. Disk-app concurrency was refused by the reached one-slot loader. Invalid/crashing mutant executables from older audits were not supplied and were not built. |
| Initech 123 | Row-1 real-Lotus fixture values; positive/negative/scientific numbers, rejected huge number, label spill/alignment, 104-character input, cancel/backspace, F1/F2/F5/Tab/PageDown/End/Delete attempts, cell clicks, arrows to A23/M1, resizing, global hotkey collision, menu/key Quit and relaunch, 2050 attempted numeric entries with a 2048-cell boundary, editing an existing cell after capacity exhaustion | No functioning formula evaluation, command menu, Save/Retrieve, range editing, worksheet formats, graph/print or macro UI was reached. Unreached downstream menu features are not claimed to have been individually executed. Home/arrows and normal label/number entry worked. End/Delete attempts were not treated as proven mismatches without a decisive applicable oracle. |
| Database | USE/CLOSE, all registered navigation/query/mutation families; record bounds/BOF/EOF, FOR/WHILE/REST/NEXT/RECORD/OFF; filters, deleted flags, REPLACE/type rejection/character truncation, append/recall/pack/zap and reopen; work areas/aliases; date/number/string expressions; dozens of functions/settings; program loading, variables, IF/ELSE and 1000-iteration DO WHILE; macro attempt; index/import/export/report/label/file-command probes; full disk; multi-area quit/recovery; desktop return; redirected DOS stdin | Editors/ASSIST, structure creation, aggregates, reporting/printing, index building and many commands/settings were not usable. No live historical dBASE emulator was run. No real printer/network/locking environment or DBT memo table was supplied. The test table is NAME C(10), CITY C(12), BAL N(8,2); early LAST/FIRST probes used nonexistent fields and are excluded from new defect claims. WAIT consumed the first C of a queued CLEAR in db-suite; that LEAR error is harness sequencing, not a product finding. |
| DOS | All built-ins found in command.c; DIR/TYPE/VER/CLS, directory navigation/creation/removal, COPY/DEL/REN aliases, ECHO/BREAK/SET/PROMPT/DATE/TIME/EXIT, redirects, batch parameters/SHIFT/GOTO/CALL/IF/FOR and environment expansion, console EOF/interrupt, child SAMIR/redirection, self-copy aliases, directory targets, 40-match deletion, all-files consent, readonly, invalid/reserved/long names, binary/text switches, concatenation, F3, secondary shell; named manual utilities inventoried individually | Missing external utilities have no deeper execution path to test. PATH/VOL/VERIFY were again unrecognized; these were already noted in pass-5 coverage and are not given new L IDs. No printer, alternate terminal, floppy swap, physical damaged disk, network or serial peer was attached. Bare CTTY/MODE/FORMAT/RECOVER probes never reached a utility. |
| Film and look | Interactive film image booted, compared with preview.webp; saving modal dragged to both screen corners, Return/Escape/Ctrl-Q/outside clicks tried, stable late frames captured; Platinum caution alert compared at native size; real Lotus screen compared with native sheet; Mac window/icon/type reference inspected | Film document bodies remained blank as already noted in earlier audit coverage. No pie chart, 570- figure or populated film worksheet was reachable; no new claim is made about those intentional numbers. Saving wording/progress is a deliberate scene prop, so a static progress bar with a responsive drag/modal loop is not mislabeled a CPU hang. No exact gamma/RGB equality or pixel-perfect font certification is claimed. |
| Performance and endurance | A 210-cycle mixed-app session: 70 launches/exits each for TenantFix, CTENANT and 123, 140 title moves, 140 collapse transitions, entries and clicks; 45-second held menu; released modifiers at the end. Separate 2050-entry spreadsheet stress; long command sessions, awkward storage and app ordering | No overnight soak, memory-pressure sweep across RAM sizes, real-486 cycle budget, flicker video/high-speed sampling, sound, networking, printing or removable-media hardware coverage. No general leak-free or production-stability claim. |

The mixed-app soak lasted 551.03 seconds. All 210 loads and exits completed; after the initial larger application allocations, the reported free heap stayed 3418352 for the remaining 208 teardown samples. That supports reclamation in this exercised sequence, not proof about every allocator. Its repeat-state screenshots were checked. The cell-entry session lasted 176.07 seconds and precisely exposed the 2048-cell limit. See evidence/soak-summary.json, evidence/soak-verification.json and evidence/cell-stress-summary.json.

Important controls that passed: byte-preserving normal/self COPY, real directory copies, path-qualified bulk deletion, consent decline, ordinary Trash recovery and collision suffixing, menu delivery, many app lifecycles, simple database persistence, a 1000-iteration program loop/IF, arithmetic and most tested string/date extraction functions, and single-area desktop suspension/return. The original III+ numeric goldens specifically vindicate negative half-tie rounding and -2^2=4; those were not filed as bugs. DOS batch environment expansion worked. The first DOS suite's attempts after CD backslash were in the wrong directory; only the corrected suite/location controls support the recent-fix verdicts.

## 5. Twenty fixes to make a period user believe in the system

1. Make disk-full database appends transactional enough to leave a reopenable table (L001).
2. Retire every open directory view when its Trash tree is purged (L002).
3. Make multiple work areas and QUIT safe (L003).
4. Commit destructive dialog buttons only on release inside (L004).
5. Enforce file protection consistently (L005, L006, L118).
6. Honor database SAFETY before ZAP (L007).
7. Add worksheet Save/Retrieve and actual persistent documents (L011).
8. Warn before discarding an edited worksheet (L008).
9. Make ordinary formulas and @ functions calculate (L009).
10. Deliver the Lotus command menu and cell editor (L010, L013).
11. Provide a useful NOTES document workflow (K07).
12. Make supplied text documents open in a viewer/editor (F07).
13. Honor database filters in real navigation/scans (K05).
14. Complete indexed database lookup and rebuild workflows (L051, L052).
15. Correct the date and expression-type results before trusting application logic (L044, L045, L049).
16. Make COPY switch parsing and directory navigation behave like DOS (L025, L028).
17. Keep the primary shell usable and restore its basic editing tools (L033-L035, L107).
18. Remove Finder's small inventory/window ceilings or explain and recover from them (G04, G06).
19. Provide naming, clipboard and alternate file views (F11, F12).
20. Finish the period visual weight, icons and alert art (F16, L022, L023).

These are ranked user outcomes, not a proposed source patch order. The separate NITS.tsv preserves every numbered defect, including lower-ranked command and polish gaps.

## Cleanup and handoff

Every audit emulator was quit through its own QMP connection and waited for. Per-session records identify the command, namespace-local PID, image hashes and exit status; all recorded QEMU exits are zero. PIDs can repeat across sandbox PID namespaces, so the session label/command is part of their identity. No process belonging to another user/run was killed. Evidence/driver error notes are retained rather than silently renamed into successful tests. The primary interactive driver was also ended after its last guest.

The authoritative final counts, times, cited-screenshot verification, original-image hash comparison, ASCII/link checks and cleanup accounting are in evidence/final-validation.json and evidence/audit-summary.json. REPORT.md and NITS.tsv are ASCII. All guest screenshots are in shots/, raw traces and inspected extracted data in evidence/, and replay/preparation scripts plus disposable disk copies remain in this working directory.
