@echo off
ver > A:\VERSION.TXT
path A:\
B:
command /c DT3CASE.BAT <ANSWERS.TXT >TRACE.TXT
echo CAPTURE-DONE
