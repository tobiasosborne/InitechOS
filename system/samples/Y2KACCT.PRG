* Initech Systems Corporation -- Accounts Receivable
* Invoice aging, month-end run. Open INVOICE before DO Y2KACCT.
STORE CTOD('01/31/00') TO ASOF
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
