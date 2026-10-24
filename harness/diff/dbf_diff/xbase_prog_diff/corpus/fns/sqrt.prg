* Ref: ../dbase3-decomp/specs/functions/numeric-and-date-functions.md
* MINT-ERRORS: 61 11 11 11
? "[FNS-BEGIN]"
? "SQRT(4)", SQRT(4), TYPE("SQRT(4)")
? "SQRT(2)", SQRT(2), TYPE("SQRT(2)")
? "SQRT(0)", SQRT(0), TYPE("SQRT(0)")
? "negative-domain"
? SQRT(-1)
? "wrong-type"
? SQRT("X")
? "missing-arg"
? SQRT()
? "extra-arg"
? SQRT(1,2)
? "[FNS-END]"
RETURN
