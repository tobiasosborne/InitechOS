* TALLY.PRG -- PRIVATE memory variables
* Initech Systems Corporation.  Sample program for Using InitechBase.
* The procedure ADDUP has its own MCOUNT; the caller's MCOUNT is hidden
* while ADDUP runs and is the same afterwards.
MCOUNT = 100
DO ADDUP
? 'MCOUNT AFTER ADDUP:', MCOUNT
RETURN

PROCEDURE ADDUP
PRIVATE MCOUNT
MCOUNT = 1
? 'MCOUNT IN ADDUP:', MCOUNT
RETURN
