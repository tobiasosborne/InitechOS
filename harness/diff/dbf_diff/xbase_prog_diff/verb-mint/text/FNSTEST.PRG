* MINT-ERRORS: 16
* Ref: ../dbase3-decomp/specs/commands/programming-and-io.md:494
? "[W2-BEGIN]"
STORE "EXPANDED" TO WORD
? "[EXPRESSION OUTPUT EXPANDS VARIABLE]"
? WORD
? "[TEXT PRESERVES LITERAL CONTENT]"
TEXT
  leading spaces
&WORD stays literal
&& is literal text
1+2 is not evaluated

last line
ENDTEXT
? "[EMPTY TEXT BLOCK]"
TEXT
ENDTEXT
? "[UNMATCHED ENDTEXT]"
ENDTEXT
? "[W2-END]"
RETURN
