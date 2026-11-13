* SERVICE.PRG -- Years of service
* Initech Systems Corporation.  Sample program for Programming with
* InitechBase, Chapter 16.  Database: STAFF.DBF in USE.  For each employee
* the complete years of service as of the date MASOF are computed, and the
* staff are counted in three classes.
STORE 0 TO MNEW, MMID, MLONG
GO TOP
DO WHILE .NOT. EOF()
  IF .NOT. DELETED()
    MYEARS = INT((MASOF - HIRED) / 365)
    ? LASTNAME + DTOC(HIRED) + STR(MYEARS, 4)
    DO CASE
    CASE MYEARS < 5
      MNEW = MNEW + 1
    CASE MYEARS < 10
      MMID = MMID + 1
    OTHERWISE
      MLONG = MLONG + 1
    ENDCASE
  ENDIF
  SKIP
ENDDO
? 'UNDER 5 YEARS    ' + STR(MNEW, 3)
? '5 TO 9 YEARS     ' + STR(MMID, 3)
? '10 YEARS OR MORE ' + STR(MLONG, 3)
RETURN
