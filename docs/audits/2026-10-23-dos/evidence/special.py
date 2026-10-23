#!/usr/bin/env python3
from pathlib import Path
import sys,time,json
R=Path(__file__).resolve().parent.parent;sys.path.insert(0,str(R));from audit_driver import Driver
kind=sys.argv[1];d=Driver()
def cmds(*a):
 for x in a:d.type(x)
def shot(n):d.shot(kind+'-'+n)
try:
 image='images/'+kind+'-fixture.img'
 if kind.startswith('prog-') or kind in ['app123','appword','appsamir']:image='images/program-fixture.img'
 d.start(kind,'images/multi-boot-fixture.img',image);shot('boot')
 if kind.startswith('full'):
  cmds('CD \\T','CLS','DIR /W','ECHO TEST > FULL.OUT','DIR FULL.OUT','TYPE FULL.OUT','COPY LONG.TXT FULL.BIN','DIR FULL.BIN','TYPE ONE.TXT | SORT','ECHO APPEND >> ONE.TXT','DIR ONE.TXT','COPY ZERO.TXT ZERO2.TXT','MD FAILDIR');shot('outcomes')
 elif kind.startswith('root'):
  # no autoexec: accept startup clock prompts.
  d.key('ret');d.key('ret');cmds('CLS','DIR /W','ECHO TEST > NEW.TXT','MD NEWDIR','DIR NEW*.*');shot('capacity')
 elif kind.startswith('spc'):
  cmds('CLS','COPY EXACT.BIN COPY.BIN /B /V','COPY PLUS.BIN COPYPLUS.BIN /B','MD SUB','COPY EXACT.BIN SUB\\','REN COPY.BIN RENAMED.BIN','DIR /W','TYPE PLUS.BIN > TYPE.BIN','DEL COPYPLUS.BIN','DIR /W');shot('io')
 elif kind=='fat16':
  cmds('CLS','DIR','TYPE CHAIN.TXT > C:\\FAT16.OUT','ECHO TEST > NEW.TXT','MD SUB','COPY CHAIN.TXT C:\\FATCOPY.TXT /B','DIR C:\\FAT*.*');shot('read-write')
 elif kind in ['badbpb','nofat']:
  cmds('DIR','C:','DIR','VER');shot('recovery')
 elif kind=='dirty':
  cmds('CLS','CHKDSK');shot('scan');cmds('CHKDSK /F');shot('fix-prompt');d.type('y');shot('fix-done');cmds('CHKDSK','DIR *.CHK','ECHO AFTER > POST.TXT','TYPE POST.TXT');shot('rescan')
 elif kind.startswith('prog-'):
  cmds('CD \\T','CLS',{'prog-zero':'ZERO.COM','prog-garbage':'GARBAGE.COM','prog-mz16':'DOS16.EXE','prog-64k':'MAX64.COM'}[kind]);d.pause(.5);shot('result');cmds('ECHO ALIVE');shot('after-command')
 elif kind in ['app123','appword','appsamir']:
  cmds('CLS',{'app123':'A:\\APPS\\123.EXE','appword':'A:\\APPS\\WORD.EXE','appsamir':'SAMIR.COM'}[kind]);d.pause(.5);shot('launch');d.key('ctrl-c');shot('break');
finally:
 if d.proc:d.quit()
