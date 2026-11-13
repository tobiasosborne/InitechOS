* RAISEDPT.PRG -- Salary raises by department
* Initech Systems Corporation.  Sample program for Programming with
* InitechBase, Chapter 18.  Database: STAFF.DBF in USE.  Each department
* receives its own percentage; the program shows each change and the total
* added to the payroll.  Run it on a copy of the database.
MADDED = 0
GO TOP
DO WHILE .NOT. EOF()
  DO CASE
  CASE DEPT = 'SOFT'
    MPCT = 3
  CASE DEPT = 'CUST'
    MPCT = 4
  OTHERWISE
    MPCT = 2
  ENDCASE
  MOLD = SALARY
  REPLACE SALARY WITH ROUND(SALARY * (1 + MPCT / 100), 2)
  MADDED = MADDED + SALARY - MOLD
  ? LASTNAME + STR(MPCT, 3) + '%' + STR(MOLD, 12, 2) + STR(SALARY, 12, 2)
  SKIP
ENDDO
? 'ADDED TO THE PAYROLL' + STR(MADDED, 12, 2)
RETURN
