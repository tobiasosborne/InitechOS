* Ref: ../dbase3-decomp/specs/functions/system-and-database-functions.md
* MINT-ERRORS: 11
? "[FNS-BEGIN]"
? "screen", PCOL(), TYPE("PCOL()")
SET DEVICE TO SCREEN
@ 2,3 SAY "X"
? "screen-after-say", PCOL(), TYPE("PCOL()")
SET DEVICE TO PRINT
@ 2,3 SAY "X"
STORE PCOL() TO POSVAL
SET DEVICE TO SCREEN
? "printer-at-2-3", POSVAL, TYPE("POSVAL")
? "extra-arg"
? PCOL(1)
? "[FNS-END]"
RETURN
