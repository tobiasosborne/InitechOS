* PAGERPT.PRG -- A report of numbered pages
* Initech Systems Corporation.  Sample program for Programming with
* InitechBase, Chapter 15.  Database: STAFF.DBF in USE.  Prints the staff
* roster in pages of MLINES lines, each with a heading and a page number.
MLINES = 6
MPAGE = 0
MLINE = MLINES
GO TOP
DO WHILE .NOT. EOF()
  IF MLINE >= MLINES
    DO HEADING
  ENDIF
  ? EMPNO + '  ' + LASTNAME + FIRSTNAME + '  ' + EXT
  MLINE = MLINE + 1
  SKIP
ENDDO
?
? '*** END OF ROSTER ***'
RETURN

PROCEDURE HEADING
MPAGE = MPAGE + 1
?
? 'INITECH STAFF ROSTER' + SPACE(20) + 'PAGE' + STR(MPAGE, 3)
? 'NO.   NAME                      EXT'
MLINE = 0
RETURN
