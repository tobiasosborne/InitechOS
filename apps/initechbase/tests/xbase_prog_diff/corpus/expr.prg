* expr.prg -- computed expressions: arithmetic, precedence, string concat, $, logicals.
* Corpus citation: ../dbase3-decomp/specs/language/expressions-and-operators.md
*   sec 3 (arithmetic), sec 9.1 (precedence: * over +, ^ binds), sec 6 (string +),
*   sec 4 (relational -> L), sec 6 ($ substring).
* dBASE III PLUS 1.1; ? emits a leading newline then the value (no trailing NL).
? 2 + 3 * 4
? (2 + 3) * 4
? 2 ^ 3
? 10 - 4 - 3
? 'AB' + 'CD'
? 'left' $ 'a left turn'
? 1 = 2
? 5 > 3 .AND. 2 < 4
? .NOT. 1 = 1
