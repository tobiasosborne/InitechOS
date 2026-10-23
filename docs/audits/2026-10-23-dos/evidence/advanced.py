#!/usr/bin/env python3
from pathlib import Path
import sys,time,re,json,hashlib
from robust_driver import Robust,R
mode=sys.argv[1];d=Robust();label='advanced-'+mode
prompt=rb'[A-Z]:\\[^\r\n>]*>\Z';editor=rb'\r\n\*\Z'
def cmds(*xs):
 for x in xs:d.command(x)
def shot(n):d.shot(label+'-'+n)
try:
 fixture='images/fat_data.img' if mode=='fatstock' else 'images/followup-fixture.img' if mode=='control' else 'images/more-fixture.img'
 d.start(label,'images/geometry-boot-fixture.img',fixture)
 if mode=='fatstock':
  d.keywait('ret',b'Enter new time:');d.keywait('ret',prompt);cmds('CLS','DIR','TYPE HELLO.TXT','COPY CHAIN.TXT COPY.TXT /B /V','TYPE BLOCK.BIN > BLOCK2.BIN','COPY EMPTY.TXT EMPTY2.TXT','DIR');shot('io')
 elif mode=='editor':
  cmds('CD \\T','CLS');d.enter('EDLIN TINY.TXT',editor)
  for line in ['1,4L','1,2,5C','L','5,6,1M','L','1,6Sone'] :d.enter(line,editor)
  d.type('1,6Rone',False);d.key('ctrl-z');d.enter('ONE',editor);d.enter('1TMERGE.TXT',editor);d.enter('L',editor);shot('commands')
  d.enter('E',prompt);cmds('TYPE TINY.TXT','TYPE TINY.BAK');shot('save-backup')
  d.enter('EDLIN TINY.TXT',editor);d.enter('1,2P',editor);d.enter('99,100D',editor);d.enter('Q',b'Abort edit');d.enter('y',prompt);shot('range-abort')
  d.enter('EDLIN TINY.BAK',prompt);shot('bak-refusal')
  d.enter('EDLIN ELARGE.TXT',editor);d.enter('1W',editor);d.enter('1A',editor);d.enter('1,3L',editor);shot('large-window');d.enter('E',prompt,40);cmds('DIR ELARGE.*');shot('large-save')
 elif mode=='control':
  cmds('CD \\T','CLS')
  start=d.serial.stat().st_size;d.type('TYPE HUGE.TXT',False);d.key('ret',0);d.waitfor(b'HUGE LINE 00005',start,5);d.key('ctrl-s');time.sleep(.3);n1=d.serial.stat().st_size;shot('ctrls');time.sleep(1);n2=d.serial.stat().st_size;d.key('spc');d.key('ctrl-c');time.sleep(1);n3=d.serial.stat().st_size;shot('ctrlc');d.log(['output-control-observation',n1,n2,n3]);
  # Wait for the actual TYPE completion before further commands (roughly 70 seconds).
  for j in range(4):
   try:d.waitfor(prompt,start,25);break
   except TimeoutError:d.log(['still-printing',j])
  else:raise RuntimeError('TYPE did not return within 100s')
  cmds('ECHO AFTER-TYPE');shot('type-finished')
  d.enter('MORE < LONG.TXT',b'-- More --|--More--|-- More--',10);shot('more-page1');d.key('ret');time.sleep(.2);shot('more-page2');d.key('ctrl-c');time.sleep(.3);shot('more-ctrlc')
 elif mode=='keyboard':
  cmds('CD \\T','CLS','ECHO TEMPLATE');d.type('ECHO ',False);d.key('f4');d.key('shift-p');d.key('f3');d.keywait('ret',prompt);shot('f4')
  cmds('CLS');d.type('ECHO ',False);d.key('caps_lock');d.key('a');d.key('shift-a');d.key('shift_r-b');d.key('caps_lock');d.key('shift-z');d.key('shift_r-z');d.keywait('ret',prompt);shot('caps')
  cmds('CLS','IF EXIST ONE.TXT ECHO IF-YES','SET X=expanded','ECHO %X%','ECHO OFF','ECHO','ECHO.');shot('interactive-expansion')
  d.type('ECHO '+'X'*122);d.waitfor(prompt);d.type('ECHO '+'Y'*123);d.waitfor(prompt);shot('127-128')
  cmds('PROMPT $E[31mRED$E[0m$G');shot('ansi')
 elif mode=='driveutils':
  cmds('CD \\T','CLS','TYPE D:\\DHOME\\SAME.TXT','ATTRIB D:\\DHOME\\SAME.TXT','ATTRIB +R D:\\DHOME\\SAME.TXT','TYPE D:\\DHOME\\SAME.TXT','CHKDSK C:','CHKDSK D:','TREE D:\\DHOME /F');shot('failures')
  cmds('XCOPY A:\\T\\ONE.TXT D:\\DHOME\\ /V','DIR D:\\DHOME','D:','CHKDSK','ATTRIB SAME.TXT','TREE /F');shot('selected-drive')
 elif mode=='misc':
  cmds('CD \\T','CLS','ERR0.COM','COMMAND /C ERR7.COM','IF ERRORLEVEL 7 ECHO CHILD-EXIT7','IF NOT ERRORLEVEL 7 ECHO CHILD-STATUS-LOST','ERR7.COM','IF ERRORLEVEL 7 ECHO DIRECT-EXIT7','TEN one two three four five six seven eight nine ten eleven');shot('errorlevel-shift')
  cmds('CLS','OPENPROB.COM','DIR OPEN.TXT');shot('open-delete')
  cmds('CLS','TYPE BINARY.BIN');shot('binary-type')
 elif mode=='disktools':
  for command,tag in [('DISKCOMP A: A:','same'),('DISKCOMP A: A: /1 /8','switches'),('DISKCOPY A: A:','copy')]:
   cmds('CLS');start=d.serial.stat().st_size;d.type(command);seen=0
   until=time.monotonic()+50
   while time.monotonic()<until:
    b=d.serial.read_bytes()[start:]
    if b'another diskette (Y/N)?' in b:break
    count=b.count(b'Insert FIRST')+b.count(b'Insert SECOND')+b.count(b'Press any key when ready')
    if count>seen:d.key('spc',.05);seen=count
    else:time.sleep(.03)
   else:raise TimeoutError('disk swap loop')
   shot(tag+'-done');d.enter('n',prompt);shot(tag+'-returned')
 elif mode=='format':
  cmds('CLS');d.enter('FORMAT D:',b'Enter current Volume Label');d.enter('WRONG',prompt);shot('wrong-label')
  d.enter('FORMAT D:',b'Enter current Volume Label');d.enter('EXTRA',b'Proceed with Format');d.enter('n',prompt);cmds('TYPE D:\\DHOME\\SAME.TXT');shot('cancel-preserved')
  d.enter('FORMAT D: /V',b'Enter current Volume Label');d.enter('EXTRA',b'Proceed with Format');d.enter('y',b'Volume label \(11');d.enter('NEWDISK',prompt);cmds('DIR D:\\','VOL D:','ECHO TEST > D:\\NEW.TXT','TYPE D:\\NEW.TXT');shot('formatted')
  cmds('SYS D:','DIR D:\\ /W');shot('sys')
  d.enter('FORMAT C:',b'Enter current Volume Label');d.enter('BOOT',b'Proceed with Format');d.enter('n',prompt);shot('boot-cancel')
  cmds('FORMAT A: /S','FORMAT D: /S','DIR C:\\ /W');shot('system-switch')
 elif mode=='fdisk':
  cmds('CLS');d.enter('FDISK',b'Enter choice:');d.enter('4',b'Press ESC to return to Fdisk Options');shot('partitions');d.keywait('esc',b'Enter choice:');d.enter('5',prompt);shot('running-volume')
finally:
 if d.proc:d.quit()
