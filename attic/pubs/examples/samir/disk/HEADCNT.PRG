* HEADCNT.PRG -- Head count by department
* Initech Systems Corporation.  Sample program for Programming with
* InitechBase, Chapter 14.  Databases: DEPTS.DBF in work area 2 and
* STAFF.DBF with STAFFDPT.NDX in work area 1.  For each department, the
* staff are counted through the index on the department.
SELECT 2
GO TOP
DO WHILE .NOT. EOF()
  MDEPT = DEPT
  MDNAME = DEPTNAME
  SELECT 1
  MCOUNT = 0
  SEEK MDEPT
  DO WHILE DEPT = MDEPT .AND. .NOT. EOF()
    IF .NOT. DELETED()
      MCOUNT = MCOUNT + 1
    ENDIF
    SKIP
  ENDDO
  SELECT 2
  ? MDNAME + STR(MCOUNT, 4)
  SKIP
ENDDO
RETURN
