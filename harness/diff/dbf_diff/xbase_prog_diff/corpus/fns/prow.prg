* Ref: ../dbase3-decomp/specs/functions/system-and-database-functions.md
* MINT-ERRORS: 11
? "[FNS-BEGIN]"
? "screen", PROW(), TYPE("PROW()")
SET DEVICE TO SCREEN
@ 2,3 SAY "X"
? "screen-after-say", PROW(), TYPE("PROW()")
SET DEVICE TO PRINT
@ 2,3 SAY "X"
STORE PROW() TO POSVAL
SET DEVICE TO SCREEN
? "printer-at-2-3", POSVAL, TYPE("POSVAL")
? "extra-arg"
? PROW(1)
? "[FNS-END]"
RETURN
