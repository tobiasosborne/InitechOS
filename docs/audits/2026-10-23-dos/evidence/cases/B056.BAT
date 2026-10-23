@ECHO OFF
A:
CD \T
CLS
ECHO CASE B056 cross-drive-vol
ECHO ON
VOL A:
VOL C:
VOL D:
DIR D: /W
DIR B:
CD B:\
B:
Z:
@ECHO AUDIT-DONE-B056
