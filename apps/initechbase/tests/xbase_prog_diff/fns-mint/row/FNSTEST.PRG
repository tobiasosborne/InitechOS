* Ref: ../dbase3-decomp/specs/functions/system-and-database-functions.md
* MINT-ERRORS: 11
? "[FNS-BEGIN]"
? "range", ROW()>=0, ROW()<25, TYPE("ROW()")
@ 2,3 SAY "X"
STORE ROW() TO POSVAL
? "at-2-3", POSVAL, TYPE("POSVAL")
@ 0,0 SAY ""
STORE ROW() TO POSVAL
? "origin", POSVAL, TYPE("POSVAL")
@ 23,78 SAY "X"
STORE ROW() TO POSVAL
? "edge", POSVAL, TYPE("POSVAL")
? "extra-arg"
? ROW(1)
? "[FNS-END]"
RETURN
