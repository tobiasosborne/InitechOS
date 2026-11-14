@echo off
ver > A:\VERSION.TXT
path A:\
B:
command /c BATCH.BAT one two three four five six seven eight nine ten > TRACE.TXT
echo CAPTURE-DONE
