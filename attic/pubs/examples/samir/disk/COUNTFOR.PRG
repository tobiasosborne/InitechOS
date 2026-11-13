* COUNTFOR.PRG -- Counting with LOCATE and CONTINUE
* Initech Systems Corporation.  Sample program for Programming with
* InitechBase, Chapter 21.  Database: STAFF.DBF in USE.  Counts the
* employees hired before the date MBEFORE and lists them.
MCOUNT = 0
LOCATE FOR HIRED < MBEFORE
DO WHILE FOUND()
  ? LASTNAME + DTOC(HIRED)
  MCOUNT = MCOUNT + 1
  CONTINUE
ENDDO
? STR(MCOUNT, 3) + ' EMPLOYEES HIRED BEFORE ' + DTOC(MBEFORE)
RETURN
