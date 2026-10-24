* Ref: ../dbase3-decomp/specs/functions/system-and-database-functions.md
* MINT-ERRORS: 11 11 11
? "[FNS-BEGIN]"
? TYPE("VERSION()"), LEN(VERSION())>0
? "version", "["+VERSION()+"]", TYPE("VERSION()"), LEN(VERSION())
? "zero", "["+VERSION(0)+"]", TYPE("VERSION(0)"), LEN(VERSION(0))
? "one", "["+VERSION(1)+"]", TYPE("VERSION(1)"), LEN(VERSION(1))
? "two", "["+VERSION(2)+"]", TYPE("VERSION(2)"), LEN(VERSION(2))
? "wrong-type"
? VERSION("X")
? "logical-type"
? VERSION(.T.)
? "extra-args"
? VERSION(1,2)
? "[FNS-END]"
RETURN
