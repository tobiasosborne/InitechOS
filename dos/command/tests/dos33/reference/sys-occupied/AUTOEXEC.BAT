@echo off
sys b: > A:\TRACE.TXT
echo SYS-EXIT-%ERRORLEVEL% >> A:\TRACE.TXT
echo CAPTURE-DONE
