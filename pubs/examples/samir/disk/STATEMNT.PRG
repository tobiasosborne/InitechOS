* STATEMNT.PRG -- Initech Accounting System: Customer Statement
* Initech Systems Corporation, Finance and Accounting.
* Database: INVOICE.DBF in USE with the index INVCUST.NDX.  Asks for a
* customer and prints the statement: a heading, one line for each open
* invoice, and the balance.  Each part is a procedure.
ACCEPT 'Customer: ' TO MCUST
MCUST = UPPER(MCUST)
SEEK MCUST
IF .NOT. FOUND()
  ? 'NO INVOICES FOR ' + MCUST
  RETURN
ENDIF
MBALANCE = 0
DO HEADER
DO WHILE CUST = MCUST .AND. .NOT. EOF()
  IF .NOT. PAID
    DO LINE
    MBALANCE = MBALANCE + AMOUNT
  ENDIF
  SKIP
ENDDO
DO FOOTER WITH MBALANCE
RETURN

PROCEDURE HEADER
? 'INITECH SYSTEMS CORPORATION -- STATEMENT OF ACCOUNT'
? 'CUSTOMER: ' + CUST
? REPLICATE('-', 40)
RETURN

PROCEDURE LINE
? INVNO + '   DUE ' + DTOC(DUEDATE) + '   ' + STR(AMOUNT, 10, 2)
RETURN

PROCEDURE FOOTER
PARAMETERS PBALANCE
? REPLICATE('-', 40)
? 'BALANCE DUE' + SPACE(19) + STR(PBALANCE, 10, 2)
RETURN
