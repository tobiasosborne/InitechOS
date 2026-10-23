# InitechBase / SAMIR hands-on audit

2026-10-23. Supplied image commit: e67b8311092873fe16e6c6d62ad82f894bfb8d7d.

48 new findings: 3 P1, 43 P2, 2 P3. The most serious results are malformed persisted date bytes, an uninterruptible program loop, and a reproducible recursive-procedure crash/hang. The previous disk-full APPEND corruption and duplicate-open exit hang did not recur. Filters, ordinary indexes, reports, labels, variables, many expressions and file commands are substantially improved.

Evidence inventory: 49 copied-image QEMU sessions, 272 PNG files (including explicit aliases for corrected runs), and 1091 typed input sequences. Three early harness attempts did not reach SAMIR and are excluded from product findings. Exactly one audit emulator ran at a time. Every started emulator has a stopped session record. No source edits, builds, make, commits, tracker commands, external messages, X :0 input or host-desktop captures were performed.

## Replay and evidence conventions

All new L numbers below belong to this report. Earlier IDs are always qualified by their audit date; identical numeric suffixes are unrelated. P1 means data corruption/loss, crash/hang or blocked core work; P2 means incorrect or missing behavior; P3 means presentation/diagnostic polish.

Unless a finding says DOS route, use copies of images/flair_tenants_interactive.img and the named data fixture, wait for FLAIR-LIVE-READY, and press Ctrl+Alt+backslash. All listed commands are typed at the dot prompt, one command per Enter. The DOS route uses tracer_boot.img; press Enter at the startup date and time prompts, then run SAMIR.COM. No mouse coordinates are required by the reached SAMIR paths: the requested editors/designers failed before their mouse/key workflows became available.

Replay in a separate writable copy of this audit directory to avoid replacing evidence. The supplied images remain in images/. For a self-contained replay, evidence/rebuild_fixtures.py reconstructs every fixture from the archived small inputs; see evidence/REPLAY.txt. Original fixture creation is recorded in evidence/prepare.py, prepare_full.py, prepare_run.py, prepare_paging.py, prepare_control.py and prepare_master.py; evidence/fixtures/ also preserves the exact small inputs. The original richer sample files came from the already-built manual sample tree; their source paths and hashes are in evidence/fixture-manifest.json. No sample builder was run. The script-generated full disk has zero free bytes before boot.

For an individual scenario, run `python3 evidence/run_suite.py evidence/pack-two.json` (substitute the desired plan). The JSON has the exact command/key order and screenshot points. run_queue.py runs plans sequentially. redirect.py is its own standalone scenario. The root audit_driver.py and evidence/audit_driver.py contain the QMP driver. Every session uses -display none, -cpu 486, -m 16 and -qmp stdio, with serial output to a file. Raw serial and per-command offsets are preserved; stdout runner logs are convenient excerpts, not replacements for raw evidence.

Every cited finding screenshot was opened and checked. evidence/qa-*.png are review contact sheets; the original unmodified guest captures are in shots/. Stopped-image mcopy extractions, raw DBF/DBT/NDX/MEM/report files and decoded JSON support byte-level claims. Source diagnoses are explicitly guesses. A screenshot of a stationary loop alone does not prove a hang: the command journal, attempted recovery and QMP state accompany those claims.

Historical expectations use the named local dBASE III PLUS HELP-derived specifications, mined files and minted transcripts under /home/tobias/Projects/dbase3-decomp. The copied sources and provenance map are in evidence/references/. The actual III+ report capture real-dbase-report.png was viewed as an appearance reference. These are existing primary-derived materials, not a claim that a live historical interpreter was run tonight. Shipped manual references are under /home/tobias/Projects/initech-os/pubs/manuals/; quoted promises include file and line. Memory/usability expectations are labelled explicitly.

## New findings

### L001 - Replacing a date with an invalid date writes non-date bytes

Area: DBF dates. Severity: P1.

Steps: Fresh rich-fixture.img, desktop route. USE EDGE; REPLACE WHEN WITH CTOD("02/30/92"); DISPLAY ID,WHEN; USE; USE EDGE. Stop the guest and extract EDGE.DBF with mcopy.

Observed: Record 1 WHEN becomes the eight ASCII bytes ,)/-1124 (hex 2c292f2d31313234), rather than eight blanks. DISPLAY shows 11/24/00. The invalid bytes survive closing and reopening. This is a persistence defect beyond earlier audit L045, which described display of empty/invalid dates.

Expected and basis: A blank/invalid CTOD result must leave a valid blank date representation. Named reference: dbase3-decomp/specs/file-formats/dbf.md, section 5: D fields contain YYYYMMDD or blanks. This corrupts the field representation even though the table remains openable.

Evidence: [shots/data-wide.png](shots/data-wide.png), [evidence/data-serial.log](evidence/data-serial.log), [evidence/data-files/EDGE.DBF](evidence/data-files/EDGE.DBF), [evidence/data-bytes.json](evidence/data-bytes.json).

Guess: os/samir/fs/dbf.c:1654 converts every XB_D through ymd_from_jdn, including the zero/blank sentinel; only XB_U takes the blank branch at line 1675.

### L002 - Escape and Ctrl-C cannot stop a running loop

Area: Program interruption. Severity: P1.

Steps: Fresh rich-fixture.img, desktop route. DO LOOP. Wait 2 seconds; press Esc, wait 2 seconds; Ctrl-C, wait 2 seconds; type ? "RECOVERED" and Enter; wait 2 seconds. LOOP.PRG installs ON ESCAPE and runs DO WHILE .T. incrementing N.

Observed: The screen remains at . DO LOOP; no trap output, prompt, or recovery command appears. QMP remains responsive and reports running, HLT=0. Only stopping this audit QEMU ended the session. This is failure to interrupt a program, not a claim that the infinite loop should terminate by itself.

Expected and basis: A user must be able to interrupt a mistaken loop without destroying the session. Named reference: dbase3-decomp/specs/commands/programming-and-io.md, ON ESCAPE/SET ESCAPE discussion. The shipped quick reference openly calls the trap inert; that documents the limitation but does not make an uncancellable loop dependable.

Evidence: [shots/escape-loop-running.png](shots/escape-loop-running.png), [shots/escape-loop-escape.png](shots/escape-loop-escape.png), [evidence/escape-loop-serial.log](evidence/escape-loop-serial.log), [evidence/escape-loop-qmp-state.json](evidence/escape-loop-qmp-state.json), [evidence/escape-loop.json](evidence/escape-loop.json), [evidence/fixtures/LOOP.PRG](evidence/fixtures/LOOP.PRG).

### L003 - Recursive internal procedures destroy the running session

Area: Procedure stack. Severity: P1.

Steps: Fresh program-fixture.img, desktop route: DO RECINT. Wait 3 seconds, press Esc and Ctrl-C, wait 2 seconds, type ? "RECOVERED", then wait 2 seconds. RECINT.PRG calls procedure REC, whose body calls REC again.

Observed: No error, return prompt or recovery output appears. This reproduced independently after a longer session and from a fresh boot. QMP still responds; the isolated capture has EIP=dbd6bba7 and ESP=00054871, outside a sensible executable/stack layout. No panic text was printed. This is an observed crash/hang; stack corruption is a diagnosis, not a proven mechanism.

Expected and basis: Bound recursive calls and report a nesting/space error without losing the session. Shipped Volume One u5-entries-a.tex:336-341 says procedures may call themselves "to a depth of 64". Safe handling beyond the supported depth is also my reliability requirement.

Evidence: [shots/recursion-alone-recursive.png](shots/recursion-alone-recursive.png), [shots/recursion-alone-recovery.png](shots/recursion-alone-recovery.png), [evidence/recursion-alone-serial.log](evidence/recursion-alone-serial.log), [evidence/recursion-alone-qmp-state.json](evidence/recursion-alone-qmp-state.json), [evidence/programedges-qmp-state.json](evidence/programedges-qmp-state.json), [evidence/fixtures/RECINT.PRG](evidence/fixtures/RECINT.PRG).

Guess: os/samir/cmd/proc.c:592-607 allocates an 8192-byte automatic body per call; recursive stack growth may overwrite guest state before the intended nesting limit.

### L004 - Desktop-launched status mode prints ANSI control sequences

Area: Status display. Severity: P2.

Steps: Boot flair_tenants_interactive.img with rich-fixture.img, press Ctrl+Alt+backslash, USE CLIENTS, SET STATUS ON, then ? 2+2. Allow output to settle. SET STATUS OFF restores ordinary command output.

Observed: Escape sequences such as [23;1H and [0;7m fill and scroll the screen instead of positioning a status bar. The same setting works through tracer_boot.img and SAMIR.COM, so this is specific to the desktop route, not the old unrecognized SET STATUS command.

Expected and basis: SET STATUS ON should draw the status bar. Shipped quick reference qr-set.tex:16 documents "The status bar" for STATUS; the real III+ status layout is described in dbase3-decomp/specs/full-screen/assist-ui-and-editors.md section 10. The DOS-route screenshot is the control.

Evidence: [shots/desktop-status-status.png](shots/desktop-status-status.png), [shots/desktop-status-typing.png](shots/desktop-status-typing.png), [shots/dos-status-type.png](shots/dos-status-type.png), [evidence/desktop-status-serial.log](evidence/desktop-status-serial.log), [evidence/dos-status-serial.log](evidence/dos-status-serial.log).

Guess: os/samir/pal/pal_milton.c:566 and :607 bind the status display to ANSI sequences; the DOS boot logs ANSI-WIRED/ANSI.SYS, whereas the tested desktop boot does not.

### L005 - A successful memo replacement reads as empty until USE reopens the table

Area: Memo fields. Severity: P2.

Steps: Fresh rich-fixture.img: USE EDGE; ? MEMO; REPLACE MEMO WITH "changed memo"; ? MEMO,LEN(MEMO); GO 2; GO 1; ? MEMO; USE; USE EDGE; ? MEMO,LEN(MEMO). Repeat with "again".

Observed: Immediately after each successful REPLACE, the memo is empty and LEN is 0, even after moving away and back. Closing/reopening returns "changed memo" with length 12, then "again" on the second cycle. DBF pointers and DBT appended text are persisted correctly.

Expected and basis: Reads in the same work area should immediately reflect a successful write. Named reference: dbase3-decomp/specs/commands/data-definition-and-manipulation.md, REPLACE; dbt.md section 4 describes the memo pointer. The read-after-write requirement is also my data-consistency expectation.

Evidence: [shots/memo-write.png](shots/memo-write.png), [evidence/memo-serial.log](evidence/memo-serial.log), [evidence/memo-files/EDGE.DBF](evidence/memo-files/EDGE.DBF), [evidence/memo-files/EDGE.DBT](evidence/memo-files/EDGE.DBT), [evidence/memo.json](evidence/memo.json).

Guess: os/samir/cmd/workarea.c:63 retains the open memo handle, while os/samir/fs/dbf.c:1763 opens a separate writable handle and appends through it; the existing reader is not refreshed.

### L006 - An asterisk-filled numeric overflow is presented as zero

Area: Numeric fields. Severity: P2.

Steps: Fresh rich-fixture.img: USE EDGE; REPLACE SMALL WITH 99999; DISPLAY ID,SMALL. Close/reopen EDGE and display again; extract the stopped DBF.

Observed: SMALL N(4,1) is stored as **** at record offset 290, but DISPLAY prints 0.0. The same false zero appears after reopening.

Expected and basis: Preserve an unmistakable overflow indication in listings instead of presenting an ordinary zero. The storage rule is in shipped Volume One u5-entries-b.tex:577: "A number too large for its field is stored as asterisks." The expectation that the listing must not conceal that failure is my usability/data-integrity judgment; I did not run a live III+ overflow listing.

Evidence: [shots/data-wide.png](shots/data-wide.png), [evidence/data-serial.log](evidence/data-serial.log), [evidence/data-files/EDGE.DBF](evidence/data-files/EDGE.DBF), [evidence/data-bytes.json](evidence/data-bytes.json).

Guess: os/samir/fs/dbf.c:847 parses all-asterisk N fields with dec_parse and explicitly falls back to 0.0.

### L007 - TRANSFORM reports overflow for a decimal value that fits its picture

Area: Numeric picture formatting. Severity: P2.

Steps: Fresh rich-fixture.img: ? TRANSFORM(123.4,"999.99"). Compare ? STR(123.4,6,2) and the adjacent decimal cases in transform.json.

Observed: The six-character picture produces ***.** for 123.4, despite room for 123.40. Controls: STR(123.4,6,2) gives 123.40, and TRANSFORM(12.34,"999.99") gives 12.34; even 0.12 in "9.99" incorrectly becomes *.**.

Expected and basis: The three integer and two fractional digit positions should produce 123.40. Named reference: dbase3-decomp/specs/full-screen/picture-function-templates.md section 3 and its 9,999.99 example; the arithmetic fit of this exact case is independently obvious.

Evidence: [shots/expressions-types.png](shots/expressions-types.png), [evidence/expressions-serial.log](evidence/expressions-serial.log), [shots/verify-final-transform.png](shots/verify-final-transform.png), [evidence/verify-final-serial.log](evidence/verify-final-serial.log).

Guess: os/samir/core/fn_builtins.c:1580 passes the count of digit slots to dec_format as the total width, omitting space for the decimal point.

### L008 - INDEX ON cannot create an index for an empty table

Area: Indexes. Severity: P2.

Steps: Fresh rich-fixture.img: USE EMPTY; INDEX ON NAME TO ZERO; SEEK "NONE".

Observed: INDEX returns 29 File is not accessible. No ZERO.NDX is created. SEEK then returns 26 Database is not indexed. EMPTY.DBF is a valid zero-record table with NAME C(12).

Expected and basis: Create a valid empty index that can receive future appends. Named reference: dbase3-decomp/specs/file-formats/ndx.md, empty-root layout, and commands/data-definition-and-manipulation.md, INDEX. This is a common new-table workflow.

Evidence: [shots/index-fresh-empty.png](shots/index-fresh-empty.png), [evidence/index-fresh-serial.log](evidence/index-fresh-serial.log), [evidence/fixtures/EMPTY.DBF](evidence/fixtures/EMPTY.DBF), [evidence/index-fresh-files/EMPTY.DBF](evidence/index-fresh-files/EMPTY.DBF).

Guess: os/samir/cmd/mutate.c:1124 infers key width from the current evaluated value, which is empty when there is no record.

### L009 - Four-letter command abbreviations are rejected

Area: Command language. Severity: P2.

Steps: USE CLIENTS. Enter DISP; REPL NAME WITH "TEST"; APPE BLANK; LOCA FOR BAL>1000; SELE 2; INDE ON NAME TO TEST; STOR 5 TO X.

Observed: Every abbreviation returns 16 *** Unrecognized command verb. Full spellings work for the corresponding implemented commands.

Expected and basis: Named reference: dbase3-decomp/specs/runtime/interpreter-and-execution-model.md:188-224 documents first-four-letter recognition. The shipped Volume One u2-commands.tex:26 explicitly instructs full spellings, so this is a historical feature gap, not an undocumented manual contradiction.

Evidence: [shots/missing-abbrev.png](shots/missing-abbrev.png), [evidence/missing-serial.log](evidence/missing-serial.log), [evidence/missing.json](evidence/missing.json).

### L010 - DISPLAY STATUS is parsed as a field expression

Area: Inspection. Severity: P2.

Steps: USE CLIENTS; DISPLAY STATUS.

Observed: 12 Variable not found is returned; no environment/work-area report is shown.

Expected and basis: Named reference: dbase3-decomp/specs/commands/navigation-query-display.md:713-727 specifies open files and system parameters. This is separate from the fixed command SET STATUS and the old DISPLAY STRUCTURE gap.

Evidence: [shots/missing-inspection.png](shots/missing-inspection.png), [evidence/missing-serial.log](evidence/missing-serial.log).

### L011 - DISPLAY MEMORY is parsed as a field expression

Area: Inspection. Severity: P2.

Steps: USE CLIENTS; DISPLAY MEMORY.

Observed: 12 Variable not found is returned; no memory-variable listing is shown.

Expected and basis: Named reference: dbase3-decomp/specs/commands/navigation-query-display.md:731-743 specifies names, types, sizes, scopes and values of memory variables.

Evidence: [shots/missing-inspection.png](shots/missing-inspection.png), [evidence/missing-serial.log](evidence/missing-serial.log).

### L012 - MODIFY STRUCTURE is unavailable

Area: Structure editor. Severity: P2.

Steps: USE CLIENTS; MODIFY STRUCTURE.

Observed: 16 *** Unrecognized command verb. No field editor opens, so rename/widen/narrow/add/delete with existing data cannot be reached.

Expected and basis: Named reference: dbase3-decomp/specs/commands/data-definition-and-manipulation.md section 2 and full-screen/assist-ui-and-editors.md section 6. Earlier K08 covers CREATE and record editors, not this reached modification command.

Evidence: [shots/missing-inspection.png](shots/missing-inspection.png), [evidence/missing-serial.log](evidence/missing-serial.log).

### L013 - QUIT inside a PRG is rejected rather than exiting

Area: Program lifecycle. Severity: P2.

Steps: Run DO QUITPRG using rich-fixture.img. The file prints BEFORE QUIT, executes QUIT, and would then print AFTER QUIT.

Observed: BEFORE QUIT is followed by 16 *** Unrecognized command verb and a dot prompt. The database process remains alive; AFTER QUIT is not run because the program aborted.

Expected and basis: QUIT should close files and end the database session when executed by a program. Named reference: dbase3-decomp/specs/commands/programming-and-io.md, QUIT; shipped Quick Reference qr-commands.tex:37 entry QUIT says "Closes all files and leaves InitechBase".

Evidence: [shots/programmore-real-quitprogram.png](shots/programmore-real-quitprogram.png), [evidence/programmore-real-serial.log](evidence/programmore-real-serial.log), [evidence/fixtures/QUITPRG.PRG](evidence/fixtures/QUITPRG.PRG).

### L014 - PACK with two open indexes leaves live index state unusable

Area: Indexes and PACK. Severity: P2.

Steps: Fresh rich-fixture.img: USE CLIENTS; INDEX ON NAME TO BYNAME; INDEX ON BAL TO BYBAL; SET INDEX TO BYNAME,BYBAL; SET ORDER TO 1; PACK; REINDEX; GO TOP; LIST. Then CLOSE INDEX; GO TOP; LIST. Finally USE; USE CLIENTS INDEX BYNAME,BYBAL; GO TOP; LIST.

Observed: PACK reports success, but REINDEX returns 114 Index damaged, GO TOP returns 5 Record is out of range, and LIST returns 10 Syntax error. Closing indexes restores physical traversal; reopening both restores sorted traversal. The one-index control passes. This reproduced without any deleted records, so record compaction is not required. The stopped DBF and NDX bytes remain readable; persistent index corruption is not claimed.

Expected and basis: PACK must preserve usable open indexes, rebuilding them as needed. Named reference: dbase3-decomp/specs/commands/data-definition-and-manipulation.md section 10 (PACK).

Evidence: [shots/pack-two-after.png](shots/pack-two-after.png), [evidence/pack-two-serial.log](evidence/pack-two-serial.log), [evidence/pack-two-files/BYNAME.NDX](evidence/pack-two-files/BYNAME.NDX), [evidence/pack-two-files/BYBAL.NDX](evidence/pack-two-files/BYBAL.NDX), [evidence/pack-two-files/CLIENTS.DBF](evidence/pack-two-files/CLIENTS.DBF), [evidence/pack-index-serial.log](evidence/pack-index-serial.log).

Guess: inspect the multi-index lifetime across os/samir/cmd/mutate.c:945-959 and fs/ndx.c:2655 (ndx_rebuild). A single-index run does not reproduce it.

### L015 - INDEX ON fails when the current pointer is at EOF

Area: Indexes. Severity: P2.

Steps: Fresh rich-fixture.img: USE CLIENTS; GO BOTTOM; SKIP; ? EOF(),RECNO(); INDEX ON NAME TO EOFIDX. Then GO TOP; INDEX ON NAME TO TOPIDX.

Observed: EOF is true at record 4. The first INDEX returns 29 File is not accessible and creates no file; the same expression succeeds after GO TOP.

Expected and basis: INDEX ON scans the table independently of the incoming current-record position. Named reference: dbase3-decomp/specs/commands/data-definition-and-manipulation.md, INDEX; my equivalent-input expectation is that moving to EOF does not make the output file inaccessible.

Evidence: [shots/index-boundary-at-eof.png](shots/index-boundary-at-eof.png), [evidence/index-boundary-serial.log](evidence/index-boundary-serial.log), [evidence/index-boundary-files/TOPIDX.NDX](evidence/index-boundary-files/TOPIDX.NDX).

Guess: os/samir/cmd/mutate.c:1124-1130 derives key width/type from the current record before scanning. At EOF a character field evaluates as a zero-length value.

### L016 - A valid blank date prevents creation of a date index

Area: Indexes. Severity: P2.

Steps: Fresh rich-fixture.img: USE EDGE; INDEX ON WHEN TO BLANKDAT. EDGE record 3 has eight blank bytes in WHEN. Control on another fresh copy: REPLACE ALL WHEN WITH CTOD("01/01/92"); GO TOP; INDEX ON WHEN TO VALIDATE.

Observed: The original table returns 9 Data type mismatch. Filling all dates with valid dates and moving to GO TOP succeeds. The shipped STAFF/HIRED date-index example also succeeds. This is specifically the valid blank date case, not missing date indexing generally.

Expected and basis: Blank dates are legal database values and must be indexable alongside ordinary dates. Named references: dbase3-decomp/specs/file-formats/dbf.md section 5 (blank D fields) and ndx.md section 4.2 (date keys).

Evidence: [shots/date-index-blank.png](shots/date-index-blank.png), [shots/verify-final-date-control.png](shots/verify-final-date-control.png), [shots/verify-final-manual-date.png](shots/verify-final-manual-date.png), [evidence/date-index-serial.log](evidence/date-index-serial.log), [evidence/verify-final-serial.log](evidence/verify-final-serial.log), [evidence/fixtures/EDGE.DBF](evidence/fixtures/EDGE.DBF).

Guess: os/samir/cmd/mutate.c:244-249 accepts XB_N and XB_D, but not the XB_U representation returned for a blank DBF date.

### L017 - INDEX accepts 101-character keys beyond the III+ maximum

Area: Index bounds. Severity: P2.

Steps: Fresh rich-fixture.img: USE EDGE; GO TOP; INDEX ON LEFT(WIDE,100) TO KEY100; GO TOP; INDEX ON LEFT(WIDE,101) TO KEY101. Extract both NDX files after stopping.

Observed: Both succeed. KEY101.NDX records key_length=101 in the little-endian word at offset 12. A direct 254-character key is rejected with 29 File is not accessible, rather than the specific key-size diagnostic.

Expected and basis: Reject keys longer than 100 with the III+ index-too-big diagnostic. Named reference: the III+ message catalog "Index is too big (100 char maximum)" and dbase3-decomp/specs/commands/data-definition-and-manipulation.md, INDEX limits. This is a compatibility/boundary defect; I did not feed the generated 101-byte index into a live III+ interpreter.

Evidence: [evidence/index-boundary-serial.log](evidence/index-boundary-serial.log), [evidence/index-boundary-files/KEY100.NDX](evidence/index-boundary-files/KEY100.NDX), [evidence/index-boundary-files/KEY101.NDX](evidence/index-boundary-files/KEY101.NDX), [shots/artifact-controls-index-length.png](shots/artifact-controls-index-length.png), [evidence/artifact-controls-serial.log](evidence/artifact-controls-serial.log).

### L018 - A corrupt existing NDX is reported as nonexistent

Area: Index diagnostics. Severity: P2.

Steps: Fresh rich-fixture.img: USE CLIENTS; SET INDEX TO BAD.NDX. BAD.NDX is the 559-byte fixture of repeated "not an index" text.

Observed: The command says 1 File does not exist, although the file is present and extractable from the same image.

Expected and basis: Distinguish a malformed index from a missing pathname, so the user can take the right repair action. Named reference: dbase3-decomp/specs/file-formats/ndx.md, header validation invariants; the accurate-diagnostic requirement is my usability judgment.

Evidence: [shots/errors-files.png](shots/errors-files.png), [evidence/errors-serial.log](evidence/errors-serial.log), [evidence/fixtures/BAD.NDX](evidence/fixtures/BAD.NDX), [evidence/index-fresh-files/BAD.NDX](evidence/index-fresh-files/BAD.NDX).

### L019 - The ordinary dot prompt truncates commands at 128 bytes

Area: Command input. Severity: P2.

Steps: Use desktop route with default SET STATUS OFF. Type the exact 254-character command in keys.json: ? LEN(" followed by 245 x characters, then "). Press Enter. Repeat the 255-character variant with 246 y characters.

Observed: Only the first 128 bytes are accepted; the remaining 126/127 keystrokes ring the bell. The closing quote/parenthesis is lost and execution reports 35 Unterminated string. This is not a legitimate rejection of a 255-byte command: the documented 254-byte case fails too. DOS-route controls reproduce the 128-byte cap with status off; with status on, the 254-byte command returns 245 and a 255-byte command is cleanly rejected with error 18.

Expected and basis: Accept the promised 254-character line. Shipped Volume One ua-keys.tex:20-22 says printable characters are taken "up to 254 characters"; u2-commands.tex:16 repeats the limit. Reject an overlong line coherently rather than executing its truncated prefix.

Evidence: [shots/keys-254.png](shots/keys-254.png), [shots/keys-255.png](shots/keys-255.png), [evidence/keys.json](evidence/keys.json), [evidence/keys-serial.log](evidence/keys-serial.log), [evidence/key-lengths.json](evidence/key-lengths.json), [shots/keys-dos-off.png](shots/keys-dos-off.png), [shots/keys-dos-on.png](shots/keys-dos-on.png), [shots/keys-dos-overlong.png](shots/keys-dos-overlong.png), [evidence/keys-dos-serial.log](evidence/keys-dos-serial.log).

Guess: os/samir/pal/pal_milton.c:650-654 uses the cooked DOS stream when status is off; the observed 128-byte cap occurs below SAMIR line framing.

### L020 - A 7018-byte, 1000-line program fails with insufficient memory

Area: Program capacity. Severity: P2.

Steps: Fresh program-fixture.img: DO SHORT1K. The fixture sets N=0, repeats N=N+1 1000 times, prints N and returns; its exact CRLF file size is 7018 bytes.

Observed: 43 Insufficient memory is returned without running the program. An ordinary compact program with IF, CASE, LOOP, EXIT and procedures runs successfully in the control session.

Expected and basis: This small program should fit the advertised file limit and a 16 MiB workstation. Shipped Volume One u5-entries-a.tex:331-333 states a maximum of 8191 characters. The 1000-line case is explicitly requested by this audit brief. This is a capacity limitation, not a crash.

Evidence: [shots/programedges-1000.png](shots/programedges-1000.png), [evidence/programedges-serial.log](evidence/programedges-serial.log), [evidence/fixtures/SHORT1K.PRG](evidence/fixtures/SHORT1K.PRG), [evidence/programs-real-serial.log](evidence/programs-real-serial.log).

Guess: os/samir/cmd/flow.c:54 fixes FLOW_MAX_LINES at 512, an additional limit not stated with the documented file-size limit.

### L021 - DO inside a PRG cannot call another existing PRG file

Area: Program calls. Severity: P2.

Steps: Fresh control-fixture.img: DO OUTER. OUTER.PRG contains DO USEPRG, then prints CHILD RETURNED. USEPRG.PRG exists and works when invoked directly at the dot prompt.

Observed: The nested call returns 16 *** Unrecognized command verb. CHILD RETURNED is never printed. Internal named procedures work in FLOW.PRG.

Expected and basis: Resolve a DO target to either a procedure or another program file. Named reference: dbase3-decomp/specs/commands/programming-and-io.md:71-85, resolution order. The shipped manual describes only internal procedure calls from a program, so this is a historical capability gap.

Evidence: [shots/control-nested.png](shots/control-nested.png), [evidence/control-serial.log](evidence/control-serial.log), [evidence/programs-real-serial.log](evidence/programs-real-serial.log), [evidence/fixtures/OUTER.PRG](evidence/fixtures/OUTER.PRG), [evidence/fixtures/USEPRG.PRG](evidence/fixtures/USEPRG.PRG).

### L022 - RETRY in an error handler does not retry the failed statement

Area: Error recovery. Severity: P2.

Steps: Fresh retry-fixture.img: DO RETRYFIX. Main code declares public XX and TRYCNT, sets XX="bad", and executes ? XX+1. Its ON ERROR procedure sets the shared XX to 6, increments TRYCNT and executes RETRY. Main code then prints DONE,TRYCNT. Query XX,TRYCNT afterwards.

Observed: Only DONE 1 appears; the repaired ? XX+1 is never retried, so 7 is missing. The following query proves XX=6 and TRYCNT=1. Public variables make this independent of handler-local variable lifetime. The program returns normally.

Expected and basis: RETRY must execute the failing command again after the handler fixes its cause. Named reference: dbase3-decomp/specs/commands/programming-and-io.md:232-243 quotes the III+ HELP behavior and the ERRORPRG.PRG usage.

Evidence: [shots/retry-control-result.png](shots/retry-control-result.png), [evidence/retry-control-serial.log](evidence/retry-control-serial.log), [evidence/fixtures/RETRYFIX.PRG](evidence/fixtures/RETRYFIX.PRG), [evidence/control-serial.log](evidence/control-serial.log).

### L023 - BOF stays false for a physically empty database

Area: Record boundaries. Severity: P2.

Steps: USE EMPTY; ? RECCOUNT(),EOF(),BOF(); GO TOP; ? EOF(),BOF().

Observed: The count is 0 and EOF is true, but BOF is false both immediately after USE and after GO TOP.

Expected and basis: For an empty file, both boundary predicates should be true. Named reference: dbase3-decomp/specs/commands/navigation-query-display.md:999. That local specification includes cross-implementation corroboration; I did not mint this case in a live III+ session.

Evidence: [shots/verify-final-empty.png](shots/verify-final-empty.png), [evidence/verify-final-serial.log](evidence/verify-final-serial.log), [evidence/fixtures/EMPTY.DBF](evidence/fixtures/EMPTY.DBF).

### L024 - CREATE/MODIFY REPORT cannot open a designer

Area: Report design. Severity: P2.

Steps: USE CLIENTS; CREATE REPORT NEWREP; MODIFY REPORT STAFFRPT.

Observed: Both return 16 *** Unrecognized command verb. No report designer opens.

Expected and basis: The named III+ command should be available. Named reference: dbase3-decomp/specs/full-screen/assist-ui-and-editors.md section 8.1. This finding concerns the reached entry point; deeper workflows are not claimed to have run.

Evidence: [shots/ui-gaps-design.png](shots/ui-gaps-design.png), [evidence/ui-gaps-serial.log](evidence/ui-gaps-serial.log).

### L025 - CREATE LABEL cannot open a designer

Area: Label design. Severity: P2.

Steps: USE CLIENTS; CREATE LABEL NEWLAB.

Observed: 16 *** Unrecognized command verb. No label designer opens.

Expected and basis: The named III+ command should be available. Named reference: dbase3-decomp/specs/full-screen/assist-ui-and-editors.md section 8.2. This finding concerns the reached entry point; deeper workflows are not claimed to have run.

Evidence: [shots/ui-gaps-design.png](shots/ui-gaps-design.png), [evidence/ui-gaps-serial.log](evidence/ui-gaps-serial.log).

### L026 - MODIFY COMMAND cannot edit a PRG

Area: Program editing. Severity: P2.

Steps: MODIFY COMMAND FLOW, with FLOW.PRG on the disk.

Observed: 16 *** Unrecognized command verb. No text editor opens.

Expected and basis: The named III+ command should be available. Named reference: dbase3-decomp/specs/full-screen/assist-ui-and-editors.md section 9. This finding concerns the reached entry point; deeper workflows are not claimed to have run.

Evidence: [shots/ui-gaps-design.png](shots/ui-gaps-design.png), [evidence/ui-gaps-serial.log](evidence/ui-gaps-serial.log).

### L027 - COPY STRUCTURE is unavailable

Area: Structure export. Severity: P2.

Steps: USE CLIENTS; COPY STRUCTURE TO STRUCT.

Observed: 16 *** Unrecognized command verb; no structure-only DBF is created.

Expected and basis: The named III+ command should be available. Named reference: dbase3-decomp/specs/commands/data-definition-and-manipulation.md, COPY STRUCTURE. This finding concerns the reached entry point; deeper workflows are not claimed to have run.

Evidence: [shots/ui-gaps-data.png](shots/ui-gaps-data.png), [evidence/ui-gaps-serial.log](evidence/ui-gaps-serial.log).

### L028 - COPY STRUCTURE EXTENDED is unavailable

Area: Structure export. Severity: P2.

Steps: USE CLIENTS; COPY STRUCTURE EXTENDED TO EXTSTR.

Observed: 16 *** Unrecognized command verb; no field-definition table is created.

Expected and basis: The named III+ command should be available. Named reference: dbase3-decomp/specs/commands/data-definition-and-manipulation.md, COPY STRUCTURE EXTENDED. This finding concerns the reached entry point; deeper workflows are not claimed to have run.

Evidence: [shots/ui-gaps-data.png](shots/ui-gaps-data.png), [evidence/ui-gaps-serial.log](evidence/ui-gaps-serial.log).

### L029 - JOIN is not recognized

Area: Multi-table data. Severity: P2.

Steps: USE CLIENTS; JOIN WITH CLIENTS TO JOINED FOR .T.

Observed: 16 *** Unrecognized command verb, rather than entering the JOIN command path or diagnosing the operands. No join is performed.

Expected and basis: The named III+ command should be available. Named reference: dbase3-decomp/specs/commands/data-definition-and-manipulation.md, JOIN. This finding concerns the reached entry point; deeper workflows are not claimed to have run.

Evidence: [shots/ui-gaps-data.png](shots/ui-gaps-data.png), [evidence/ui-gaps-serial.log](evidence/ui-gaps-serial.log).

### L030 - UPDATE is not recognized

Area: Multi-table data. Severity: P2.

Steps: USE CLIENTS; UPDATE ON NAME FROM CLIENTS REPLACE BAL WITH BAL.

Observed: 16 *** Unrecognized command verb, rather than entering the UPDATE command path or diagnosing the source work area.

Expected and basis: The named III+ command should be available. Named reference: dbase3-decomp/specs/commands/data-definition-and-manipulation.md, UPDATE. This finding concerns the reached entry point; deeper workflows are not claimed to have run.

Evidence: [shots/ui-gaps-data.png](shots/ui-gaps-data.png), [evidence/ui-gaps-serial.log](evidence/ui-gaps-serial.log).

### L031 - LIST rejects TO PRINT

Area: Printed listings. Severity: P2.

Steps: USE CLIENTS; LIST NAME TO PRINT.

Observed: 36 Unrecognized phrase/keyword in command. It never reaches the printer availability check. REPORT FORM TO PRINT reaches error 126 instead.

Expected and basis: The named III+ command should be available. Named reference: dbase3-decomp/specs/commands/navigation-query-display.md, LIST syntax. This finding concerns the reached entry point; deeper workflows are not claimed to have run.

Evidence: [shots/ui-gaps-data.png](shots/ui-gaps-data.png), [evidence/ui-gaps-serial.log](evidence/ui-gaps-serial.log).

### L032 - Neither INDEX UNIQUE nor SET UNIQUE is supported

Area: Index uniqueness. Severity: P2.

Steps: USE CLIENTS; INDEX ON NAME TO UNIQUE1 UNIQUE; SET UNIQUE ON; SET UNIQUE OFF.

Observed: INDEX ... UNIQUE and SET UNIQUE ON/OFF each return 10 Syntax error. The ordinary INDEX ON NAME TO UNIQUE2 control succeeds.

Expected and basis: The named III+ command should be available. Named reference: dbase3-decomp/specs/commands/set-commands.md:165 and data-definition-and-manipulation.md, INDEX UNIQUE. This finding concerns the reached entry point; deeper workflows are not claimed to have run.

Evidence: [shots/ui-gaps-unique.png](shots/ui-gaps-unique.png), [evidence/ui-gaps-serial.log](evidence/ui-gaps-serial.log).

### L033 - @ SAY and the form entry point are unavailable

Area: Forms. Severity: P2.

Steps: DO FORM; then @ 2,3 SAY "HELLO". FORM.PRG contains @ 2,3 SAY "Name:" GET NAMEVAR followed by READ.

Observed: 16 *** Unrecognized command verb at the @ command. No form is drawn; READ and its editing keys cannot be reached through that program.

Expected and basis: The named III+ command should be available. Named reference: dbase3-decomp/specs/full-screen/assist-ui-and-editors.md and dbase3-decomp/specs/full-screen/say-get-read.md. This finding concerns the reached entry point; deeper workflows are not claimed to have run.

Evidence: [shots/programmore-real-form.png](shots/programmore-real-form.png), [evidence/programmore-real-serial.log](evidence/programmore-real-serial.log).

### L034 - TEXT/ENDTEXT literal blocks are unavailable

Area: Program output. Severity: P2.

Steps: DO TEXT. TEXT.PRG contains two literal text lines between TEXT and ENDTEXT.

Observed: 16 *** Unrecognized command verb; neither text line appears.

Expected and basis: The named III+ command should be available. Named reference: dbase3-decomp/specs/commands/programming-and-io.md:494-517. This finding concerns the reached entry point; deeper workflows are not claimed to have run.

Evidence: [shots/programmore-real-form.png](shots/programmore-real-form.png), [evidence/programmore-real-serial.log](evidence/programmore-real-serial.log).

### L035 - SET ECHO is unavailable

Area: Program tracing. Severity: P2.

Steps: SET ECHO ON; DO USEPRG; SET ECHO OFF.

Observed: Both SET ECHO forms return 10 Syntax error; the program runs without an execution trace.

Expected and basis: The named III+ command should be available. Named reference: dbase3-decomp/specs/commands/set-commands.md:150. This finding concerns the reached entry point; deeper workflows are not claimed to have run.

Evidence: [shots/settings-trace.png](shots/settings-trace.png), [evidence/settings-serial.log](evidence/settings-serial.log).

### L036 - SET FIXED is unavailable

Area: Numeric display. Severity: P2.

Steps: SET FIXED ON; SET DECIMALS TO 2; ? 3,3.5; SET FIXED OFF.

Observed: Both SET FIXED forms return 10 Syntax error. The values print as 3 and 3.5.

Expected and basis: The named III+ command should be available. Named reference: dbase3-decomp/specs/commands/set-commands.md:153 and section SET FIXED. This finding concerns the reached entry point; deeper workflows are not claimed to have run.

Evidence: [shots/settings-format.png](shots/settings-format.png), [evidence/settings-serial.log](evidence/settings-serial.log).

### L037 - SET MARGIN is unavailable

Area: Printing layout. Severity: P2.

Steps: SET MARGIN TO 5.

Observed: 10 Syntax error.

Expected and basis: The named III+ command should be available. Named reference: dbase3-decomp/specs/commands/set-commands.md:225-233. This finding concerns the reached entry point; deeper workflows are not claimed to have run.

Evidence: [shots/settings-format.png](shots/settings-format.png), [evidence/settings-serial.log](evidence/settings-serial.log).

### L038 - SET PRINT is unavailable

Area: Printing. Severity: P2.

Steps: SET PRINT ON; SET PRINT OFF.

Observed: Both return 10 Syntax error, independently of printer availability.

Expected and basis: The named III+ command should be available. Named reference: dbase3-decomp/specs/commands/set-commands.md:158. This finding concerns the reached entry point; deeper workflows are not claimed to have run.

Evidence: [shots/settings-format.png](shots/settings-format.png), [evidence/settings-serial.log](evidence/settings-serial.log).

### L039 - SET DEVICE is unavailable

Area: Output routing. Severity: P2.

Steps: SET DEVICE TO PRINT; SET DEVICE TO SCREEN.

Observed: Both return 10 Syntax error.

Expected and basis: The named III+ command should be available. Named reference: dbase3-decomp/specs/commands/set-commands.md:359-370. This finding concerns the reached entry point; deeper workflows are not claimed to have run.

Evidence: [shots/settings-format.png](shots/settings-format.png), [evidence/settings-serial.log](evidence/settings-serial.log).

### L040 - Command-history controls are unavailable

Area: Command history. Severity: P2.

Steps: ? "FIRST"; Up; Enter; LIST HISTORY; SET HISTORY ON.

Observed: Up does not recall the previous command, LIST HISTORY returns 3 File is already open in the no-table state, and SET HISTORY ON returns 10 Syntax error.

Expected and basis: The named III+ command should be available. Named reference: dbase3-decomp/specs/commands/navigation-query-display.md, LIST/DISPLAY HISTORY; set-commands.md, HISTORY. This finding concerns the reached entry point; deeper workflows are not claimed to have run.

Evidence: [shots/keys-history.png](shots/keys-history.png), [evidence/keys-serial.log](evidence/keys-serial.log).

### L041 - RETURN TO MASTER returns only one level

Area: Program stack. Severity: P2.

Steps: Fresh master-fixture.img: DO MASTER2. The master calls FIRST; FIRST calls SECOND; SECOND executes RETURN TO MASTER.

Observed: The output is MASTER, FIRST, SECOND, FIRST RESUMED, MASTER RESUMED. FIRST RESUMED should have been skipped.

Expected and basis: Return directly to the top-level routine, discarding intermediate frames. Named reference: dbase3-decomp/specs/commands/programming-and-io.md:207-225 quotes the III+ HELP definition. A one-level control is intentionally not used as proof, because ordinary RETURN and RETURN TO MASTER are equivalent there.

Evidence: [shots/master-deep-result.png](shots/master-deep-result.png), [evidence/master-deep-serial.log](evidence/master-deep-serial.log), [evidence/fixtures/MASTER2.PRG](evidence/fixtures/MASTER2.PRG).

### L042 - LABEL FORM rejects SAMPLE

Area: Labels. Severity: P2.

Steps: Fresh rich-fixture.img: USE STAFF; LABEL FORM STAFFLBL SAMPLE. STAFFLBL.LBL is valid and LABEL FORM ... TO FILE succeeds in reports.json.

Observed: 36 Unrecognized phrase/keyword in command is returned.

Expected and basis: Offer the label-alignment sample mode. Named reference: dbase3-decomp/specs/full-screen/assist-ui-and-editors.md section 8.2 and its LABEL FORM ... SAMPLE syntax. This is separate from the previously missing LABEL FORM entry point, which now works.

Evidence: [shots/label-sample-result.png](shots/label-sample-result.png), [evidence/label-sample-serial.log](evidence/label-sample-serial.log), [evidence/reports-files/LABELS.TXT](evidence/reports-files/LABELS.TXT).

### L043 - Failed output commands leave invalid destinations that block retry

Area: Disk-full recovery. Severity: P2.

Steps: DOS route with full-fixture.img: USE CLIENTS; INDEX ON NAME TO FULLIDX; SORT TO SORTED ON NAME; USE; COPY FILE CLIENTS.DBF TO COPIED.DBF. ERASE FILLER.BIN; USE CLIENTS; repeat INDEX and SORT; USE SORTED.

Observed: Each failed command leaves a zero-byte destination. After disk space is freed, INDEX and SORT return 7 File already exists, and USE SORTED returns 15 Not a dBASE database. The source CLIENTS remains intact. This is failed-output cleanup, distinct from the fixed old disk-full APPEND corruption.

Expected and basis: Either remove a new incomplete output on failure or provide a clear recovery path that does not treat the failed placeholder as completed user data. This expectation is my transactional-output/usability judgment; DBF/NDX references establish that these zero-byte files are invalid.

Evidence: [shots/full-recovery-failure.png](shots/full-recovery-failure.png), [shots/full-recovery-retry.png](shots/full-recovery-retry.png), [evidence/full-recovery-serial.log](evidence/full-recovery-serial.log), [evidence/full-recovery-files/FULLIDX.NDX](evidence/full-recovery-files/FULLIDX.NDX), [evidence/full-recovery-files/SORTED.DBF](evidence/full-recovery-files/SORTED.DBF), [evidence/full-recovery-files/COPIED.DBF](evidence/full-recovery-files/COPIED.DBF), [evidence/full-recovery-bytes.json](evidence/full-recovery-bytes.json).

### L044 - Mutation commands without an open table report syntax errors

Area: Command diagnostics. Severity: P2.

Steps: USE with no filename closes the table. Then DELETE; RECALL; PACK; REPLACE NAME WITH "x".

Observed: Every command returns 10 Syntax error. These command forms are syntactically valid and work with a table open.

Expected and basis: Report No database is in USE, rather than asking the user to fix valid syntax. Named reference: the III+ error catalog and dbase3-decomp/specs/commands/data-definition-and-manipulation.md, each command requires an open database. This is a reached precondition error, not the older unsupported-editor diagnostic in K08.

Evidence: [evidence/settings-serial.log](evidence/settings-serial.log), [shots/artifact-controls-no-table.png](shots/artifact-controls-no-table.png), [evidence/artifact-controls-serial.log](evidence/artifact-controls-serial.log).

### L045 - SET RELATION accepts a link but never moves the child

Area: Related work areas. Severity: P2.

Steps: SELECT 2; USE DEPTS ALIAS D INDEX DEPTS; SELECT 1; USE STAFF; SET RELATION TO DEPT INTO D; GO 1; ? DEPT,D->DEPT; GO 5; ? DEPT,D->DEPT.

Observed: The two queries print SOFT ACCT and FACL ACCT. The indexed child stays on its first ACCT record instead of following the parent key.

Expected and basis: Move the child to the matching department as the parent changes. Named reference: dbase3-decomp/specs/commands/set-commands.md, SET RELATION. Shipped Quick Reference qr-set.tex:14 explicitly labels it "recorded, not maintained by this version"; this is a tested historical feature gap, not an undisclosed manual promise.

Evidence: [shots/last-controls-relation.png](shots/last-controls-relation.png), [evidence/last-controls-serial.log](evidence/last-controls-serial.log), [evidence/fixtures/STAFF.DBF](evidence/fixtures/STAFF.DBF), [evidence/fixtures/DEPTS.DBF](evidence/fixtures/DEPTS.DBF), [evidence/fixtures/DEPTS.NDX](evidence/fixtures/DEPTS.NDX).

### L046 - SET TALK ON gives no replacement count

Area: Command feedback. Severity: P2.

Steps: USE CLIENTS; SET TALK ON; REPLACE ALL BAL WITH BAL+1.

Observed: Three records are changed, but the command returns directly to the dot prompt without a count or other completion feedback.

Expected and basis: Show the processed-record count under TALK ON. This expectation comes from my memory of dBASE result counts, corroborated by the HELP-derived SET TALK purpose in dbase3-decomp/specs/commands/set-commands.md. Shipped qr-set.tex:17 openly says TALK is "recorded, not acted on by this version".

Evidence: [shots/last-controls-talk.png](shots/last-controls-talk.png), [evidence/last-controls-serial.log](evidence/last-controls-serial.log), [evidence/last-controls-files/CLIENTS.DBF](evidence/last-controls-files/CLIENTS.DBF), [evidence/last-controls-bytes.json](evidence/last-controls-bytes.json).

### L047 - Disk-full output errors identify the source or no file at all

Area: Disk-full diagnostics. Severity: P3.

Steps: DOS route with full-fixture.img: USE CLIENTS; SORT TO SORTED ON NAME. Then USE; COPY FILE CLIENTS.DBF TO COPIED.DBF.

Observed: SORT says "Disk full when writing file: CLIENTS.DBF", although the attempted destination is SORTED.DBF. COPY reports the same message with nothing after the colon. This repeated in full-real and full-recovery.

Expected and basis: Name the output file whose write failed. This is my diagnostic-clarity expectation; the message itself promises a filename. The underlying disk-full condition is correctly detected in these cases.

Evidence: [shots/full-recovery-failure.png](shots/full-recovery-failure.png), [evidence/full-recovery-serial.log](evidence/full-recovery-serial.log), [evidence/full-real-serial.log](evidence/full-real-serial.log).

### L048 - Rejected keystrokes draw bell glyphs across the desktop console

Area: Console polish. Severity: P3.

Steps: Desktop route, status off: type the 254-character command in keys.json past the 128-byte input cap.

Observed: The rejected characters emit 126 BEL bytes (0x07), and the guest screen paints rows of small bullet-like glyphs for those bytes. The DOS-route screenshot suppresses those marks.

Expected and basis: A bell indication should not paint control-character glyphs into the command transcript. This is my terminal/usability expectation, independently of the wrong input limit in L019.

Evidence: [shots/keys-254.png](shots/keys-254.png), [shots/keys-dos-off.png](shots/keys-dos-off.png), [evidence/key-lengths.json](evidence/key-lengths.json), [evidence/keys-serial.log](evidence/keys-serial.log).

Guess: the desktop console does not interpret BEL on the same path that lacks ANSI status handling; the DOS route has ANSI.SYS active.

## Still present

These are earlier findings observed again, not new NITS rows. Prior report text is indexed in evidence/prior-database-findings.json. The relevant earlier reports were the 2026-10-04 pass-5 audit, 2026-10-05 full drive and 2026-10-15 look pass; the remaining earlier reports were screened for subsystem overlap.

| Earlier IDs | This session | Evidence |
| --- | --- | --- |
| 2026-10-04/K08, remaining portions | BROWSE, EDIT, bare APPEND, CREATE, ASSIST, HELP, SUM, COUNT, COPY TO and DISPLAY STRUCTURE remain unavailable or fail. AVERAGE also failed; it is retained under the earlier aggregate gap rather than assigned a duplicate new number. DO of a disk PRG now works. | initial-serial.log; data-serial.log; missing-serial.log; shots/missing-inspection.png |
| 2026-10-05/L044 | DATE()/YEAR(DATE()) still use 1926 for the current 2026 clock. | regress-serial.log; shots/regress-date.png; manualfn-serial.log |
| 2026-10-05/L045 | Empty/invalid dates still display malformed year characters. New L001 additionally proves malformed on-disk date bytes. | shots/regress-date.png; regress-serial.log |
| 2026-10-05/L046 | The next dot prompt still follows query output on the same line. The current shipped manual explicitly documents this layout (Volume One l1-quick-start.tex:92-93); the old visual symptom remains, but it is not refiled as a new bug. | shots/regress-close.png; manualfn-serial.log |
| 2026-10-05/L050 | 1/0 raises error 39 rather than the minted III+ overflow result. | regress-serial.log; shots/regress-close.png; references/mint-results-002.md |
| 2026-10-05/L059, L060, L061, L062, L063, L066, L115 | SET DEFAULT, PATH, ESCAPE, CONFIRM, BELL, PROCEDURE and FUNCTION remain rejected. | regress-serial.log; shots/regress-misc.png |
| 2026-10-05/L067, L068, L069, L070, L072, L073, L074, L075, L076, L077, L078 | EXP, LOG, SQRT, TIME, DISKSPACE, VERSION, OS, PCOL, PROW, ROW and COL each return 31 Invalid function name when tested individually. | missing-serial.log; shots/missing-functions.png |
| 2026-10-15/M014 | The pale gray text on green console remains. The real III+ report capture provides a reference; no new duplicate palette finding is filed. | shots/paging-pages.png; references/real-dbase-report.png |

## Now fixed

These verdicts concern the exact earlier failure, not complete feature certification.

| Earlier IDs | Successful retest and limits | Evidence |
| --- | --- | --- |
| 2026-10-05/L001 | Full-disk APPEND BLANK stops at 12 complete records, reports refusal for subsequent appends, and reopens at count 12. After deleting one record and PACK, the extracted file has count 11, header 129, record length 31, physical size 502 and a complete logical record area. | full-real-serial.log; full-real-bytes.json; full-real-files/CLIENTS.DBF; shots/full-real-append.png; shots/full-real-reopen.png |
| 2026-10-05/L079 | APPEND now identifies disk full as error 56 instead of calling the table read-only. Other output diagnostics/recovery have new findings L043 and L047. | full-real-serial.log |
| 2026-10-05/L003 | Opening CLIENTS in a second area returns error 3; QUIT restores FLAIR. The old hang does not recur. | exit-two-serial.log; shots/exit-two-before.png; shots/exit-two-after.png |
| 2026-10-05/L007 | SAFETY ON asks before ZAP. N preserves one record; Y empties it. | regress-serial.log; shots/regress-zap-no.png |
| 2026-10-04/K05, K06, K09 | Filters exclude WADDAMS as expected; deleted rows have a leading asterisk; USE CLIENTS defaults to .DBF. | regress-serial.log; shots/regress-list.png |
| 2026-10-04/K13 | SAMIR consumes USE/LIST/QUIT from redirected input and returns to a working DOS shell. | redirect-serial.log; shots/redirect-result.png; shots/redirect-shell-return.png |
| 2026-10-05/L047, L048, L049 | CLOSE INDEX/FORMAT preserve the table; ONE->NAME resolves; TYPE("BAL") returns N and expression/type controls work. | regress-serial.log; shots/regress-close.png; manualcheck-serial.log |
| 2026-10-05/L051, L052 | Ordinary INDEX ON and REINDEX work, including lookup, key replacement, insertion and multiple order selection. Empty/EOF/blank-date and multi-index PACK defects are separately reported. | regress-serial.log; data-serial.log; pack-index-serial.log; extracted NDX files |
| 2026-10-05/L053 | APPEND FROM a separate clone doubles the three records to six and survives reopening. Self-import is refused as already open. | io-serial.log; shots/io-append.png |
| 2026-10-05/L054, L055, L116 | SAVE writes a valid MEM file, RESTORE replaces a deliberately changed value, and CLEAR MEMORY makes TYPE return U and a subsequent variable reference fail. | memory-serial.log; shots/memory-restore.png; shots/memory-cleared.png; regress-files/MEMSAVE.MEM |
| 2026-10-05/L056, L057 | Valid report and label forms execute and write files. Scopes, grouped totals, SUMMARY, PLAIN, HEADING, NOEJECT and multi-page report output were exercised. Designers and LABEL SAMPLE remain new gaps. No working printer was attached. | reports-serial.log; reports-files/; report-bytes.json; paging-files/; paging-bytes.json |
| 2026-10-05/L058, L064, L065 | ALTERNATE writes the expected transcript; CONSOLE OFF suppresses program output while dot-prompt output remains visible; HEADING OFF/ON controls the listing header. | regress-files/OUTPUT.TXT; programedges-serial.log; shots/programedges-console.png; last-controls-serial.log; shots/last-controls-headings.png |
| 2026-10-05/L071 | RECSIZE returns 31 for CLIENTS and 0 with no table. | missing-serial.log; manualfn-serial.log |
| 2026-10-05/L108, L109, L110, L111, L112, L113 | Command macros, SORT, TYPE, RENAME, ERASE and TOTAL entry points execute. SORT output order and extracted DBF geometry were checked. TOTAL was a basic named-group fixture, not an exhaustive aggregation oracle. | regress-serial.log; regress-bytes.json; io-serial.log |
| 2026-10-05/L114 | RUN, ! and RUN inside a PRG execute DOS commands and return on the DOS launch route. A forced missing COMSPEC returns 92 cleanly. Desktop launch returned 92 even with an audit-supplied matching transient processor file; that route is not claimed working. | programmore-real-serial.log; shots/programmore-real-run.png; run-missing-serial.log; run-present-serial.log |
| 2026-10-15/M021, M027, M028 | LIST now has headings, fixed stored-field decimals and right-aligned numbers. | shots/002-list.png; shots/regress-list.png |
| 2026-10-15/M022, partial | SET STATUS is accepted and draws a bar through DOS. The former missing command is fixed; desktop launch has new L004. | shots/dos-status-type.png; desktop-status-serial.log |

## Coverage and limitations

| Area | Exercised | Not reached or not established |
| --- | --- | --- |
| Prompt and settings | Two launch routes; ordinary/repeated input, Up, Left, Home, End, Delete, Backspace, Escape, 254/255-byte lines; status on/off; TALK, ECHO, CONSOLE, EXACT, DELETED, DECIMALS, all eight date formats including manual French examples, CENTURY, SAFETY, BELL, CONFIRM, DEFAULT, PATH, INDEX, RELATION, FILTER, ORDER, UNIQUE, FIXED, MARGIN, PRINT, DEVICE, bare SET; inspection/help/history commands. | Full history/editor navigation could not be tested because the interfaces are absent. No claim that all 151 catalog errors were triggered. |
| Fields and mutation | Independent C(254), N(19,17), narrow N, D, L, M fixtures; truncation, type mismatch, numeric overflow, invalid/blank dates, memo replacement/read/reopen; APPEND BLANK/FROM, DELETE/RECALL/PACK/ZAP, persistence. | CREATE and MODIFY STRUCTURE do not open, so in-editor field creation, rename, widen/narrow, insertion/deletion, cancellation and Ctrl-PgDn memo editing were unreachable. High-precision values were observed but not compared to a live III+ numerical oracle. |
| Navigation and queries | GO TOP/BOTTOM/0/beyond EOF, SKIP at BOF/EOF, empty and all-filtered tables, LOCATE/CONTINUE, indexed/unindexed SEEK/FIND, LIST/DISPLAY fields, FOR, WHILE, NEXT, REST, OFF, headings, deleted marks, aliases and relations. | BROWSE/EDIT are unavailable; aggregate and broad export workflows covered by old K08 cannot run. JOIN/UPDATE are new rejected entry points, not completed data operations. |
| Indexes | Character, numeric, date and expression keys; duplicate keys; 100/101/254-character keys; empty file, EOF start, corrupt NDX; multiple indexes and SET ORDER; insertion, replacement and deletion; one/two-index PACK and REINDEX; stopped bytes. | UNIQUE cannot be enabled. Not an exhaustive large B-tree split/rebalance or many-thousand-record soak. |
| Expressions | Arithmetic and precedence; negative ROUND/MOD; STR/VAL; comparison direction and EXACT; substring operator; type coercion; nested functions, IIF short-circuiting; date formats, 1900/2000 cases; string-length boundaries; individual missing-function probes. | No invented failures for canonical year 00 = 1900, -2^2 = 4, asymmetric prefix comparisons or the absence of later-only ==. |
| Programs | File DO, IF/ELSE, CASE, WHILE/LOOP/EXIT, internal procedures, parameters, PRIVATE/PUBLIC behavior, macros, ON ERROR/ERROR/MESSAGE, RETRY, nested file DO, RETURN TO MASTER, malformed terminators, 1000-line and oversize files, recursion, loop interruption, ACCEPT/INPUT/WAIT, TEXT, @ forms, RUN/!, QUIT inside PRG. | @ form entry is absent, so READ validation and its keys were unreachable. Key/escape traps are inert; a running loop could not be interrupted. No external network/locking environment. |
| Reports/labels | Valid FRM/LBL fixtures; plain/default output, NEXT/FOR/WHILE, SUMMARY, HEADING, NOEJECT, TO FILE; 52-record paging; numeric totals and output Ctrl-Z; TO PRINT error path; SAMPLE. | Physical printing and paper alignment were not tested: REPORT TO PRINT explicitly returns 126, printer unavailable. Designers do not open. |
| Disk and lifecycle | Full floppy during append, index, sort and copy; PACK at full capacity; retry after freeing space; extracted headers/records/indexes/memos/reports; same-table two-area exit, normal QUIT, re-entry, redirected stdin. | No 2 GiB test, damaged physical media, power-failure injection or exhaustive disk-full memo rewrite. Ctrl+Alt+backslash inside SAMIR did not switch to the desktop; it remained a typed-input event, and the current record/variable survived its cancellation. QUIT/relaunch starts a new database session. |
| Look | Native guest text, headings, right alignment, decimals, deletion marker, status bar, report page output and control-character artifacts; compared with named III+ material. | Editors/designers have no screen to compare. No pixel-geometry claim is made against a scan or a differently sized historical capture. |

### Printed examples and false-positive controls

All 66 selected Volume One function example files were typed through QEMU. Sixty-one matched their printed transcripts after newline/trailing-space normalization. Three current-clock examples differ from the book's fixed 1992 clock and expose the already-known 1926 year bug; they are not three new bugs. Two initial differences were harness state contamination (an open STAFF table and an installed ON ERROR trap). Both matched when rerun with that state reset. The printed STAFF/HIRED index example also matched. See manualfn-expectations.json, manual-comparison.json and manualcheck-serial.log. This is not a claim to have replayed every example in both volumes.

COPY FILE padding to 512-byte blocks was investigated by byte extraction and deliberately not filed: the shipped Volume One u5-entries-a.tex:239-242 documents zero-filling the final block, and the local real-III+ differential fixture agrees. Likewise, retained bytes past a DBF logical EOF after PACK/ZAP are permitted by the supplied DBF reference; no unsupported truncation defect is claimed. ERROR()/MESSAGE() report the last ON ERROR-handled error, not every untrapped dot-prompt error, which matches the reference. Valid date indexes, string arguments to internal procedures, report totals and normal one-index PACK were retained as passing controls.

### Harness corrections

The first programs and programmore runs stopped at DOS startup date input; their commands never reached SAMIR. The corrected programs-real and programmore-real runs accepted the date/time defaults first. The initial full desktop attempt encountered a disk-full Finder alert before the hotkey could launch SAMIR; full-real and full-recovery use DOS. These three sessions are diagnostic artifacts only. Corrected screenshot aliases and hashes are in corrected-shot-aliases.json. The exit-two after screenshot includes a subsequent Ctrl-C recovery probe that painted Program terminated over the restored desktop. This happened after SAMIR had exited; the same desktop/console interaction is separately reported in /home/tobias/Projects/initech-os/docs/audits/2026-10-23-das/REPORT.md, L002, and is not duplicated as a SAMIR finding. Early captures taken before a long line settled are not used as proof of that line's result. Later runner versions wait for serial quiescence. Baseline reference dates are not mistaken for the present clock.

## Fifteen fixes to make this a dependable period workstation

1. Stop recursive procedures safely before they damage the guest stack. (L003)
2. Make Escape/Ctrl-C reliably break running programs. (L002)
3. Keep blank and invalid dates as valid blank DBF bytes. (L001)
4. Refresh memo reads immediately after successful writes. (L005)
5. Keep every open index usable after PACK. (L014)
6. Honor the advertised command length on both input paths. (L019)
7. Wire the desktop text tenant to the terminal controls used by status mode. (L004, L048)
8. Implement CREATE, BROWSE, EDIT, interactive APPEND and MODIFY STRUCTURE. (2026-10-04/K08; L012)
9. Show numeric overflow instead of a believable zero, and fix decimal pictures. (L006, L007)
10. Make indexing independent of empty/EOF state and accept blank dates. (L008, L015, L016)
11. Provide working @ SAY/GET/READ forms and program editing. (L033, L026)
12. Finish program calling and recovery semantics, including QUIT and RETURN TO MASTER. (L021, L022, L041, L013)
13. Add the report/label designers so users can create the forms the renderer consumes. (L024, L025)
14. Make relations, unique indexes, aggregates and structure exports productive. (L045, L032, L027; 2026-10-04/K08)
15. Restore familiar four-letter verbs, history, inspection and command feedback. (L009, L040, L010, L011, L046)

## Cleanup

All QEMU processes started by these scripts were stopped by their own driver handles. evidence/cleanup.jsonl and the per-session JSON files record exit codes and image hashes. No other emulator process was signalled. The final structural check is evidence/final-validation.json. Read-only fsck.fat checks found no allocation errors in the two stopped full-disk test images (full-real-fsck.txt and full-recovery-fsck.txt). REPORT.md and NITS.tsv are ASCII-only; NITS.tsv contains one four-column row per new finding and no header.
