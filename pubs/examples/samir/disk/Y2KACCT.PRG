* Y2KACCT.PRG -- Initech Accounting System: Open Invoice Aging Report
* Initech Systems Corporation, Finance and Accounting.
* Module AR-AGING.  Database: INVOICE.DBF in USE.
* The reporting date is the memory variable ASOF, set at the dot prompt
* before the report is run, for example
*     . ASOF = CTOD('01/31/00')
* Dates are keyed and stored in the standard MM/DD/YY form.  An invoice
* more than 30 days past due is flagged OVERDUE, and the unpaid overdue
* amounts are totalled for the Collections summary.
? 'INITECH SYSTEMS CORP -- ACCOUNTS RECEIVABLE'
? 'OPEN INVOICE AGING REPORT'
? 'RUN DATE: ' + DTOC(ASOF)
? '-----------------------------------------------'
? 'INVNO  DUE DATE   AGE(DAYS)  STATUS'
OVERTOT = 0
GO TOP
DO WHILE .NOT. EOF()
  AGE = ASOF - DUEDATE
  STAT = IIF(AGE > 30, 'OVERDUE', 'CURRENT')
  ? INVNO + '  ' + DTOC(DUEDATE) + '  ' + STR(AGE, 6, 0) + '   ' + STAT
  IF AGE > 30 .AND. .NOT. PAID
    OVERTOT = OVERTOT + AMOUNT
  ENDIF
  SKIP
ENDDO
? '-----------------------------------------------'
? 'TOTAL UNPAID OVERDUE: ' + STR(OVERTOT, 10, 2)
