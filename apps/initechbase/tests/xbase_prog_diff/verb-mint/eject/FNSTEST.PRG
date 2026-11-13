* MINT-ERRORS:
* Ref: ../dbase3-decomp/specs/commands/programming-and-io.md:1011
* No command-specific operand error: III+ ignores trailing tokens.
? "[W2-BEGIN]"
? "[DEFAULT PRINTER POSITION]"
? PROW(), PCOL()
SET PRINT ON
?? "ABC"
SET PRINT OFF
? "[PRINTER POSITION AFTER ABC]"
? PROW(), PCOL()
EJECT
? "[PRINTER POSITION AFTER EJECT]"
? PROW(), PCOL()
? "[TRAILING OPERAND PROBE]"
EJECT BAD
? "[W2-END]"
RETURN
