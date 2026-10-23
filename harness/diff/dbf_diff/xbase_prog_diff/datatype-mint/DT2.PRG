SET TALK OFF
SET SAFETY OFF
ON ERROR ? "  ->ERR#" + LTRIM(STR(ERROR())) + " " + MESSAGE()
SET ALTERNATE TO DT2LOG
SET ALTERNATE ON
? "[E] start"
USE E4
GO 1
REPLACE WHEN WITH CTOD("  /  /  ")
? "E1 [" + DTOC(WHEN) + "]"
GO 2
REPLACE WHEN WITH CTOD("")
? "E2 [" + DTOC(WHEN) + "]"
GO 3
REPLACE WHEN WITH CTOD("13/45/92")
? "E3 [" + DTOC(WHEN) + "]"
? "E4 [" + DTOC(CTOD("04/31/91")) + "]"
? "E5 [" + DTOC(CTOD("02/29/91")) + "]"
? "E6 [" + DTOC(CTOD("00/10/91")) + "]"
? "E7 [" + DTOC(CTOD("12/00/91")) + "]"
USE
? "[F] start"
USE E5
GO 1
REPLACE SMALL WITH 99999
USE
USE E5
GO 1
? "F1 [" + STR(SMALL,25,3) + "]"
? "F2 ", SMALL > 1000000
? "F3 ", SMALL > 99999
? "F4 ", SMALL < 0
x = SMALL
? "F5 [", x, "]"
? "F6 [", x + 1, "]"
? "F7 [" + TRANSFORM(x,"999999") + "]"
? "F8 [", VAL("****"), "]"
? "F9 [", x * 0, "]"
? "F10 [" + STR(x/1E90,10,3) + "]"
REPLACE ID WITH SMALL
? "F11 [", ID, "]"
GO 2
REPLACE SMALL WITH -999
? "F12 [", SMALL, "]"
GO 3
REPLACE SMALL WITH 999.95
? "F13 [", SMALL, "]"
USE
? "[Z] done"
SET ALTERNATE TO
QUIT
