* proc.prg -- PROCEDURE / DO / RETURN, PUBLIC memvar, modified by the called proc.
* Corpus citation: ../dbase3-decomp/specs/commands/control-flow-and-procedures.md
*   sec 7 (PROCEDURE/DO/RETURN), sec 8 (DO ... WITH / PARAMETERS by position).
* ../dbase3-decomp/specs/language/memory-variables.md sec 3.2 (PUBLIC survives RETURN),
*   sec 3.1 rule 1 (STORE/= to a VISIBLE var modifies it -- the PUBLIC is visible
*   in the callee, so DOUBLE's REPLACE-into-memvar updates the caller's R).
PUBLIC R
R = 21
DO DOUBLE
? R
DO ADD WITH 5
? R
PROCEDURE DOUBLE
  R = R * 2
  RETURN
PROCEDURE ADD
  PARAMETERS K
  R = R + K
  RETURN
