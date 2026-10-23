#!/usr/bin/env python3
import sys, json, time, subprocess as sp, shutil
from pathlib import Path
R=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(R))
from audit_driver import Driver
E=R/'evidence'; I=R/'images'
cases=[]
def case(name,*lines):cases.append({'id':f'B{len(cases)+1:03d}','name':name,'lines':list(lines)})
case('dir-paths','DIR .','DIR ..\\T\\ONE.TXT','DIR A:ONE','DIR .TXT')
case('dir-errors','DIR NOEXIST\\*.TXT','DIR ONE.TXT\\','DIR /Z','DIR ONE.TXT TWO.TXT')
case('dir-wide','DIR /W','DIR SUB\\ /W')
case('dir-300','DIR \\MANY /W')
case('dir-boundaries','DIR \\D16 /W','DIR \\D32 /W','DIR \\D64 /W')
case('cd-paths','CD SUB\\','CD','CD ..','CHDIR A:\\T\\SUB','CD \\','CD','CD \\T','CD MISSING','CD ONE.TXT')
case('md-rd-errors','MD EMPTY','MD ONE.TXT','RD SUB','RD \\','MD A?B','MD CON','MD NUL','MD AUX','MD COM1','MD PRN')
case('md-rd-normal','MD NEWDIR','DIR NEWDIR','CD NEWDIR','RD .','CD ..','RD NEWDIR','DIR NEWDIR')
case('copy-self','COPY SELF.BIN SELF.BIN','COPY SELF.BIN .\\SELF.BIN','COPY SELF.BIN A:\\T\\SELF.BIN','COPY SELF.BIN ..\\T\\SELF.BIN','DIR SELF.BIN')
case('copy-protect','COPY ONE.TXT READONLY.TXT','DEL READONLY.TXT','TYPE READONLY.TXT','ATTRIB READONLY.TXT')
case('copy-directory','COPY ONE.TXT DEST','TYPE DEST\\KEEP.TXT','TYPE DEST\\ONE.TXT')
case('copy-empty','COPY ZERO.TXT ZCOPY.TXT','DIR ZCOPY.TXT','COPY ZERO.TXT ZCOPY2.TXT /A','DIR ZCOPY2.TXT')
case('copy-binary','COPY EOF.TXT EBIN.TXT /B /V','COPY EOF.TXT /A EASC.TXT','DIR E*.TXT','TYPE EBIN.TXT','TYPE EASC.TXT')
case('copy-plus','COPY ONE.TXT+TWO.TXT JOIN.TXT','COPY ONE.TXT /B+TWO.TXT /B JOINB.TXT /B','TYPE JOIN.TXT','TYPE JOINB.TXT')
case('copy-plus-implicit','COPY ONE.TXT IMP.TXT','COPY IMP.TXT+TWO.TXT','TYPE IMP.TXT')
case('copy-wild-output','COPY SUB\\*.TXT DEST\\*.BAK /B','DIR DEST /W')
case('copy-wild-combine','COPY SUB\\*.TXT COMBINED.OUT','TYPE COMBINED.OUT')
case('copy-target-source-later','COPY ONE.TXT+TWO.TXT TWO.TXT','TYPE TWO.TXT','COPY ONE.TXT+MISSING.TXT SAFE.TXT','DIR SAFE.TXT')
case('copy-timestamps','COPY OLD.TXT OCOPY.TXT','COPY FUTURE.TXT FCOPY.TXT','COPY OLD.TXT OASC.TXT /A','DIR O*.TXT','DIR F*.TXT')
case('copy-omitted','CD SUB','COPY ..\\ONE.TXT','TYPE ONE.TXT','CD ..')
case('copy-con-output','COPY ONE.TXT CON','COPY EOF.TXT CON /A')
case('del-paths','COPY ONE.TXT KEEP.TXT','COPY TWO.TXT SUB\\DELME.TMP','DEL SUB\\*.TMP','TYPE KEEP.TXT','DIR SUB /W')
case('del-directory','MD DELDIR','COPY ONE.TXT DELDIR\\ONE.TXT','DEL DELDIR','DIR DELDIR')
case('rename-mask','COPY ONE.TXT RENA.TXT','COPY TWO.TXT RENB.TXT','REN REN?.TXT *.BAK','DIR REN*.*')
case('rename-errors','REN ONE.TXT TWO.TXT','REN ONE.TXT SUB\\NEW.TXT','REN ONE.TXT BAD?NAME','DIR ONE.TXT')
case('rename-readonly','REN READONLY.TXT LOCKED.TXT','DIR LOCKED.TXT','REN LOCKED.TXT READONLY.TXT')
case('type-misc','TYPE ZERO.TXT','TYPE NOCR.TXT','TYPE NOLF.TXT','TYPE *.TXT','TYPE MISSING.TXT','TYPE ONE.TXT TWO.TXT')
case('redir-bytes','TYPE BINARY.BIN > BTYP.BIN','TYPE ZERO.TXT > ZTYP.TXT','TYPE NOLF.TXT > NTYP.TXT','TYPE LONG.TXT > LTYP.TXT','DIR ?TYP.*')
case('redir-append','ECHO FIRST > REDIR.TXT','ECHO SECOND >> REDIR.TXT','TYPE REDIR.TXT','BOGUS > BADOUT.TXT','TYPE BADOUT.TXT')
case('redir-readonly','ECHO LOST > READONLY.TXT','ECHO LOST >> READONLY.TXT','TYPE READONLY.TXT','TYPE ONE.TXT > NUL')
case('find-basic','FIND "Alpha" < LINES.TXT','FIND /N "a" < LINES.TXT','FIND /C "a" < LINES.TXT','FIND /V "a" < LINES.TXT')
case('find-files','FIND "Alpha" LINES.TXT','IF ERRORLEVEL 1 ECHO RCGE1','FIND "none" < LINES.TXT','IF ERRORLEVEL 1 ECHO NOMATCH-RCGE1','FIND unquoted < LINES.TXT','IF ERRORLEVEL 2 ECHO BADARGS-RCGE2')
case('sort-basic','SORT < LINES.TXT','SORT /R < LINES.TXT','SORT /+4 < LINES.TXT')
case('sort-limits','SORT < LONG.TXT > SORTLONG.TXT','SORT < BINARY.BIN > SORTBIN.BIN','DIR SORT*.*')
case('pipes','TYPE LINES.TXT | FIND /N "a" | SORT /R','TYPE ONE.TXT | MORE','TYPE ZERO.TXT | MORE','ECHO X | FIND "X" > PIPE.TXT','TYPE PIPE.TXT')
case('echo','ECHO','ECHO OFF','ECHO','ECHO ON','ECHO.','ECHO MiXeD $x  spaces','ECHO ONWARD')
case('date-values','DATE 02-29-2000','DATE 02-29-1900','DATE 02-29-2100','DATE 12-31-2099','DATE 1/1/80','DATE 31-12-99','DATE 0-0-0')
case('date-formats','DATE','DATE 1.2.99','DATE 1-2-99 EXTRA','DATE 01-01-1980','DATE 12-31-2079','DATE 01-01-1980')
case('time-values','TIME 23:59:59','TIME 24:00','TIME 12:60','TIME 13:14:15.99','TIME 1:02p','TIME 00:00','TIME')
case('env','SET foo=MiXeD','SET FOO','SET missing','SET FOO=','SET FOO','PATH','PATH ;','PATH','PATH A:\\DOS','PATH')
case('env-capacity',*[f'SET V{i}='+'x'*95 for i in range(6)],'SET',*[f'SET V{i}=' for i in range(6)])
case('builtin-gaps','VERIFY','VERIFY ON','VERIFY OFF','CTTY CON','IF EXIST ONE.TXT ECHO IF-PROMPT-ONLY-OTHER-TEST','BREAK INVALID')
case('attrib-flags','ATTRIB +R +A ONE.TXT','ATTRIB ONE.TXT','ATTRIB -R -A ONE.TXT','ATTRIB +H ONE.TXT','ATTRIB +S ONE.TXT','ATTRIB ONE.TXT')
case('attrib-subdirs','ATTRIB +R SUB\\*.TXT /S','ATTRIB SUB\\*.TXT /S','ATTRIB -R SUB\\*.TXT /S','ATTRIB EMPTY','ATTRIB NOFILE.TXT')
case('label','LABEL ABCDEFGHIJK','VOL','LABEL BAD.LABEL','VOL','LABEL ABCDEFGHIJKL','VOL','LABEL INITECH')
case('label-input','ECHO REPORTS > NAME.TXT','LABEL < NAME.TXT','VOL','LABEL INITECH')
case('xcopy-se','XCOPY SUB DEST\\XC /S /E /V','DIR DEST\\XC /W','XCOPY SUB DEST\\XC /E','XCOPY SUB SUB\\NEST /S','XCOPY ONE.TXT ONE.TXT')
case('xcopy-readonly','XCOPY READONLY.TXT DEST\\ /V','ATTRIB DEST\\READONLY.TXT','XCOPY TWO.TXT DEST\\READONLY.TXT','TYPE DEST\\READONLY.TXT')
case('xcopy-rootrelative','XCOPY ONE.TXT DEST\\REL.TXT','IF ERRORLEVEL 1 ECHO RCGE1','IF ERRORLEVEL 4 ECHO RCGE4')
case('xcopy-absolute','XCOPY A:\\T\\ONE.TXT A:\\T\\DEST\\ /V','DIR DEST\\ONE.TXT')
case('tree','TREE A:\\T\\SUB /F','TREE A:\\T\\NOPE','TREE A:\\T\\ONE.TXT','TREE /Z')
case('tree-deep','TREE A:\\DEEP /F')
case('chkdsk-file','CHKDSK A:\\T\\LONG.TXT','CHKDSK A:\\T\\NOPE','CHKDSK /Z')
case('cross-drive-cwd','CD C:\\CHOME','CD D:\\DHOME','CD','CD C:','CD D:','TYPE C:SAME.TXT','TYPE D:SAME.TXT','C:','CD','D:','CD','A:')
case('cross-drive-copy','COPY C:\\CHOME\\SAME.TXT CFILE.TXT','COPY D:\\DHOME\\SAME.TXT DFILE.TXT','COPY ONE.TXT D:\\DHOME\\AONE.TXT','TYPE CFILE.TXT','TYPE DFILE.TXT','TYPE D:\\DHOME\\AONE.TXT')
case('cross-drive-vol','VOL A:','VOL C:','VOL D:','DIR D: /W','DIR B:','CD B:\\','B:','Z:')
case('cross-drive-tools','CHKDSK D:','ATTRIB D:\\DHOME\\SAME.TXT','TREE D: /F','LABEL D: NEWLABEL')
case('missing-program','NOSUCH','A:\\NOSUCH','DOS\\NOSUCH','COMMAND /C ECHO CHILD','COMMAND /C DIR','EXIT','ECHO AFTER-EXIT')
case('known-absent','APPEND','ASSIGN','BACKUP','CHCP','COMP','EXE2BIN','FASTOPEN','FC','GRAPHICS','JOIN','KEYB','MODE','NLSFUNC','PRINT','RECOVER','REPLACE','RESTORE','SELECT','SHARE','SUBST','DEBUG')
case('batch-args','CALL \\CASES\\ARGS.BAT one two three four five six seven eight nine ten eleven')
case('batch-flow','CALL \\CASES\\FLOW.BAT hello','CALL \\CASES\\NEST.BAT','CALL \\CASES\\GSPACE.BAT')
case('batch-recursion','CALL \\CASES\\SELF.BAT','ECHO RECURSION-RETURNED')
case('batch-long','CALL \\CASES\\BIG.BAT','ECHO BIG-RETURNED')
case('batch-errorlevel','FIND "nonexistent" < ONE.TXT','IF ERRORLEVEL 1 ECHO FIND-FAILED','IF NOT ERRORLEVEL 2 ECHO FIND-LT2','FIND "ALPHA" < ONE.TXT','IF ERRORLEVEL 1 ECHO WRONG-FIND-SUCCESS','IF NOT ERRORLEVEL 1 ECHO FIND-SUCCESS')
case('batch-drive','COPY \\CASES\\ARGS.BAT D:\\ARGS.BAT','CALL D:\\ARGS.BAT first second','ECHO DRIVE-BATCH-RETURNED')
case('sys-reach','SYS A:','SYS B:','SYS D:','FORMAT A:','FORMAT B:','FORMAT D: /BAD','DISKCOPY C: D:','DISKCOMP C: D:')
case('del-many','DEL \\MANY\\F*.TXT','DIR \\MANY /W')
# More standalone files for CALL behavior.
extras={
'ARGS.BAT':'@ECHO OFF\nECHO ZERO=%0 ONE=%1 NINE=%9\nSHIFT\nECHO SHIFT ZERO=%0 ONE=%1 EIGHT=%8 NINE=%9\nSHIFT\nECHO SHIFT2 ONE=%1 NINE=%9\n',
'FLOW.BAT':'@ECHO OFF\nREM ignored\nIF "%1"=="hello" ECHO STRING-YES\nIF NOT "%1"=="HELLO" ECHO CASE-SENSITIVE\nIF EXIST A:\\T\\ONE.TXT ECHO EXIST-YES\nIF NOT EXIST A:\\T\\MISS ECHO NOT-EXIST\nFOR %%F IN (A B C) DO ECHO LOOP=%%F\nFOR %%F IN (A:\\T\\SUB\\*.TXT) DO ECHO FILE=%%F\nGOTO end\nECHO BAD-FALLTHROUGH\n:end\nECHO FLOW-END\n',
'NEST.BAT':'@ECHO OFF\nECHO PARENT-BEFORE\nCALL \\CASES\\CHILD.BAT arg\nECHO PARENT-AFTER\n',
'CHILD.BAT':'@ECHO OFF\nECHO CHILD=%1\n',
'GSPACE.BAT':'@ECHO OFF\nGOTO label with spaces\nECHO WRONG\n:label with spaces\nECHO SPACE-OK\n',
'SELF.BAT':'@ECHO OFF\nCALL \\CASES\\SELF.BAT\nECHO UNWIND\n',
'BIG.BAT':'@ECHO OFF\nECHO BIG-BEGIN\n'+('REM '+'x'*75+'\n')*65+'ECHO BIG-END\n',
'BREAK.BAT':'@ECHO OFF\nECHO BEFORE-PAUSE\nPAUSE\nECHO AFTER-PAUSE\n',
'CHAIN.BAT':'@ECHO OFF\nECHO CHAIN-BEFORE\n\\CASES\\CHILD.BAT arg\nECHO CHAIN-AFTER\n'}
def prepare():
 img=I/'suite-fixture.img';shutil.copyfile(I/'edge-fixture.img',img)
 sp.run(['mmd','-i',str(img),'::CASES'],check=True)
 dest=E/'cases';dest.mkdir(exist_ok=True)
 for c in cases:
  content='@ECHO OFF\r\nA:\r\nCD \\T\r\nCLS\r\nECHO CASE '+c['id']+' '+c['name']+'\r\nECHO ON\r\n'+'\r\n'.join(c['lines'])+'\r\n@ECHO AUDIT-DONE-'+c['id']+'\r\n'
  extras[c['id']+'.BAT']=content
 for n,t in extras.items():
  p=dest/n;p.write_bytes(t.replace('\r\n','\n').replace('\n','\r\n').encode());sp.run(['mcopy','-o','-i',str(img),str(p),'::CASES/'+n],check=True)
 (E/'cases.json').write_text(json.dumps(cases,indent=2));print('Prepared',len(cases),'cases',flush=True)

def run(label='suite',ids=None):
 d=Driver();results=[]
 try:
  d.start(label,'images/multi-boot-fixture.img','images/suite-fixture.img');d.shot(label+'-boot')
  for c in cases:
   if ids and c['id'] not in ids:continue
   before=d.serial.stat().st_size;start=time.monotonic();d.type('\\CASES\\'+c['id']+'.BAT')
   marker=('\nAUDIT-DONE-'+c['id']+'\r').encode();limit=time.monotonic()+12
   answered=0
   while time.monotonic()<limit:
    data=d.serial.read_bytes()[before:]
    if marker in data:break
    prompts=data.count(b'(F = file, D = directory)?')
    if prompts>answered:
     d.shot(label+'-'+c['id']+'-target-prompt');d.key('d' if c['id']=='B047' else 'f');answered=prompts
    time.sleep(.1)
   else:
    d.shot(label+'-'+c['id']+'-BLOCKED');raise RuntimeError('Case blocked '+c['id']+' '+repr(data[-500:]))
   time.sleep(.1);d.shot(label+'-'+c['id']+'-'+c['name'])
   data=d.serial.read_bytes()[before:];(E/f'{label}-{c["id"]}.txt').write_bytes(data)
   results.append(dict(c,seconds=round(time.monotonic()-start,3),serial_file=f'{label}-{c["id"]}.txt'))
   (E/f'{label}-results.json').write_text(json.dumps(results,indent=2));print('PASS-DONE',c['id'],c['name'],flush=True)
 finally:
  if d.proc:d.quit()
if __name__=='__main__':
 if sys.argv[1]=='prepare':prepare()
 else:run(sys.argv[1],set(sys.argv[2:]) or None)
