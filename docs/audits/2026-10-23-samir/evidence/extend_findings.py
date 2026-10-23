from pathlib import Path
import json
E=Path(__file__).resolve().parent;F=json.loads((E/'findings-base.json').read_text())
def add(key,area,sev,title,steps,saw,expected,files,guess=''):
 F.append(dict(key=key,area=area,severity=sev,title=title,steps=steps,observed=saw,expected=expected,evidence=files,source_guess=guess))
add('recursion','Procedure stack','P1','Recursive internal procedures destroy the running session',
 'Fresh program-fixture.img, desktop route: DO RECINT. Wait 3 seconds, press Esc and Ctrl-C, wait 2 seconds, type ? "RECOVERED", then wait 2 seconds. RECINT.PRG calls procedure REC, whose body calls REC again.',
 'No error, return prompt or recovery output appears. This reproduced independently after a longer session and from a fresh boot. QMP still responds; the isolated capture has EIP=dbd6bba7 and ESP=00054871, outside a sensible executable/stack layout. No panic text was printed. This is an observed crash/hang; stack corruption is a diagnosis, not a proven mechanism.',
 'Bound recursive calls and report a nesting/space error without losing the session. Shipped Volume One u5-entries-a.tex:336-341 says procedures may call themselves "to a depth of 64". Safe handling beyond the supported depth is also my reliability requirement.',
 ['shots/recursion-alone-recursive.png','shots/recursion-alone-recovery.png','evidence/recursion-alone-serial.log','evidence/recursion-alone-qmp-state.json','evidence/programedges-qmp-state.json','evidence/fixtures/RECINT.PRG'],
 'Guess: os/samir/cmd/proc.c:592-607 allocates an 8192-byte automatic body per call; recursive stack growth may overwrite guest state before the intended nesting limit.')
add('pack-two','Indexes and PACK','P2','PACK with two open indexes leaves live index state unusable',
 'Fresh rich-fixture.img: USE CLIENTS; INDEX ON NAME TO BYNAME; INDEX ON BAL TO BYBAL; SET INDEX TO BYNAME,BYBAL; SET ORDER TO 1; PACK; REINDEX; GO TOP; LIST. Then CLOSE INDEX; GO TOP; LIST. Finally USE; USE CLIENTS INDEX BYNAME,BYBAL; GO TOP; LIST.',
 'PACK reports success, but REINDEX returns 114 Index damaged, GO TOP returns 5 Record is out of range, and LIST returns 10 Syntax error. Closing indexes restores physical traversal; reopening both restores sorted traversal. The one-index control passes. This reproduced without any deleted records, so record compaction is not required. The stopped DBF and NDX bytes remain readable; persistent index corruption is not claimed.',
 'PACK must preserve usable open indexes, rebuilding them as needed. Named reference: dbase3-decomp/specs/commands/data-definition-and-manipulation.md section 10 (PACK).',
 ['shots/pack-two-after.png','evidence/pack-two-serial.log','evidence/pack-two-files/BYNAME.NDX','evidence/pack-two-files/BYBAL.NDX','evidence/pack-two-files/CLIENTS.DBF','evidence/pack-index-serial.log'],
 'Guess: inspect the multi-index lifetime across os/samir/cmd/mutate.c:945-959 and fs/ndx.c:2655 (ndx_rebuild). A single-index run does not reproduce it.')
add('eof-index','Indexes','P2','INDEX ON fails when the current pointer is at EOF',
 'Fresh rich-fixture.img: USE CLIENTS; GO BOTTOM; SKIP; ? EOF(),RECNO(); INDEX ON NAME TO EOFIDX. Then GO TOP; INDEX ON NAME TO TOPIDX.',
 'EOF is true at record 4. The first INDEX returns 29 File is not accessible and creates no file; the same expression succeeds after GO TOP.',
 'INDEX ON scans the table independently of the incoming current-record position. Named reference: dbase3-decomp/specs/commands/data-definition-and-manipulation.md, INDEX; my equivalent-input expectation is that moving to EOF does not make the output file inaccessible.',
 ['shots/index-boundary-at-eof.png','evidence/index-boundary-serial.log','evidence/index-boundary-files/TOPIDX.NDX'],
 'Guess: os/samir/cmd/mutate.c:1124-1130 derives key width/type from the current record before scanning. At EOF a character field evaluates as a zero-length value.')
add('blank-date-index','Indexes','P2','A valid blank date prevents creation of a date index',
 'Fresh rich-fixture.img: USE EDGE; INDEX ON WHEN TO BLANKDAT. EDGE record 3 has eight blank bytes in WHEN. Control on another fresh copy: REPLACE ALL WHEN WITH CTOD("01/01/92"); GO TOP; INDEX ON WHEN TO VALIDATE.',
 'The original table returns 9 Data type mismatch. Filling all dates with valid dates and moving to GO TOP succeeds. The shipped STAFF/HIRED date-index example also succeeds. This is specifically the valid blank date case, not missing date indexing generally.',
 'Blank dates are legal database values and must be indexable alongside ordinary dates. Named references: dbase3-decomp/specs/file-formats/dbf.md section 5 (blank D fields) and ndx.md section 4.2 (date keys).',
 ['shots/date-index-blank.png','shots/verify-final-date-control.png','shots/verify-final-manual-date.png','evidence/date-index-serial.log','evidence/verify-final-serial.log','evidence/fixtures/EDGE.DBF'],
 'Guess: os/samir/cmd/mutate.c:244-249 accepts XB_N and XB_D, but not the XB_U representation returned for a blank DBF date.')
add('index-length','Index bounds','P2','INDEX accepts 101-character keys beyond the III+ maximum',
 'Fresh rich-fixture.img: USE EDGE; GO TOP; INDEX ON LEFT(WIDE,100) TO KEY100; GO TOP; INDEX ON LEFT(WIDE,101) TO KEY101. Extract both NDX files after stopping.',
 'Both succeed. KEY101.NDX records key_length=101 in the little-endian word at offset 12. A direct 254-character key is rejected with 29 File is not accessible, rather than the specific key-size diagnostic.',
 'Reject keys longer than 100 with the III+ index-too-big diagnostic. Named reference: the III+ message catalog "Index is too big (100 char maximum)" and dbase3-decomp/specs/commands/data-definition-and-manipulation.md, INDEX limits. This is a compatibility/boundary defect; I did not feed the generated 101-byte index into a live III+ interpreter.',
 ['evidence/index-boundary-serial.log','evidence/index-boundary-files/KEY100.NDX','evidence/index-boundary-files/KEY101.NDX','shots/index-fresh-expressions.png'])
add('corrupt-index','Index diagnostics','P2','A corrupt existing NDX is reported as nonexistent',
 'Fresh rich-fixture.img: USE CLIENTS; SET INDEX TO BAD.NDX. BAD.NDX is the 559-byte fixture of repeated "not an index" text.',
 'The command says 1 File does not exist, although the file is present and extractable from the same image.',
 'Distinguish a malformed index from a missing pathname, so the user can take the right repair action. Named reference: dbase3-decomp/specs/file-formats/ndx.md, header validation invariants; the accurate-diagnostic requirement is my usability judgment.',
 ['shots/errors-files.png','evidence/errors-serial.log','evidence/fixtures/BAD.NDX','evidence/index-fresh-files/BAD.NDX'])
add('line-limit','Command input','P2','The ordinary dot prompt truncates commands at 128 bytes',
 'Use desktop route with default SET STATUS OFF. Type the exact 254-character command in keys.json: ? LEN(" followed by 245 x characters, then "). Press Enter. Repeat the 255-character variant with 246 y characters.',
 'Only the first 128 bytes are accepted; the remaining 126/127 keystrokes ring the bell. The closing quote/parenthesis is lost and execution reports 35 Unterminated string. This is not a legitimate rejection of a 255-byte command: the documented 254-byte case fails too.',
 'Accept the promised 254-character line. Shipped Volume One ua-keys.tex:20-22 says printable characters are taken "up to 254 characters"; u2-commands.tex:16 repeats the limit. Reject an overlong line coherently rather than executing its truncated prefix.',
 ['shots/keys-254.png','shots/keys-255.png','evidence/keys.json','evidence/keys-serial.log','evidence/key-lengths.json'],
 'Guess: os/samir/pal/pal_milton.c:650-654 uses the cooked DOS stream when status is off; the observed 128-byte cap occurs below SAMIR line framing.')
add('program-lines','Program capacity','P2','A 7018-byte, 1000-line program fails with insufficient memory',
 'Fresh program-fixture.img: DO SHORT1K. The fixture sets N=0, repeats N=N+1 1000 times, prints N and returns; its exact CRLF file size is 7018 bytes.',
 '43 Insufficient memory is returned without running the program. An ordinary compact program with IF, CASE, LOOP, EXIT and procedures runs successfully in the control session.',
 'This small program should fit the advertised file limit and a 16 MiB workstation. Shipped Volume One u5-entries-a.tex:331-333 states a maximum of 8191 characters. The 1000-line case is explicitly requested by this audit brief. This is a capacity limitation, not a crash.',
 ['shots/programedges-1000.png','evidence/programedges-serial.log','evidence/fixtures/SHORT1K.PRG','evidence/programs-real-serial.log'],
 'Guess: os/samir/cmd/flow.c:54 fixes FLOW_MAX_LINES at 512, an additional limit not stated with the documented file-size limit.')
add('nested-file','Program calls','P2','DO inside a PRG cannot call another existing PRG file',
 'Fresh control-fixture.img: DO OUTER. OUTER.PRG contains DO USEPRG, then prints CHILD RETURNED. USEPRG.PRG exists and works when invoked directly at the dot prompt.',
 'The nested call returns 16 *** Unrecognized command verb. CHILD RETURNED is never printed. Internal named procedures work in FLOW.PRG.',
 'Resolve a DO target to either a procedure or another program file. Named reference: dbase3-decomp/specs/commands/programming-and-io.md:71-85, resolution order. The shipped manual describes only internal procedure calls from a program, so this is a historical capability gap.',
 ['shots/control-nested.png','evidence/control-serial.log','evidence/programs-real-serial.log','evidence/fixtures/OUTER.PRG','evidence/fixtures/USEPRG.PRG'])
add('retry','Error recovery','P2','RETRY in an error handler does not retry the failed statement',
 'Fresh control-fixture.img: DO RETRY. Main code prints XX before it exists. Its ON ERROR procedure assigns XX=7, increments TRYCNT, and executes RETRY. Main code then prints DONE,TRYCNT.',
 'Only DONE 1 appears; the repaired ? XX is never retried, so 7 is missing. The program returns normally. A bare dot-prompt RETRY is also unrecognized, but the finding rests on the valid handler context.',
 'RETRY must execute the failing command again after the handler fixes its cause. Named reference: dbase3-decomp/specs/commands/programming-and-io.md:232-243 quotes the III+ HELP behavior and the ERRORPRG.PRG usage.',
 ['shots/control-retry.png','evidence/control-serial.log','evidence/fixtures/RETRY.PRG'])
add('empty-bof','Record boundaries','P2','BOF stays false for a physically empty database',
 'USE EMPTY; ? RECCOUNT(),EOF(),BOF(); GO TOP; ? EOF(),BOF().',
 'The count is 0 and EOF is true, but BOF is false both immediately after USE and after GO TOP.',
 'For an empty file, both boundary predicates should be true. Named reference: dbase3-decomp/specs/commands/navigation-query-display.md:999. That local specification includes cross-implementation corroboration; I did not mint this case in a live III+ session.',
 ['shots/verify-final-empty.png','evidence/verify-final-serial.log','evidence/fixtures/EMPTY.DBF'])
# Missing entry points are kept separate from the already-known K08 core editor/aggregate/export set.
for key,area,title,steps,saw,ref,shot,log in [
 ('report-designer','Report design','CREATE/MODIFY REPORT cannot open a designer','USE CLIENTS; CREATE REPORT NEWREP; MODIFY REPORT STAFFRPT.','Both return 16 *** Unrecognized command verb. No report designer opens.','assist-ui-and-editors.md section 8.1','ui-gaps-design','ui-gaps'),
 ('label-designer','Label design','CREATE LABEL cannot open a designer','USE CLIENTS; CREATE LABEL NEWLAB.','16 *** Unrecognized command verb. No label designer opens.','assist-ui-and-editors.md section 8.2','ui-gaps-design','ui-gaps'),
 ('program-editor','Program editing','MODIFY COMMAND cannot edit a PRG','MODIFY COMMAND FLOW, with FLOW.PRG on the disk.','16 *** Unrecognized command verb. No text editor opens.','assist-ui-and-editors.md section 9','ui-gaps-design','ui-gaps'),
 ('structure-copy','Structure export','COPY STRUCTURE is unavailable','USE CLIENTS; COPY STRUCTURE TO STRUCT.','16 *** Unrecognized command verb; no structure-only DBF is created.','data-definition-and-manipulation.md, COPY STRUCTURE','ui-gaps-data','ui-gaps'),
 ('extended-copy','Structure export','COPY STRUCTURE EXTENDED is unavailable','USE CLIENTS; COPY STRUCTURE EXTENDED TO EXTSTR.','16 *** Unrecognized command verb; no field-definition table is created.','data-definition-and-manipulation.md, COPY STRUCTURE EXTENDED','ui-gaps-data','ui-gaps'),
 ('join','Multi-table data','JOIN is not recognized','USE CLIENTS; JOIN WITH CLIENTS TO JOINED FOR .T.','16 *** Unrecognized command verb, rather than entering the JOIN command path or diagnosing the operands. No join is performed.','data-definition-and-manipulation.md, JOIN','ui-gaps-data','ui-gaps'),
 ('update','Multi-table data','UPDATE is not recognized','USE CLIENTS; UPDATE ON NAME FROM CLIENTS REPLACE BAL WITH BAL.','16 *** Unrecognized command verb, rather than entering the UPDATE command path or diagnosing the source work area.','data-definition-and-manipulation.md, UPDATE','ui-gaps-data','ui-gaps'),
 ('list-print','Printed listings','LIST rejects TO PRINT','USE CLIENTS; LIST NAME TO PRINT.','36 Unrecognized phrase/keyword in command. It never reaches the printer availability check. REPORT FORM TO PRINT reaches error 126 instead.','navigation-query-display.md, LIST syntax','ui-gaps-data','ui-gaps'),
 ('unique','Index uniqueness','Neither INDEX UNIQUE nor SET UNIQUE is supported','USE CLIENTS; INDEX ON NAME TO UNIQUE1 UNIQUE; SET UNIQUE ON; SET UNIQUE OFF.','Each returns 10 Syntax error. The ordinary INDEX ON NAME TO UNIQUE2 control succeeds.','set-commands.md:165 and data-definition-and-manipulation.md, INDEX UNIQUE','ui-gaps-unique','ui-gaps'),
 ('at-form','Forms','@ SAY and the form entry point are unavailable','DO FORM; then @ 2,3 SAY "HELLO". FORM.PRG contains @ 2,3 SAY "Name:" GET NAMEVAR followed by READ.','16 *** Unrecognized command verb at the @ command. No form is drawn; READ and its editing keys cannot be reached through that program.','assist-ui-and-editors.md and say-get-read.md','programmore-real-form','programmore-real'),
 ('text-block','Program output','TEXT/ENDTEXT literal blocks are unavailable','DO TEXT. TEXT.PRG contains two literal text lines between TEXT and ENDTEXT.','16 *** Unrecognized command verb; neither text line appears.','programming-and-io.md:494-517','programmore-real-form','programmore-real'),
 ('echo','Program tracing','SET ECHO is unavailable','SET ECHO ON; DO USEPRG; SET ECHO OFF.','Both SET ECHO forms return 10 Syntax error; the program runs without an execution trace.','set-commands.md:150','settings-trace','settings'),
 ('fixed','Numeric display','SET FIXED is unavailable','SET FIXED ON; SET DECIMALS TO 2; ? 3,3.5; SET FIXED OFF.','Both SET FIXED forms return 10 Syntax error. The values print as 3 and 3.5.','set-commands.md:153 and section SET FIXED','settings-format','settings'),
 ('margin','Printing layout','SET MARGIN is unavailable','SET MARGIN TO 5.','10 Syntax error.','set-commands.md:225-233','settings-format','settings'),
 ('print-toggle','Printing','SET PRINT is unavailable','SET PRINT ON; SET PRINT OFF.','Both return 10 Syntax error, independently of printer availability.','set-commands.md:158','settings-format','settings'),
 ('device','Output routing','SET DEVICE is unavailable','SET DEVICE TO PRINT; SET DEVICE TO SCREEN.','Both return 10 Syntax error.','set-commands.md:359-370','settings-format','settings'),
 ('history','Command history','Command-history controls are unavailable','? "FIRST"; Up; Enter; LIST HISTORY; SET HISTORY ON.','Up does not recall the previous command, LIST HISTORY returns 3 File is already open in the no-table state, and SET HISTORY ON returns 10 Syntax error.','navigation-query-display.md, LIST/DISPLAY HISTORY; set-commands.md, HISTORY','keys-history','keys')
]:
 add(key,area,'P2',title,steps,saw,'The named III+ command should be available. Named reference: dbase3-decomp/specs/'+ref+'. This finding concerns the reached entry point; deeper workflows are not claimed to have run.', ['shots/'+shot+'.png','evidence/'+log+'-serial.log'])
# Correct prefixes for reference paths are explained in REPORT's reference key.
(E/'findings-expanded.json').write_text(json.dumps(F,indent=2));print(len(F),'findings so far')
