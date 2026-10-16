* DEPTLIST.PRG -- The staff of a department, through a filter
* Initech Systems Corporation.  Sample program for Programming with
* InitechBase, Chapter 20.  Database: STAFF.DBF in USE.  Sets a filter to
* the department asked for, lists it, and removes the filter.
ACCEPT 'Department code: ' TO MDEPT
MDEPT = UPPER(MDEPT)
SET FILTER TO DEPT = MDEPT
GO TOP
IF EOF()
  ? 'NO STAFF IN ' + MDEPT
ELSE
  LIST LASTNAME, FIRSTNAME, EXT
ENDIF
SET FILTER TO
RETURN
