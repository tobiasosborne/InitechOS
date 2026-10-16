* OPENINV.PRG -- Initech Accounting System: Open Invoice List
* Initech Systems Corporation, Finance and Accounting.
* Database: INVOICE.DBF in USE.  Lists the invoices not yet paid and
* counts them.
? 'OPEN INVOICES'
? 'INVNO  CUSTOMER        AMOUNT'
MCOUNT = 0
GO TOP
DO WHILE .NOT. EOF()
  IF .NOT. PAID
    ? INVNO + '  ' + CUST + '  ' + STR(AMOUNT, 10, 2)
    MCOUNT = MCOUNT + 1
  ENDIF
  SKIP
ENDDO
? STR(MCOUNT, 3) + ' OPEN INVOICES'
RETURN
