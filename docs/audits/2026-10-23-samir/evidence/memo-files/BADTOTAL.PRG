* BADTOTAL.PRG -- Ledger total, with a misspelled field name
* Initech Systems Corporation.  Sample program for Programming with
* InitechBase, Chapter 12 (Handling Errors) and Chapter 13 (Testing).
* Database: INVOICE.DBF in USE.  The field is AMOUNT, not AMOUNTS.
MTOTAL = 0
GO TOP
DO WHILE .NOT. EOF()
  MTOTAL = MTOTAL + AMOUNTS
  SKIP
ENDDO
? 'LEDGER TOTAL ' + STR(MTOTAL, 10, 2)
RETURN
