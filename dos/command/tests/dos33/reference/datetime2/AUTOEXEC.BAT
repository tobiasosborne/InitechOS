@echo off
ver > A:\VERSION.TXT
path A:\
B:
command /c DT2CASE.BAT <ANSWERS.TXT >TRACE.TXT
echo CAPTURE-DONE
