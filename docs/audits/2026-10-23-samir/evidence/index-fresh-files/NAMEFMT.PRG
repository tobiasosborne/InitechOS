* NAMEFMT.PRG -- Names in capitals and small letters
* Initech Systems Corporation.  Sample program for Programming with
* InitechBase, Chapter 17.  Database: STAFF.DBF in USE.  The roster keeps
* names in capitals; letters to the staff print them with only the first
* letter in capitals.
GO TOP
DO WHILE .NOT. EOF()
  MFIRST = TRIM(FIRSTNAME)
  MLAST = TRIM(LASTNAME)
  MFIRST = LEFT(MFIRST, 1) + LOWER(SUBSTR(MFIRST, 2))
  MLAST = LEFT(MLAST, 1) + LOWER(SUBSTR(MLAST, 2))
  ? 'Dear ' + MFIRST + ' ' + MLAST + ','
  SKIP
ENDDO
RETURN
