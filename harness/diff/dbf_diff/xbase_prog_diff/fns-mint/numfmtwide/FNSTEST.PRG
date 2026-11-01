* MINT-ERRORS: 63 63 63 63 63 63 63 63 63
* STR matrix: all widths 1..20, decimals 0/2/9/15, signs and rounding edges.
* General-display counterparts are independently retained in numdisplay.
? "[FNS-BEGIN]"
SET DECIMALS TO 15
STORE 11 TO W
DO WHILE W <= 20
STORE 0 TO D
DO WHILE D <= 15
? "S"+LTRIM(STR(W,2,0))+":"+LTRIM(STR(D,2,0))+":10^20", "["+STR(10^20,W,D)+"]"
? "S"+LTRIM(STR(W,2,0))+":"+LTRIM(STR(D,2,0))+":-(10^20)", "["+STR(-(10^20),W,D)+"]"
? "S"+LTRIM(STR(W,2,0))+":"+LTRIM(STR(D,2,0))+":SQRT(.999999999)", "["+STR(SQRT(.999999999),W,D)+"]"
? "S"+LTRIM(STR(W,2,0))+":"+LTRIM(STR(D,2,0))+":9.995", "["+STR(9.995,W,D)+"]"
? "S"+LTRIM(STR(W,2,0))+":"+LTRIM(STR(D,2,0))+":-9.995", "["+STR(-9.995,W,D)+"]"
DO CASE
CASE D = 0
STORE 2 TO D
CASE D = 2
STORE 9 TO D
CASE D = 9
STORE 15 TO D
OTHERWISE
STORE 16 TO D
ENDCASE
ENDDO
STORE W + 1 TO W
ENDDO
? "[FNS-END]"
RETURN
