* PAYTOTAL.PRG -- Payroll totals, exempt and non-exempt staff
* Initech Systems Corporation.  Sample program for Using InitechBase.
* Use with STAFF.DBF in USE.  Deleted records are left out.
STORE 0 TO MEXEMPT, MHOURLY, MCOUNT
GO TOP
DO WHILE .NOT. EOF()
  IF DELETED()
    SKIP
    LOOP
  ENDIF
  IF EXEMPT
    MEXEMPT = MEXEMPT + SALARY
  ELSE
    MHOURLY = MHOURLY + SALARY
  ENDIF
  MCOUNT = MCOUNT + 1
  SKIP
ENDDO
? 'EXEMPT STAFF     ' + STR(MEXEMPT, 10, 2)
? 'NON-EXEMPT STAFF ' + STR(MHOURLY, 10, 2)
? 'TOTAL PAYROLL    ' + STR(MEXEMPT + MHOURLY, 10, 2)
? 'EMPLOYEES        ' + STR(MCOUNT, 10)
RETURN
