* AMTCHECK.PRG -- Asking until the answer is right
* Initech Systems Corporation.  Sample program for Programming with
* InitechBase, Chapter 6.  Asks for an amount from 1 to 999 and asks
* again until the answer is a number in that range.
MOK = .F.
DO WHILE .NOT. MOK
  INPUT 'Amount (1 to 999): ' TO MAMOUNT
  IF TYPE(MAMOUNT) = 'N'
    MOK = MAMOUNT >= 1 .AND. MAMOUNT <= 999
  ENDIF
  IF .NOT. MOK
    ? 'PLEASE TYPE A NUMBER FROM 1 TO 999'
    ?
  ENDIF
ENDDO
? 'AMOUNT ACCEPTED: ' + LTRIM(STR(MAMOUNT, 10, 2))
RETURN
