* ERRDEMO.PRG -- An error handler
* Initech Systems Corporation.  Sample program for Programming with
* InitechBase, Chapter 12.  The procedure ERRHAND reports the error and
* the record; the misspelled field AMOUNTS stops the program.
ON ERROR DO ERRHAND
MTOTAL = 0
GO TOP
DO WHILE .NOT. EOF()
  MTOTAL = MTOTAL + AMOUNTS
  SKIP
ENDDO
ON ERROR
RETURN

PROCEDURE ERRHAND
? 'ERRHAND: ERROR ' + LTRIM(STR(ERROR())) + ' AT RECORD ' + LTRIM(STR(RECNO()))
?
RETURN
