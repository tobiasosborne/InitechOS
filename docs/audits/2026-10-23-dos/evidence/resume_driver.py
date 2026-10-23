#!/usr/bin/env python3
import sys,time,json,subprocess as sp,shutil
from pathlib import Path
from robust_driver import Robust,R
E=R/'evidence';I=R/'images';mode=sys.argv[1];label='resume-'+mode;d=Robust();prompt=rb'[A-Z]:\\[^\r\n>]*>\Z'
def cmds(*xs):
 for x in xs:d.command(x)
def shot(n):time.sleep(.25);d.shot(label+'-'+n)
def stopped():time.sleep(.2);return d.serial.stat().st_size
try:
 fixture='followup-fixture' if mode=='control' else 'root64real-fixture' if mode=='root64real' else 'resume-fixture'
 d.start(label,'images/single-boot-fixture.img' if mode=='fdiskcreate' else 'images/geometry-boot-fixture.img','images/'+fixture+'.img')
 if mode=='root64real':
  d.keywait('ret',b'Enter new time');d.keywait('ret',prompt);cmds('CLS')
 else:cmds('CD \\T','CLS')
 if mode=='root64real':
  cmds('ECHO TEST > NEW.TXT','MD NEWDIR','DIR /W');shot('full-root')
 elif mode in ['control','controlshort']:
  start=stopped();d.type('TYPE HUGE.TXT',False);d.key('ret',0);d.waitfor(b'HUGE LINE 00005',start,5);d.key('ctrl-c',.05);shot('ctrlc')
  for j in range(5):
   try:d.waitfor(prompt,start,25);break
   except TimeoutError:d.log(['still-output',j])
  else:raise TimeoutError('TYPE after Ctrl-C')
  cmds('ECHO AFTER-CANCEL');shot('returned')
  start=stopped();d.type('TYPE LONG.TXT',False);d.key('ret',0);d.waitfor(b'LINE 00005',start,5);d.key('ctrl-s');time.sleep(.4);n1=stopped();shot('ctrls');time.sleep(1);n2=stopped();d.log(['pause-counts',n1,n2]);d.key('spc');d.waitfor(prompt,start,40);cmds('ECHO AFTER-PAUSE');shot('resumed')
 elif mode=='xcopy':
  cmds('TYPE D:\\DHOME\\SAME.TXT','XCOPY A:\\T\\ONE.TXT D:\\DHOME\\ /V','DIR D:\\DHOME','DIR A:\\DHOME');shot('wrong-volume')
  cmds('TYPE A:\\DHOME\\ONE.TXT','TYPE D:\\DHOME\\ONE.TXT');shot('location')
  d.type('XCOPY D:\\DHOME\\SAME.TXT A:\\T\\DSAME.TXT');time.sleep(.5);shot('source-drive')
  if b'(F = file, D = directory)?' in d.serial.read_bytes()[-200:]:d.key('f');d.waitfor(prompt)
  cmds('MD SUB\\EMPTY','MD SUB\\BRANCH','COPY TWO.TXT SUB\\BRANCH\\','MD XDEST','XCOPY SUB XDEST /S /E /V','TREE XDEST /F');shot('empty-subdir')
 elif mode=='xcopyoverwrite':
  cmds('MD A:\\DHOME','ECHO KEEP-A > A:\\DHOME\\ONE.TXT','TYPE A:\\DHOME\\ONE.TXT','XCOPY A:\\T\\ONE.TXT D:\\DHOME\\ /V','TYPE A:\\DHOME\\ONE.TXT','DIR D:\\DHOME');shot('overwritten')
 elif mode=='batch':
  cmds('CALL \\CASES\\FLOW.BAT hello','\\CASES\\FLOW.BAT hello','SET TEST=VALUE','BENV.BAT','CHAIN.BAT','GOMISS.BAT','FOR %F IN (ONE TWO) DO IF EXIST ONE.TXT ECHO %F');shot('flow')
  d.enter('\\CASES\\BREAK.BAT',b'Strike a key');shot('pause-before');d.key('ctrl-c');d.waitfor(prompt);shot('pause-break');cmds('ECHO AFTER-BATCH');shot('pause-return')
  d.enter('LOOP.BAT',b'LOOPING');d.key('ctrl-c');time.sleep(.5);shot('loop-break')
  if not d.serial.read_bytes().endswith(b'>'):raise RuntimeError('batch break did not return')
  cmds('ERR7.COM','IF ERRORLEVEL 7 ECHO STATUS7','IF ERRORLEVEL 8 ECHO WRONG8','IF NOT ERRORLEVEL 8 ECHO BELOW8','REM COMMENT');
 elif mode=='keyboard':
  cmds('ECHO Quarter');d.key('f3');d.keywait('ret',prompt);d.key('f2');d.key('shift-q');d.type('Month');d.waitfor(prompt);d.type('ECHO Friday',False);d.key('f5');d.key('f3');d.keywait('ret',prompt);d.type('WRONG',False);d.key('esc');d.key('f3');d.keywait('ret',prompt);shot('template')
  cmds('CLS','ECHO TEMPLATE');
  for _ in range(5):d.key('f1',.06)
  d.key('delete');d.key('insert');d.type('X',False);d.key('insert');d.key('f3');d.keywait('ret',prompt);shot('insert-delete')
  d.type('ECHO CANCELED',False);d.keywait('ctrl-c',prompt);d.key('f3');d.keywait('ret',prompt);shot('cancel-template')
  cmds('CLS');
  for _ in range(5):d.key('backspace')
  d.type('ECHO ',False);d.key('caps_lock');d.key('a');d.key('shift-a');d.key('shift_r-b');d.key('caps_lock');d.key('shift-z');d.key('shift_r-z');d.keywait('ret',prompt);shot('caps')
  cmds('ECHO AFTER-BUILTIN');d.key('f3');d.keywait('ret',prompt);cmds('ERR0.COM');d.key('f3');d.keywait('ret',prompt);cmds('BENV.BAT');d.key('f3');d.keywait('ret',prompt);cmds('NOSUCH');d.key('f3');d.keywait('ret',prompt);cmds('ECHO FILE > RECALL.TXT');d.key('f3');d.keywait('ret',prompt);d.keywait('ret',prompt);d.key('f3');d.keywait('ret',prompt);shot('recall-kinds')
  cmds('CLS');d.type('ECHO '+'X'*122);d.waitfor(prompt);d.type('ECHO '+'Y'*123);d.waitfor(prompt);shot('line-limit')
  cmds('CLS');d.enter('COPY CON CON2.TXT',b'CON2.TXT\r\n');d.type('FIRST');d.type('SECOND');d.key('ctrl-z');d.keywait('ret',prompt);cmds('TYPE CON2.TXT');shot('con-eof')
 elif mode=='external':
  cmds('FIND "Alpha" < LINES.TXT','FIND /V /C "a" < LINES.TXT','FIND /N /C "a" < LINES.TXT','FIND bad < ONE.TXT','IF ERRORLEVEL 2 ECHO BAD-ARGS-RC2','IF NOT ERRORLEVEL 2 ECHO BAD-ARGS-LOW','SORT /BAD < LINES.TXT','IF ERRORLEVEL 1 ECHO SORT-FAIL','IF NOT ERRORLEVEL 1 ECHO SORT-RC0');shot('filters')
  cmds('FIND "Alpha" < LINES.TXT');start=stopped();d.type('MORE < LINES.TXT');time.sleep(.5);shot('more-short');
  d.waitfor(prompt,start)
  cmds('CLS');start=stopped();d.type('MORE < PAGES.TXT');time.sleep(.6);shot('more-page1');n1=stopped();time.sleep(.7);n2=stopped();d.log(['more-paused',n1,n2]);d.key('ret');time.sleep(.4);shot('more-page2');d.key('ctrl-c');time.sleep(.5);shot('more-break');
  if not d.serial.read_bytes().endswith(b'>'):
   for _ in range(5):d.key('spc');time.sleep(.1)
  d.waitfor(prompt,start);cmds('ECHO AFTER-MORE');shot('more-return')
  cmds('CLS');d.enter('EDLIN EEOF.TXT',rb'\r\n\*\Z');d.enter('I',rb'1:\*');d.type('inserted');d.key('ctrl-z');d.key('ret');time.sleep(.3);shot('edlin-ctrlz');d.key('ctrl-c');d.waitfor(rb'\r\n\*\Z');d.enter('L',rb'\r\n\*\Z');d.enter('E',prompt);cmds('TYPE EEOF.TXT');shot('edlin-save')
 elif mode=='edlineof':
  d.enter('EDLIN EEOF.TXT',rb'\r\n\*\Z');d.enter('I',rb'1:\*');d.type('inserted');d.key('ctrl-z');d.key('ret');time.sleep(.3);shot('ctrlz');d.key('ctrl-c');d.waitfor(rb'\r\n\*\Z');d.enter('L',rb'\r\n\*\Z');shot('list');d.enter('E',prompt);cmds('TYPE EEOF.TXT');shot('saved')
 elif mode in ['gaps','gaps2']:
  cmds('CLS');
  for i in range(6):d.shot(label+'-cursor-'+str(i));time.sleep(.2)
  cmds('EXIT','VER','BREAK OFF','BREAK','BREAK ON','BREAK','BREAK INVALID');shot('builtins')
  d.enter('DIR \\MANY /P',b'Press any key');shot('dir-first');d.key('ctrl-c');time.sleep(.5);shot('dir-break')
  for _ in range(16):
   if d.serial.read_bytes().endswith(b'>'):break
   d.key('spc');time.sleep(.1)
  d.key('ctrl-c');d.waitfor(prompt);cmds('ECHO AFTER-DIR');shot('dir-returned')
 elif mode=='format':
  cmds('FORMAT D: /S','DIR D:\\');shot('system-before')
  cmds('FORMAT D: /B');d.enter('FORMAT D:',b'Enter current Volume Label');d.enter('EXTRA',b'Proceed with Format');d.enter('y',prompt,30);cmds('SYS D:','DIR D:\\ /W');shot('system-reserved')
  d.enter('FORMAT C:',b'Enter current Volume Label');d.enter('BOOT',b'Proceed with Format');d.enter('y',prompt,30);cmds('DIR C:\\','ECHO ALIVE','TYPE ONE.TXT');shot('boot-format')
 elif mode in ['app123','appword']:
  app='123.EXE' if mode=='app123' else 'WORD.EXE'
  cmds('CD \\APPS','DIR '+app)
  d.type(app);time.sleep(2);shot('launch');
  if d.serial.read_bytes().endswith(b'>'):cmds('A:\\APPS\\'+app);shot('qualified')
 elif mode=='copyedge':
  cmds('COPY /B EOF.TXT PREFIX.BIN','COPY /A EOF.TXT PREFIX.TXT','TYPE PREFIX.TXT','COPY /V ONE.TXT PREFIX2.TXT','DIR PREFIX*.*');shot('prefixes')
 elif mode=='promptversion':
  d.type('PROMPT $V$G');time.sleep(.3);shot('version');d.type('PROMPT');d.waitfor(prompt)
 elif mode=='promptcodes':
  for index,value in enumerate(['$N$Q$P$G$L$B$$','$D $T$_AB$HC$G','$V$G','$E[31mRED$E[0m$G','$X$G']):
   d.type('PROMPT '+value);time.sleep(.2);shot('code-'+str(index))
  d.type('PROMPT');d.waitfor(prompt);cmds('ECHO RESTORED');shot('restored')
 elif mode=='attribrecursive':
  cmds('MD SUB\\DEEP','COPY ONE.TXT SUB\\DEEP\\ONE.TXT','ATTRIB +R +A SUB\\*.TXT /S','ATTRIB SUB\\*.TXT /S','DEL SUB\\DEEP\\ONE.TXT','COPY TWO.TXT SUB\\DEEP\\ONE.TXT','TYPE SUB\\DEEP\\ONE.TXT');shot('protected');cmds('ATTRIB -R SUB\\*.TXT /S','DEL SUB\\DEEP\\ONE.TXT','DIR SUB\\DEEP');shot('unprotected')
 elif mode=='syswrite':
  cmds('DIR D:\\');d.enter('FORMAT D:',b'Enter current Volume Label');d.enter('EXTRA',b'Proceed with Format');d.enter('y',prompt,30);cmds('SYS D:','DIR D:\\','ECHO NEW-DATA > D:\\NEW.TXT','TYPE D:\\NEW.TXT','DIR D:\\');shot('after-write')
 elif mode=='fdiskcreate':
  d.enter('FDISK',b'Enter choice:');d.enter('3',b'Delete Primary DOS Partition');d.enter('1',b'continue');d.enter('y',b'Press ESC');d.keywait('esc',b'Enter choice:');d.enter('4',b'Press ESC');shot('deleted');d.keywait('esc',b'Enter choice:');d.enter('1',b'Create Primary DOS Partition');d.enter('1',b'maximum size');d.enter('y',b'System will now restart');shot('created')
 elif mode=='fdiskactive':
  d.enter('FDISK',b'Enter choice:');d.enter('2',b'Enter the number');d.enter('2',b'Press ESC');d.keywait('esc',b'Enter choice:');d.enter('4',b'Press ESC');shot('changed');d.keywait('esc',b'Enter choice:');d.keywait('esc',b'System will now restart');shot('restart')
 elif mode=='fdiskmenus':
  d.enter('FDISK',b'Enter choice:');d.enter('1',b'Create Primary DOS Partition');d.enter('1',b'Primary DOS partition already exists');shot('create-existing');d.keywait('esc',b'Enter choice:');d.enter('3',b'Delete Primary DOS Partition');d.enter('1',b'continue');shot('delete-warning');d.enter('n',b'Press ESC');d.keywait('esc',b'Enter choice:');d.enter('4',b'Press ESC');shot('cancel-preserved');d.keywait('esc',b'Enter choice:');d.keywait('esc',prompt)
 elif mode=='fdisk':
  d.enter('FDISK',b'Enter choice:');d.enter('2',b'Enter|active|Active');time.sleep(.3);shot('active-menu');d.key('esc');time.sleep(.2);d.key('esc');time.sleep(.2);shot('cancel');
 elif mode in ['typeahead','typeaheadpaced']:
  start=stopped();d.type('TYPE HUGE.TXT',False);d.key('ret',0);d.waitfor(b'HUGE LINE 00005',start,10)
  events=[]
  for c in 'abcdefghijklmnopqrst':
   for down in [True,False]:events.append({'type':'key','data':{'down':down,'key':{'type':'qcode','data':c}}})
  d.log(['rapid-20-keys','abcdefghijklmnopqrst']);
  if mode=='typeahead':d.cmd('input-send-event',{'events':events})
  else:
   for c in 'abcdefghijklmnopqrst':d.key(c,.05)
  d.waitfor(b'HUGE LINE 01999',start,50);time.sleep(.3);shot('buffered');d.key('ret');time.sleep(.4);shot('executed')
 else:raise ValueError(mode)
finally:
 if d.proc:d.quit()
