* NEWINV.PRG -- Initech Accounting System: Enter a New Invoice
* Initech Systems Corporation, Finance and Accounting.
* Database: INVOICE.DBF in USE with the index INVNO.NDX.  Asks for the
* invoice, refuses a number already in the ledger, and adds the record.
ACCEPT 'Invoice number: ' TO MINVNO
MINVNO = UPPER(MINVNO)
SEEK MINVNO
IF FOUND()
  ? 'INVOICE ' + MINVNO + ' IS ALREADY IN THE LEDGER'
  RETURN
ENDIF
ACCEPT 'Customer: ' TO MCUST
INPUT 'Amount: ' TO MAMOUNT
ACCEPT 'Due date (MM/DD/YY): ' TO MDUE
APPEND BLANK
REPLACE INVNO WITH MINVNO, CUST WITH UPPER(MCUST)
REPLACE AMOUNT WITH MAMOUNT, DUEDATE WITH CTOD(MDUE), PAID WITH .F.
? 'INVOICE ' + MINVNO + ' ENTERED AS RECORD ' + STR(RECNO(), 4)
RETURN
