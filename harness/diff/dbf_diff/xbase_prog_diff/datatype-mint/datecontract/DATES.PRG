SET TALK OFF
SET SAFETY OFF
ON ERROR ? "MINT-ERROR", ERROR(), MESSAGE()
SET ALTERNATE TO DATELOG
SET ALTERNATE ON
SET DATE AMERICAN
SET CENTURY OFF
? "B01 ["+DTOC(CTOD(""))+"]"
? "B02 ["+DTOC(CTOD("  /  /  "))+"]"
SET CENTURY ON
? "B03 ["+DTOC(CTOD(""))+"]"
? "C01 ["+DTOC(CTOD("02/30/26"))+"]"
? "C02 ["+DTOC(CTOD("02/31/90"))+"]"
? "C03 ["+DTOC(CTOD("04/31/85"))+"]"
? "C04 ["+DTOC(CTOD("02/29/85"))+"]"
? "C05 ["+DTOC(CTOD("02/30/92"))+"]"
? "C06 ["+DTOC(CTOD("02/29/88"))+"]"
? "C07 ["+DTOC(CTOD("13/40/85"))+"]"
? "C08 ["+DTOC(CTOD("00/10/91"))+"]"
? "C09 ["+DTOC(CTOD("12/00/91"))+"]"
? "C10 ["+DTOC(CTOD("13/01/85"))+"]"
? "C11 ["+DTOC(CTOD("01/32/85"))+"]"
? "C12 ["+DTOC(CTOD("garbage"))+"]"
B = CTOD("")
D = CTOD("09/01/85")
? "L01 ", D < B
? "L02 ", B > D
? "L03 ", B < D
? "L04 ", D > B
? "L05 ", B <= D
? "L06 ", D >= B
? "L07 ", B >= D
? "L08 ", D <= B
? "L09 ", B = CTOD("")
? "L10 ", B <> D
? "L11 ", B < CTOD("")
? "L12 ", B <= CTOD("")
SET CENTURY OFF
? "C13 ["+DTOC(CTOD("02/30/92"))+"]"
SET ALTERNATE TO
QUIT
