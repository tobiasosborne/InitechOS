* RAISE.PRG -- New salary after a percentage raise
* Initech Systems Corporation.  Sample program for Using InitechBase.
* Use with STAFF.DBF in USE, positioned on an employee.  The raise in
* percent is passed to the procedure NEWSAL as a parameter.
DO NEWSAL WITH SALARY, 4
? TRIM(LASTNAME), MNEW
RETURN

PROCEDURE NEWSAL
PARAMETERS MOLD, MPCT
PUBLIC MNEW
MNEW = ROUND(MOLD * (1 + MPCT / 100), 2)
RETURN
