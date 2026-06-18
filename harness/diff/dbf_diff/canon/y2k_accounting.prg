*-----------------------------------------------------------------------------
* Y2KACCT.PRG -- Initech Accounts Receivable: Invoice Aging Report
*
* Initech Systems Corporation -- Finance & Accounting Section
* Module:   AR-AGING (receivables aging / overdue dunning support)
* Database: INVOICE.DBF (the open accounts-receivable ledger)
* Author:   Accounting Systems Group
*
* PURPOSE
*   Produces the open-invoice aging report the Collections desk runs at
*   month-end. For each open invoice it computes the age in days against the
*   reporting "as of" date (memory variable ASOF, supplied by the operator at
*   the dot prompt or by the nightly batch from the system date), flags any
*   invoice more than 30 days past due as OVERDUE, and accumulates the total
*   unpaid overdue balance for the Collections summary.
*
* INVOICE.DBF SCHEMA (the reusable accounts-receivable ledger -- see note below)
*   INVNO    C(5)     Invoice number (e.g. "A1001")
*   CUST     C(10)    Customer / account name
*   AMOUNT   N(10,2)  Invoice amount, dollars and cents
*   DUEDATE  D        Payment due date
*   PAID     L        .T. once the invoice has been settled
*
*   Schema note (for downstream payroll/disbursement modules that share this
*   ledger): the five fields above are fixed; new modules append fields, they
*   do not renumber these. DUEDATE is a native dBASE Date; dates are keyed in
*   by the clerks in the standard MM/DD/YY form and stored via CTOD at the data
*   entry screen, exactly as the rest of the shop has always done.
*
* REPORTING DATE (ASOF)
*   The driver / operator sets ASOF before running this report, e.g.
*       . ASOF = CTOD('01/31/00')
*   so the report can be re-run "as of" any month-end. The aging math is a
*   plain Date subtraction: AGE = ASOF - DUEDATE yields whole days.
*
* Ref (Law 1):
*   - ../dbase3-decomp/specs/functions/numeric-and-date-functions.md
*       (CTOD parses MM/DD/YY; DTOC renders MM/DD/YY under SET CENTURY OFF;
*        Date arithmetic D - D -> N days; YEAR/DAY/MONTH.)
*   - ../dbase3-decomp/specs/runtime/dates-and-century.md
*       (SET CENTURY OFF is the default; the stored / displayed year is the
*        two low-order digits; the two-digit data-entry year is base-1900.)
*   - ../dbase3-decomp/specs/commands/navigation-query-display.md
*       (GO TOP / SKIP / EOF() record walk; ? expression display.)
*   - ../dbase3-decomp/specs/commands/control-flow-and-procedures.md
*       (DO WHILE / ENDDO, IF / ENDIF, IIF.)
*-----------------------------------------------------------------------------
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
