* Ref: ../dbase3-decomp/specs/functions/system-and-database-functions.md
* MINT-ERRORS: 11
? "[FNS-BEGIN]"
? "range", COL()>=0, COL()<80, TYPE("COL()")
@ 2,3 SAY "X"
STORE COL() TO POSVAL
? "at-2-3", POSVAL, TYPE("POSVAL")
@ 0,0 SAY ""
STORE COL() TO POSVAL
? "origin", POSVAL, TYPE("POSVAL")
@ 23,78 SAY "X"
STORE COL() TO POSVAL
? "edge", POSVAL, TYPE("POSVAL")
? "extra-arg"
? COL(1)
? "[FNS-END]"
RETURN
