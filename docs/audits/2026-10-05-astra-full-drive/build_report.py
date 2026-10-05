#!/usr/bin/env python3
"""Assemble the ASCII audit deliverables from session observations."""
from pathlib import Path
import json,collections,datetime
R=Path(__file__).resolve().parent
F=[]
def add(area,sev,title,steps,saw,expected,*evidence):
 F.append(dict(id=f'L{len(F)+1:03d}',area=area,severity=sev,summary=title,steps=steps,observed=saw,expected=expected,evidence=list(evidence)))

add('database','P1','Disk-full APPEND corrupts the DBF record count and prevents reopening',
 'Run prepare_fixtures.py and the documented db-full preparation; replay db-full.json with run_text_suite.py. Boot tenants + db-full-fixture.img; backslash; USE CLIENTS.DBF. Repeat APPEND BLANK and REPLACE NAME WITH "Rnn",BAL WITH n for n=0..17. Then USE, USE CLIENTS.DBF, LIST.',
 'The first nine appends worked. Later appends failed. RECCOUNT() still reported 12. After closing, USE could not reopen the table. Independent extraction found a 502-byte DBF with header count 21, header length 129 and record length 31: only 12 complete records exist, whereas the header requires 781 bytes including EOF. The original three rows remain in the bytes but the table is unusable through SAMIR.',
 'A refused append must preserve a self-consistent, reopenable table and the previous records. This is my data-integrity expectation; DBF header/count meaning is documented in dbase3-decomp/specs/file-formats/dbf.md. Disk-full is a normal floppy workload.',
 'db-full.json','evidence/db-full-serial.log','evidence/db-full-verification.json','evidence/db-full-CLIENTS.DBF','shots/120-db-full-appended.png','shots/121-db-full-reopen.png')
add('Trash','P1','An open purged folder remains writable and leaks an orphan cluster',
 'Replay reproduce_trash_deleted_window.py. Fresh tenants + trash-fixture.img: open INITECH (600,64); drag A (192,101) to Trash (600,422); open Trash and its A at (75,124); Special > Empty Trash, Return. In the surviving A window press Ctrl-N.',
 'A still displayed CHILD after both directories and KEEP.TXT had been purged. Ctrl-N reported NEWFOLD parent=36 and the view acquired bogus IDB1/document entries. The new folder was not reachable from root or Trash. fsck.fat -n found one unused allocated cluster. An independent fresh replay reproduced the same orphan.',
 'Close or invalidate every window whose directory is deleted, and reject writes to freed storage. This is my filesystem/window-lifetime expectation. MAC6 p.112 says emptying Trash permanently removes its contents; a surviving usable directory view is inconsistent with that operation.',
 'reproduce_trash_deleted_window.py','evidence/trash-edges-serial.log','evidence/trash-independent-serial.log','evidence/trash-independent-fsck.txt','shots/101-trash-purge-open-descendant.png','shots/102-newfolder-in-purged-child.png')
add('database','P1','QUIT hangs after the same table is opened in two work areas',
 'Replay db_exit_probe.py, case db-exit-two. On tenants + db-fixture.img press backslash. SELECT 1; USE CLIENTS.DBF ALIAS ONE; SELECT 2; USE CLIENTS.DBF ALIAS TWO; QUIT. Wait 18 seconds; send Ctrl-C and Return.',
 'No desktop return, prompt, or recovery followed. QMP still responded; the CPU snapshot had HLT=1 and interrupts disabled (EFL=0x93). The two-area mutation variant also hung. The single-area USE/LIST/QUIT control returned to FLAIR.',
 'QUIT must close the work areas and restore the suspended desktop, or reject an unsupported second open safely. DB-manual work-area/alias chapter documents independent selected areas; clean process teardown is my OS expectation. This is an observed guest hang, not the runner timeout itself.',
 'db_exit_probe.py','evidence/db-exit-two-serial.log','evidence/db-exit-two-qmp-state.json','evidence/db-exit-control-serial.log','shots/db-exit-two-after.png','shots/db-exit-two-recovery.png')
add('dialogs','P1','Empty Trash buttons commit on mouse-down, before release can cancel',
 'Fresh tenants + flair_data: stage README.TXT in Trash. Choose Special > Empty Trash at (172,28)->(200,65). Press OK at (464,168), keep the button held for 0.5 s, then move to (414,218) and release outside. Cancel can be tested the same way at (391,168).',
 'OK purged the file while the button was still held. The dialog and item were already gone in the held screenshot; release outside did not cancel. Cancel also dismissed immediately on mouse-down. Raw event ordering places EMPTIED between mouseDown and mouseUp.',
 'Track the button until release inside; release outside cancels the press. Named reference: local Control Manager, TrackControl button-like parts. This is distinct from the old fixed title-bar Close-box finding G02.',
 'evidence/desktop-main-serial.log','evidence/actions.jsonl','shots/006-empty-confirm.png','shots/010-ok-button-held.png','shots/011-ok-drag-out.png','evidence/references/control-manager.md')
add('DOS','P1','COPY overwrites a read-only destination',
 'Boot tracer_boot + dos-fixture.img. READONLY.TXT is explicitly +r in prepare_fixtures.py. COPY README.TXT READONLY.TXT; TYPE READONLY.TXT.',
 'COPY reported one copied and replaced READONLY-SENTINEL with the README text. It did not require removing the read-only attribute.',
 'Refuse modification of a read-only destination. DOS-manual, ATTRIB and access-denied diagnostics, documents protection against changes. This is separate from the repaired directory-destination COPY case.',
 'prepare_fixtures.py','evidence/fixture-preparation.json','evidence/dos-corrected-serial.log','shots/e008-readonly.png')
add('DOS','P1','DEL removes a read-only file',
 'Fresh tracer_boot + dos-fixture.img: DEL READONLY.TXT, then TYPE READONLY.TXT, without first copying onto it.',
 'DEL silently removed the protected file. This is not inferred from the earlier COPY, which could have changed attributes.',
 'Honor the read-only state. DOS-manual ATTRIB says read-only prevents accidental deletion.',
 'evidence/dos-edge-serial.log','evidence/dos-edge-listing-root.txt','shots/110-dos-root-path.png')
add('database','P1','SET SAFETY ON does not confirm ZAP',
 'Fresh tenants + db-fixture.img; backslash; USE CLIENTS.DBF; SET SAFETY ON; ZAP; ? RECCOUNT(). Replay db-final.json.',
 'ZAP returned immediately with no Y/N question and RECCOUNT() became zero. No consent response was sent. The same behavior occurred in the broader and full work-area tests.',
 'DB-manual, ZAP, printed p.U5-284 explicitly specifies a ZAP filename? (Y/N) prompt when SAFETY is ON. Preserve the rows until an affirmative response.',
 'db-final.json','evidence/db-final-serial.log','shots/db-final-04.png','shots/db-final-05.png','evidence/references/dbase3-using.txt')
add('Initech 123','P1','Quit discards an edited worksheet without an unsaved-work warning',
 'Launch 123 from the normal APPS folder. Enter Hello, 123, 1.5 and -7 across row 1; add another label. Press Ctrl-Q, then launch 123 again.',
 'The app immediately exited; there was no save/discard/cancel choice. Relaunch showed an empty A1 worksheet and the entered work was gone.',
 'Protect unsaved work on exit or clearly disclose a volatile demo before accepting it. My period spreadsheet/Mac-document expectation is a dirty-document warning; Lotus reference Chapter 9 supplies a distinct Quit workflow. The missing save route is recorded separately.',
 'evidence/desktop-main-serial.log','shots/027-123-row-entry.png','shots/044-123-long-label-stored.png','shots/045-123-quit-unsaved.png','shots/046-123-relaunch-empty.png')
add('Initech 123','P2','Ordinary formulas and @ functions cannot be committed',
 'In a fresh 123 sheet enter 123 in B1, 1.5 in C1 and -7 in D1. In A2 type +B1*2 and Return. Escape, then type @SUM(B1..D1) and Return.',
 'Both valid entries remained uncommitted in EDIT mode; A2 had no calculated result. Number and label controls did commit.',
 'Compute 246 and 117.5. Named reference: the real Lotus R2.2 MINT01 capture and mint/mint01.steps contain these exact formulas. This is a missing core calculation feature, not a claim that every formula parser branch was tested.',
 'shots/028-123-formula.png','shots/041-123-function-rejected.png','evidence/desktop-main-serial.log','evidence/references/lotus22-entered.png')
add('Initech 123','P2','The slash command menu does not open',
 'In READY press slash and then f,s, without Return.',
 'Slash produced no menu. The following fs became a LABEL entry in the current cell.',
 'Slash enters the Lotus command menu, with Worksheet/Range/Copy/Move/File and other choices. Named reference: L22-manual Chapter 1, Using 1-2-3 Menus; the real MINT01 /fs save capture. A Mac menu alternative would also need to expose these operations.',
 'shots/030-123-file-save.png','evidence/references/lotus22-save.png','evidence/desktop-main-serial.log')
add('Initech 123','P1','There is no reached worksheet Save operation',
 'Enter a worksheet, try /fs, and inspect the lower File menu (its only offered item is Quit). Quit and relaunch as in L008.',
 'No filename prompt, Save item, saved worksheet, or recoverable work appeared. Only Quit was available in the app File menu.',
 'A productive worksheet must be saved to disk. Named reference: L22-manual Chapter 5 /File Save, and the actual Lotus save-prompt capture. This is the persistence gap beyond the slash menu interaction itself.',
 'shots/030-123-file-save.png','shots/138-123-file-menu.png','shots/046-123-relaunch-empty.png','evidence/app-boundaries-serial.log','evidence/references/lotus22-save.png')
for title,steps,saw,exp,shot in [
 ('F1 provides no worksheet help','In READY press F1.','The sheet remained in READY and no help text or Help screen appeared.','HELP (F1), L22-manual p.1-11, displays context-sensitive help.','029-123-f1-help'),
 ('F2 cannot edit an existing cell','Select A1 containing Hello and press F2.','A1 remained selected in READY; the edit line stayed empty.','EDIT (F2), L22-manual p.1-11, enters EDIT with the current cell contents.','032-123-f2-edit'),
 ('F5 does not start Goto','In READY press F5, then type Z100.','No address prompt appeared; Z100 became a LABEL entry at the old cell.','GOTO (F5), L22-manual p.1-11, moves directly to the specified cell/range.','031-123-goto'),
 ('Page Down does not advance one worksheet screen','In READY at A1 press Page Down.','The address and pointer remained A1. Arrow-based movement did work to A23.','PGDN, L22-manual p.1-10, moves down one screen.','061-123-pgdn-ready'),
 ('Tab does not page horizontally','Use Right twelve times to reach M1; press Tab.','M1 and the F..M column viewport remained unchanged.','TAB in READY, L22-manual p.1-10, moves right one screen. This is not a claim that Tab should move one cell.','060-123-tab'),
 ('Clicking a worksheet cell does not select it','From A1 click the visible E4 area at (360,180).','The address and highlight remained A1.','My expectation for a worksheet in a native mouse-driven document window is direct cell selection; this is a memory/usability expectation, not a claim about DOS Lotus mouse configuration.','034-123-mouse-cell'),
 ('Shrinking the window lets the current cell disappear off screen','Shrink the 640x440 123 window by about 288x194 with its grow box. Move Right to E1. Try the horizontal right arrow.','The address became E1, but the view showed only A..D and no cell pointer. The visible scrollbar did not reveal E1.','Keep the current cell visible for the actual resized viewport. This is my native-window/worksheet visibility expectation; the normal full-size arrow-follow control worked.','039-123-pointer-off-small-window'),
 ('Entry silently stops at 75 characters instead of the documented 240','At A1 type the alphabet four times (104 letters), without Return; then press Return.','The entry line stopped after the third alphabet minus its final three letters (75 characters); the remaining keystrokes did not extend it. No length error or scrolling edit line appeared.','L22-manual pp.1-16/1-17 permits 240-character entries. Preserve all 104 characters or explicitly refuse them.','048-123-long-label-input'),
 ('The unmodified backslash hotkey steals the repeating-label prefix','Boot tenants + db-fixture.img; open root APPS (192,101), then 123 (211,124). At B1 press backslash to begin a repeating label.','The entire desktop suspended into SAMIR. QUIT returned to the sheet, which had not received the prefix. On flair_data the same key attempted the unavailable text app and resumed.','L22-manual p.1-17 defines backslash as the repeating-label prefix. Reserve a non-conflicting system shortcut while applications accept text. The existence of a system hotkey is deliberate; its collision is the defect.','ab-backslash-launches-database'),
 ('The worksheet status line omits the period clock/date','Launch 123 and inspect the bottom status line at native size.','The status line was blank throughout entry and navigation.','The real Lotus R2.2 MINT01 capture shows date/time at the lower left. This is a reference-backed presentation omission, with the intentional native white/navy adaptation accepted.','026-123-launch')]:
 add('Initech 123','P3' if 'clock/date' in title else 'P2',title,steps,saw,exp,'shots/'+shot+'.png','evidence/desktop-main-serial.log' if not shot.startswith('ab-') else 'evidence/app-boundaries-serial.log')
add('look','P3','Empty Trash leaves the caution-icon area completely blank',
 'Stage a file and open Special > Empty Trash. Compare the dialog with the local Mac OS 8.1 caution-alert capture.',
 'The large left-hand icon space was empty; only the text and two buttons identified the destructive alert.',
 'The actual s8_alert_modal capture contains a yellow caution triangle in this same layout. This is a named-reference drawing omission, not a demand to change the intentional wording or two menu bars.',
 'shots/006-empty-confirm.png','evidence/references/mac81-alert.png')
add('look','P3','The Trash alert has a heavy black slab border instead of the Platinum alert edge',
 'Open the same confirmation and compare the complete frame at native size.',
 'A roughly seven-pixel black outer band dominated the dialog. The reference has a thin black edge with a subtle raised/tinted border.',
 'Named reference: system7-decomp/specs/sys8/controls.md section 6 and s8_alert_modal.png. The gap is the alert frame, a surface earlier audits had not reached with buttons; general window-frame criticism stays under F16.',
 'shots/006-empty-confirm.png','evidence/references/mac81-alert.png','evidence/references/mac81-controls.md')
add('Trash','P3','The confirmation understates the disk space occupied by a folder tree',
 'On trash-fixture.img stage A, containing CHILD/KEEP.TXT, and open Empty Trash before purging.',
 'The alert counted three items but said they use 1K. Independent FAT inspection found three one-cluster objects at 512 bytes/cluster, totaling 1536 bytes, which needs 2K when rounded upward.',
 'A figure labeled disk space should reflect allocated storage, including folder clusters, or identify itself as logical file bytes. This is my filesystem-feedback expectation; the Mac alert reference uses a disk-space figure.',
 'shots/100-trash-tree-count.png','evidence/trash-tree-space.json','images/trash-fixture.img')
add('DOS','P2','CD backslash does not return to the root',
 r'Fresh tracer_boot + dos-fixture: MD TEST; CD TEST; CD \; CD. Then CD A:\; CD as a control.',
 r'The unqualified root form left the location A:\TEST without an error. The drive-qualified form returned to A:\.',
 r'DOS-manual CHDIR explicitly says CD \ returns to the root. Both root spellings should resolve consistently.',
 'evidence/dos-edge-serial.log','shots/110-dos-root-path.png')
add('DOS','P2','MD accepts a wildcard character as a literal directory name',
 'MD BAD?NAME; CD BAD?NAME; CD; then inspect the stopped FAT disk with mdir.',
 'The directory was created and the guest entered A:\\BAD?NAME. mtools sanitized its display to BAD_NAME, exposing an invalid DOS name on disk.',
 'Reject wildcard characters in new directory names. This follows DOS-manual filename/wildcard rules and my DOS 8.3 naming expectation. Long over-8.3 names were correctly refused in the control tests.',
 'evidence/dos-edge-serial.log','shots/111-dos-illegal-names.png','evidence/dos-edge-listing-root.txt')
add('DOS','P2','MD accepts reserved device names as real directories',
 'On a fresh DOS fixture issue MD CON; CD CON; CD. MD AUX was also exercised in dos-corrected.',
 'Both reserved names were accepted as directories; CD CON put the shell in A:\\CON.',
 'Reject reserved DOS device names for filesystem objects. Expectation from my DOS 3.x memory; DOS-manual character-device sections define CON and AUX as devices.',
 'evidence/dos-edge-serial.log','shots/111-dos-illegal-names.png','evidence/dos-corrected-serial.log')
add('DOS','P2','COPY treats /A and /B switches as filenames and ignores the requested destination',
 'The fixture CTRLZ.TXT contains BEFORE, byte 0x1A, AFTER and CR/LF. COPY CTRLZ.TXT /A ASCII2.TXT; TYPE ASCII2.TXT; DIR. Then COPY /A CTRLZ.TXT ASCII3.TXT and COPY CTRLZ.TXT /B BIN2.TXT; TYPE each requested target.',
 'Each copy reported success, but none of ASCII2.TXT, ASCII3.TXT or BIN2.TXT existed. DIR exposed a literal /A file; the /B variant likewise used the switch as its destination. The prefix /A run used the newly created switch-named file as its source and CTRLZ.TXT as its destination. A trailing /A was also ignored in the earlier test.',
 'DOS-manual COPY accepts /A and /B before or after filenames. Parse switches separately, honor the requested destination, and apply text/binary semantics. The earlier trailing-/A screenshot alone is not used to claim source EOF truncation, because that position applies to the destination.',
 'shots/133-dos-source-ascii-switch.png','shots/134-dos-prefix-and-binary-switch.png','evidence/dos-ascii-serial.log','evidence/dos-ascii-root-raw.json')
add('DOS','P2','COPY cannot concatenate explicitly named files with plus',
 'On dos-fixture.img, COPY KEEP.TXT+CTRLZ.TXT MERGE.TXT; TYPE MERGE.TXT.',
 'Both sources existed, but COPY said File not found and created no usable merged file.',
 'DOS-manual COPY documents concatenation with plus. This is the explicit-name concatenation grammar, separate from known K11 wildcard enumeration.',
 'evidence/dos-corrected-serial.log')
add('DOS','P2','New filesystem entries receive an invalid zero month/day timestamp',
 'In dos-env: ECHO METADATA > STAMP.TXT; DATE; TIME. Stop QEMU and inspect STAMP.TXT with mdir -a. New directories and COPY destinations in the other DOS sessions provide controls.',
 'The guest clock reported October 4, 2026, but new entries had 1980-00-00 00:00. mdir exposed the invalid month/day. This also appeared on Finder-created service entries.',
 'Store the actual last-modified date/time, or at minimum a valid FAT date. DOS-manual file/directory descriptions include modification timestamps. This is metadata correctness, not the already-known abbreviated DIR layout.',
 'shots/118-dos-timestamp-clock.png','evidence/dos-env-root-after.txt','evidence/dos-corrected-root-after.txt')
add('DOS','P2','FOR works in a batch but is not recognized at the prompt',
 'SET AUDIT=VALUE; run ENV.BAT as a control. Then type FOR %f IN (ONE TWO) DO ECHO %f directly at the prompt.',
 'The batch FOR printed ONE and TWO. The single-percent interactive form returned Bad command or file name.',
 'DOS-manual FOR, printed p.145, explicitly instructs using one percent sign outside a batch file.',
 'shots/115-dos-batch-environment.png','shots/116-dos-interactive-control.png','evidence/dos-env-serial.log','fixtures/ENV.BAT')
add('DOS','P2','IF is not recognized at the interactive prompt',
 'With SELF.BIN present, type IF EXIST SELF.BIN ECHO YES. Compare the IF EXIST control in ENV.BAT.',
 'The direct command returned Bad command or file name; the batch control printed BATCH-IF-YES.',
 'My DOS 3.x command-processor expectation is that IF can execute a conditional command at the prompt. The original manual documents IF syntax; the interactive applicability here is labeled memory-based.',
 'shots/116-dos-interactive-control.png','evidence/dos-env-serial.log','fixtures/ENV.BAT')
add('DOS','P2','F3 command recall does nothing',
 'Fresh DOS: ECHO REPEAT, Return. At the empty next prompt press F3, wait one second, then Return.',
 'The command line stayed empty and Return produced another empty prompt. No previous command was copied.',
 'DOS-manual MS-DOS editing keys, printed pp.154-155, specifies F3 plus Return to repeat the previous command.',
 'shots/126-dos-f3.png','evidence/dos-final-serial.log')
add('DOS','P2','COMMAND /C cannot launch a secondary command processor',
 'On the supplied tracer_boot + dos-fixture.img prompt, type COMMAND /C ECHO CHILD.',
 'Bad command or file name; CHILD was never printed.',
 'DOS-manual COMMAND documents /C execution in a secondary processor. This finding is limited to the supplied runnable image and disk, not unbooted artifacts elsewhere in the repository.',
 'shots/127-dos-secondary-shell.png','evidence/dos-final-serial.log')
add('DOS','P1','EXIT terminates the permanent primary shell and leaves no usable prompt',
 'Fresh tracer_boot + dos-fixture.img. The boot trace specifies SHELL=COMMAND.COM /P /E:512. Type EXIT, wait, then VER and Return.',
 'The trace ended SHELL-EXIT / SHELL-DONE. VER produced no output and the screen stayed at the EXIT command. Only the external QMP controller could end the run.',
 'A permanent primary DOS shell must remain available; /P should prevent exiting it into a dead machine. The precise primary-shell EXIT rule is from my DOS memory, with /P permanence grounded in DOS-manual COMMAND.',
 'shots/128-dos-permanent-exit.png','evidence/dos-final-serial.log','evidence/actions.jsonl')
for cmd,purpose in [('CHKDSK','inspect/check the filesystem'),('FORMAT','initialize a disk'),('DISKCOPY','copy a disk'),('ATTRIB','inspect or change read-only/archive attributes'),('TREE','inspect the directory tree'),('FIND','filter text for a string'),('SORT','sort text input'),('MORE','page text output')]:
 add('DOS','P2',cmd+' is unavailable from the supplied DOS installation',
  'Boot tracer_boot + dos-fixture.img; type '+cmd+' at the prompt, as in dos-corrected.json.',
  'The shell returned Bad command or file name. No '+cmd+' executable was available on the tested disk or reached through the configured path.',
  'DOS-manual has a named '+cmd+' command section providing a way to '+purpose+'. This is an installation/feature gap on these images, not a claim about host source files or an instruction to format the original images.',
  'evidence/dos-corrected-serial.log','dos-corrected.json')
add('database','P2','DATE() returns the current year as 1926 instead of 2026',
 'Fresh SAMIR: SET CENTURY ON; ? DATE(); ? YEAR(DATE()). Compare the contemporaneous DOS DATE control.',
 'SAMIR printed 10/04/1926 and 1926. DOS on these images reported 10/04/2026.',
 'DB-manual DATE obtains the current system date. This is not criticism of the deliberately retained CTOD year-00 -> 1900/accounting behavior; it is the wrong century for the live system clock.',
 'shots/c002-date.png','shots/118-dos-timestamp-clock.png','evidence/db-confirm-serial.log')
add('database','P2','Empty and invalid dates print malformed digits',
 'In a fresh SAMIR prompt: ? CTOD(""); ? CTOD("99/99/99"); ? CTOD("02/29/00"). Compare valid CTOD("02/29/04").',
 'The three empty/invalid results displayed 11/24//- rather than a blank date or a relevant error. The valid leap-day control displayed normally.',
 'Named reference: local dates-and-century spec, CTOD and blank-date sections, backed by the III+ blank-date idiom. Represent an empty date without invented digits.',
 'shots/c002-date.png','shots/b010-dates.png','evidence/db-confirm-serial.log','evidence/references/dbase-dates.md')
add('database','P3','The next dot prompt is glued to query output',
 'Type ? 2, then another query; also try a string expression or DBF().',
 'The next . prompt appeared on the same line immediately after the value, e.g. 4. ? EXP(1) and CLIENTS. ? BOF(). Numeric values and the next command visually ran together.',
 'My memory of the dBASE dot-prompt interface is a fresh prompt line after a completed command. Keep output and the input prompt visually distinct; the manual examples also separate them.',
 'shots/db-final-01.png','evidence/db-functions-serial.log','shots/c002-date.png')
add('database','P2','CLOSE FORMAT and CLOSE INDEX close the database itself',
 'USE CLIENTS.DBF; CLOSE FORMAT; ? RECCOUNT(); LIST. Reopen and try CLOSE INDEX. The broader test also tried CLOSE PROCEDURE.',
 'RECCOUNT became zero and LIST no longer listed the table. Reopening the DBF restored its rows. A command for a different file type closed the active database too.',
 'DB-manual CLOSE, printed p.U5-53, distinguishes FORMAT, INDEX, PROCEDURE and DATABASES. Only the requested file type should close.',
 'shots/c004-close-indexes.png','evidence/db-confirm-serial.log','evidence/db-functions-serial.log')
add('database','P2','A valid alias-qualified field cannot be read',
 'SELECT 1; USE CLIENTS.DBF ALIAS ONE; GO 1; SELECT 2; USE CLIENTS.DBF ALIAS TWO; GO 2; ? BAL; SELECT ONE; ? BAL; ? TWO->BAL; SELECT 2; ? BAL.',
 'Direct queries returned 123.45 in ONE and -42 in TWO, but TWO->BAL returned 1 File does not exist. Both work areas were open and selectable.',
 'DB-manual work areas/aliases explicitly permits alias->field to read a field in an unselected open area. The alias is not a filename to reopen.',
 'shots/c006-areas.png','evidence/db-confirm-serial.log')
add('database','P2','TYPE reports its string argument type instead of the named expression type',
 'USE CLIENTS.DBF; ? TYPE("BAL"),TYPE("1+1"),TYPE("MISSING"),TYPE("DATE()").',
 'The output was C C C C, including for the numeric field, arithmetic and undefined variable.',
 'DB-manual TYPE(), printed p.U6-86, evaluates the expression named by its character argument. Expected N, N, U and D (the local function reference documents the date code).',
 'shots/db-final-01.png','evidence/db-final-serial.log','evidence/references/dbase-functions.md')
add('database','P2','Division by zero raises an error instead of the original overflow result',
 'At the dot prompt type ? 1/0.',
 'SAMIR printed 39 Numeric overflow (data was lost).',
 'The real III+ C1/E2 captures, summarized in local re/mint-results-002.md, explicitly establish asterisk-filled overflow output and no trapped error for 1/0. This is a measured compatibility difference, not a mathematical preference.',
 'shots/db-final-02.png','evidence/db-final-serial.log','evidence/references/dbase-minted-002.md')
for title,command,purpose in [
 ('INDEX ON cannot create an index','INDEX ON NAME TO TESTIDX','create a persistent NDX for keyed navigation'),
 ('REINDEX is unavailable','REINDEX','rebuild database indexes'),
 ('APPEND FROM cannot import records','APPEND FROM CLIENTS.DBF','append records from another DBF'),
 ('SAVE TO cannot persist memory variables','SAVE TO MEMSAVE','save variables in a MEM file'),
 ('RESTORE FROM is unavailable','RESTORE FROM MEMSAVE','restore a saved variable set'),
 ('REPORT FORM is unavailable','REPORT FORM REPORT','run a report definition'),
 ('LABEL FORM is unavailable','LABEL FORM LABEL','run a label definition')]:
 add('database','P2',title,'Open CLIENTS.DBF in SAMIR, then issue '+command+'.',
  ('7 File already exists; no import occurred.' if command.startswith('APPEND') else '16 Unrecognized command verb; the requested workflow did not start. For absent named input files, no file-specific diagnostic was reached.'),
  'DB-manual has a named '+command.split()[0]+' command section to '+purpose+'. Existing K08 covers other already-audited editor/aggregate/export gaps; this is an additional reached command.',
  'evidence/db-functions-serial.log' if command.split()[0] not in ['REPORT','LABEL'] else 'evidence/db-suite-serial.log','db-functions.json' if command.split()[0] not in ['REPORT','LABEL'] else 'db-suite.json')
for option,command,purpose in [
 ('ALTERNATE','SET ALTERNATE TO ALT.TXT','capture commands/output to a text transcript'),
 ('DEFAULT','SET DEFAULT TO A','select the default drive for file searches'),
 ('PATH','SET PATH TO A:\\','set the database file search path'),
 ('ESCAPE','SET ESCAPE ON','control program interruption'),
 ('CONFIRM','SET CONFIRM ON','require Return to complete field entry'),
 ('BELL','SET BELL OFF','control the entry/error bell'),
 ('CONSOLE','SET CONSOLE OFF','control screen output'),
 ('HEADING','SET HEADING OFF','control column headings'),
 ('PROCEDURE','SET PROCEDURE TO SIMPLE','load a procedure library')]:
 add('database','P2','SET '+option+' is unavailable',
  'At the dot prompt issue '+command+'. ON/OFF pairs and bare TO were also used where listed in db-functions.json.',
  'The option was refused with 7 File already exists; it did not take effect.',
  'DB-manual has a specific SET '+option+' section to '+purpose+'. The misleading generic error-number translation is already known under K08; the new finding here is the missing setting/workflow.',
  'evidence/db-functions-serial.log','db-functions.json')
for call,expected in [('EXP(1)','approximately 2.71828'),('LOG(1)','zero'),('SQRT(4)','two'),('TIME()','the current time string'),('RECSIZE()','31 for this fixture, including its deletion byte'),('DISKSPACE()','the available disk byte count'),('VERSION()','a version identification string'),('OS()','an operating-system identification string'),('PCOL()','the printer column position'),('PROW()','the printer row position'),('ROW()','the screen row position'),('COL()','the screen column position')]:
 add('database','P2',call.split('(')[0]+'() is absent from the function library',
  'USE CLIENTS.DBF; ? '+call+'.',
  '31 Invalid function name, including with the valid ordinary argument shown here.',
  'DB-manual Chapter 6 documents this function; local function specs enumerate it from III+ HELP.DBS. Return '+expected+'. This is function availability, not a proposed exact OS/version literal or an untested printer driver.',
  'evidence/db-functions-serial.log','db-functions.json')
add('database','P2','Disk full is misreported as a read-only file',
 'Repeat the full-disk APPEND workload from L001; watch the first failure after R08. The fixture DBF has no read-only attribute and earlier edits succeed.',
 'Every failed APPEND/REPLACE said 111 Cannot write to a read-only file. The actual volume had zero free bytes; the file was writable. This sends the user to the wrong recovery action.',
 'Report disk full (the original III+ catalog includes a disk-full diagnostic), and identify the affected file. This is the distinct error-feedback defect in L001, not its corrupt record-count defect.',
 'shots/120-db-full-appended.png','evidence/db-full-root-before.txt','evidence/db-full-serial.log')
add('Initech 123','P2','The 2049th occupied cell fails as unexplained EDIT mode',
 'Replay cell_stress.py: fresh 123, repeat typing 1 followed by Down 2050 times, starting at A1. Inspect attempts 2048 and 2049; then Escape, Home, replace existing A1 with 2.',
 '2048 cells committed successfully. The next valid 1 at A2049 did not commit or advance: the mode changed to EDIT with no memory/capacity explanation. Another attempt appended a second 1 to the stranded edit. Editing existing A1 still worked.',
 'Either support the next cell or visibly explain the capacity limit without presenting a valid number as an input-editing problem. This is my resource-failure feedback expectation, not a claim that a finite-memory worksheet must hold every possible cell.',
 'cell_stress.py','evidence/cell-stress-serial.log','shots/cell-2048.png','shots/cell-2049.png','shots/cell-2050.png','shots/cell-existing-edit.png')
for cmd,purpose in [('APPEND','search additional directories for data files'),('ASSIGN','redirect drive references'),('BACKUP','back up files'),('CHCP','inspect/select the console code page'),('COMP','compare files'),('CTTY','select the controlling terminal'),('DISKCOMP','compare disks'),('EXE2BIN','convert executable file formats'),('FASTOPEN','provide the documented file-open cache'),('FC','compare file contents'),('GRAPHICS','enable graphics-screen printing'),('JOIN','join a drive into a directory'),('KEYB','select a keyboard layout'),('LABEL','change a disk volume label'),('MODE','configure console/printer/serial modes'),('NLSFUNC','load country/code-page support'),('PRINT','queue print jobs'),('RECOVER','attempt damaged-file recovery'),('REPLACE','replace files as a batch operation'),('RESTORE','restore backed-up files'),('SELECT','install/configure a DOS disk'),('SHARE','provide DOS file-sharing/locking support'),('SUBST','substitute a drive for a path'),('SYS','transfer system files to a disk'),('XCOPY','copy directory trees'),('DEBUG','inspect/debug DOS programs'),('EDLIN','edit text files at the DOS prompt')]:
 add('DOS','P2',cmd+' is unavailable from the supplied DOS installation',
  'Boot tracer_boot + dos-fixture.img and type '+cmd+' as in dos-utilities.json.',
  'Bad command or file name; no command-specific parameter prompt, diagnostic, or program started.',
  'The original DOS-manual documents '+cmd+' to '+purpose+'. This is a missing supplied command/utility, limited to the tested installation. No untested hardware behavior or implementation outside the supplied disks is inferred.',
  'evidence/dos-utilities-serial.log','dos-utilities.json')
add('database','P2','A macro variable containing a command is not expanded for execution',
 'Run DO STRESS on db-program-fixture.img, prepared as documented in the journal. STRESS.PRG loops N to 1000, prints LOOP-PASS, stores "? N + 5" to CMD, then executes &CMD.',
 'The loop and IF branch worked, then the macro statement returned 16 Unrecognized command verb instead of printing 1005. N remained 1000 at the prompt.',
 'DB-manual macro substitution and the local III+ language reference permit an ampersand memory-variable substitution in commands. This is independent of the old DO-file/USE integration issue K08: this program loaded and ran preceding statements.',
 'fixtures/STRESS.PRG','evidence/db-program-serial.log','shots/program-01.png')
for title,command,purpose in [
 ('SORT TO is unavailable','SORT TO SORTED ON NAME','write a sorted copy of the active table'),
 ('TYPE is not recognized at the database prompt','TYPE COPY.PRG','view an ASCII file, or report that the requested file is absent'),
 ('RENAME is not recognized at the database prompt','RENAME COPY.PRG TO RENAMED.PRG','rename a file, or identify a missing source'),
 ('ERASE is not recognized at the database prompt','ERASE RENAMED.PRG','erase a file, or identify a missing source'),
 ('TOTAL ON is unavailable','TOTAL ON CITY TO TOTAL','create grouped totals'),
 ('RUN cannot invoke a DOS command','RUN VER','run a DOS command and return to the database')]:
 add('database','P2',title,
  'USE CLIENTS.DBF, then '+command+'. The exact preceding file-operation attempts are in db-program.json.',
  '16 Unrecognized command verb. SORT and TOTAL had the correct existing fields. The TYPE/RENAME/ERASE files were absent after the refused COPY FILE control; no file-specific diagnostic was reached.',
  'DB-manual has the named '+command.split()[0]+' command to '+purpose+'. The finding is the unrecognized command entry point; successful missing-file handling is not claimed.',
  'evidence/db-program-serial.log','db-program.json')
add('database','P2','SET FUNCTION cannot redefine a function key',
 'SET FUNCTION 1 TO "LIST" at the SAMIR dot prompt.',
 '7 File already exists; no key definition was accepted.',
 'DB-manual SET FUNCTION documents this key-definition command. The generic error translation is already known; this is the absent setting.',
 'evidence/db-program-serial.log','shots/program-16.png')
add('database','P2','CLEAR MEMORY cannot clear variables',
 'After DO STRESS sets N to 1000, issue CLEAR MEMORY and ? N.',
 'CLEAR MEMORY was unrecognized and N remained 1000. RELEASE of a named variable worked in a separate control.',
 'DB-manual CLEAR MEMORY specifies removing memory variables. Recognize the documented operation or provide a truthful unsupported response.',
 'evidence/db-program-serial.log','shots/program-18.png')
add('apps','P3','CTENANT loses its input readouts when its window repaints',
 'Fresh normal APPS: launch CTENANT at (143,124); press Q; click content (250,250). Capture, then drag its title (300,180) by (+50,+50).',
 'Both Key q lines and the Click readout vanished after the move, leaving only the startup sentence. The window was not closed or relaunched.',
 'My ordinary GUI expectation is that moving/revealing a window preserves its displayed state. CTENANT is a test application, so this is recorded as a polish/readout defect rather than loss of a user document.',
 'shots/135-ctenant-before-move.png','shots/136-ctenant-after-move.png','evidence/gui-polish-serial.log')
add('Trash','P2','Trash stages and purges a locked file without a protection-specific refusal',
 'Boot tenants + trash-fixture.img, whose LOCKED.TXT is explicitly +r. Open root; drag LOCKED.TXT (123,153) to Trash; choose Empty Trash and press Return.',
 'The protected file staged and purged with the ordinary confirmation. There was no locked-file explanation or explicit unlock step.',
 'MAC6 pp.88 and 112 says an individually locked document cannot be discarded until unlocked; DOS-manual ATTRIB also documents the underlying protection. If the GUI intentionally permits an override, it must disclose that choice. This is the GUI protection affordance, separate from DOS DEL in L006.',
 'evidence/trash-fixture-preparation.json','evidence/trash-restore-serial.log','shots/105-trash-locked-confirm.png','shots/106-trash-locked-refused.png')

def links(paths):return ', '.join('['+p+']('+p+')' for p in paths)
def make():
 header=(R/'report_intro.txt').read_text()
 footer=(R/'report_coverage.txt').read_text()
 parts=[header,'\n## 2. New findings\n\nP1 = data loss/corruption, crash/hang, or blocked core task. P2 = broken or missing feature. P3 = look/polish nit. Each numbered entry is one defect; related symptoms and shared old diagnostics are identified explicitly.\n']
 for f in F:
  parts.append('\n### '+f['id']+' - '+f['summary']+'\n\nArea: '+f['area']+'. Severity: '+f['severity']+'.\n\nSteps: '+f['steps']+'\n\nObserved: '+f['observed']+'\n\nExpected and basis: '+f['expected']+'\n\nEvidence: '+links(f['evidence'])+'.\n')
 parts.append(footer)
 text=''.join(parts);text.encode('ascii');(R/'REPORT.md').write_text(text,encoding='ascii')
 (R/'NITS.tsv').write_text('id\tarea\tseverity\tsummary\n'+''.join('\t'.join(f[k] for k in ['id','area','severity','summary'])+'\n' for f in F),encoding='ascii')
 (R/'evidence/findings.json').write_text(json.dumps(F,indent=2),encoding='ascii')
 print(len(F),dict(collections.Counter(f['severity'] for f in F)))
if __name__=='__main__':make()
