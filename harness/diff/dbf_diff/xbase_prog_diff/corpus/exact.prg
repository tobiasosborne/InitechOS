* exact.prg -- SET EXACT effect on the directional '=' string operator.
* THE mutant target program (plan S6.4: "flip the '=' prefix direction -> corpus RED").
* Corpus citation: ../dbase3-decomp/specs/language/expressions-and-operators.md sec 4.4
*   "SET EXACT OFF (default): left = right is true iff left BEGINS WITH right,
*    comparing only LEN(right) chars."  Worked cases (verbatim from sec 4.4):
*      'Smith' = 'S'  -> .T.   (left begins with right)
*      'S' = 'Smith'  -> .F.   (left 'S' does NOT begin with right 'Smith')
*    SET EXACT ON: must match exactly (trailing blanks ignored):
*      'Smith' = 'S'  -> .F.
* The directionality (which side is the prefix) is the load-bearing semantic the
* mutant perturbs; logicals render as T/F (engine ?/LIST form).
? 'Smith' = 'S'
? 'S' = 'Smith'
SET EXACT ON
? 'Smith' = 'S'
? 'Smith' = 'Smith '
