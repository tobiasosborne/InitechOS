@ECHO OFF
IF "%1"=="" GOTO NONAME
IF EXIST %1 ECHO %1 is on this diskette.
IF NOT EXIST %1 ECHO %1 cannot be found.
GOTO END
:NONAME
ECHO Type the name of a file after CHECK.
:END
