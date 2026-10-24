* Ref: ../dbase3-decomp/specs/functions/system-and-database-functions.md
* MINT-ERRORS: 87 87 11 11 11
? "[FNS-BEGIN]"
USE BASE
? "no-index", "["+NDX(1)+"]", TYPE("NDX(1)"), LEN(NDX(1))
? "slot-eight", "["+NDX(8)+"]", TYPE("NDX(8)")
INDEX ON NAME TO T1
? "indexed", "["+NDX(1)+"]", TYPE("NDX(1)"), LEN(NDX(1))
? "slot-eight-open", "["+NDX(8)+"]"
CLOSE INDEX
? "closed", "["+NDX(1)+"]"
? "wrong-type"
? NDX("X")
? "missing-arg"
? NDX()
? "extra-arg"
? NDX(1,2)
USE
? "[FNS-END]"
RETURN
