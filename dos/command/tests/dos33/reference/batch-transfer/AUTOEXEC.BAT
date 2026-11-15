@echo off
path A:\
B:
command /c PLAIN.BAT >PLAIN.OUT
command /c TCALL.BAT >TCALL.OUT
command /c TIF.BAT >TIF.OUT
command /c TFOR.BAT >TFOR.OUT
echo CAPTURE-DONE
