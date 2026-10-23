* NOTES.PRG -- The remarks of the staff roster
* Initech Systems Corporation.  Sample program for Programming with
* InitechBase, Chapter 19.  Database: STAFF.DBF in USE.  Prints the
* remark of every employee who has one.  NOTES is a memo field.
GO TOP
DO WHILE .NOT. EOF()
  IF LEN(TRIM(NOTES)) > 0
    ? TRIM(LASTNAME) + ': ' + TRIM(NOTES)
  ENDIF
  SKIP
ENDDO
RETURN
