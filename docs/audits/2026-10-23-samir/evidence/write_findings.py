from pathlib import Path
import json
E=Path(__file__).resolve().parent
F=[]
def add(key,area,sev,title,steps,saw,expected,files,guess=''):
 F.append(dict(key=key,area=area,severity=sev,title=title,steps=steps,observed=saw,expected=expected,evidence=files,source_guess=guess))
add('date-write','DBF dates','P1','Replacing a date with an invalid date writes non-date bytes',
 'Fresh rich-fixture.img, desktop route. USE EDGE; REPLACE WHEN WITH CTOD("02/30/92"); DISPLAY ID,WHEN; USE; USE EDGE. Stop the guest and extract EDGE.DBF with mcopy.',
 'Record 1 WHEN becomes the eight ASCII bytes ,)/-1124 (hex 2c292f2d31313234), rather than eight blanks. DISPLAY shows 11/24/00. The invalid bytes survive closing and reopening. This is a persistence defect beyond earlier audit L045, which described display of empty/invalid dates.',
 'A blank/invalid CTOD result must leave a valid blank date representation. Named reference: dbase3-decomp/specs/file-formats/dbf.md, section 5: D fields contain YYYYMMDD or blanks. This corrupts the field representation even though the table remains openable.',
 ['shots/data-wide.png','evidence/data-serial.log','evidence/data-files/EDGE.DBF','evidence/data-bytes.json'],
 'Guess: os/samir/fs/dbf.c:1654 converts every XB_D through ymd_from_jdn, including the zero/blank sentinel; only XB_U takes the blank branch at line 1675.')
add('escape-loop','Program interruption','P1','Escape and Ctrl-C cannot stop a running loop',
 'Fresh rich-fixture.img, desktop route. DO LOOP. Wait 2 seconds; press Esc, wait 2 seconds; Ctrl-C, wait 2 seconds; type ? "RECOVERED" and Enter; wait 2 seconds. LOOP.PRG installs ON ESCAPE and runs DO WHILE .T. incrementing N.',
 'The screen remains at . DO LOOP; no trap output, prompt, or recovery command appears. QMP remains responsive and reports running, HLT=0. Only stopping this audit QEMU ended the session. This is failure to interrupt a program, not a claim that the infinite loop should terminate by itself.',
 'A user must be able to interrupt a mistaken loop without destroying the session. Named reference: dbase3-decomp/specs/commands/programming-and-io.md, ON ESCAPE/SET ESCAPE discussion. The shipped quick reference openly calls the trap inert; that documents the limitation but does not make an uncancellable loop dependable.',
 ['shots/escape-loop-running.png','shots/escape-loop-escape.png','evidence/escape-loop-serial.log','evidence/escape-loop-qmp-state.json','evidence/escape-loop.json','evidence/fixtures/LOOP.PRG'])
add('desktop-status','Status display','P2','Desktop-launched status mode prints ANSI control sequences',
 'Boot flair_tenants_interactive.img with rich-fixture.img, press Ctrl+Alt+backslash, USE CLIENTS, SET STATUS ON, then ? 2+2. Allow output to settle. SET STATUS OFF restores ordinary command output.',
 'Escape sequences such as [23;1H and [0;7m fill and scroll the screen instead of positioning a status bar. The same setting works through tracer_boot.img and SAMIR.COM, so this is specific to the desktop route, not the old unrecognized SET STATUS command.',
 'SET STATUS ON should draw the status bar. Shipped quick reference qr-set.tex documents "The status bar" for STATUS; the real III+ status layout is described in dbase3-decomp/specs/full-screen/assist-ui-and-editors.md section 10. The DOS-route screenshot is the control.',
 ['shots/008-status-recovery.png','shots/dos-status-type.png','evidence/initial-serial.log','evidence/dos-status-serial.log'],
 'Guess: os/samir/pal/pal_milton.c:566 and :607 bind the status display to ANSI sequences; the DOS boot logs ANSI-WIRED/ANSI.SYS, whereas the tested desktop boot does not.')
add('memo-cache','Memo fields','P2','A successful memo replacement reads as empty until USE reopens the table',
 'Fresh rich-fixture.img: USE EDGE; ? MEMO; REPLACE MEMO WITH "changed memo"; ? MEMO,LEN(MEMO); GO 2; GO 1; ? MEMO; USE; USE EDGE; ? MEMO,LEN(MEMO). Repeat with "again".',
 'Immediately after each successful REPLACE, the memo is empty and LEN is 0, even after moving away and back. Closing/reopening returns "changed memo" with length 12, then "again" on the second cycle. DBF pointers and DBT appended text are persisted correctly.',
 'Reads in the same work area should immediately reflect a successful write. Named reference: dbase3-decomp/specs/commands/data-definition-and-manipulation.md, REPLACE; dbt.md section 4 describes the memo pointer. The read-after-write requirement is also my data-consistency expectation.',
 ['shots/memo-write.png','evidence/memo-serial.log','evidence/memo-files/EDGE.DBF','evidence/memo-files/EDGE.DBT','evidence/memo.json'],
 'Guess: os/samir/cmd/workarea.c:63 retains the open memo handle, while os/samir/fs/dbf.c:1763 opens a separate writable handle and appends through it; the existing reader is not refreshed.')
add('numeric-overflow','Numeric fields','P2','An asterisk-filled numeric overflow is presented as zero',
 'Fresh rich-fixture.img: USE EDGE; REPLACE SMALL WITH 99999; DISPLAY ID,SMALL. Close/reopen EDGE and display again; extract the stopped DBF.',
 'SMALL N(4,1) is stored as **** at record offset 290, but DISPLAY prints 0.0. The same false zero appears after reopening.',
 'Preserve an unmistakable overflow indication in listings instead of presenting an ordinary zero. The storage rule is in shipped Volume One u5-entries-b.tex:577: "A number too large for its field is stored as asterisks." The expectation that the listing must not conceal that failure is my usability/data-integrity judgment; I did not run a live III+ overflow listing.',
 ['shots/data-wide.png','evidence/data-serial.log','evidence/data-files/EDGE.DBF','evidence/data-bytes.json'],
 'Guess: os/samir/fs/dbf.c:847 parses all-asterisk N fields with dec_parse and explicitly falls back to 0.0.')
add('transform','Numeric picture formatting','P2','TRANSFORM reports overflow for a decimal value that fits its picture',
 'Fresh rich-fixture.img: ? TRANSFORM(123.4,"999.99"). Compare ? STR(123.4,6,2) and the adjacent decimal cases in transform.json.',
 'The six-character picture produces ***.** for 123.4, despite room for 123.40.',
 'The three integer and two fractional digit positions should produce 123.40. Named reference: dbase3-decomp/specs/full-screen/picture-function-templates.md section 3 and its 9,999.99 example; the arithmetic fit of this exact case is independently obvious.',
 ['shots/expressions-types.png','evidence/expressions-serial.log'],
 'Guess: os/samir/core/fn_builtins.c:1580 passes the count of digit slots to dec_format as the total width, omitting space for the decimal point.')
add('empty-index','Indexes','P2','INDEX ON cannot create an index for an empty table',
 'Fresh rich-fixture.img: USE EMPTY; INDEX ON NAME TO ZERO; SEEK "NONE".',
 'INDEX returns 29 File is not accessible. No ZERO.NDX is created. SEEK then returns 26 Database is not indexed. EMPTY.DBF is a valid zero-record table with NAME C(12).',
 'Create a valid empty index that can receive future appends. Named reference: dbase3-decomp/specs/file-formats/ndx.md, empty-root layout, and commands/data-definition-and-manipulation.md, INDEX. This is a common new-table workflow.',
 ['shots/index-fresh-empty.png','evidence/index-fresh-serial.log','evidence/fixtures/EMPTY.DBF','evidence/index-fresh-files/EMPTY.DBF'],
 'Guess: os/samir/cmd/mutate.c:1124 infers key width from the current evaluated value, which is empty when there is no record.')
add('abbreviations','Command language','P2','Four-letter command abbreviations are rejected',
 'USE CLIENTS. Enter DISP; REPL NAME WITH "TEST"; APPE BLANK; LOCA FOR BAL>1000; SELE 2; INDE ON NAME TO TEST; STOR 5 TO X.',
 'Every abbreviation returns 16 *** Unrecognized command verb. Full spellings work for the corresponding implemented commands.',
 'Named reference: dbase3-decomp/specs/runtime/interpreter-and-execution-model.md:188-224 documents first-four-letter recognition. The shipped Volume One u2-commands.tex:26 explicitly instructs full spellings, so this is a historical feature gap, not an undocumented manual contradiction.',
 ['shots/missing-abbrev.png','evidence/missing-serial.log','evidence/missing.json'])
add('display-status','Inspection','P2','DISPLAY STATUS is parsed as a field expression',
 'USE CLIENTS; DISPLAY STATUS.',
 '12 Variable not found is returned; no environment/work-area report is shown.',
 'Named reference: dbase3-decomp/specs/commands/navigation-query-display.md:713-727 specifies open files and system parameters. This is separate from the fixed command SET STATUS and the old DISPLAY STRUCTURE gap.',
 ['shots/missing-inspection.png','evidence/missing-serial.log'])
add('display-memory','Inspection','P2','DISPLAY MEMORY is parsed as a field expression',
 'USE CLIENTS; DISPLAY MEMORY.',
 '12 Variable not found is returned; no memory-variable listing is shown.',
 'Named reference: dbase3-decomp/specs/commands/navigation-query-display.md:731-743 specifies names, types, sizes, scopes and values of memory variables.',
 ['shots/missing-inspection.png','evidence/missing-serial.log'])
add('modify-structure','Structure editor','P2','MODIFY STRUCTURE is unavailable',
 'USE CLIENTS; MODIFY STRUCTURE.',
 '16 *** Unrecognized command verb. No field editor opens, so rename/widen/narrow/add/delete with existing data cannot be reached.',
 'Named reference: dbase3-decomp/specs/commands/data-definition-and-manipulation.md section 2 and full-screen/assist-ui-and-editors.md section 6. Earlier K08 covers CREATE and record editors, not this reached modification command.',
 ['shots/missing-inspection.png','evidence/missing-serial.log'])
add('quit-program','Program lifecycle','P2','QUIT inside a PRG is rejected rather than exiting',
 'Run DO QUITPRG using rich-fixture.img. The file prints BEFORE QUIT, executes QUIT, and would then print AFTER QUIT.',
 'BEFORE QUIT is followed by 16 *** Unrecognized command verb and a dot prompt. The database process remains alive; AFTER QUIT is not run because the program aborted.',
 'QUIT should close files and end the database session when executed by a program. Named reference: dbase3-decomp/specs/commands/programming-and-io.md, QUIT; shipped Quick Reference qr-commands.tex entry QUIT says "Closes all files and leaves InitechBase".',
 ['shots/programmore-quitprogram.png','evidence/programmore-real-serial.log','evidence/fixtures/QUITPRG.PRG'])
(E/'findings-base.json').write_text(json.dumps(F,indent=2))
print(len(F),'confirmed base findings')
