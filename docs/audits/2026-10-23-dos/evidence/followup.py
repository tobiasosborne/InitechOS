#!/usr/bin/env python3
from pathlib import Path
import sys,time,json,subprocess as sp,shutil
R=Path(__file__).resolve().parent.parent;sys.path.insert(0,str(R));from audit_driver import Driver
E=R/'evidence';I=R/'images'
if len(sys.argv)>1 and sys.argv[1]=='prepare':
 img=I/'followup-fixture.img';shutil.copyfile(I/'program-fixture.img',img)
 def put(name,data):
  p=E/'special-fixtures'/name;p.write_bytes(data);sp.run(['mcopy','-o','-i',str(img),str(p),'::T/'+name],check=True)
 put('PIPE1.$$$',b'USER-OWNED-PIPE-FILE\r\n');put('QUOTES.TXT',b'He said "yes"\r\nHe said no\r\n');put('SORTCOL.TXT',b'01 zebra\r\n02 apple\r\n');put('SORT512.TXT',b''.join(f'ROW {i:04d}\r\n'.encode() for i in range(600)));put('HUGE.TXT',b''.join(f'HUGE LINE {i:05d} abcdefghijklmnopqrstuvwxyz\r\n'.encode() for i in range(10000)));put('BATCH2.BAT',b'@ECHO OFF\r\nECHO BATCH2-RAN\r\n')
 for n in ['SAMIR.COM','CLIENTS.DBF']:
  b=sp.check_output(['mcopy','-i',str(I/'samir_list.img'),'::'+n,'-']);p=E/'special-fixtures'/n;p.write_bytes(b);sp.run(['mcopy','-o','-i',str(I/'program-fixture.img'),str(p),'::'+n],check=True)
 sys.exit()
d=Driver();label='followup'
def cmds(*xs):
 for x in xs:d.type(x)
def shot(n):d.shot(label+'-'+n)
try:
 d.start(label,'images/multi-boot-fixture.img','images/followup-fixture.img')
 cmds('CD \\T','CLS','TYPE PIPE1.$$$','TYPE ONE.TXT | SORT','DIR PIPE1.$$$','TYPE PIPE1.$$$');shot('pipe-clobber')
 cmds('CLS','PATH A:\\NO1;A:\\NO2;A:\\DOS','PATH','CHKDSK','A:\\DOS\\CHKDSK.COM');shot('path-third-directory');cmds('PATH A:\\DOS')
 cmds('CLS','A:\\T\\BATCH2','A:\\T\\BATCH2.BAT','BATCH2','COMMAND','COMMAND /C COMMAND /C ECHO NESTED','SET TEST=parent','COMMAND /C SET TEST=child','SET TEST','ECHO %TEST%');shot('commands-env')
 cmds('CLS','CD C:\\CHOME','CD D:\\DHOME','CD C:','CD D:','CD','TREE D:\\DHOME /F','TYPE D:\\DHOME\\SAME.TXT');shot('cd-tree-drive')
 cmds('CLS','SORT /+4 < SORTCOL.TXT','SORT < SORT512.TXT > S600.OUT','DIR S600.OUT');shot('sort-column-cap')
 cmds('CLS','FIND /BAD "ALPHA" < ONE.TXT','FIND "" < ONE.TXT','FIND "He said ""yes""" < QUOTES.TXT','FIND "absent" < ONE.TXT','IF ERRORLEVEL 1 ECHO RC1','IF NOT ERRORLEVEL 1 ECHO RC0');shot('find-quotes-exit')
 cmds('CLS','COMMAND /C \\CASES\\CHAIN.BAT','ECHO AFTER-CHAIN','ECHO OFF','ECHO','ECHO ON');shot('chain-and-echo')
 cmds('CLS');d.type('TYPE HUGE.TXT',False);d.key('ret',0);d.key('ctrl-s',.1);time.sleep(.3);before=d.serial.stat().st_size;shot('ctrls-paused');time.sleep(.8);after=d.serial.stat().st_size;d.log(['ctrls-byte-count',before,after]);d.key('spc',.2);d.key('ctrl-c',.2);shot('type-interrupted');cmds('ECHO AFTER-TYPE');shot('type-recovery')
 cmds('CLS');d.type('TYPE HUGE.TXT',False);d.key('ret',0);d.key('ctrl-c',.05);d.pause(.5);shot('type-fast-ctrlc');cmds('ECHO AFTER-BREAK');shot('type-fast-recovery')
 cmds('CLS','MORE < LONG.TXT');shot('more-page1');d.key('ret');shot('more-page2');d.key('ctrl-c');shot('more-break');cmds('ECHO AFTER-MORE');shot('more-recovery')
 cmds('CLS','\\CASES\\BREAK.BAT');d.type('abcdefghijklmnopqrst',False);d.key('ret');shot('typeahead20');d.key('ctrl-c')
finally:
 if d.proc:d.quit()
