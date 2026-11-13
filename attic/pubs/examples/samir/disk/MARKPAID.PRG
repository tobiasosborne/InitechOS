* MARKPAID.PRG -- Initech Accounting System: Record a Payment
* Initech Systems Corporation, Finance and Accounting.
* Database: INVOICE.DBF in USE with the index INVNO.NDX.  Asks for the
* invoice paid, shows it, asks for confirmation, and marks it paid.
ACCEPT 'Invoice paid: ' TO MINVNO
MINVNO = UPPER(MINVNO)
SEEK MINVNO
IF .NOT. FOUND()
  ? 'NO INVOICE ' + MINVNO
  RETURN
ENDIF
IF PAID
  ? 'INVOICE ' + MINVNO + ' IS ALREADY PAID'
  RETURN
ENDIF
? INVNO + '  ' + CUST + '  ' + STR(AMOUNT, 10, 2)
?
WAIT 'Mark this invoice paid (Y/N)? ' TO MANSWER
IF UPPER(MANSWER) = 'Y'
  REPLACE PAID WITH .T.
  ? 'INVOICE ' + MINVNO + ' MARKED PAID'
ELSE
  ? 'NOTHING CHANGED'
ENDIF
RETURN
