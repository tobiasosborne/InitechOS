* Ref: ../dbase3-decomp/specs/functions/system-and-database-functions.md
* MINT-ERRORS: 11 11 11
? "[FNS-BEGIN]"
? "no-key", INKEY(), TYPE("INKEY()")
? "no-key-again", INKEY(), TYPE("INKEY()")
? "timeout-one"
? INKEY(1)
? "wrong-type"
? INKEY("X")
? "extra-args"
? INKEY(1,2)
? "[FNS-END]"
RETURN
