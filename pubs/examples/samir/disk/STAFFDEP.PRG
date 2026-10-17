* STAFFDEP.PRG -- Staff list with department names
* Initech Systems Corporation.  Sample program for Programming with
* InitechBase, Chapter 14.  Databases: STAFF.DBF in work area 1 and
* DEPTS.DBF with DEPTS.NDX in work area 2.  For each employee, the
* department code is looked up in DEPTS and its name printed.
SELECT 1
GO TOP
DO WHILE .NOT. EOF()
  IF .NOT. DELETED()
    MDEPT = DEPT
    MNAME = TRIM(FIRSTNAME) + ' ' + LASTNAME
    SELECT 2
    SEEK MDEPT
    IF FOUND()
      MDNAME = DEPTNAME
    ELSE
      MDNAME = '(NO SUCH DEPARTMENT)'
    ENDIF
    SELECT 1
    ? LEFT(MNAME + SPACE(24), 24) + '  ' + MDNAME
  ENDIF
  SKIP
ENDDO
RETURN
