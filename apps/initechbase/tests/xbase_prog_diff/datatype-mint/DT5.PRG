SET TALK OFF
SET SAFETY OFF
ON ERROR ? "  ->ERR#" + LTRIM(STR(ERROR())) + " " + MESSAGE()
SET ALTERNATE TO DT5LOG
SET ALTERNATE ON
USE E5
GO 1
x = SMALL
e = 10^300
? "I0 [" + STR(e/10^299,12,3) + "]"
? "I1 [" + STR(x/e,12,3) + "]"
? "I2 ", x = x*2, x > e
? "I3 [" + STR(x-x,12,3) + "]"
? "I4 [" + STR(x/(e*10^8),12,3) + "]"
? "I5 ", x = 1/0
? "I6 [", 1/0, "]"
? "I7 ", x = VAL("****")
? "I8 [" + STR(x/(e*10^7),14,3) + "]"
USE
SET ALTERNATE TO
QUIT
