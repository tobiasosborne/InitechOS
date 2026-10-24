* Ref: ../dbase3-decomp/specs/functions/numeric-and-date-functions.md
* MINT-ERRORS: 58 58 11 11 11
? "[FNS-BEGIN]"
? "LOG(1)", LOG(1), TYPE("LOG(1)")
? "LOG(2)", LOG(2), TYPE("LOG(2)")
? "LOG(0.5)", LOG(0.5), TYPE("LOG(0.5)")
? "zero-domain"
? LOG(0)
? "negative-domain"
? LOG(-1)
? "wrong-type"
? LOG("X")
? "missing-arg"
? LOG()
? "extra-arg"
? LOG(1,2)
? "[FNS-END]"
RETURN
