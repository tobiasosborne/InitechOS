* INTPOST.PRG -- Initech Accounting System: Monthly Finance-Charge Posting
* Initech Systems Corporation, Finance and Accounting.
* Module AR-INTPOST.  Database: INVOICE.DBF in USE.
* Before the run, set the monthly rate RATE (0.015 is one and one half
* per cent) and SCALE, the decimal places of the billing statement:
*     . RATE = 0.015
*     . SCALE = 0
* Each open invoice is charged AMOUNT * RATE, posted rounded to SCALE
* places; the rounding adjustment of every posting is swept into the
* BOLTON suspense account, so that the ledger foots at close.
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
