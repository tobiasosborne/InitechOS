@echo off
ver > A:\VERSION.TXT
path A:\
B:
call ENV.BAT > TRACE.TXT
echo CAPTURE-DONE
