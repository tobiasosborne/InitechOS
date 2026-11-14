@echo off
time 12:34:10.00
echo hello|dir >ONE.OUT
echo hello|dir >TWO.OUT
time 23:58:10.00
echo hello|dir >THREE.OUT
echo hello|type A.TXT|dir >FOUR.OUT
echo PIPE-DONE
