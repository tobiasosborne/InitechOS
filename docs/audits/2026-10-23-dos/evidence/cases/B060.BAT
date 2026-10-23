@ECHO OFF
A:
CD \T
CLS
ECHO CASE B060 batch-args
ECHO ON
CALL \CASES\ARGS.BAT one two three four five six seven eight nine ten eleven
@ECHO AUDIT-DONE-B060
