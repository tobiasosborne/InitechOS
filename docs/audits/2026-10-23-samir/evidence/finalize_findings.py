from pathlib import Path
import json
E=Path(__file__).resolve().parent;F=json.loads((E/'findings-expanded.json').read_text())
def add(key,area,sev,title,steps,saw,expected,files,guess=''):
 F.append(dict(key=key,area=area,severity=sev,title=title,steps=steps,observed=saw,expected=expected,evidence=files,source_guess=guess))
add('master-return','Program stack','P2','RETURN TO MASTER returns only one level',
 'Fresh master-fixture.img: DO MASTER2. The master calls FIRST; FIRST calls SECOND; SECOND executes RETURN TO MASTER.',
 'The output is MASTER, FIRST, SECOND, FIRST RESUMED, MASTER RESUMED. FIRST RESUMED should have been skipped.',
 'Return directly to the top-level routine, discarding intermediate frames. Named reference: dbase3-decomp/specs/commands/programming-and-io.md:207-225 quotes the III+ HELP definition. A one-level control is intentionally not used as proof, because ordinary RETURN and RETURN TO MASTER are equivalent there.',
 ['shots/master-deep-result.png','evidence/master-deep-serial.log','evidence/fixtures/MASTER2.PRG'])
add('label-sample','Labels','P2','LABEL FORM rejects SAMPLE',
 'Fresh rich-fixture.img: USE STAFF; LABEL FORM STAFFLBL SAMPLE. STAFFLBL.LBL is valid and LABEL FORM ... TO FILE succeeds in reports.json.',
 '36 Unrecognized phrase/keyword in command is returned.',
 'Offer the label-alignment sample mode. Named reference: dbase3-decomp/specs/full-screen/assist-ui-and-editors.md section 8.2 and its LABEL FORM ... SAMPLE syntax. This is separate from the previously missing LABEL FORM entry point, which now works.',
 ['shots/label-sample-result.png','evidence/label-sample-serial.log','evidence/reports-files/LABELS.TXT'])
add('failed-destinations','Disk-full recovery','P2','Failed output commands leave invalid destinations that block retry',
 'DOS route with full-fixture.img: USE CLIENTS; INDEX ON NAME TO FULLIDX; SORT TO SORTED ON NAME; USE; COPY FILE CLIENTS.DBF TO COPIED.DBF. ERASE FILLER.BIN; USE CLIENTS; repeat INDEX and SORT; USE SORTED.',
 'Each failed command leaves a zero-byte destination. After disk space is freed, INDEX and SORT return 7 File already exists, and USE SORTED returns 15 Not a dBASE database. The source CLIENTS remains intact. This is failed-output cleanup, distinct from the fixed old disk-full APPEND corruption.',
 'Either remove a new incomplete output on failure or provide a clear recovery path that does not treat the failed placeholder as completed user data. This expectation is my transactional-output/usability judgment; DBF/NDX references establish that these zero-byte files are invalid.',
 ['shots/full-recovery-failure.png','shots/full-recovery-retry.png','evidence/full-recovery-serial.log','evidence/full-recovery-files/FULLIDX.NDX','evidence/full-recovery-files/SORTED.DBF','evidence/full-recovery-files/COPIED.DBF','evidence/full-recovery-bytes.json'])
add('full-name','Disk-full diagnostics','P3','Disk-full output errors identify the source or no file at all',
 'DOS route with full-fixture.img: USE CLIENTS; SORT TO SORTED ON NAME. Then USE; COPY FILE CLIENTS.DBF TO COPIED.DBF.',
 'SORT says "Disk full when writing file: CLIENTS.DBF", although the attempted destination is SORTED.DBF. COPY reports the same message with nothing after the colon. This repeated in full-real and full-recovery.',
 'Name the output file whose write failed. This is my diagnostic-clarity expectation; the message itself promises a filename. The underlying disk-full condition is correctly detected in these cases.',
 ['shots/full-recovery-failure.png','evidence/full-recovery-serial.log','evidence/full-real-serial.log'])
add('no-table-error','Command diagnostics','P2','Mutation commands without an open table report syntax errors',
 'USE with no filename closes the table. Then DELETE; RECALL; PACK; REPLACE NAME WITH "x".',
 'Every command returns 10 Syntax error. These command forms are syntactically valid and work with a table open.',
 'Report No database is in USE, rather than asking the user to fix valid syntax. Named reference: the III+ error catalog and dbase3-decomp/specs/commands/data-definition-and-manipulation.md, each command requires an open database. This is a reached precondition error, not the older unsupported-editor diagnostic in K08.',
 ['evidence/settings-serial.log','shots/settings-no-open.png'])
# Make source references unambiguous, using the actual installed reference tree.
paths={
'assist-ui-and-editors.md':'full-screen/assist-ui-and-editors.md',
'say-get-read.md':'full-screen/say-get-read.md',
'set-commands.md':'commands/set-commands.md',
'programming-and-io.md':'commands/programming-and-io.md',
'navigation-query-display.md':'commands/navigation-query-display.md',
'data-definition-and-manipulation.md':'commands/data-definition-and-manipulation.md'}
for f in F:
 for name,path in paths.items():f['expected']=f['expected'].replace('dbase3-decomp/specs/'+name,'dbase3-decomp/specs/'+path)
 for k in ['expected','observed','steps']:
  f[k]=f[k].replace('qr-set.tex documents','qr-set.tex:16 documents').replace('qr-commands.tex entry QUIT','qr-commands.tex:37 entry QUIT')
 if f['key']=='desktop-status':f['evidence']=['shots/desktop-status-status.png','shots/desktop-status-typing.png','shots/dos-status-type.png','evidence/desktop-status-serial.log','evidence/dos-status-serial.log']
 if f['key']=='transform':f['evidence']+=['shots/verify-final-transform.png','evidence/verify-final-serial.log'];f['observed']+=' Controls: STR(123.4,6,2) gives 123.40, and TRANSFORM(12.34,"999.99") gives 12.34; even 0.12 in "9.99" incorrectly becomes *.**.'
 if f['key']=='quit-program':f['evidence'][0]='shots/programmore-real-quitprogram.png'
 if f['key']=='index-length':f['evidence'][-1]='shots/index-boundary-width.png' # serial+bytes carry the width proof; this screenshot may end at desktop, replaced by reviewed proof below.
 if f['key']=='line-limit':f['evidence']+=['shots/keys-dos-off.png','shots/keys-dos-on.png','shots/keys-dos-overlong.png','evidence/keys-dos-serial.log'];f['observed']+=' DOS-route controls reproduce the 128-byte cap with status off; with status on, the 254-byte command returns 245 and a 255-byte command is cleanly rejected with error 18.'
(E/'findings-final-draft.json').write_text(json.dumps(F,indent=2));print(len(F),'findings')
