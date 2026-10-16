@ECHO OFF
REM SAVE.BAT copies files with XCOPY and checks the exit code.
XCOPY %1 %2
IF ERRORLEVEL 1 ECHO Check the file names and try again.
IF NOT ERRORLEVEL 1 ECHO The files were copied.
