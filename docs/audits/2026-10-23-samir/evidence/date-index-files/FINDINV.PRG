* FINDINV.PRG -- Initech Accounting System: Find an Invoice
* Initech Systems Corporation, Finance and Accounting.
* Database: INVOICE.DBF in USE with the index INVNO.NDX.  Asks for an
* invoice number and displays the invoice, or reports that there is none.
ACCEPT 'Invoice number: ' TO MINVNO
MINVNO = UPPER(MINVNO)
SEEK MINVNO
IF FOUND()
  ? 'CUSTOMER  ' + CUST
  ? 'AMOUNT    ' + STR(AMOUNT, 10, 2)
  ? 'DUE       ' + DTOC(DUEDATE)
  ? 'PAID      ' + IIF(PAID, 'YES', 'NO')
ELSE
  ? 'NO INVOICE ' + MINVNO
ENDIF
RETURN
