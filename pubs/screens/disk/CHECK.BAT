@ECHO OFF
IF EXIST %1 ECHO %1 is on this diskette.
IF NOT EXIST %1 ECHO %1 cannot be found.
