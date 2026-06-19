*-----------------------------------------------------------------------------
* Y2KACCT.PRG -- Initech AR aging report, SELF-DRIVING build for InitechOS.
*
* This is the canon AR-aging program (canon/y2k_accounting.prg, bead 586.1, the
* enforced Year-2000 bug) with ONE added leading driver line so it runs straight
* from the dot prompt by typing `DO Y2KACCT` -- no operator-typed ASOF needed.
*
* WHY A DRIVER LINE (initech-9a0f): the in-emulator harness can only inject the
* QMP key vocabulary (a-z, 0-9, space, return) -- it CANNOT type '=', '(', ')',
* "'", '/'. So the canon test's operator step `ASOF = CTOD('01/31/00')` cannot be
* keyed at the dot prompt. We instead bake the SAME step into the program as a
* STORE (the canon harness sets ASOF the identical way; test_canon_y2k.c set_asof
* runs `ASOF = CTOD('01/31/00')` on the same interp before the body). A standalone
* report that sets its own reporting "as of" date is authentic dBASE.
*
* THE Y2K BUG IS ENFORCED (Law 4): ASOF is keyed two-digit '01/31/00', which CTOD
* parses BASE-1900 (SET CENTURY OFF default) -> 1900-01-31, NOT 2000-01-31; and
* the year-2000 DUEDATE values on INVOICE.DBF are stored base-1900 too (the mint
* mirrors the canon harness make_invoices). The aging arithmetic is therefore
* wrong by ~100 years for the 1999 invoices: they mis-age to ~ -36000 days,
* mislabel CURRENT, and the TOTAL UNPAID OVERDUE wrongly reports 0.00.
*
* The body below is byte-for-byte the canon/y2k_accounting.prg body (lines after
* the comment banner) -- do NOT "fix" it (Law 4: canon is enforced, not fixed).
* Ref: harness/diff/dbf_diff/canon/y2k_accounting.prg + .out (the golden the host
*      gate test-canon-y2k asserts); test_canon_y2k.c set_asof (the same ASOF).
*-----------------------------------------------------------------------------
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
