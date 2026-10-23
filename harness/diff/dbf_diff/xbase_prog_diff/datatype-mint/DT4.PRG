SET TALK OFF
SET SAFETY OFF
ON ERROR ? "  ->ERR#" + LTRIM(STR(ERROR())) + " " + MESSAGE()
SET ALTERNATE TO DT4LOG
SET ALTERNATE ON
USE E5
GO 1
x = SMALL
e = 10^98
? "H0 [" + STR(e/10^97,12,3) + "]"
? "H1 [" + STR(x/e,12,3) + "]"
? "H2 ", x = e*100
? "H3 [" + STR(VAL("****")/e,12,3) + "]"
? "H4 [" + STR(VAL("*")/e,12,3) + "]"
? "H5 [" + STR(x/(e*100),12,9) + "]"
? "H6 [" + STR(x/1000000,12,3) + "]"
USE
SET ALTERNATE TO
QUIT
