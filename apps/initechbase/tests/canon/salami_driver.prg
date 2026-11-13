*-----------------------------------------------------------------------------
* SALAMI.PRG -- Bolton's salami-slicing rounding-error routine, SELF-DRIVING
*               build for InitechOS.
*
* This is the canon finance-charge posting program (canon/salami.prg, bead
* 586.2 -- the enforced "too much too fast" misplaced-decimal skim) with TWO
* added leading driver lines so it runs straight from the dot prompt by typing
* `DO SALAMI` -- no operator-typed RATE/SCALE needed.
*
* WHY DRIVER LINES (initech-4hte): the in-emulator harness can only inject the
* QMP key vocabulary (a-z, 0-9, space, return, dot) -- it CANNOT type '='. So
* the canon test's operator step (RATE = 0.015 / SCALE = 0, set on the same
* interp by test_canon_salami.c set_params) cannot be keyed at the dot prompt.
* We bake the SAME two memvars into the program as STOREs (the canon harness
* set_params sets RATE/SCALE the identical way before the body). A standalone
* finance-charge routine that predefines its own posting parameters is authentic
* dBASE -- exactly the y2kacct_driver.prg pattern (which bakes its ASOF STORE).
*
* THE SALAMI BUG IS ENFORCED (Law 4): SCALE is keyed 0 (the misplaced decimal --
* whole-dollar rounding precision) instead of the correct 2 (cents). So POSTED
* rounds each charge to the nearest DOLLAR and ADJUST sweeps the entire sub-
* DOLLAR remainder (|ADJUST| up to 0.5, 100x too large per posting) into the
* hidden BOLTON suspense account -- which balloons ~100x faster than a sub-cent
* skim ever could. That is the "too much too fast" panic. RATE = 0.015 (1.5%/mo)
* is the standard finance charge (fixed in both builds; only SCALE carries the
* misplaced decimal). The host gate test-canon-salami sets RATE/SCALE identically
* and asserts the SAME buggy skim (A1004 sweeps 0.4998; BOLTON balloons to 0.38).
*
* The body below (everything after the two STORE lines) is byte-for-byte the
* canon/salami.prg body -- do NOT "fix" it (Law 4: canon is enforced, not fixed).
* Ref: harness/diff/dbf_diff/canon/salami.prg + .out (the golden the host gate
*      test-canon-salami asserts); test_canon_salami.c set_params (the same
*      RATE=0.015 / SCALE=0); canon/y2kacct_driver.prg (the sibling self-driver).
*-----------------------------------------------------------------------------
STORE 0.015 TO RATE
STORE 0 TO SCALE
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
