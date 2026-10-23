#!/usr/bin/env python3
from pathlib import Path
import sys,time,json,hashlib
R=Path(__file__).resolve().parent.parent;sys.path.insert(0,str(R));from audit_driver import Driver
label=sys.argv[1] if len(sys.argv)>1 else 'interactive'
d=Driver()
def cmds(*a):
 for s in a:d.type(s)
def shot(n):d.shot(label+'-'+n)
try:
 d.start(label,'images/multi-boot-fixture.img','images/program-fixture.img')
 cmds('CD \\T','CLS','ECHO Quarter');d.key('f3');d.key('ret');d.key('f2');d.key('shift-q');d.type('Month');d.type('ECHO Friday',False);d.key('f5');d.key('f3');d.key('ret');d.type('WRONG',False);d.key('esc');d.key('f3');d.key('ret');shot('template-f2-f3-f5-esc')
 cmds('CLS','ECHO TEMPLATE');
 for _ in range(5):d.key('f1',.05)
 d.key('delete');d.key('insert');d.type('X',False);d.key('insert');d.key('f3');d.key('ret');shot('template-f1-insert-delete')
 d.type('ECHO CANCELED',False);d.key('ctrl-c');shot('ctrlc');d.key('f3');d.key('ret');shot('f3-after-break')
 cmds('CLS');
 for _ in range(5):d.key('backspace',.05)
 d.type('ECHO ',False);d.key('caps_lock');d.key('a');d.key('shift-a');d.key('shift_r-b');d.key('caps_lock');d.key('shift-z');d.key('shift_r-z');d.key('ret');shot('caps-both-shifts')
 cmds('CLS');d.type('ECHO '+'X'*122);shot('line127');d.type('ECHO '+'Y'*123);shot('line128')
 cmds('CLS','FOR %F IN (ONE TWO) DO ECHO %F','IF EXIST ONE.TXT ECHO IF-YES','ECHO.','ECHO OFF','ECHO');shot('interactive-builtins')
 cmds('PROMPT $N$Q$P$G$L$B$$','ECHO PROMPTTEST');shot('prompt-symbols');cmds('PROMPT $D $T$_AB$HC$G','ECHO CLOCK');shot('prompt-date-backspace');cmds('PROMPT $E[31mRED$E[0m$G','ECHO ANSI');shot('prompt-ansi');cmds('PROMPT','CLS')
 cmds('COPY CON CONFILE.TXT');d.type('FIRST');d.type('SECOND');d.key('ctrl-z');d.key('ret');cmds('TYPE CONFILE.TXT');shot('copy-con-eof')
 cmds('\\CASES\\BREAK.BAT');shot('batch-before-break');d.key('ctrl-c');shot('batch-after-break');cmds('ECHO RECOVERED');shot('batch-recovered')
 cmds('CLS','DEL DEST\\*.*');d.type('n');cmds('DIR DEST /W','DEL DEST\\*.*');d.type('y');cmds('DIR DEST /W');shot('del-confirm')
 cmds('CLS','DIR \\MANY /P');shot('dir-page1');d.key('spc');shot('dir-page2');d.key('ctrl-c');shot('dir-page-break');
 # Discard any remaining page prompts with enough spaces, then capture result.
 for _ in range(15):d.key('spc',.04)
 d.key('ctrl-c');cmds('CLS','FIND "Alpha" LINES.TXT');shot('find-file-waits');d.key('ctrl-z');d.key('ret');shot('find-file-eof');cmds('FIND "NONE" < ONE.TXT','IF ERRORLEVEL 1 ECHO NOMATCH-ERRORLEVEL-1','FIND bad < ONE.TXT','IF ERRORLEVEL 2 ECHO BADARG-ERRORLEVEL-2');shot('find-errorlevel')
 cmds('CLS','EDLIN EDIT.TXT','I','first line','second line','third line');d.key('ctrl-c');cmds('L','2D','L','E','TYPE EDIT.TXT','DIR EDIT.*');shot('edlin-create')
 cmds('EDLIN EDIT.TXT','1,2L','1','changed line','L','Q');d.type('n');cmds('L','Q');d.type('y');cmds('TYPE EDIT.TXT');shot('edlin-abort')
 cmds('EDLIN NOCR.TXT','L','Q');d.type('y');shot('edlin-no-crlf')
 cmds('EDLIN LONG.TXT','1,3L','Q');d.type('y');shot('edlin-large')
 cmds('CLS','LABEL');d.key('ret');d.type('n');cmds('VOL','LABEL');d.key('ret');d.type('y');cmds('VOL','LABEL INITECH');shot('label-delete-confirm')
 cmds('CLS','XCOPY A:\\T\\SUB A:\\T\\DEST\\ /S /E /W /P');shot('xcopy-wait');d.key('spc');shot('xcopy-confirm');d.key('n');d.key('y');d.key('n');cmds('DIR DEST /W');shot('xcopy-p-results')
 cmds('CLS','FDISK');shot('fdisk-main');d.type('4');shot('fdisk-display');d.key('esc');d.key('esc');shot('fdisk-exit')
finally:
 if d.proc:d.quit()
