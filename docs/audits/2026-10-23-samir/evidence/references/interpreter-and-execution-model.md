# Interpreter and Execution Model — dBASE III PLUS 1.1

How the dBASE III PLUS 1.1 (1986) interpreter reads, parses, and runs commands:
the dot-prompt REPL versus program-file (`.PRG`) execution, the command line
grammar (one verb per line), the **4-letter command-abbreviation rule**, keyword
and verb recognition, **macro `&` substitution and its substitute-then-parse
timing**, line continuation with `;`, the three comment forms (`*` full line,
`&&` trailing, `NOTE`), the command **HISTORY** recall buffer, line-by-line
interpretation with **no compile step** (contrast Clipper/dBASE IV which
compile), and the *optional* dBCODE/dBLINKER tokenizing toolchain that produces
the "linked"/pseudo-compiled programs such as `APPSGEN.PRG`.

This file is the *execution-engine* spec. It is the umbrella over the more
detailed command specs and assumes the reader will consult:
`../commands/programming-and-io.md` (DO/PROCEDURE/PARAMETERS/RETURN/SUSPEND/
RESUME/CANCEL/RETRY, ON ERROR/ON ESCAPE/ON KEY, TEXT, WAIT/ACCEPT/INPUT),
`../commands/set-commands.md` (SET HISTORY/DOHISTORY/ECHO/STEP/DEBUG/TALK/
ESCAPE/TYPEAHEAD/EXACT), `../language/expressions-and-operators.md` and
`../language/coercion-table.md` (operator typing, the `C + N` error),
`../file-formats/prg-and-aux-files.md` (the `.PRG`/dBCODE byte layout).

## Status / confidence

- **Period surface authority** (what exists, exact verbs/keywords): the mined
  III+ HELP index `archive/golden-mined/HELP_topics.txt` (the 406-item
  COMPLETENESS oracle) and `archive/golden-mined/ASSIST.HLP.strings.txt`
  (period command-format text). The error catalog is
  `archive/golden-mined/DBASE.MSG.strings.txt` (each line = an emitted message;
  its ordinal is the `ERROR()` code per `../commands/programming-and-io.md`).
- **The keyword/verb dictionary is verified against real bytes**: `DBC.COM`
  (dBCODE 2.06) embeds the complete command-verb table (ACCEPT..WAIT plus
  `DO CASE`/`DO WHILE`) and the function and SET-keyword tables; I list it
  below. [verified: DBC.COM strings + HELP_topics]
- **Real idioms** are cited from the 29 plaintext sample `.PRG` files in
  `goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/`.
- The Clipper-lineage Harbour/xHarbour docs are a **semantic** oracle only
  (macro `&` forms, history of the language); they are NOT period-exact syntax
  and every III+-vs-later delta is tagged inline.
- The single biggest hazard is repeated here in bold: **`C + N` is a TYPE-
  MISMATCH ERROR in III+, not auto-stringification.** The dbase.com `+` help
  (`archive/dbase-com-help/OPS_PLUS.md`) documents the *modern* dBASE PLUS rule
  ("concatenate any other data type to a string") and is **wrong for III+**.
  See Section 9. [verified: DBASE.MSG "Data type mismatch." +
  ../language/coercion-table.md]

---

## 1. Two execution contexts: the dot prompt and program files

dBASE III PLUS runs commands in exactly one of two modes; the parser and verb
set are the *same* in both, but a handful of verbs are legal only inside a
program file.

### 1.1 The dot-prompt REPL (interactive mode)

At the interactive prompt the cursor sits at a single period (`.`) at the bottom
of the screen — hence "the dot prompt." The loop is the classic REPL:

1. Read one logical command line from the keyboard (the type-ahead buffer feeds
   it; see SET TYPEAHEAD).
2. Substitute any macros (`&`) in the line (Section 6).
3. Parse the substituted text into `verb [arguments]` (Section 3-5).
4. Execute it, mutating interpreter state (open work areas, memvars, SET flags).
5. If `SET TALK ON` (the default), echo the command's result; print the dot
   again. On a parse/runtime error, emit the message and re-prompt.

The dot prompt is reached from the ASSIST full-screen menu by pressing Esc, and
returned-to from a program by `RETURN` at the outermost level, `CANCEL`, or an
unhandled error. The status-line invitation is the literal message
`Type a command (or ASSIST) and press the ENTER key (` ... [verified:
DBASE.MSG.strings.txt line "Type a command (or ASSIST) and press the ENTER key"].

Whether the session *starts* at the dot prompt or in ASSIST is governed by
`CONFIG.DB`. The golden `CONFIG.DB` contains exactly:

```
STATUS=ON
COMMAND=ASSIST
```

`COMMAND=ASSIST` is the auto-executed startup command — it drops the user into
the ASSIST menu rather than the bare dot prompt. [verified:
goldens/dbase-iii-plus-1.1-pristine/files/System_1/CONFIG.DB (od -c) +
../environment config spec]. Removing/altering `COMMAND=` leaves the session at
the dot prompt. The `COMMAND=` directive may be any single dBASE command
(commonly `DO <mainprog>` to launch an application). [documented: CONFIG.DB
golden + period CONFIG.DB conventions]

### 1.2 Program-file execution (`DO <file>`)

`DO <program file>` opens a `.PRG` (default extension; any extension may be
named explicitly) and runs it **line by line** from the top. Each line is read,
macro-substituted, parsed, and executed exactly as if typed at the dot prompt,
then the next line is read. There is **no separate compile/parse-everything pass
in III+** (Section 8). Real top-of-program idiom:

```
* Program.: CHECK.PRG
SET TALK OFF
SELECT 2
USE Tax
SELECT 1
USE Checks INDEX Chkno
DO Chkmask
```

[verified: CHECK.PRG lines 1-14]

`DO` may target either a standalone `.PRG` or a **PROCEDURE** of that name found
in the file named by `SET PROCEDURE TO` or in the current program file
(Section 7). Control structures (`DO WHILE...ENDDO`, `DO CASE...ENDCASE`,
`IF...ENDIF`, `TEXT...ENDTEXT`) are honored across line boundaries — the
interpreter, on reading the opening verb, scans forward consuming lines until
the matching terminator. A program ends at `RETURN`, `CANCEL`, physical
end-of-file (implicit RETURN), or an unhandled error.

Verbs valid **only** inside a program file raise `Valid only in programs.`
when typed at the dot prompt (e.g. `PARAMETERS`, `LOOP`, `EXIT`, the loop/case
terminators). [verified: DBASE.MSG.strings.txt "Valid only in programs." +
HELP_topics LOOP/EXIT/PARAMETERS being program constructs]

### 1.3 DO nesting

`DO` calls nest (a program/procedure DOes another). The nesting depth is bounded
— exceeding it raises `DOs nested too deep.` On error inside a nested DO the
interpreter prints the call chain via the `Called from - ` prefix.
[verified: DBASE.MSG.strings.txt "DOs nested too deep." + "Called from - "].
The period-documented III+ limit is 20 nested DO levels and 32 simultaneously
open program/procedure files. [oracle-resolves: exact DO-nesting limit (the
message exists; the numeric bound needs the real interpreter under DOSBox)]

---

## 2. The logical command line

### 2.1 One verb per line; 254-char limit

A dBASE command occupies **one logical line**. There is no statement separator:
you cannot put two commands on one physical line (no `;`-as-separator — `;` is
*continuation*, Section 5). A logical line that exceeds **254 characters** is an
error: `Line exceeds maximum of 254 characters.` [verified:
DBASE.MSG.strings.txt "Line exceeds maximum of 254 characters."]. The 254 limit
is measured *after* line continuation is joined but the implementation detail of
whether it is measured before or after macro expansion is unresolved.
[oracle-resolves: whether the 254 cap applies pre- or post-`&` expansion]

### 2.2 Tokenization is whitespace- and keyword-driven

The interpreter splits the line into the leading verb token, then a sequence of
keyword-introduced clauses and expressions. Whitespace separates tokens; case is
**insensitive** for verbs, keywords, field/variable names, and function names
(`SET TALK OFF`, `set talk off`, and `Set Talk Off` are identical — samples mix
both freely, e.g. PROG.PRG uses lowercase `clear`, `set talk off`, `use`,
`enddo` while CHECK.PRG uses uppercase). String **literals** preserve case.
[verified: PROG.PRG lines 2-16 vs CHECK.PRG + HELP_topics OPERATOR/EXPRESSION]

Failures the tokenizer/parser raise (from the error catalog, ASCII-folded):

| Message (DBASE.MSG) | When |
|---|---|
| `*** Unrecognized command verb.` | first token is not a known verb |
| ` is not a dBASE command.` | (variant) the typed word is not a command |
| `Unrecognized phrase/keyword in command.` | a clause keyword is wrong/misplaced |
| `^--- Keyword not found.` | a keyword expected at the caret position is absent |
| `Syntax error.` | general malformed command |
| `Unbalanced parenthesis.` | mismatched `(` `)` in an expression |
| `Unterminated string.` | a string literal has no closing delimiter |
| `^^ Expected ON or OFF.` | a SET toggle got something other than ON/OFF |
| `^--- Truncated.` / `^--- Out of range.` | argument too long / out of bounds |
| `Data type mismatch.` | operand types incompatible (Section 9) |

[verified: DBASE.MSG.strings.txt — the lines above appear verbatim]. The `^`/`^^`
markers are caret pointers the interpreter prints under the offending token on
the echoed command line.

### 2.3 String literal delimiters

A character literal may be delimited by single quotes `'...'`, double quotes
`"..."`, or square brackets `[...]`. The three are interchangeable; brackets are
used when the literal must itself contain both quote characters. Samples use all
three: `'9/30/85'` (PROG.PRG l.7), `"999,999.99"` (CHECK.PRG l.112),
`'ABCE'` (FLIGHT.PRG l.40). A literal with no closing delimiter raises
`Unterminated string.` [verified: samples + DBASE.MSG.strings.txt;
documented: HELP_topics STRING OP / EXPRESSION]

---

## 3. Verb recognition and the 4-letter abbreviation rule

### 3.1 The rule

dBASE recognizes a command verb (and most keywords) by its **first four
letters**. Any verb may be abbreviated to its first four or more characters, and
extra trailing letters are accepted as long as they continue the real keyword.
Canonical examples:

```
REPL  ==  REPLACE          (REPL, REPLA, REPLAC, REPLACE all parse)
DISP  ==  DISPLAY
LIST  ==  LIST             (already 4 letters)
MODI  ==  MODIFY
PROC  ==  PROCEDURE
DELE  ==  DELETE
APPE  ==  APPEND
```

[documented: period dBASE III PLUS reference convention "minimum four-letter
abbreviation"; cross-checked against the DBC.COM verb table which holds the full
spellings]. The interpreter matches the typed token's first four characters
against the verb table (Section 3.3); the 5th-and-later characters, if present,
must agree with the table entry or the match fails.

### 3.2 Caveats and edge cases

- Verbs **3 letters or shorter** are matched in full and are not abbreviable
  (`USE`, `DO`, `SET`, `GO`, `?`, `??`, `@`).
- The four-letter rule applies to the *verb* and to clause keywords (e.g. `FOR`,
  `WHILE`, `TO`, `WITH`, `ALL`, `NEXT`, `RECORD`, `FIELDS`); short ones are
  matched whole.
- Two-word verbs are matched word-by-word on the same prefix logic:
  `DO CASE`/`DO WHILE`, `SET <toggle>`, `MODIFY COMMAND`, `LIST STRUCTURE`,
  `COPY FILE`, `DELETE FILE`, etc. `DO CASE` and `DO WHILE` are stored as
  distinct single tokens in the dBCODE verb table — i.e. `DO` followed by
  `CASE`/`WHILE` is a compound keyword, distinct from `DO <file>`. [verified:
  DBC.COM verb table contains literal entries `DO CASE` and `DO WHILE`]
- Function names follow the same prefix-recognition but are conventionally
  written in full; the full set is in the DBC.COM function table.
- A misspelled/over-short verb that does not resolve gives
  `*** Unrecognized command verb.` [verified: DBASE.MSG.strings.txt]

### 3.3 The verb / keyword / SET dictionary (verified against DBC.COM bytes)

`DBC.COM` (dBCODE 2.06, the tokenizer) embeds the symbol tables it must
recognize. These are the **III+ command verbs** (the canonical full spellings;
this is the period authority for what verbs EXIST):

```
ACCEPT  APPEND  AVERAGE  BROWSE  CALL  CANCEL  CASE  CHANGE  CLEAR  CLOSE
CONTINUE  COPY  COUNT  CREATE  DELETE  DIRECTORY  DISPLAY  EDIT  EJECT  ELSE
ENDCASE  ENDDO  ENDIF  ERASE  EXIT  EXPORT  FIND  GOTO  IMPORT  INDEX  INPUT
INSERT  JOIN  LABEL  LIST  LOAD  LOCATE  LOGOUT  LOOP  MODIFY  NOTE  OTHERWISE
PACK  PARAMETERS  PRIVATE  PROCEDURE  PUBLIC  QUIT  READ  RECALL  REINDEX
RELEASE  RENAME  REPLACE  REPORT  RESET  RESTORE  RESUME  RETRY  RETURN  SAVE
SEEK  SELECT  SKIP  SORT  STORE  SUSPEND  TEXT  TOTAL  TYPE  UNLOCK  UPDATE  WAIT
DO CASE   DO WHILE
```

[verified: DBC.COM strings, the contiguous ACCEPT..WAIT block + `DO CASE`/
`DO WHILE`]. Note `LOGOUT`, `RESET`, `UNLOCK` are the III+ multi-user
(Administrator) additions present in the binary; `IF`, `@`, `?`, `??`, `&`,
`SET`, `USE`, `GO`, `RUN`/`!`, `ZAP`, `DO`, `SUM` appear in HELP_topics and are
handled by the verb dispatcher even where the dBCODE table folds them (e.g.
`SUM`==`@SUM` topic; `GO`/`GOTO` one verb; `IF`/`ELSE`/`ENDIF` present as
ELSE/ENDIF with IF the opener).

The III+ **function** table embedded in DBC.COM (semantics live in
`../functions/`):

```
ACCESS  CDOW  CMONTH  CTOD  DATE  DELETED  DISKSPACE  DTOC  ERROR  FIELD  FILE
FKLABEL  FKMAX  FLOCK  FOUND  GETENV  INKEY  ISALPHA  ISCOLOR  ISLOWER  ISUPPER
LEFT  LOCK  LOCNDX  LOWER  LTRIM  LUPDATE  MESSAGE  MONTH  PCOL  PROW  READKEY
RECCOUNT  RECNO  RECSIZE  REPLICATE  RIGHT  RLOCK  ROUND  RTRIM  SPACE  SQRT
STUFF  SUBSTR  TIME  TRANSFORM  TRIM  UPPER  VERSION  YEAR   (+ ABS, ASC, AT,
CHR, DOW, EXP, INT, LEN, LOG, MAX, MIN, MOD, STR, VAL, etc. in the full set)
```

[verified: DBC.COM function block]. `ACCESS`, `FLOCK`, `RLOCK`, `LOCK`,
`LOCNDX`, `READKEY`, `ISCOLOR`, `GETENV`, `DISKSPACE`, `MESSAGE`, `VERSION` are
III-PLUS-era additions (multi-user / environment). [verified: DBC.COM]

The SET-keyword table (toggle/value names) embedded in DBC.COM begins:
`ALTERNATE BELL CARRY CATALOG CENTURY COLOR CONFIRM CONSOLE DEBUG DECIMALS
DEFAULT DELIMITERS DEVICE DOHISTORY ECHO ENCRYPTION ESCAPE EXACT EXCLUSIVE
FIELDS FILTER FIXED FORMAT FUNCTION HEADINGS HELP HISTORY INTENSITY MARGIN
MEMOWIDTH MENUS ODOMETER ORDER PATH PRINTER ...` — `ENCRYPTION`, `EXCLUSIVE`,
`ODOMETER` are the multi-user additions. [verified: DBC.COM SET block]. Full SET
semantics are in `../commands/set-commands.md`.

---

## 4. Parse model and evaluation order within a command

For each command line the engine:

1. **Joins continuations** (Section 5): physical lines ending in `;` are spliced
   into one logical line.
2. **Strips comments** (Section 10): a `*`/`NOTE` line is discarded whole; a
   `&&` trailing comment is removed from the end of the line.
3. **Substitutes macros** (Section 6): every `&name` / `&name.` is replaced by
   the *string value* of the named memvar, left-to-right, BEFORE parsing.
4. **Identifies the verb** by 4-letter prefix (Section 3).
5. **Parses clauses/expressions** for that verb and **evaluates expressions**
   using the operator precedence and type rules in
   `../language/expressions-and-operators.md`.
6. **Executes**, then (if `SET TALK ON`) echoes the result.

Steps 1-3 are textual transforms producing the final command text; step 4
onward is the actual command parse. This **substitute-then-parse** ordering is
what makes the macro operator powerful and dangerous (Section 6).

---

## 5. Line continuation with `;`

A **semicolon as the last non-blank character of a physical line** continues the
command onto the next physical line. The two (or more) physical lines are joined
into one logical command BEFORE parsing. Trailing whitespace after the `;` is
allowed. Real idioms:

```
@ 18,15 SAY "Check must be a positive amount - "+;
            "please reenter"
```
[verified: CHECK.PRG lines 119-120]

```
REPLACE Cname WITH mname, Cflt_no WITH answer, Cflt_date WITH ctod(mdate),;
        Cdep_city WITH dep_city, Cdes_city WITH des_city,;
        ...
```
[verified: SHOW_FLT.PRG lines 114-116]

```
IF balance<>mbalance .OR. lastchk<>mlastchk .OR. ;
   ...
```
[verified: CBMENU.PRG line 93]

Notes / edge cases:
- The leading whitespace of the continuation line is preserved when joined, so
  `"a"+; <NL> "b"` becomes `"a"+ "b"` — fine because surrounding spaces around
  the `+` operator are insignificant. But splitting **inside a string literal**
  injects the indentation spaces into the string; programmers split at operator
  boundaries (`+;`) for exactly this reason. [verified: CHECK.PRG, CLRCASH.PRG
  l.152 split at `+;`]
- A `;` that is *not* last on the line (e.g. inside a string literal) is just a
  character, not a continuation. [inferred from the "last char" rule + samples]
- `;` is **never** a statement separator in III+ (unlike C-family languages).
  This is a hard III+ rule. [verified: no sample uses `;` mid-line as separator;
  one-verb-per-line throughout]
- The joined logical line is still subject to the 254-char maximum (Section 2.1).
  [verified: DBASE.MSG "Line exceeds maximum of 254 characters."]

---

## 6. Macro substitution: the `&` operator (substitute-then-parse)

### 6.1 What it is

The ampersand `&` is the **macro substitution** operator. `&<name>` is replaced,
*as text*, by the **character-string contents** of the memory variable `<name>`
BEFORE the line is parsed. It is textual, not value, substitution: dBASE splices
the variable's string value into the command text and then parses the result.
This is "substitute-then-parse." [verified: xHarbour macro.txt "Invoking the
macro compiler: &variable / &(expression) / &variable.text" + III+ sample
usage]. The variable named by `&` **must be of type Character**; the macro
inserts its raw string. [documented: macro.txt + samples — all macro targets are
character memvars]

### 6.2 Forms

```
&name          replace with the string value of memvar `name`
&name.         the trailing dot terminates the name and is consumed
&name.text     concatenate the value of `name` with the literal text that
               follows the dot (the dot is the name delimiter, not output)
&(expression)  (xBase extension) evaluate expression to a string, then splice
```

[documented: xHarbour macro.txt — the `&var.` dot-terminator and `&(expr)` forms].
**III+-vs-later delta:** the dot-terminated `&name.` form and simple `&name` are
the III+ surface; the `&(expression)` indirection form is a Clipper/xBase
extension whose III+ availability is unconfirmed. [oracle-resolves: whether
`&(expr)` parses in III+ 1.1]

### 6.3 Real III+ idioms (the dominant use cases)

**(a) Substituting a value into a command argument** — the variable holds a file
name, an index key, a search target:

```
file1="Dep"+LTRIM(STR(myear,4,0))
USE Deposits
COPY TO &file1 FOR YEAR(Date)=myear
```
[verified: YEAREND.PRG lines 52, 56-57]. Here `&file1` expands to e.g. `Dep1985`
and the command parsed is `COPY TO Dep1985 FOR YEAR(Date)=myear`.

```
FIND &key
FIND &mflt_no
FIND &answer
```
[verified: FLT_RSVE.PRG l.63, FLT_CNCL.PRG l.76, SHOW_FLT.PRG l.48]. `FIND`
takes a *literal* search argument (not an expression), so the macro is the
idiomatic way to FIND a runtime-computed value.

**(b) Macro-as-stored-expression** — a character variable holds an entire dBASE
expression that is spliced into `?`/`DO`/`LOCATE` and evaluated when the line
runs:

```
command = "'    '+STR(Chkno,4,0)+'  '+DTOC(Date)+'  '+Payto+'  '+;
          STR(Amt,10,2)"
...
? &command
```
[verified: RPRTPRO.PRG lines 63-64, 146/150/153/etc.]. The string `command`
holds an expression; `? &command` becomes `? '    '+STR(Chkno,4,0)+...` and is
then evaluated against the *current* record. This is how a III+ program builds a
data-driven report column at runtime — the closest III+ has to a code block.

```
repo1 = "Reporta" + mchoice2
DO &repo1
```
[verified: RPRTPRO.PRG lines 115-116]. `&repo1` expands to e.g. `Reporta1` and
`DO Reporta1` calls the procedure built by name — runtime dispatch by name.

```
LOCATE FOR .NOT. Clear .AND. &compare
IF .NOT. Clear .AND. &compare
```
[verified: RPRTPRO.PRG lines 327, 344]. A whole boolean sub-expression is
spliced into the FOR/IF condition.

### 6.4 Timing and pitfalls (critical for a reimplementation)

- Substitution happens **once per execution of the line**, left-to-right,
  *before* the verb is even identified. The value spliced is the variable's
  value **at that moment**. In a loop, `? &command` re-reads `command` each
  iteration (it is constant here, but it could change). [inferred from
  substitute-then-parse + RPRTPRO loop]
- Because expansion is textual, the spliced text is re-parsed: a macro can
  introduce *any* tokens — operators, keywords, even (in principle) a different
  verb. The macro variable's content can therefore cause `Syntax error.`,
  `Data type mismatch.`, `Unterminated string.`, etc. at the splice site.
- A macro that expands to an undefined identifier yields `Variable not found.`;
  a macro naming a missing field yields `Field not found.` [documented:
  DBASE.MSG.strings.txt]
- `&` cannot recurse arbitrarily; nested `&` inside the substituted text is a
  later-dialect concern. III+ does a single textual pass. [oracle-resolves:
  whether III+ re-scans substituted text for further `&`]
- The macro target must exist and be Character. Substituting a numeric or
  logical variable is an error (the value is not a string to splice).
  [documented: macro.txt + samples all use C vars] [oracle-resolves: exact
  error when `&` names a non-character var in III+]

### 6.5 `&` is NOT the trailing-comment `&&`

A single `&` adjacent to an identifier is macro substitution. Two ampersands
`&&` introduce a **trailing comment** (Section 10). The lexer must distinguish
`&name` (macro) from `&&` (comment) — `&&` is recognized first as a comment
introducer. [verified: PROG.PRG l.2 `clear  &&  clear screen` vs FIND `&key`]

---

## 7. Subprogram model: DO / PROCEDURE / PARAMETERS / RETURN

(Full clause-by-clause syntax in `../commands/programming-and-io.md`; here is the
execution-model view.)

- `DO <file>|<procedure> [WITH <param list>]` transfers control. The engine
  looks for a `PROCEDURE <name>` first in the file named by `SET PROCEDURE TO`,
  then treats `<name>` as a standalone `.PRG`. On return, execution resumes at
  the line after the `DO`. [documented: HELP_topics DO / PROCEDURE /
  SET PROCEDURE]
- A single `.PRG` may contain multiple `PROCEDURE <name>` blocks after its main
  body; the main body runs top-down until the first `PROCEDURE`/`RETURN`, and
  procedures are reached only by `DO`. Real layout: RPRTPRO.PRG has its main
  logic, then `PROCEDURE Reporta1`, `PROCEDURE Reporta2`, ... in one file.
  [verified: RPRTPRO.PRG lines 116-158]
- `WITH <expr list>` passes parameters by position; the called program's
  `PARAMETERS <name list>` binds them. Missing `PARAMETERS` when called WITH
  args raises `No PARAMETER statement found.`; a count mismatch raises
  `Wrong number of parameters.` [verified: DBASE.MSG.strings.txt]
- `DO <proc> WITH <var>` passes **memvars by reference** and other expressions
  by value (the III+ convention). [documented: HELP_topics PARAMETERS; sample
  `DO Numwords WITH mamt` (CHECK.PRG l.163), `DO Printer WITH newrow,pr`
  (RPRTPRO.PRG l.136)] [oracle-resolves: precise by-ref/by-value rule for
  literals vs memvars in III+]
- `RETURN` ends the current program/procedure and pops one DO level; `RETURN`
  at the top level returns to the dot prompt (or to ASSIST/CONFIG `COMMAND`).
  Implicit `RETURN` at physical EOF. [verified: CHECK.PRG l.293, CBMENU.PRG
  l.176; documented: HELP_topics RETURN]
- The control-flow terminators must balance: an open `DO WHILE` with no `ENDDO`
  raises `Mismatched DO WHILE and ENDDO.` [verified: DBASE.MSG.strings.txt]
  `ENDDO`/`ENDCASE`/`ENDIF` may carry a trailing comment word (the loop label),
  e.g. `ENDDO lisa`, `ENDDO loop1` — purely decorative, ignored by the parser.
  [verified: CHECK.PRG lines 197 `ENDDO lisa`, 287 `ENDDO loop1`]

---

## 8. No compile step in III+ (vs Clipper / dBASE IV) — and the optional dBCODE path

### 8.1 III+ is a pure line-by-line interpreter

dBASE III PLUS 1.1 **interprets** program files at runtime: it reads a `.PRG`
line, expands/parses/executes it, then reads the next line. There is **no
ahead-of-time compilation**, no `.obj`, no link step required to run a program.
A `.PRG` is plain ASCII text (the samples are exactly that). The cost is speed
(every line is re-lexed every time it executes, including each loop iteration);
the benefit is the dot prompt and `.PRG` use the identical engine and a program
can build and run code at runtime via `&` (Section 6).

This is the defining contrast with the Clipper lineage and with dBASE IV:

| Trait | dBASE III PLUS 1.1 | Clipper / dBASE IV |
|---|---|---|
| Program execution | interpreted line-by-line at runtime | compiled to p-code/`.obj` ahead of time |
| Syntax errors found | only when the line is *reached* at runtime | at compile time, before run |
| `&` macro | re-parsed at runtime by the interpreter | re-compiled at runtime by a macro compiler |
| Distributable form | the `.PRG` text (or optional dBCODE, 8.2) | linked `.exe` |
| `==` operator, DECLARE arrays, auto C+N stringify | **absent** in III+ | present in IV/later |

[verified: III+ `.PRG` samples are plaintext + ../file-formats/prg-and-aux-files.md;
contrast documented: Harbour/Clipper docs (oracles/) describe the
compile-to-pcode model]. The macro subsystem description in Harbour
(`oracles/harbour/doc/vm.txt`: "a reduced compiler ... used at runtime to
generate pcode") is the *Clipper-lineage* implementation of the same `&`
feature — III+ has no pcode, it re-parses text. [verified: vm.txt + III+
interpreter model]

### 8.2 The optional dBCODE / dBLINKER "linked" / tokenized form (APPSGEN.PRG)

III+ ships an *optional* toolchain to tokenize/protect programs for the
RunTime+ product — this is **not** the normal execution path, but it explains
why some shipped `.PRG` files (notably `APPSGEN.PRG`, the Applications
Generator) are **not** plaintext:

- `DBC.COM` = **dBCODE 2.06** (03 March 1986, Ashton-Tate) — the
  "RunTime+ Pseudo-Compiler": it tokenizes a `.prg`/`.src` source into a
  dBCODE-III object `.prg` (using the verb/function/SET symbol tables of
  Section 3.3 as its tokenizer dictionary). [verified: DBC.COM banner + symbol
  tables]
- `DBL.COM` = **dBLINKER 2.06** — the "RunTime+ Linker": it links dBCODE `.prg`
  objects into one linked program and emits an optional `.map`. Its diagnostics
  name the format: `"... is not a dBCODE III file!"`. [verified: DBL.COM strings]

The disk README states the App Generator ships pre-tokenized for speed and is
run by `DBASE APPSGEN` or, from the dot prompt, `DO APPSGEN`. [verified:
goldens README, quoted in ../file-formats/prg-and-aux-files.md]. The dBASE
interpreter detects a dBCODE-tokenized program and executes the token stream
directly rather than lexing text — but the byte-level codec of the dBCODE-III
stream is **not reverse-engineered** (its bytes show ~8 bits/byte entropy,
i.e. encrypted/compressed). It is tracked as `[oracle-resolves]` in
`../file-formats/prg-and-aux-files.md` (Section 2) and `GAPS.md`. A
reimplementation may ship interpreting plaintext `.PRG` only and treat dBCODE as
out of scope, but must recognize that a tokenized program is a distinct,
cross-linked (`re/`) input form. [verified: prg-and-aux-files.md cross-link]

**Correction carried from prg-and-aux-files.md:** `FAST.PRG` is **plaintext**,
not tokenized — its high bytes are CP437 box-drawing glyphs inside `@...SAY`
string literals. The bytes win (Law 2). [verified: prg-and-aux-files.md §1.5]

---

## 9. Expression evaluation in the interpreter — the III+ typing rules

When the parser reaches an expression it evaluates it with strict typing. The
rules that most affect a reimplementation of the *engine* (full operator detail
in `../language/expressions-and-operators.md` and `../language/coercion-table.md`):

- **`C + N` (Character plus Numeric) is a TYPE-MISMATCH ERROR.** III+ does
  **not** auto-stringify the numeric operand. To build a string from a number
  you must call `STR()` (or `LTRIM(STR(...))`) explicitly — which is exactly
  what every sample does:
  ```
  "LAST CHECK # "+LTRIM(STR(lastchk,4,0))         (CHECK.PRG l.19)
  "Dep"+LTRIM(STR(myear,4,0))                     (YEAREND.PRG l.52)
  "("+STR(Code,1)+") "+Title                      (CHECK.PRG l.240)
  ```
  No sample ever writes `"x" + somenumber` directly. The interpreter raises
  `Data type mismatch.` for `C + N`. [verified: DBASE.MSG.strings.txt "Data type
  mismatch." + every sample uses explicit STR(); contrast the WRONG-for-III+
  modern doc archive/dbase-com-help/OPS_PLUS.md which says you may "concatenate
  any other data type to a string" — that is dBASE PLUS, NOT III+].
  **III+-vs-later delta, highest priority.**
- `+` on two Character operands **concatenates**; if the result would exceed the
  255-char string limit it raises `+ : Concatenated string too large.`
  Subtraction `C - C` concatenates trimming trailing blanks of the left operand
  to the right; overflow raises `- : Concatenated string too large.` [verified:
  DBASE.MSG.strings.txt + OPS_PLUS.md/OPS_MINUS.md semantics]
- `Date + N` / `Date - N` shifts the date by N days. `Date - Date` yields a
  numeric day count. [documented: OPS_PLUS.md/OPS_MINUS.md, III+-stable core]
- String comparison `=` / `<>` / `#` and SEEK/FIND/index lookups obey
  **SET EXACT** (default OFF = "begins with" partial match; ON = full match
  ignoring trailing blanks). `"Smith" = "S"` is TRUE with EXACT OFF. There is
  **no `==` operator in III+** — the `==` (always-exact) operator described in
  archive/dbase-com-help/SET_EXACT.md is a later-dialect addition.
  [verified: SET_EXACT.md (semantics) + HELP_topics SET EXACT/RELATIONAL OP;
  III+-vs-later delta: `==` absent in III+].
- Numeric overflow during evaluation raises `Numeric overflow (data was lost).`
  / `Numeric overflow.`; `^`/`**` with a negative base and fractional exponent
  raises `^ or ** : Negative base, fractional exponent.` Domain errors give
  function-specific messages (`LOG() : Zero or negative.`, `SQRT() : Negative.`,
  `CHR() : Out of range.`, `SUBSTR() : Start point out of range.`, etc.).
  [verified: DBASE.MSG.strings.txt]
- A non-logical expression where a condition is required (IF/DO WHILE/FOR)
  raises `Not a Logical expression.`; a non-numeric where numeric is required
  raises `Not a numeric expression.` / `Not a numeric expression`; a
  non-character where character is required raises `Not a Character expression.`
  [verified: DBASE.MSG.strings.txt]

The logical literals are `.T.`/`.F.` (the `.Y.`/`.N.` aliases are accepted by
the period interpreter but are NOT attested in any local source — see
[oracle-resolves] below); the logical operators are `.AND.`, `.OR.`, `.NOT.`;
relational `=`, `<>`, `#`, `<`, `>`, `<=`, `>=`; the substring/containment
operator `$` (e.g. `ans $ "YyNn"`). [verified: samples CHECK.PRG l.135
`.NOT. ans$"YyNn"`, SHOW_FLT.PRG l.64 `lockcount # 0`; + HELP_topics LOGICAL OP /
RELATIONAL OP / STRING OP — these cover `.T.`/`.F.`, the operators, and `$`;
`.Y.`/`.N.` are [documented] only]

---

## 10. Comments: `*`, `&&`, and `NOTE`

dBASE III PLUS has three comment forms, all verified in the samples and in the
DBC.COM verb table (`NOTE` is a real verb):

1. **Full-line `*`** — a physical line whose first non-blank character is `*` is
   a comment; the whole line is discarded before parsing. Ubiquitous:
   ```
   * Program.: CHECK.PRG
   * set up working files
   *This is a counting program
   ```
   [verified: CHECK.PRG l.1, l.7; PROG.PRG l.1]. The `*...*` boxed banners in the
   samples are just many `*`-lines. [verified: CBMENU.PRG lines 7-12]
2. **Trailing `&&`** — `&&` and everything after it to end-of-line is a comment,
   appended after a real command:
   ```
   clear             &&  clear screen
   ```
   [verified: PROG.PRG l.2]. The lexer must strip the `&&...` tail *before* macro
   substitution and parsing, and must not confuse `&&` with the macro `&`
   (Section 6.5).
3. **`NOTE`** — the verb `NOTE` introduces a full-line comment (equivalent to
   `*`); the rest of the line is ignored. `NOTE` is a recognized verb in the
   dBCODE table. [verified: DBC.COM verb table contains `NOTE`; documented:
   HELP_topics implies NOTE/comment under STARTER/SYNTAX]. (None of the 29
   samples use `NOTE`; they prefer `*`.)

Edge cases:
- A `*` that is not the first non-blank char is NOT a comment — it is the
  multiplication/exponent operator (`**`) or part of an expression.
- `&&` may follow a continued line's final segment; it terminates that logical
  line's parse. [inferred from the strip-comment-then-join ordering]
- Comment stripping happens per physical line for `*`/`NOTE` (full line) and for
  `&&` (tail), conceptually before continuation-joining of code segments.
  [inferred from observed sample behavior]

---

## 11. The command HISTORY buffer and up-arrow recall

dBASE III PLUS keeps a ring buffer of recently executed commands for
interactive recall.

- `SET HISTORY TO <expN>` sets the buffer size (number of commands retained).
  **Default size = 20**; recording is **ON** by default at the dot prompt.
  [verified: ../commands/set-commands.md §3.6 (default 20) + HELP_topics
  SET HISTORY]
- `SET HISTORY ON/OFF` enables/disables recording without changing the size.
  [documented: HELP_topics SET HISTORY + DBC.COM SET table contains HISTORY]
- **Up-arrow / down-arrow** at the dot prompt walks back/forward through the
  buffer, redisplaying a prior command on the input line for re-edit and
  re-execution. (This is the III+ analogue of shell history.) [documented:
  period III+ HISTORY behavior + HELP_topics MACRO/HISTORY]
- `LIST HISTORY` / `DISPLAY HISTORY` print the buffered commands (DISPLAY pauses
  per screen; LIST scrolls). Both are first-class topics in the index
  (`LIST HISTORY`, `DISPLAY HISTORY`). [verified: HELP_topics lines 31, 54]
- **`SET DOHISTORY ON/OFF`** (default OFF) controls whether commands executed
  from *inside* a DO program file are *also* recorded into HISTORY — normally
  only interactive (dot-prompt) commands are saved. Used for debugging a program
  by inspecting the trail of executed lines. [documented: HELP_topics
  SET DOHISTORY + ../commands/set-commands.md] **III-vs-later delta:**
  `SET DOHISTORY` exists in III+ and was removed in dBASE IV. [documented:
  set-commands.md §2 note]

The HISTORY buffer is interactive-session state; it is separate from the
program-execution stack and from the I/O echo (`SET ALTERNATE` writes a session
transcript to a `.TXT` file — that is a *different* mechanism, see
../commands/set-commands.md).

---

## 12. Tracing / debugging hooks the engine exposes

These SET toggles instrument the interpreter loop (semantics in
`../commands/set-commands.md`; listed here because they are the
execution-model's debug surface):

- `SET TALK ON/OFF` (default ON): when ON, each command echoes its result to the
  console; programs open with `SET TALK OFF` to suppress this. The most common
  first program line in the samples. [verified: CHECK.PRG l.13, PROG.PRG l.3,
  CBMENU.PRG l.18, etc.]
- `SET ECHO ON/OFF` (default OFF): echoes each command line *as it executes* — a
  program trace. Output goes to screen, or to the printer if `SET DEBUG ON`.
  [documented: HELP_topics SET ECHO + set-commands.md §2]
- `SET DEBUG ON/OFF` (default OFF): redirects the SET ECHO trace to the printer.
  [documented: HELP_topics SET DEBUG + set-commands.md §2]
- `SET STEP ON/OFF` (default OFF): single-step — the interpreter pauses before
  each command. The pause prompt is the literal
  `Press SPACE to step, S to suspend, or Esc to cancel...`. [verified:
  DBASE.MSG.strings.txt that exact string + HELP_topics SET STEP]
- `SET ESCAPE ON/OFF` (default ON): when ON, Esc during program execution
  interrupts it (the `*** INTERRUPTED ***` / Cancel-Ignore-Suspend prompt). When
  OFF, Esc is ignored — applications do `SET ESCAPE OFF` to trap the user inside
  a menu. [verified: CBMENU.PRG l.19 `SET ESCAPE OFF` + DBASE.MSG
  "*** INTERRUPTED ***", "Cancel, Ignore, or Suspend? (C, I, or S) "]
- Program-state verbs the engine honors mid-run: `SUSPEND` (pause a program,
  drop to dot prompt with state intact; `Do suspended`), `RESUME` (continue a
  suspended program; resuming when none is suspended raises `Not suspended.`),
  `CANCEL` (abort to dot prompt; `Do cancelled`), `RETRY` (re-execute the line
  that errored — used in ON ERROR handlers, e.g. ERRORPRG.PRG retries on
  network/lock errors), `QUIT` (close all files, exit to DOS). [verified:
  DBASE.MSG.strings.txt "Do suspended"/"Do cancelled"/"Not suspended."; sample
  ERRORPRG.PRG uses RETRY/CANCEL].
- `SET TYPEAHEAD TO <expN>` sizes the keyboard buffer that feeds the REPL
  read step; `0` disables type-ahead (required for some `INKEY()`-driven menus,
  cf. CBMENU.PRG's `INKEY()` loop). III+ default 20. [documented: HELP_topics
  SET TYPEAHEAD + set-commands.md §3 (III+ default 20, modern 50 is later)]
  [oracle-resolves: III+ TYPEAHEAD default]

`ON ERROR <command>`, `ON ESCAPE <command>`, `ON KEY [<command>]` install
interpreter-level traps that fire on a runtime error, an Esc, or any keystroke
respectively; the trap command runs, then (for ON ERROR, if it does `RETRY`)
control returns to the failed line. The error-handler reads `ERROR()` (numeric
code), `MESSAGE()` (text), and `LINENO()`/program context. Real handler:
ERRORPRG.PRG `DO CASE / CASE ERROR()=108 .OR. ERROR()=109 / RETRY ...`.
[verified: ERRORPRG.PRG lines 15-52; full syntax in
../commands/programming-and-io.md]

---

## 13. Coverage against the HELP_topics oracle (406-item index)

This document is the *execution-model* spec; it fully covers the following
in-scope index items and points at the dedicated specs for the rest.

**Fully covered here** (execution/parse/REPL/macro/comment/history/trace):
`MACRO` (l.62/@MACRO), `DO WHILE`/`DO CASE` as parse constructs (control-flow
detail -> programming-and-io.md), `DISPLAY HISTORY` (l.31), `LIST HISTORY`
(l.54), `SET HISTORY` (l.126), `SET DOHISTORY` (l.209), `SET ECHO` (l.210),
`SET STEP` (l.221), `SET DEBUG` (l.207), `SET TALK` (l.222), `SET ESCAPE`
(l.211), `SET TYPEAHEAD` (l.136), `SET PROCEDURE` (l.134, exec view),
`PROCEDURE` (l.74, exec view), `PARAMETERS` (l.72, binding/error view),
`SYNTAX` (l.147), `OPERATOR`/`MATHEMATICAL OP`/`RELATIONAL OP`/`LOGICAL OP`/
`STRING OP` (l.156-160, the typing rules in §9), `EXPRESSION`/`EXP 2` (l.151-152,
evaluation), `CONDITION` (l.153, logical-expr requirement). The startup/CONFIG
path (`COMMAND=`) and the dot prompt vs ASSIST distinction (MAIN MENU l.1,
ASSIST l.14) are covered in §1.

**In scope but delegated** (named here, documented in their dedicated spec, to
avoid duplication): `DO` (full WITH/file/proc clauses), `RETURN`, `CANCEL`,
`SUSPEND`, `RESUME`, `RETRY`, `LOOP`, `EXIT`, `IF`/`ELSE`/`ENDIF`,
`CASE`/`OTHERWISE`/`ENDCASE`, `ENDDO`, `TEXT`/`ENDTEXT`, `WAIT`, `ACCEPT`,
`INPUT`, `STORE`, `PRIVATE`/`PUBLIC`/`RELEASE`, `SAVE`/`RESTORE` ->
`../commands/programming-and-io.md`. The full SET catalog ->
`../commands/set-commands.md`. Operator precedence and the coercion matrix ->
`../language/expressions-and-operators.md` + `../language/coercion-table.md`.
The `.PRG`/dBCODE byte format -> `../file-formats/prg-and-aux-files.md`.

**Out of scope for this file** (data, full-screen, reports, file formats): the
remaining ~360 index items.

---

## Open questions [oracle-resolves]

These need the real DBASE.EXE/DBASE.OVL under DOSBox (or RE of the OVL) to
settle; each goes to `GAPS.md`:

1. **DO-nesting limit and open-program-file count.** `DOs nested too deep.`
   exists; the documented "20 DO levels / 32 open files" needs confirmation
   against the binary.
2. **254-char limit timing.** Is the maximum-line check applied before or after
   `&` macro expansion (and before or after `;` continuation join)?
3. **`&(expression)` indirection form.** Does III+ 1.1 accept the parenthesized
   macro form, or only `&name` / `&name.`? (Harbour documents `&(expr)` as the
   xBase superset.)
4. **Macro re-scan / recursion.** Does III+ re-scan the substituted text for a
   further `&` (one pass vs fixed-point)?
5. **`&` on a non-Character variable.** Exact error/behavior when `&` names a
   numeric/logical/date memvar.
6. **`DO ... WITH` parameter passing.** Precise by-reference (memvar) vs
   by-value (literal/expression) rule and whether changed parameters propagate
   back in III+ 1.1.
7. **SET TYPEAHEAD III+ default** (20 vs the modern 50) and range.
8. **dBCODE-III token stream codec.** The byte-level encoding/encryption of
   tokenized programs (APPSGEN.PRG) — entropy ~8 bits/byte; not RE'd. (Shared
   with prg-and-aux-files.md.)
9. **`NOTE` exact semantics** — whether `NOTE` accepts continuation `;` or is
   strictly to-end-of-line (no sample uses it).

---

## Sources

- `archive/golden-mined/HELP_topics.txt` — the 406-item III+ command/function/
  concept index (COMPLETENESS oracle; topic line numbers cited inline).
- `archive/golden-mined/ASSIST.HLP.strings.txt` — period ASSIST command-format
  text (USE, SET FORMAT/FILTER/CATALOG/VIEW, CREATE..., etc.).
- `archive/golden-mined/DBASE.MSG.strings.txt` — the error/prompt catalog;
  every quoted message (`*** Unrecognized command verb.`,
  `Line exceeds maximum of 254 characters.`, `Unrecognized phrase/keyword in
  command.`, `Unterminated string.`, `Data type mismatch.`,
  `Mismatched DO WHILE and ENDDO.`, `DOs nested too deep.`,
  `Valid only in programs.`, `No PARAMETER statement found.`,
  `Wrong number of parameters.`, `Do suspended`/`Do cancelled`/`Not suspended.`,
  `Called from - `, `Press SPACE to step, S to suspend, or Esc to cancel...`,
  `Type a command (or ASSIST) and press the ENTER key (`, the `^`/`^^` caret
  markers, `+/- : Concatenated string too large.`, `Numeric overflow ...`) is
  verbatim from this file.
- `archive/dbase-com-help/OPS_PLUS.md`, `OPS_MINUS.md`, `OPS_COMPARISON.md`,
  `SET_EXACT.md` — modern (dBASE PLUS) operator/EXACT semantics; III+-stable
  core, with the auto-stringification and `==` notes flagged as later-dialect
  (NOT III+).
- `oracles/xharbour/doc/en/macro.txt` — the `&variable` / `&(expression)` /
  `&variable.text` macro forms (semantic oracle; Clipper-lineage, not period
  syntax).
- `oracles/harbour/doc/vm.txt` — the Clipper-lineage compile-to-pcode model
  (contrast to III+ pure interpretation).
- `goldens/.../System_1/CONFIG.DB` — `STATUS=ON` / `COMMAND=ASSIST` startup.
- `goldens/.../Sample_Programs_and_Utilities/DBC.COM` (dBCODE 2.06) &
  `DBL.COM` (dBLINKER 2.06) — the embedded verb/function/SET symbol tables
  (Section 3.3) and the optional tokenize/link toolchain (Section 8.2).
- Sample `.PRG` programs (real III+ idioms, all paths under
  `goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/`):
  `CHECK.PRG` (STR()-concat, `;` continuation, `ENDDO <label>`, structure),
  `PROG.PRG` (`*` and `&&` comments, lowercase verbs), `ERRORPRG.PRG`
  (ON ERROR / `ERROR()` / RETRY / CANCEL), `YEAREND.PRG` & `RPRTPRO.PRG` &
  `FLT_RSVE.PRG`/`FLT_CNCL.PRG`/`SHOW_FLT.PRG` (the `&` macro idioms),
  `CBMENU.PRG` (DO dispatch, SET TALK/ESCAPE OFF, INKEY loop).
- Cross-links: `../commands/programming-and-io.md`,
  `../commands/set-commands.md`, `../language/expressions-and-operators.md`,
  `../language/coercion-table.md`, `../file-formats/prg-and-aux-files.md`.

---

## Verification

Adversarial cross-check (2026-06-16) of every semantics-critical claim against a
second independent source: the mined III+ catalogs
(`archive/golden-mined/DBASE.MSG.strings.txt`, `HELP_topics.txt`), the real
`DBC.COM`/`DBL.COM` byte strings, the modern dbase.com help
(`archive/dbase-com-help/`), the Clipper-lineage oracle docs
(`oracles/{x,}harbour/doc/`), and the 29 sample `.PRG` files.

**Confirmed verbatim / re-derived (no change needed):**
- All ~30 quoted error/prompt strings (`Data type mismatch.`,
  `Line exceeds maximum of 254 characters.`, `*** Unrecognized command verb.`,
  `Valid only in programs.`, `DOs nested too deep.`, `Called from - `,
  `Mismatched DO WHILE and ENDDO.`, `No PARAMETER statement found.`,
  `Wrong number of parameters.`, `Do suspended`/`Do cancelled`/`Not suspended.`,
  `Unterminated string.`, `Unbalanced parenthesis.`, the §2.2 tokenizer table,
  the §9 domain-error messages, `Press SPACE to step...`, `*** INTERRUPTED ***`,
  `Cancel, Ignore, or Suspend...`, `Type a command (or ASSIST)...`) are present
  byte-for-byte in `DBASE.MSG.strings.txt`. [verified]
- `DBC.COM` verb table (ACCEPT..WAIT, `DO CASE`, `DO WHILE`, `NOTE`, the
  multi-user `LOGOUT`/`RESET`/`UNLOCK`), the function table (incl. `ACCESS`
  `FLOCK` `RLOCK` `LOCNDX` `READKEY` `ISCOLOR` `GETENV` `DISKSPACE` `MESSAGE`
  `VERSION` `TRANSFORM`), and the SET-keyword table (incl. `DOHISTORY`
  `ENCRYPTION` `EXCLUSIVE` `ODOMETER`) all present. Banners confirmed:
  `dBCODE (2.06) 03 MARCH 1986`, `dBASE III RunTime+ Pseudo-Compiler`;
  `dBLINKER (2.06)`, `dBASE III RunTime+ Linker`, `is not a dBCODE III file!`.
  [verified]
- Macro forms `&variable` / `&( expression )` / `&variable.text` are exactly the
  three documented in `oracles/xharbour/doc/en/macro.txt` (lines 17/19/21); the
  pcode/"reduced compiler at runtime" contrast is `oracles/harbour/doc/vm.txt`
  line 75-76. [verified]
- Sample citations: CHECK.PRG l.1/7/13/19/112/119-120/135/163/197/287/293,
  PROG.PRG l.1-16, YEAREND.PRG l.52/56-57, RPRTPRO.PRG l.63-64/115-116/327/344,
  FLT_RSVE.PRG l.63, FLT_CNCL.PRG l.76, SHOW_FLT.PRG l.48/64/114-116,
  CLRCASH.PRG l.152, FLIGHT.PRG l.40, CBMENU.PRG l.18-19/93, ERRORPRG.PRG
  l.16-50 — all match the golden bytes. [verified]

**III-PLUS CORRECTNESS audit (the key task):** the doc's three flagged
later-dialect deltas are all correct and the local sources back the flags:
- `C + N` auto-stringification is documented only in the *modern*
  `archive/dbase-com-help/OPS_PLUS.md` (line 31: "You may concatenate any other
  data type to a string"); III+ raises `Data type mismatch.` (catalog) and every
  sample uses explicit `STR()`. Correctly flagged WRONG-for-III+. [verified]
- `==` always-exact operator is documented only in the modern `SET_EXACT.md`
  (line 17) / `OPS_COMPARISON.md` (line 63); it is absent from `HELP_topics.txt`.
  Correctly flagged absent in III+. [verified]
- `DECLARE`/arrays: absent from `HELP_topics.txt` and `ASSIST.HLP.strings.txt`;
  the doc never imports them. [verified — no IV-era array rule leaked in]
- `SET DOHISTORY` is present in `DBC.COM` and `HELP_topics.txt` (l.209) and
  absent from the Harbour/xHarbour lineage — consistent with "III+ feature,
  removed in dBASE IV." [verified]

**Corrections applied:**
1. §9 STR()-concat example block: the third sample was misquoted as
   `"(Enter "+STR(Code,1)+") "+Title`. The actual byte content of
   CHECK.PRG l.240 is `"("+STR(Code,1)+") "+Title` (the `(Enter ...)` text
   appears only at CHECK.PRG l.38/250, never combined with `STR(Code,1)`). A
   code-block citation must be byte-true (Law 2); corrected.
2. §9 logical-literals sentence over-claimed `[verified]` coverage of the
   `.Y.`/`.N.` aliases, which are NOT attested in any local source (only
   `.T.`/`.F.`, the operators, and `$` are). Narrowed the verification tag and
   demoted `.Y.`/`.N.` to `[documented]` with an [oracle-resolves] pointer.
3. Removed a stray unterminated ```` ``` ```` code fence at end-of-file (the
   Sources list is a markdown list, not a code block); it left the fence count
   odd and would render the Sources section as a code block.

**Unicode / code-block hygiene:** confirmed the author's claim — the only
non-ASCII characters in the file are `—` (em-dash) and `§` (section sign), and a
fence-aware scan shows ZERO of them fall inside any ```` ``` ```` code block.
Compliant with CLAUDE.md "ASCII-clean in code blocks; Unicode fine in prose."
[verified]

**Completeness vs the HELP_topics index:** the in-scope execution/parse/macro/
history/trace topics the doc claims to cover (MACRO l.62, DISPLAY/LIST HISTORY
l.31/54, SET HISTORY/DOHISTORY/ECHO/STEP/DEBUG/TALK/ESCAPE/TYPEAHEAD/PROCEDURE,
PROCEDURE/PARAMETERS, SYNTAX/EXPRESSION/CONDITION/OPERATOR family l.147-160) all
exist at the cited line numbers. The delegated and out-of-scope partitions are
honest. No in-scope index item is silently omitted.

**Residual [oracle-resolves] gaps (unchanged — need DBASE.EXE/DBASE.OVL under
DOSBox):** the nine Open-questions items stand, plus the newly-surfaced
`.Y.`/`.N.` logical-literal aliases (accepted by the period interpreter but not
in any local source). None is resolvable from the present corpus.
