* CUSTTOT.PRG -- Initech Accounting System: Totals by Customer
* Initech Systems Corporation, Finance and Accounting.
* Database: INVOICE.DBF in USE with the index INVCUST.NDX first.  The
* invoices of each customer are together in the index; the inner loop
* totals one customer while CUST stays the same.
GO TOP
DO WHILE .NOT. EOF()
  MCUST = CUST
  STORE 0 TO MTOTAL, MCOUNT
  DO WHILE CUST = MCUST .AND. .NOT. EOF()
    MTOTAL = MTOTAL + AMOUNT
    MCOUNT = MCOUNT + 1
    SKIP
  ENDDO
  ? MCUST + STR(MCOUNT, 4) + STR(MTOTAL, 12, 2)
ENDDO
RETURN
