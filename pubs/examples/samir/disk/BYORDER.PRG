* BYORDER.PRG -- One roster, two orders
* Initech Systems Corporation.  Sample program for Programming with
* InitechBase, Chapter 20.  Database: STAFF.DBF in USE with the indexes
* STAFFNAM.NDX and STAFFSAL.NDX, in that order.  Prints the first three
* employees by name, then the three highest salaries.
SET ORDER TO 1
GO TOP
? 'FIRST BY NAME'
MCOUNT = 0
DO WHILE MCOUNT < 3 .AND. .NOT. EOF()
  ? '  ' + LASTNAME
  MCOUNT = MCOUNT + 1
  SKIP
ENDDO
SET ORDER TO 2
GO BOTTOM
? 'HIGHEST SALARIES'
MCOUNT = 0
DO WHILE MCOUNT < 3 .AND. .NOT. BOF()
  ? '  ' + LASTNAME + STR(SALARY, 12, 2)
  MCOUNT = MCOUNT + 1
  SKIP -1
ENDDO
SET ORDER TO 1
RETURN
