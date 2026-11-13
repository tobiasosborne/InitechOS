* INVFMT.PRG -- Initech Accounting System: Formatted Invoice Register
* Initech Systems Corporation, Finance and Accounting.
* Database: INVOICE.DBF in USE.  One line per invoice, in fixed columns.
? 'INVOICE  CUSTOMER    DUE         AMOUNT  STATUS'
GO TOP
DO WHILE .NOT. EOF()
  MSTAT = IIF(PAID, 'PAID', 'OPEN')
  MLINE = INVNO + '    ' + CUST + '  ' + DTOC(DUEDATE)
  ? MLINE + '  ' + TRANSFORM(AMOUNT, '999999.99') + '  ' + MSTAT
  SKIP
ENDDO
RETURN
