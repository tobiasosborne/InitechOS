SET TALK OFF
SET SAFETY OFF
ON ERROR ? "  ->ERR#" + LTRIM(STR(ERROR())) + " " + MESSAGE()
SET ALTERNATE TO DT1LOG
SET ALTERNATE ON
? "[A] start"
d = CTOD("02/30/92")
? "A1 [" + DTOC(d) + "]"
? "A2 ", d = CTOD("  /  /  ")
USE E1
GO 1
? "A3 [" + DTOC(WHEN) + "]"
REPLACE WHEN WITH CTOD("02/30/92")
? "A4 [" + DTOC(WHEN) + "]"
GO 2
REPLACE WHEN WITH d
? "A5 [" + DTOC(WHEN) + "]"
USE
? "[B] start"
USE E2
GO 1
? "B1 [", SMALL, "]"
? "B2 [" + STR(SMALL,10,3) + "]"
? "B3 ", SMALL = 0
GO 2
REPLACE SMALL WITH 99999
? "B4 [", SMALL, "]"
? "B5 ", SMALL = 0
DISPLAY ID, SMALL
LIST ID, SMALL
USE
USE E2
GO 2
? "B6 [", SMALL, "]"
USE
? "[C] start"
? "C1 [" + TRANSFORM(123.4,"999.99") + "]"
? "C2 [" + TRANSFORM(0.12,"9.99") + "]"
? "C3 [" + TRANSFORM(12.34,"999.99") + "]"
? "C4 [" + TRANSFORM(1234.5,"999.99") + "]"
? "C5 [" + TRANSFORM(123.456,"999.99") + "]"
? "C6 [" + TRANSFORM(-12.3,"999.99") + "]"
? "C7 [" + TRANSFORM(-123.4,"999.99") + "]"
? "C8 [" + TRANSFORM(999.995,"999.99") + "]"
? "C9 [" + TRANSFORM(5,"9.9") + "]"
? "C10 [" + TRANSFORM(12.5,"99") + "]"
? "C11 [" + TRANSFORM(1234.5,"9,999.99") + "]"
? "[D] start"
USE E3
INDEX ON WHEN TO BLANKDAT
? "D1 indexed"
LIST ID, WHEN
GO TOP
? "D2 top ", RECNO()
USE
? "[Z] done"
SET ALTERNATE TO
QUIT
