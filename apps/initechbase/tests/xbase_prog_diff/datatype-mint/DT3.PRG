SET TALK OFF
SET SAFETY OFF
ON ERROR ? "  ->ERR#" + LTRIM(STR(ERROR())) + " " + MESSAGE()
SET ALTERNATE TO DT3LOG
SET ALTERNATE ON
USE E5
GO 1
x = SMALL
? "G1 [" + STR(x/1E98,12,3) + "]"
? "G2 ", x = 1E100
? "G3 ", x > 9.99E99, x < 1.001E100
? "G4 [" + STR(VAL("****")/1E98,12,3) + "]"
? "G5 [" + STR(VAL("1***"),12,3) + "]"
? "G6 [" + STR(-x/1E98,12,3) + "]"
? "G7 ", TYPE("SMALL")
GO 3
REPLACE WHEN WITH CTOD("  /  /  ")
? "G8 ", TYPE("WHEN"), WHEN
? "G9 ", WHEN > CTOD("12/31/99"), YEAR(WHEN), MONTH(WHEN)
? "G10 [" + DTOC(WHEN + 1) + "]"
? "G11 [", CTOD("  /  /  "), "]"
? "G12 [" + TRANSFORM(WHEN, "@D") + "]"
USE
SET ALTERNATE TO
QUIT
