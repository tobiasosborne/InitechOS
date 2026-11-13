* INVTOTAL.PRG -- Initech Accounting System: Ledger Totals
* Initech Systems Corporation, Finance and Accounting.
* Database: INVOICE.DBF in USE.  Totals the invoices of the ledger and
* those paid.  The totals are PUBLIC: they remain after the program for
* the month-end journal.
PUBLIC MPAID, MTOTAL
STORE 0 TO MPAID, MTOTAL
GO TOP
DO WHILE .NOT. EOF()
  MTOTAL = MTOTAL + AMOUNT
  IF PAID
    MPAID = MPAID + AMOUNT
  ENDIF
  SKIP
ENDDO
? 'ALL INVOICES    ' + STR(MTOTAL, 10, 2)
? 'PAID INVOICES   ' + STR(MPAID, 10, 2)
RETURN
