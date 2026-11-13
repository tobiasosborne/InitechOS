* ACCTMAIN.PRG -- Initech Accounting System: Main Menu
* Initech Systems Corporation, Finance and Accounting.
* Database: INVOICE.DBF in USE with the index INVNO.NDX.  Offers the daily
* tasks of the accounts receivable clerk until the clerk chooses 0.  The
* month-end programs Y2KACCT and INTPOST are run from the dot prompt.
SET TALK OFF
DO WHILE .T.
  ?
  ? 'INITECH ACCOUNTING SYSTEM -- ACCOUNTS RECEIVABLE'
  ? '  1  List the ledger'
  ? '  2  List the open invoices'
  ? '  3  Print the invoice register'
  ? '  0  Return to the dot prompt'
  ?
  WAIT 'Choice: ' TO MCHOICE
  DO CASE
  CASE MCHOICE = '1'
    DO LEDGER
  CASE MCHOICE = '2'
    DO OPENLIST
  CASE MCHOICE = '3'
    REPORT FORM INVRPT PLAIN
  CASE MCHOICE = '0'
    EXIT
  OTHERWISE
    ? 'PLEASE CHOOSE 0, 1, 2 OR 3'
  ENDCASE
ENDDO
RETURN

PROCEDURE LEDGER
GO TOP
LIST INVNO, CUST, AMOUNT, DUEDATE, PAID
RETURN

PROCEDURE OPENLIST
GO TOP
DO WHILE .NOT. EOF()
  IF .NOT. PAID
    ? INVNO + '  ' + CUST + '  ' + STR(AMOUNT, 10, 2)
  ENDIF
  SKIP
ENDDO
RETURN
