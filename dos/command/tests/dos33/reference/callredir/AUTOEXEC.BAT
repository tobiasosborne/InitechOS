@echo off
path A:\
B:
call CHILD.BAT > TRACE.TXT
call NOPE.BAT > NOPE.OUT
echo CAPTURE-DONE
