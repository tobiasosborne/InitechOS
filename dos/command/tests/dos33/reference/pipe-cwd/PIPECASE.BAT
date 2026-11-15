@echo off
time 12:34:10
cd START
echo hello|B:\SUB\PASS >B:\ONE.OUT
cd ..\SUB|cd >B:\CWD.OUT
cd >B:\AFTER.OUT
echo PIPE-DONE
