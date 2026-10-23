#!/usr/bin/env python3
from pathlib import Path
import sys,time,json,re
from robust_driver import Robust,R
kind=sys.argv[1];label='re-'+kind;d=Robust()
def commands(*xs):
 for x in xs:d.command(x)
def shot(n):d.shot(label+'-'+n)
try:
 d.start(label,'images/multi-boot-fixture.img','images/'+kind+'-fixture.img');
 if kind in ['badbpb','nofat'] or kind.startswith('root'):
  d.keywait('ret',b'Enter new time:');d.keywait('ret',rb'[A-Z]:\\[^\r\n>]*>\Z')
 if kind.startswith('full'):
  commands('CD \\T','CLS','DIR /W','ECHO TEST > FULL.OUT','DIR FULL.OUT','TYPE FULL.OUT');shot('redirection')
  commands('COPY LONG.TXT FULL.BIN','DIR FULL.BIN');shot('copy-error')
  commands('TYPE ONE.TXT | SORT','ECHO PIPE-RETURNED');shot('pipe-empty')
  commands('COPY ZERO.TXT ZERO2.TXT','MD FAILDIR','DIR ZERO2.TXT');shot('zero-dir')
 elif kind.startswith('root'):
  commands('CLS','ECHO TEST > NEW.TXT','MD NEWDIR','DIR NEW*.*');shot('capacity')
 elif kind.startswith('spc'):
  commands('CLS','COPY EXACT.BIN COPY.BIN /B /V','COPY PLUS.BIN COPYPLUS.BIN /B','MD SUB','COPY EXACT.BIN SUB\\','REN COPY.BIN RENAMED.BIN','TYPE PLUS.BIN > TYPE.BIN','DEL COPYPLUS.BIN','DIR /W');shot('completed-io')
 elif kind=='fat16':
  commands('CLS','TYPE CHAIN.TXT > C:\\FAT16.OUT','ECHO TEST > NEW.TXT','MD SUB','COPY CHAIN.TXT C:\\FATCOPY.TXT /B','DIR C:\\FAT*.*');shot('completed-read')
 elif kind in ['badbpb','nofat']:
  commands('CLS','DIR','C:','DIR','VER');shot('recovery')
 elif kind=='dirty':
  d.enter('CHKDSK',b'Convert lost chains to files');shot('no-fix-question');d.enter('n',rb'[A-Z]:\\[^\r\n>]*>\Z');shot('no-fix-result')
  d.enter('CHKDSK /F',b'Convert lost chains to files');shot('fix-question');d.enter('y',rb'[A-Z]:\\[^\r\n>]*>\Z');shot('fix-result')
  commands('CHKDSK','DIR *.CHK','ECHO AFTER > POST.TXT','TYPE POST.TXT');shot('rescan')
finally:
 if d.proc:d.quit()
