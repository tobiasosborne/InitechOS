* FIRSTBIG.PRG -- Initech Accounting System: First Large Open Invoice
* Initech Systems Corporation, Finance and Accounting.
* Database: INVOICE.DBF in USE.  Finds the first open invoice of 1000 or
* more; paid invoices are passed over with LOOP, and the search ends with
* EXIT when the invoice is found.
MFOUND = .F.
GO TOP
DO WHILE .NOT. EOF()
  IF PAID
    SKIP
    LOOP
  ENDIF
  IF AMOUNT >= 1000
    MFOUND = .T.
    EXIT
  ENDIF
  SKIP
ENDDO
IF MFOUND
  ? 'FIRST LARGE OPEN INVOICE: ' + INVNO + '  ' + CUST
ELSE
  ? 'NO LARGE OPEN INVOICE'
ENDIF
RETURN
