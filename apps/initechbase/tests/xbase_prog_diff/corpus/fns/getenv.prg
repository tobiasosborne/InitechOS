* Ref: ../dbase3-decomp/specs/functions/system-and-database-functions.md
* MINT-ERRORS: 11 11 11
? "[FNS-BEGIN]"
? "COMSPEC", "["+GETENV("COMSPEC")+"]", TYPE('GETENV("COMSPEC")'), LEN(GETENV("COMSPEC"))
? "NOSUCHVAR", "["+GETENV("NOSUCHVAR")+"]", TYPE('GETENV("NOSUCHVAR")'), LEN(GETENV("NOSUCHVAR"))
? "empty-name", "["+GETENV("")+"]"
? "wrong-type"
? GETENV(1)
? "missing-arg"
? GETENV()
? "extra-arg"
? GETENV("COMSPEC","X")
? "[FNS-END]"
RETURN
