* flow.prg -- control flow: IF/ELSE/ENDIF (nested), DO WHILE/ENDDO, DO CASE,
*             LOOP, EXIT, STORE/memvar assignment.
* Corpus citation: ../dbase3-decomp/specs/commands/control-flow-and-procedures.md
*   sec 2 (IF/ELSE/ENDIF), sec 3 (DO WHILE/ENDDO, LOOP, EXIT), sec 4 (DO CASE).
*   Guard MUST be Logical -- no truthiness (interp.h S5.3 header; #37 on non-L).
* ../dbase3-decomp/specs/language/memory-variables.md sec 3.1 (auto-private STORE/=).
X = 3
IF X > 5
  ? 'BIG'
ELSE
  ? 'SMALL'
ENDIF
I = 1
DO WHILE I <= 3
  ? I
  I = I + 1
ENDDO
N = 2
DO CASE
CASE N = 1
  ? 'ONE'
CASE N = 2
  ? 'TWO'
OTHERWISE
  ? 'OTHER'
ENDCASE
J = 0
DO WHILE .T.
  J = J + 1
  IF J = 2
    LOOP
  ENDIF
  IF J = 4
    EXIT
  ENDIF
  ? J
ENDDO
? 'DONE'
