@echo off
ver > A:\VERSION.TXT
path A:\
B:
command /c DTCASE.BAT <ANSWERS.TXT >TRACE.TXT
echo CAPTURE-DONE
