* STAFFLST.PRG -- Staff list for one department
* Initech Systems Corporation.  Sample program for Learning InitechBase.
* Use with STAFF.DBF in USE.  Asks for a department code and lists the
* staff of that department with their extensions.
ACCEPT 'Department code: ' TO MDEPT
MDEPT = UPPER(MDEPT)
? 'STAFF OF DEPARTMENT ' + MDEPT
? '-----------------------------'
MCOUNT = 0
GO TOP
DO WHILE .NOT. EOF()
  IF DEPT = MDEPT
    ? LASTNAME, FIRSTNAME, EXT
    MCOUNT = MCOUNT + 1
  ENDIF
  SKIP
ENDDO
? '-----------------------------'
? STR(MCOUNT, 3) + ' EMPLOYEES'
RETURN
