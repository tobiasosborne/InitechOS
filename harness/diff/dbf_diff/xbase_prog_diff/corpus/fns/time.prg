* Ref: ../dbase3-decomp/specs/functions/system-and-database-functions.md
* MINT-ERRORS: 11
? "[FNS-BEGIN]"
? TYPE("TIME()"), LEN(TIME()), SUBSTR(TIME(),3,1), SUBSTR(TIME(),6,1)
STORE TIME() TO CLOCKVAL
? "shape", TYPE("CLOCKVAL"), LEN(CLOCKVAL), SUBSTR(CLOCKVAL,3,1)=":", SUBSTR(CLOCKVAL,6,1)=":"
? "hour", VAL(LEFT(CLOCKVAL,2))>=0, VAL(LEFT(CLOCKVAL,2))<24
? "minute", VAL(SUBSTR(CLOCKVAL,4,2))>=0, VAL(SUBSTR(CLOCKVAL,4,2))<60
? "second", VAL(RIGHT(CLOCKVAL,2))>=0, VAL(RIGHT(CLOCKVAL,2))<60
? "extra-arg"
? TIME(1)
? "[FNS-END]"
RETURN
