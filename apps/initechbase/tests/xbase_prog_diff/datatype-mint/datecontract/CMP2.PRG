SET TALK OFF
SET SAFETY OFF
ON ERROR ? "MINT-ERROR", ERROR(), MESSAGE()
SET ALTERNATE TO CMP2LOG
SET ALTERNATE ON
? "N01 ", 1 < 2, 2 > 1, 1 = 2, 1 = 1
? "D01 ", CTOD("08/05/85") < CTOD("09/07/85")
? "D02 ", CTOD("08/05/85") > CTOD("09/07/85")
? "D03 ", CTOD("08/05/85") = CTOD("09/07/85")
? "D04 ", CTOD("08/05/85") = CTOD("08/05/85")
? "B01 ", CTOD("  /  /  ") < CTOD("09/07/85")
? "B02 ", CTOD("  /  /  ") > CTOD("09/07/85")
? "B03 ", CTOD("09/07/85") < CTOD("  /  /  ")
? "B04 ", CTOD("09/07/85") > CTOD("  /  /  ")
? "B05 ", CTOD("  /  /  ") = CTOD("09/07/85")
? "B06 ", CTOD("  /  /  ") <> CTOD("09/07/85")
? "B07 ", CTOD("  /  /  ") < CTOD("  /  /  ")
? "B08 ", CTOD("  /  /  ") = CTOD("  /  /  ")
SET ALTERNATE TO
QUIT
