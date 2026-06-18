*-----------------------------------------------------------------------------
* SALAMI.PRG -- Initech Accounts Receivable: Monthly Interest Posting &
*               Rounding-Adjustment Sweep
*
* Initech Systems Corporation -- Finance & Accounting Section
* Module:   AR-INTPOST (finance-charge posting / rounding adjustment)
* Database: INVOICE.DBF (the open accounts-receivable ledger -- shared S7.1)
* Author:   Accounting Systems Group (per M. Bolton, maintenance)
*
* PURPOSE
*   Posts the month-end finance charge to every open (unpaid) invoice in the
*   accounts-receivable ledger. For each invoice it computes the finance
*   charge as AMOUNT * the monthly rate, posts the charge to the customer
*   account, and sweeps the small rounding adjustment that arises because the
*   ledger carries dollars-and-cents while the charge is computed to full
*   precision. The rounding adjustment is accumulated into the corporate
*   rounding-adjustment account (the "BOLTON" suspense account) so the ledger
*   foots to the penny at close, exactly as the shop's general-ledger policy
*   requires.
*
* INVOICE.DBF SCHEMA (the reusable accounts-receivable ledger -- S7.1)
*   INVNO    C(5)     Invoice number (e.g. "A1001")
*   CUST     C(10)    Customer / account name
*   AMOUNT   N(10,2)  Invoice amount, dollars and cents
*   DUEDATE  D        Payment due date
*   PAID     L        .T. once the invoice has been settled
*
* THE MONTHLY RATE
*   The driver / operator sets RATE before running this routine, e.g.
*       . RATE = 0.015
*   (one and one-half percent per month, the standard finance charge). The
*   posting math is plain: CHARGE = AMOUNT * RATE, carried to full precision,
*   then posted rounded to the cent the customer is billed.
*
* THE ROUNDING ADJUSTMENT
*   The customer is posted the charge rounded to the billing precision the
*   statement prints (the SCALE memvar -- number of decimal places carried on
*   the statement); the remainder below that precision is the rounding
*   adjustment swept to the BOLTON suspense account so the books foot. Over
*   the run the suspense account collects every account's adjustment into one
*   tidy total for the month-end journal entry. The driver / operator sets
*   SCALE before running, matching the billing-system statement precision.
*
* Ref (Law 1):
*   - ../dbase3-decomp/specs/functions/numeric-and-date-functions.md
*       (ROUND to N places; STR fixed-format render; numeric * and -.)
*   - os/samir/core/rt.c dec_format (round half toward +inf) +
*     os/samir/core/fn_builtins.c fn_round / fn_str (the posting + render math).
*   - ../dbase3-decomp/specs/commands/navigation-query-display.md
*       (GO TOP / SKIP / EOF() record walk; ? expression display.)
*   - ../dbase3-decomp/specs/commands/control-flow-and-procedures.md
*       (DO WHILE / ENDDO, IF / ENDIF.)
*-----------------------------------------------------------------------------
? 'INITECH SYSTEMS CORP -- ACCOUNTS RECEIVABLE'
? 'MONTHLY FINANCE-CHARGE POSTING'
? 'RATE: ' + STR(RATE * 100, 6, 2) + ' PCT/MO'
? '-----------------------------------------------'
? 'INVNO  CHARGE      POSTED      ADJUSTMENT'
BOLTON = 0
GO TOP
DO WHILE .NOT. EOF()
  IF .NOT. PAID
    CHARGE = AMOUNT * RATE
    POSTED = ROUND(CHARGE, SCALE)
    ADJUST = CHARGE - POSTED
    BOLTON = BOLTON + ADJUST
    ? INVNO + '  ' + STR(CHARGE, 10, 4) + '  ' + STR(POSTED, 10, 2) + '  ' + STR(ADJUST, 10, 4)
  ENDIF
  SKIP
ENDDO
? '-----------------------------------------------'
? 'BOLTON SUSPENSE ACCOUNT: ' + STR(BOLTON, 12, 2)
