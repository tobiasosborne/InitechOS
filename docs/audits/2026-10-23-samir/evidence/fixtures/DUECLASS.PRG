* DUECLASS.PRG -- Initech Accounting System: Invoices by Size
* Initech Systems Corporation, Finance and Accounting.
* Database: INVOICE.DBF in USE.  Counts the invoices in three classes of
* amount: under 100, under 1000, and 1000 or more.
STORE 0 TO MSMALL, MMEDIUM, MLARGE
GO TOP
DO WHILE .NOT. EOF()
  DO CASE
  CASE AMOUNT < 100
    MSMALL = MSMALL + 1
  CASE AMOUNT < 1000
    MMEDIUM = MMEDIUM + 1
  OTHERWISE
    MLARGE = MLARGE + 1
  ENDCASE
  SKIP
ENDDO
? 'UNDER 100     ' + STR(MSMALL, 3)
? 'UNDER 1000    ' + STR(MMEDIUM, 3)
? '1000 OR MORE  ' + STR(MLARGE, 3)
RETURN
