# Control Flow and the Procedure Model  --  dBASE III PLUS 1.1

The structured-programming layer of dBASE III PLUS 1.1: the three control
constructs (`DO WHILE`, `IF`, `DO CASE`), the procedure model
(`PROCEDURE`/`PARAMETERS`/`RETURN`/`DO ... WITH`/`SET PROCEDURE`), parameter
passing (by-reference vs by-value), and the structural rules that govern them
(the Logical-guard rule, nesting depth, opener/closer pairing).

This file is the *structural* companion to
[`programming-and-io.md`](programming-and-io.md), which gives the per-command
syntax reference for `DO`, `PROCEDURE`, `PARAMETERS`, `RETURN`,
`SET PROCEDURE`, and the I/O verbs. Scope of the private variables created by
`PARAMETERS` and the stack-based hiding model are specified in
[`../language/memory-variables.md`](../language/memory-variables.md) section3. The
Logical type and the `.AND./.OR./.NOT.` operators that build guard expressions
are in [`../language/data-types.md`](../language/data-types.md) and
[`../language/expressions-and-operators.md`](../language/expressions-and-operators.md).

## Status / confidence

- The **period-exact syntax authority** is the mined III+ HELP database
  `archive/golden-mined/HELP.DBS.strings.txt` (the on-disk HELP.DBS text).
  Markers used by that file, following the convention established in
  `programming-and-io.md`: `_<title>` = topic title, `` `5 `` = Syntax block,
  `` `6 `` = Description, `` `9 `` = See-also, `` `8 `` = the literal string
  "dBASE III PLUS". Each construct's `` `5 `` block is quoted verbatim
  (ASCII-folded).
- The topic index `archive/golden-mined/HELP_topics.txt` confirms every
  keyword in scope is a first-class III+ command/structure word.
- The **error catalog** is `archive/golden-mined/DBASE.MSG.strings.txt`; the
  1-based line number is the error code, and the line text is the exact runtime
  message string emitted (and returned by the `MESSAGE()` function /
  catalogued by `ERROR()`).
- **Real idioms** are cited by file and line from the plaintext sample programs
  in `goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/`.
- The Clipper-lineage Harbour / xHarbour docs (`oracles/*/doc/en/*.txt`) are a
  **semantic oracle only**  --  they describe the same dialect family but include
  later (Clipper 5 / [x]Harbour) extensions. Every place III+ 1.1 differs from
  that lineage is tagged inline. **III+ 1.1 has NO arrays / `DECLARE`, NO
  `LOCAL`/`STATIC`/`FUNCTION` keywords, NO `ELSEIF`, NO `==`, NO `FOR...NEXT`,
  NO code blocks, and NO explicit `@var` reference operator**  --  do not import
  any of those into a III+ claim.

---

## 1. Overview: the three structured constructs

dBASE III PLUS has exactly **three** structured control commands, each a
matched opener/closer pair that may contain any commands (including other
structured constructs) between them:

| Opener      | Optional middle words   | Closer    | Purpose                         |
|-------------|-------------------------|-----------|---------------------------------|
| `DO WHILE`  | `LOOP`, `EXIT`          | `ENDDO`   | Pre-test loop                   |
| `IF`        | `ELSE`                  | `ENDIF`   | One- or two-way branch          |
| `DO CASE`   | `CASE`, `OTHERWISE`     | `ENDCASE` | Multi-way branch (one path)     |

[verified: HELP_topics.txt lines 36-37,41,61 list `DO CASE`, `DO WHILE`,
`EXIT`, `LOOP` as topics; lines 185,187-191 list `CASE`, `ENDCASE`, `ENDDO`,
`ENDIF` as topics + HELP.DBS `_DO CASE`/`_DO WHILE`/`IF`/`_EXIT`/`_LOOP`].

III+ has **no** `FOR...NEXT` counted loop (that is a Clipper 5 / dBASE IV
addition); a counter loop in III+ is written as `DO WHILE` with a manual
counter. III+ also has **no** `ELSEIF` and **no** `BEGIN SEQUENCE`  --  those
appear in the oracle docs (`statement.txt` IF entry shows `ELSEIF`) but are NOT
III+ 1.1. [verified: III-vs-later delta  --  absent from HELP_topics.txt +
HELP.DBS; `ELSEIF` present in `oracles/xharbour/doc/en/statement.txt` line 215;
`BEGIN SEQUENCE` documented in `oracles/xharbour/doc/en/hvm.txt` lines 321-342
(NOT a `statement.txt` entry)].

### 1.1 Lexical notes that affect all three

- One command per logical line. A logical line may be continued onto the next
  physical line by ending with a trailing semicolon `;`
  [verified: HELP rules + `CBMENU.PRG:93-94` `IF balance<>mbalance .OR. ... ;`
  continued onto line 94].
- Comment lines begin with `*` (whole-line) or `&&` (trailing) and may appear
  anywhere, including between an opener and its body  --  the parser ignores them
  when matching pairs. Sample programs put long banner comments between
  `DO CASE` and the first `CASE` (e.g. `CBMENU.PRG:82-87`).
- Keywords are case-insensitive; sample programs write `DO WHILE`, `ENDDO`,
  `Case`, etc. in mixed case.
- Structure keywords are matched at **runtime by the interpreter**, not by a
  separate compiler  --  III+ `.prg` files are interpreted, so a structural error
  (unbalanced opener/closer) surfaces as a runtime error when the line is
  reached, not at "compile" time. [inferred from III+ interpreted execution
  model + the runtime error strings in section6].

---

## 2. DO WHILE ... ENDDO

### Syntax (verbatim)

```
DO WHILE <condition>
   <commands>
   [EXIT]
   [LOOP]
ENDDO
```
[verbatim, HELP.DBS `_DO WHILE` `` `5 `` block, lines 646-650.]

Description (verbatim, `` `6 ``, lines 651-656):

> Repeats the command statements between the DO WHILE and the ENDDO as long as
> the initial condition is true. LOOP causes [dBASE III PLUS] to ignore all
> subsequent commands and return to its corresponding DO WHILE to evaluate the
> `<condition>` again. EXIT causes [dBASE III PLUS] to move out of the loop and
> execute the first command after the ENDDO.

### Semantics

- **Pre-test.** `<condition>` is evaluated *before* each pass. If it is false on
  entry, the body never runs and control falls to the line after `ENDDO`.
- `<condition>` is **re-evaluated on every iteration**, including after a `LOOP`.
- `<condition>` **must be a Logical expression**  --  see section5. A non-Logical guard
  raises error 36 `Not a Logical expression.`
- `LOOP` jumps to the *most recent* (innermost enclosing) `DO WHILE` and
  re-tests its condition; commands between `LOOP` and `ENDDO` are skipped this
  pass. [verified: HELP.DBS `_LOOP` lines 838-840 "Returns control to the
  beginning of a DO WHILE construct, where the condition is reevaluated" +
  `oracles/xharbour/doc/en/statement.txt` line 37 "LOOP branches control to the
  most recently executed FOR or DO WHILE statement"].
- `EXIT` jumps **out** of the innermost enclosing `DO WHILE` to the line after
  its `ENDDO`; it does not stop the program. [verified: HELP.DBS `_EXIT` lines
  681-684 "Escapes from a DO WHILE loop and transfers control to the command
  immediately following the ENDDO. EXIT does not stop program execution."]
- `EXIT` and `LOOP` are meaningful **only inside `DO WHILE`** (III+ has no
  `FOR` loop). Using them outside a loop is invalid. [documented: HELP.DBS
  `_EXIT`/`_LOOP` are described purely in terms of `DO WHILE`.]
- `ENDDO` may carry a trailing comment word naming the loop (purely
  documentary; the interpreter ignores everything after `ENDDO`). Real use:
  `ENDDO lisa` (`CHECK.PRG:197`) and `ENDDO loop1` (`CHECK.PRG:287`) label the
  outer loops by intent. [verified: `CHECK.PRG:197,287`.] III+ does **not**
  have named/labelled loops as a feature; the word is just a trailing token.

### Canonical idiom A  --  `DO WHILE .T.` ... `EXIT` ("DO FOREVER")

An unconditional loop whose only exit is an `EXIT` (or `RETURN`) inside the
body. The sample programs document the idiom in a comment:

```
DO WHILE .T.
* DO WHILE .T. means DO WHILE TRUE  i.e. DO FOREVER
* The DO WHILE will be terminated by an EXIT command
   ...
   IF <some condition>
      EXIT
   ENDIF
   ...
ENDDO
```
[verbatim structure + comment, `CBMENU.PRG:37-39`, also `CBMENU2.PRG:41-43`,
`CHECK.PRG:16`, `RECONCIL.PRG:33`.]

### Canonical idiom B  --  the `.NOT. EOF()` / `SKIP` table walk

The dominant III+ loop over a database file: position the record pointer, then
walk record-by-record until end-of-file, advancing with `SKIP`.

```
GO TOP
DO WHILE .NOT. EOF()
   <process current record>
   SKIP
ENDDO
```
[verified: `RECONCIL.PRG:234-246` (`GO TOP` then `DO WHILE .NOT. EOF()` ...
`SKIP` ... `ENDDO`); also `RECONCIL.PRG:171-191` and `CHECK.PRG:239-247`
(`SELECT 2 / GO TOP` ... `DO WHILE .NOT. EOF()` ... `SKIP` ... `ENDDO`).]

Notes on this idiom:
- `EOF()` is the boundary test; `SKIP` with no scope advances one record and
  *sets* `EOF()` true when it tries to move past the last record. Because the
  loop is pre-test, the body never runs on an empty/exhausted file.
- Forgetting the `SKIP` is the classic infinite loop. The `SKIP` must be the
  last (or an unconditional) statement so every pass advances. In
  `RECONCIL.PRG:175-191` the `SKIP` (line 190) is reached on every pass; the
  inner `LOOP` (line 180) is in a *different* nested loop, not this walk.
- See [`navigation-query-display.md`](navigation-query-display.md) for `SKIP`,
  `GO TOP/BOTTOM`, `EOF()`, `RECNO()` semantics.

### Canonical idiom C  --  input-validation retry loop

A bare-condition `DO WHILE` that re-prompts until the answer is acceptable:

```
answer = " "
DO WHILE .NOT. answer$"YyNn"
   answer = " "
   @ 7,25 SAY "Is this correct? (Y/N) " GET answer
   READ
ENDDO
```
[verbatim, `RECONCIL.PRG:44-49`; the same pattern recurs at
`RECONCIL.PRG:120-124,193-197,217-221`, `CHECK.PRG:135-139,183-187,223-228,
267-271`.] Here the guard is a Logical (`$` substring test), re-evaluated each
pass; the loop ends when the user types a Y/N.

### `LOOP` in practice

`RECONCIL.PRG:175-191` walks the Recon file; if the operator enters a negative
amount the body `LOOP`s back to re-`READ` without committing:

```
DO WHILE .NOT. EOF()
   mamt=Amt
   @ row,col GET mamt PICTURE "9999999.99"
   READ
   IF mamt<0.00
      LOOP
   ENDIF
   ...
   SKIP
ENDDO
```
Note that `LOOP` here re-tests `.NOT. EOF()` (it does NOT re-`READ` the same
record automatically  --  the pointer has not moved, so the same record is
re-presented; the `SKIP` below is skipped). [verified: `RECONCIL.PRG:175-191`.]

---

## 3. IF / ELSE / ENDIF

### Syntax (verbatim)

```
IF <condition>
   <commands>
[ELSE
   <commands>]
ENDIF
```
[verbatim, HELP.DBS `IF` `` `5 `` block, lines 714-718.]

Description (verbatim, `` `6 ``, lines 719-723):

> Allows conditional processing of commands. When the condition is true, the
> first set of commands is executed. When the condition is false and the ELSE
> is used, the second set of commands is executed. If the ELSE is not used, the
> commands are skipped.

### Semantics

- `<condition>` **must be Logical** (section5); a non-Logical raises error 36.
- One-armed (`IF ... ENDIF`) or two-armed (`IF ... ELSE ... ENDIF`). There is
  **at most one `ELSE`** and **no `ELSEIF`** in III+ 1.1. To chain conditions,
  either nest `IF`s or use `DO CASE` (section4). [III-vs-later delta: the oracle
  `statement.txt` IF entry shows `ELSEIF` and even claims "IF...ELSEIF...ENDIF
  ... is identical to DO CASE...ENDCASE", but `ELSEIF` is a Clipper/IV
  addition; the III+ HELP.DBS `` `5 `` block has only `ELSE`.]
- `ENDIF` is required; it may carry a trailing comment word (ignored).
- A scalar `IIF(<lcond>, <true-val>, <false-val>)` **function** exists for
  inline conditional *values* (not control flow) and is used heavily in the
  samples (e.g. `RECONCIL.PRG:184,188,189,210,237`,
  `CHECK.PRG:237`). `IIF()` is documented separately in the function reference;
  it is the value-level analogue of `IF`.

### Nested IF as a chained branch (no ELSEIF)

Because there is no `ELSEIF`, a three-way validity check nests `IF`s in the
`ELSE` arm:

```
IF mamt = 0.00
   @ 18,18 SAY "Check must have an amount - please reenter"
ELSE
   IF mamt < 0.00
      @ 18,15 SAY "Check must be a positive amount - "+;
                  "please reenter"
   ELSE
      EXIT
   ENDIF
ENDIF
```
[verbatim, `CHECK.PRG:115-124`.] Each `IF` has its own `ENDIF`; the inner
`IF/ELSE/ENDIF` lives entirely inside the outer `ELSE`. This is the idiomatic
III+ substitute for `ELSEIF`.

### IF/ELSE used as a two-way result branch

```
IF truebal = test
   @ 18,17 SAY "Checkbook and bank statement exactly balance"
ELSE
   @ 18,15 SAY "Checkbook and bank statement still do not balance"
   ...
ENDIF
```
[verbatim structure, `RECONCIL.PRG:204-231`.]

### IF with early RETURN (guard / fast-exit)

```
IF truebal = balance
   @ 19,14 SAY "Checkbook and bank statement exactly balance"
   ?
   WAIT SPACE(16)+"Press any key to return to Main menu "
   USE
   RETURN
ENDIF
```
[verbatim, `RECONCIL.PRG:98-104`.] A `RETURN` (or `EXIT`/`LOOP`) inside an `IF`
arm leaves the construct immediately; the `ENDIF` is still required textually.

---

## 4. DO CASE / CASE / OTHERWISE / ENDCASE

### Syntax (verbatim)

```
DO CASE
   CASE <condition>
      <commands>
   CASE <condition>
      <commands>
   [OTHERWISE
      <commands>]
ENDCASE
```
[verbatim, HELP.DBS `_DO CASE` `` `5 `` block, lines 633-640.]

Description (verbatim, `` `6 ``, lines 641-643):

> Executes only one path from the set of alternative CASEs. If all CASEs prove
> false and if the OTHERWISE path is there, [dBASE III PLUS] chooses it instead.

### Semantics

- `DO CASE` is a **multi-way branch that runs at most one path.** The
  interpreter evaluates each `CASE <condition>` top to bottom and executes the
  block under the **first** condition that is true, then jumps to the line
  after `ENDCASE`. Subsequent `CASE`s are NOT evaluated once one matches.
  [verified: HELP.DBS `_DO CASE` "Executes only one path" + oracle
  statement.txt IF/DO CASE equivalence note.]
- Each `CASE <condition>` **must be Logical** (section5).
- `OTHERWISE` (optional, at most one, last) runs only if **no** `CASE`
  condition was true. If every `CASE` is false and there is no `OTHERWISE`, the
  whole construct does nothing and control falls through to after `ENDCASE`.
  [verified: HELP.DBS `_DO CASE` lines 642-643.]
- There is **no fall-through** between cases (unlike C `switch`): the matched
  block ends at the next `CASE`/`OTHERWISE`/`ENDCASE`. There is no `break`
  needed and no `EXIT` semantics inside `DO CASE` (`EXIT` belongs to
  `DO WHILE`, not `DO CASE`).
- Commands (including comments) may appear between `DO CASE` and the first
  `CASE`; sample code puts banner comments there (`CBMENU.PRG:82-89`).
- III+ has **no** dBASE IV / Clipper `CASE <expr> [, <expr>]` value list and no
  `SWITCH`/`OTHERWISE` C-style semantics  --  each `CASE` is an independent Logical
  test. [III-vs-later delta; the xHarbour `switch.txt` `SWITCH/CASE/EXIT`
  construct is a later addition and is NOT III+.]

### Canonical idiom  --  the menu dispatcher

`DO CASE` is *the* III+ menu-dispatch construct: read a keystroke, then branch
to the matching action via `DO <subprogram>`.

```
DO CASE
   CASE CHR(i) $ "Xx"
      ... RETURN
   CASE CHR(i) $ "Aa"
      DO Check
   CASE CHR(i) $ "Bb"
      DO Add
   ...
   CASE CHR(i) $ "Ll"
      DO Help
ENDCASE
```
[verbatim structure, `CBMENU.PRG:80-172` (13 CASEs, no `OTHERWISE`); identical
shape in `CBMENU2.PRG:88-167`.] Each `CASE` is a Logical `$` (substring) test
matching one menu key; the matched arm dispatches with `DO`. The absence of
`OTHERWISE` is deliberate  --  the surrounding input loop (`CBMENU.PRG:52-77`)
already guarantees `i` is one of the valid keys before the `DO CASE` is reached.

### REPORTS.PRG report dispatcher

`REPORTS.PRG` opens a procedure file then runs a menu `DO CASE` that dispatches
into procedures in `RPRTPRO.PRG`:

```
SET PROCEDURE TO Rprtpro      && REPORTS.PRG:15
...
   DO CASE
      ...
      DO Reporta              && dispatch into RPRTPRO.PRG procedure
      ...
   ENDCASE
...
CLOSE PROCEDURE               && REPORTS.PRG:65
```
[verified: `REPORTS.PRG:15,63,65`; target procedures at `RPRTPRO.PRG:9` etc.]

---

## 5. The Logical-guard rule (no truthiness coercion)

**Every guard expression  --  the `<condition>` of `DO WHILE`, `IF`, and each
`CASE`  --  must evaluate to the Logical type (`.T.`/`.F.`).** dBASE III PLUS does
**NOT** coerce other types to a truth value. There is no "non-zero is true",
no "non-empty string is true", no "NIL is false". A guard that evaluates to a
Numeric, Character, Date, or Memo value is a runtime **error**, not a silent
conversion.

The exact runtime message is **error 36**:

```
Not a Logical expression.
```
[verified: DBASE.MSG line 36 + the type name "Logical" appears in the type
table at DBASE.MSG line 257; cross-checked against the dynamic type model in
`../language/data-types.md`.]

Consequences and III+ specifics:

- Write the guard with the Logical operators `.AND.`, `.OR.`, `.NOT.` and the
  relational/`$` operators, which all yield Logical. Samples: `.NOT. EOF()`
  (`RECONCIL.PRG:235`), `i=0` (`CBMENU.PRG:54`, an equality test yielding
  Logical, **not** an assignment  --  see below), `.NOT. answer$"YyNn"`
  (`RECONCIL.PRG:45`), `mamt < 0` (`CHECK.PRG:55`), `balance<>mbalance .OR.
  ...` (`CBMENU.PRG:93`).
- The literal Logicals are `.T.`/`.F.` (also `.Y.`/`.N.`). `DO WHILE .T.` is
  the canonical forever-loop (section2). [verified: DBASE.MSG lines 180-181 "False"/
  "True"; sample usage `CBMENU.PRG:37`.]
- **`=` is overloaded.** In a *guard* (an expression context) `i=0` is the
  equality comparison and yields Logical; as a *command* (`i=0` on its own
  line) it is assignment/STORE. The interpreter decides by position: a
  condition slot is always an expression. III+ has **no** `==` operator (that
  is a Clipper/IV exact-comparison operator); do not introduce it. [III-vs-
  later delta; `==` absent from III+ operator set per
  `../language/expressions-and-operators.md`.]
- There is no implicit "empty"/"falsey" test. To test emptiness you call the
  function explicitly (e.g. the Logical-returning record/field tests), never by
  putting the bare value in the guard.
- A logical *field* may be used directly in a guard (it is Logical-typed), but
  note error 87 `Operation with Logical field invalid.` guards against using a
  Logical field where a non-Logical operation is attempted; that is a separate
  type error from 36. [documented: DBASE.MSG lines 36, 87.]

---

## 6. Structural rules: pairing, nesting, depth

### 6.1 Opener/closer pairing  --  each opener needs its closer

Every `DO WHILE` needs an `ENDDO`, every `IF` an `ENDIF`, every `DO CASE` an
`ENDCASE`. Constructs must be **properly nested** (no crossing): an inner
construct opened inside an outer one must also be closed inside it. The
interpreter matches closers to the most-recent unmatched opener.

The runtime error for a `DO WHILE` whose `ENDDO` is missing or mismatched is
**error 93**:

```
Mismatched DO WHILE and ENDDO.
```
[verified: DBASE.MSG line 93.] This is the only construct-specific pairing
message in the III+ catalog; mispaired `IF/ENDIF` and `DO CASE/ENDCASE`
surface through the same family of structural/syntax errors (e.g. 10
`Syntax error.`, 35 `Unrecognized phrase/keyword in command.`) when the
unexpected closer/opener is reached. [verified: DBASE.MSG lines 10, 35;
exact code emitted for a stray `ENDIF`/`ENDCASE` in III+ 1.1 is
`[oracle-resolves]`  --  needs the binary.]

### 6.2 Nesting

The three constructs nest freely inside one another to substantial depth. Real
nesting from the samples:

- `CBMENU.PRG:52-77` nests three `DO WHILE`s deep, with `IF`/`EXIT` inside the
  innermost.
- `CHECK.PRG` reaches 5+ levels: `DO WHILE .T.` (line 16) > `DO WHILE lisa`
  (31) > `DO WHILE .T.` (35) and parallel inner loops, with `IF`/`ELSE`/`ENDIF`
  and `DO WHILE` for input validation nested inside the `IF` arms (e.g. the
  `IF balance < mamt ... DO WHILE .NOT. ans$"YyNn" ... ENDDO ... ENDIF` block
  at `CHECK.PRG:128-159`).
- `CHECK.PRG:115-124` nests `IF/ELSE` two deep inside a `DO WHILE`.

The oracle states the rule generically: "[constructs] may be nested ... to any
depth. The only requirement is that each control structure is properly nested."
[documented: `oracles/xharbour/doc/en/statement.txt` lines 63-66 (FOR entry);
consistent with III+ sample depth.] An exact per-construct nesting *cap* for
III+ 1.1 is
not documented in the local sources  --  `[oracle-resolves]` (the interpreter's
control-structure stack size in DBASE.EXE/DBASE.OVL).

### 6.3 Procedure-call (DO) nesting depth

Distinct from in-program construct nesting is the **DO call stack**  --  how deep
`DO` calls can nest (program calling program calling procedure ...). Exceeding
it is **error 100**:

```
DOs nested too deep.
```
[verified: DBASE.MSG line 100.] The documented III+ limit is **32 levels** of
`DO` nesting; the exact 1.1 figure is `[oracle-resolves]`. See
[`programming-and-io.md`](programming-and-io.md) section2 for the DO-stack model.

---

## 7. The procedure model

This section is the *model*; the per-command syntax (the `` `5 `` blocks for
`DO`, `PROCEDURE`, `PARAMETERS`, `RETURN`, `SET PROCEDURE`) lives in
[`programming-and-io.md`](programming-and-io.md) sectionsection2-5 and is not duplicated
here.

### 7.1 The unit of modularity

III+ has exactly **two** kinds of callable code unit  --  and crucially, **no
user-defined functions** at the language level (no `FUNCTION` keyword; that is
Clipper 5 / dBASE IV). [III-vs-later delta: oracle `statement.txt` documents
`FUNCTION`/`STATIC FUNCTION`; these are NOT III+ 1.1.]

1. **A standalone program file**  --  a `.prg` whose body runs top to bottom when
   you `DO <file>`. Every sample's main file is one of these
   (`CBMENU.PRG`, `RECONCIL.PRG`, `CHECK.PRG`, `HELP.PRG`).
2. **A named procedure**  --  a `PROCEDURE <name>` block, terminated by `RETURN`
   (or by the next `PROCEDURE` line, or end of file), living either:
   - inside a **procedure file** opened with `SET PROCEDURE TO <file>`, or
   - (the III+ HELP describes procedures as living "in a procedure file"  -- 
     `PROCEDURE` "Identifies the beginning of each routine in a procedure
     file" [verbatim, HELP.DBS `_PROCEDURE` lines 960-962]).

A procedure is invoked exactly like a program: `DO <name>`.

### 7.2 PROCEDURE blocks and the 32-per-file limit

```
PROCEDURE <procedure name>
   <commands>
RETURN
```

- A procedure file is an ordinary text/`.prg` file holding one or more
  `PROCEDURE <name>` blocks. Real layout: `RPRTPRO.PRG` holds 9 procedures
  (`PROCEDURE Reporta` at line 9, `Reporta1` at 126, `Reporta2` at 158,
  `Reporta3` at 217, `Reporta4` at 268, `Report14` at 275, `Report24` at 313,
  `Reporta5` at 354, `Printer` at 416), each ending in `RETURN`. [verified:
  `RPRTPRO.PRG` PROCEDURE lines 9,126,158,217,268,275,313,354,416.]
- **A procedure file holds at most 32 procedures** in dBASE III PLUS. This is
  the documented III+ cap. [documented: dBASE III PLUS reference; the exact 1.1
  enforcement and the error emitted on the 33rd procedure are `[oracle-
  resolves]`  --  not present in the local DBASE.MSG catalog.]
- Procedure names follow the III+ identifier rules (begin with a letter; then
  letters/digits/underscore; significant to the field/memvar length). The exact
  significant length for III+ procedure names (8 vs 10) is `[oracle-resolves]`.
  See [`programming-and-io.md`](programming-and-io.md) section4.

### 7.3 SET PROCEDURE TO  --  opening the procedure file

```
SET PROCEDURE TO [<procedure file>]
```
[verbatim, HELP.DBS `_SET PROCEDURE` line 1551.]

Description (verbatim, lines 1552-1555):

> Opens a named procedure file. Only one procedure file can be open at a time.
> SET PROCEDURE TO or CLOSE PROCEDURE closes the open procedure file.

- **Exactly one procedure file open at a time**  --  opening another replaces it.
  [verified: HELP.DBS `_SET PROCEDURE` lines 1552-1553.]
- `SET PROCEDURE TO` with no file, or `CLOSE PROCEDURE`, closes it.
- Default extension `.prg`.
- Real idiom: `SET PROCEDURE TO Rprtpro` (`REPORTS.PRG:15`), run the dispatch
  loop calling `DO Reporta` etc. (line 63), then `CLOSE PROCEDURE` (line 65).
  [verified: `REPORTS.PRG:15,63,65`.]

### 7.4 DO ... WITH  --  the call

```
DO <program file>/<procedure name> [WITH <parameter list>]
```
[verbatim, HELP.DBS `_DO` line 626.]

> Executes a program file or a procedure within an open procedure file.
> Parameters to the called program can be passed WITH the parameter list.
> [verbatim, HELP.DBS `_DO` lines 627-629.]

Real calls: `DO Check`, `DO Add`, `DO Help`, `DO Reconcil` (no args,
`CBMENU.PRG:122-170`); `DO Numwords WITH mamt` (one arg, `CHECK.PRG:163`);
`DO Menumask` (`CBMENU.PRG:43`).

### 7.5 Procedure search order

When `DO <name>` is issued (no path/extension), III+ resolves `<name>` as:

1. A **`PROCEDURE <name>` block in the currently open procedure file** (the one
   opened by `SET PROCEDURE TO`), if one is open. [documented: HELP.DBS `_DO`
   "or a procedure within an open procedure file" + `_SET PROCEDURE`.]
2. Otherwise, a **disk file `<name>.prg`** in the current/`SET PATH` directory,
   which is opened and run as a standalone program. [documented: HELP.DBS `_DO`;
   the `.prg` default extension is the III+ program-file convention.]

The exact precedence **when both a same-named open procedure and a `<name>.prg`
exist** (procedure wins vs. file wins) is not nailed by the local sources;
Clipper-lineage resolution favours the procedure first. `[oracle-resolves]` for
III+ 1.1 ordering and for whether `SET PATH` participates in the procedure-name
lookup. See [`programming-and-io.md`](programming-and-io.md) section2 for the fuller
treatment.

### 7.6 RETURN  --  leaving a routine

```
RETURN [TO MASTER]
```
[verbatim, HELP.DBS `_RETURN` line 1063.]

> Closes the current program file and restores control to the calling routine
> or to the dot prompt. The TO MASTER option returns control to the top level
> routine rather than to the calling one. [verbatim, HELP.DBS `_RETURN`
> 1064-1067.]

- Plain `RETURN` pops one frame: control resumes at the line **after** the `DO`
  that invoked the routine. A routine also returns implicitly at end-of-file
  and at the next `PROCEDURE` line.
- `RETURN TO MASTER` unwinds the whole DO stack back to the outermost program.
- `RETURN` releases the routine's PRIVATE variables  --  including the names
  created by `PARAMETERS` (see [`../language/memory-variables.md`](../language/memory-variables.md)
  section3); PUBLIC variables persist.
- III+ `RETURN` does **not** carry a return value (`RETURN <exp>` is the
  Clipper/IV user-defined-function form). III+ procedures communicate results
  by **modifying by-reference parameters** (section8) or PUBLIC variables.
  [III-vs-later delta: oracle `statement.txt` RETURN shows `RETURN <exp>`;
  III+ HELP.DBS shows only `RETURN [TO MASTER]`.]
- Every sample procedure/program ends in `RETURN` (e.g. `HELP.PRG:127`,
  `RECONCIL.PRG:103,228,268`, `CHECK.PRG:51,293`, `CBMENU.PRG:111,176`).

Related: `CANCEL` aborts the whole program back to the dot prompt; `RETRY`
re-runs the calling line; `SUSPEND`/`RESUME` pause/continue. Those are in
[`programming-and-io.md`](programming-and-io.md) section5.

---

## 8. Parameter passing: by-reference vs by-value

### 8.1 PARAMETERS  --  the receiving statement

```
PARAMETERS <parameter list>
```
[verbatim, HELP.DBS `_PARAMETERS` line 945.]

> Assigns local variable names to data items passed from a calling program.
> This is the receiving command for the variables passed with the DO `<file
> name>` WITH `<parameter list>`. [verbatim, HELP.DBS `_PARAMETERS` 946-949.]

- `PARAMETERS` must be the **first executable statement** of the called routine
  (comment/blank lines may precede it). Real: `NUMWORDS.PRG:8` `PARAMETERS
  mamt` is the first non-comment line; `RPRTPRO.PRG:417` `PARAMETERS row,pr` is
  the first line of `PROCEDURE Printer`.
- The parameter names are **PRIVATE to the called routine** and are released on
  `RETURN`. [documented: HELP.DBS `_PARAMETERS` "local variable names" +
  oracle `statement.txt` RETURN "All private variables ... released when control
  returns"; scope detail in
  [`../language/memory-variables.md`](../language/memory-variables.md) section3.]
- Calling a routine that has no `PARAMETERS` statement with `DO ... WITH`
  raises **error 90**:
  ```
  No PARAMETER statement found.
  ```
  [verified: DBASE.MSG line 90.]
- A `WITH` count that exceeds the `PARAMETERS` count raises **error 91**:
  ```
  Wrong number of parameters.
  ```
  [verified: DBASE.MSG line 91.] Passing *fewer* `WITH` args than parameters is
  permitted in this dialect family; the trailing parameters are uninitialized
  (Clipper initializes them to `.F.`/NIL). The precise III+ 1.1 initialization
  value of unmatched trailing parameters is `[oracle-resolves]`.

### 8.2 The by-reference vs by-value rule

This is the most important  --  and most surprising  --  III+ semantic in `DO ...
WITH`:

- **A bare memory-variable name or a bare field name** in the `WITH` list is
  passed **BY REFERENCE.** The called routine's corresponding `PARAMETERS`
  variable becomes an alias for the caller's variable; any change the callee
  makes is written back to the caller on `RETURN`.
- **Any expression** in the `WITH` list  --  anything that is not a bare name:
  `a+1`, `UPPER(s)`, a literal, a function call, or even a name wrapped in
  parentheses `(x)`  --  is passed **BY VALUE.** The callee gets a private copy;
  changes do not propagate back.

[documented: this is the Clipper/dBASE-lineage `DO ... WITH` rule, confirmed by
the oracle `oracles/harbour/doc/en/var.txt` `hb_PIsByRef()` entry (lines
883-917): a bare variable passed to a procedure is by-reference (`.T.`), a value
expression is by-value (`.F.`). The III+ HELP text describes `PARAMETERS` as
receiving "data items passed from a calling program" without spelling out the
by-reference rule; it is consistent across the dialect family.]

**III-vs-later delta  --  no `@` reference operator.** In III+, by-reference is the
*default* for a bare name; you do not (and cannot) write `@var`. The explicit
`@cVar` reference operator shown in the oracle (`var.txt` example line 910,
`Test( @cVar, ... )`) is a Clipper 5 addition. In Clipper, a bare variable in a
`DO ... WITH` is still by-reference (Clipper preserves the dBASE `DO` rule), but
in function-call syntax you need `@`. III+ has no function-call-with-args syntax
for user code, so the only mechanism is `DO ... WITH`, and bare names are
by-reference. [III-vs-later delta; verified against var.txt.]

### 8.3 Worked example (from the samples)

Caller (`CHECK.PRG:161-165`):

```
IF mamt <> tamt
   * display words on check
   DO Numwords WITH mamt
   tamt = mamt
ENDIF
```

Callee (`NUMWORDS.PRG:8`):

```
PARAMETERS mamt
```

[verified: `CHECK.PRG:163` + `NUMWORDS.PRG:8`.] `mamt` is a bare memvar, so it
is passed **by reference**: `NUMWORDS.PRG` builds the spelled-out dollar amount
into screen output (it reads `mamt`); had it reassigned `mamt`, the new value
would propagate back to `CHECK.PRG`'s `mamt`. Note the callee's parameter is
*named* `mamt` too, but that is incidental  --  the binding is positional, not by
name.

To force **by value**, the caller would write `DO Numwords WITH (mamt)`  --  the
parentheses turn the bare name into an expression. [documented: dialect rule;
the `(x)` by-value idiom is `[oracle-resolves]` for III+ 1.1 specifically.]

### 8.4 Communicating results without RETURN values

Because III+ `RETURN` carries no value (section7.6), a procedure returns a result by
one of:

1. **A by-reference parameter** the caller passes in and reads back after the
   `DO` (the primary mechanism, section8.2).
2. **A PUBLIC variable** both routines can see (e.g. `balance`, `lastchk`,
   `today` in the checkbook system are visible across `CBMENU.PRG` and its
   subprograms via the RESTORE-from-`.mem` / private-at-top mechanism;
   `CBMENU.PRG:31-35`). See
   [`../language/memory-variables.md`](../language/memory-variables.md) section3 for
   the visibility model.

---

## 9. Quick reference  --  keyword cheat sheet

| Keyword     | Belongs to    | Effect                                                      |
|-------------|---------------|-------------------------------------------------------------|
| `DO WHILE`  | loop opener   | pre-test loop; re-tests Logical guard each pass             |
| `ENDDO`     | loop closer   | end of `DO WHILE`; matched to nearest open `DO WHILE`       |
| `EXIT`      | inside loop   | jump out of innermost `DO WHILE`, to line after its `ENDDO` |
| `LOOP`      | inside loop   | jump to innermost `DO WHILE`, re-test guard                 |
| `IF`        | branch opener | run block if Logical guard `.T.`                            |
| `ELSE`      | branch middle | optional; run if `IF` guard `.F.` (max one, no `ELSEIF`)    |
| `ENDIF`     | branch closer | end of `IF`                                                 |
| `DO CASE`   | branch opener | multi-way; runs the first true `CASE` only                  |
| `CASE`      | branch arm    | a Logical test; first true one runs                         |
| `OTHERWISE` | branch arm    | optional, last; runs if no `CASE` true                      |
| `ENDCASE`   | branch closer | end of `DO CASE`                                            |
| `PROCEDURE` | proc opener   | start of a named routine (in a procedure file)             |
| `PARAMETERS`| proc header   | first stmt; names the incoming `WITH` args (PRIVATE)        |
| `RETURN`    | proc/program  | pop one DO frame; `TO MASTER` unwinds to top                |
| `DO ... WITH` | call        | invoke proc/program, pass args (bare name = by-ref)         |
| `SET PROCEDURE TO` | env      | open the (single) procedure file                           |

---

## Open questions

These are unresolved by the local sources and should be settled against the
golden binary (`goldens/dbase-iii-plus-1.1/extracted/disk1/DBASE.EXE` +
`disk2/DBASE.OVL`). [oracle-resolves]

1. **DO-name precedence when both exist.** If an open procedure file contains
   `PROCEDURE Foo` *and* a disk file `FOO.PRG` exists, which does `DO Foo` run?
   Clipper-lineage favours the procedure; confirm for III+ 1.1, and whether
   `SET PATH` participates in procedure-name resolution. (section7.5)
2. **Exact 32-procedures-per-file enforcement.** Confirm the cap is 32 in 1.1
   and identify the error string emitted on the 33rd `PROCEDURE` (not in the
   local DBASE.MSG catalog). (section7.2)
3. **Procedure-name significant length.** 8 vs 10 significant characters for
   `PROCEDURE`/`DO` names in 1.1. (section7.2)
4. **Construct nesting cap.** The maximum nesting depth of
   `DO WHILE`/`IF`/`DO CASE` (interpreter control-structure stack size), as
   distinct from the 32-level DO-call stack (error 100). (section6.2-6.3)
5. **Stray-closer error codes.** Which exact error code (10 `Syntax error.`,
   35 `Unrecognized phrase/keyword in command.`, or another) a mismatched
   `ENDIF`/`ENDCASE` raises in 1.1 (the `DO WHILE` case is settled: error 93).
   (section6.1)
6. **Unmatched-parameter initialization.** When `WITH` passes fewer args than
   `PARAMETERS` names, what value do the trailing parameters take in 1.1
   (uninitialized PRIVATE `.F.`, or undefined)? (section8.1)
7. **The `(x)` by-value idiom.** Confirm that wrapping a bare name in
   parentheses in a `WITH` list forces by-value in III+ 1.1 (as in the dialect
   family). (section8.3)

---

## Sources

- `archive/golden-mined/HELP.DBS.strings.txt`  --  period-exact III+ command help
  (the on-disk HELP.DBS). Verbatim `` `5 `` syntax blocks quoted:
  `_DO` (626-630), `_DO CASE` (633-643), `_DO WHILE` (646-656), `IF` (714-723),
  `_EXIT` (681-685), `_LOOP` (838-841), `_PARAMETERS` (945-950),
  `_PROCEDURE` (960-963), `_RETURN` (1063-1068), `_SET PROCEDURE` (1551-1556).
- `archive/golden-mined/HELP_topics.txt`  --  the 406-topic command/function index
  (lines 36-37, 41, 61, 74, 76, 87, 134, 185-191 confirm the structure
  keywords are first-class III+ topics).
- `archive/golden-mined/DBASE.MSG.strings.txt`  --  error catalog (code = line):
  10 `Syntax error.`, 35 `Unrecognized phrase/keyword in command.`,
  36 `Not a Logical expression.`, 87 `Operation with Logical field invalid.`,
  90 `No PARAMETER statement found.`, 91 `Wrong number of parameters.`,
  93 `Mismatched DO WHILE and ENDDO.`, 100 `DOs nested too deep.`;
  type names 255-260; Logical literals 180-181.
- Sample programs (`goldens/dbase-iii-plus-1.1-pristine/files/
  Sample_Programs_and_Utilities/`):
  - `CBMENU.PRG` / `CBMENU2.PRG`  --  `DO WHILE .T.` forever-loop and the 13-CASE
    `DO CASE` menu dispatcher (CBMENU 37-39, 52-77, 80-172).
  - `RECONCIL.PRG`  --  `.NOT. EOF()`/`SKIP` walks (171-191, 234-246), `LOOP`
    (180), `EXIT` (54,129,199), nested `IF/ELSE` (204-231), early `RETURN`
    (98-104).
  - `CHECK.PRG`  --  deep loop/IF nesting (16,31,35,90,110), nested `IF/ELSE`
    (115-124), labelled `ENDDO lisa`/`ENDDO loop1` (197,287), `DO ... WITH`
    by-reference call (163), EOF/SKIP walk (239-247).
  - `HELP.PRG`  --  leaf procedure invoked via `DO Help`, terminated by `RETURN`
    (127).
  - `NUMWORDS.PRG`  --  `PARAMETERS mamt` receiver (8) for `CHECK.PRG:163`.
  - `RPRTPRO.PRG`  --  9-procedure procedure file (PROCEDURE at 9,126,158,217,268,
    275,313,354,416; `PARAMETERS row,pr` at 417).
  - `REPORTS.PRG`  --  `SET PROCEDURE TO Rprtpro` (15), `DO Reporta` dispatch (63),
    `CLOSE PROCEDURE` (65).
- Semantic oracle only (Clipper/Harbour lineage, NOT period-exact, later
  extensions tagged inline):
  `oracles/xharbour/doc/en/statement.txt` (IF/PROCEDURE/RETURN/FOR entries  -- 
  source of the `ELSEIF` [line 215] / `FUNCTION` [94,100] / `RETURN <exp>`
  [108,144] deltas, the FOR/LOOP "most recently executed" note [line 37], and
  the "to any depth / properly nested" nesting rule [lines 63-66]),
  `oracles/xharbour/doc/en/hvm.txt` (`BEGIN SEQUENCE`/`BREAK` 321-342  --  the
  later sequence construct, NOT III+),
  `oracles/harbour/doc/en/var.txt` (`hb_PIsByRef()` 883-917  --  by-reference vs
  by-value confirmation and the `@` operator delta [example line 910]),
  `oracles/xharbour/doc/en/switch.txt` (the later `SWITCH/CASE` construct, NOT
  III+).
- Cross-links: [`programming-and-io.md`](programming-and-io.md) (per-command
  `DO`/`PROCEDURE`/`PARAMETERS`/`RETURN`/`SET PROCEDURE` syntax + DO-stack),
  [`../language/memory-variables.md`](../language/memory-variables.md) (scope of
  `PARAMETERS` privates, PUBLIC/PRIVATE hiding),
  [`../language/data-types.md`](../language/data-types.md) and
  [`../language/expressions-and-operators.md`](../language/expressions-and-operators.md)
  (Logical type, `.AND./.OR./.NOT.`, `=` vs absence of `==`),
  [`navigation-query-display.md`](navigation-query-display.md) (`SKIP`,
  `GO TOP`, `EOF()`, `RECNO()`).

---

## Verification

Adversarial re-verification on 2026-06-16 against the local corpus. Every
primary citation (HELP.DBS `` `5 `` syntax blocks, DBASE.MSG error codes,
HELP_topics topic lines, sample-program line numbers) was re-checked; every
oracle path was opened to confirm it exists and contains the quoted text.

**Completeness  --  all required III+ constructs present with verbatim syntax:**

| Construct | Section | Syntax source (re-verified) |
|-----------|---------|-----------------------------|
| `DO WHILE`/`LOOP`/`EXIT`/`ENDDO` | section2 | HELP.DBS `` `5DO WHILE `` block at 646; `_EXIT`/`_LOOP` at 680/837 |
| `IF`/`ELSE`/`ENDIF` | section3 | HELP.DBS `` `5IF `` block at 714 |
| `DO CASE`/`CASE`/`OTHERWISE`/`ENDCASE` | section4 | HELP.DBS `` `5DO CASE `` block at 633 |
| `PROCEDURE` | section7.2 | HELP.DBS `_PROCEDURE` `` `5 `` at 960 |
| `PARAMETERS` | section8.1 | HELP.DBS `_PARAMETERS` `` `5 `` at 945 |
| `RETURN [TO MASTER]` | section7.6 | HELP.DBS `_RETURN` `` `5 `` at 1063 |
| `DO ... WITH` | section7.4 | HELP.DBS `` `5DO <program file> `` at 626 |
| `SET PROCEDURE TO` | section7.3 | HELP.DBS `_SET PROCEDURE` `` `5 `` at 1551 |

No construct missing; no construct was added beyond the III+ surface.

**III+-correctness (no later-version rule asserted as III+ fact):** the IV /
Clipper-5 deltas are all present and correctly disclaimed  --  no `FOR...NEXT`/
`SCAN` (section1), no `ELSEIF` (section3,section1), no `==` (section5), no `FUNCTION`/`LOCAL`/`STATIC`
(section7.1), no `RETURN <exp>` (section7.6), no `@var` operator (section8.2), no `SWITCH`/
value-list `CASE` (section4), no arrays/`DECLARE`, no `BEGIN SEQUENCE` (section1). Each is
backed by absence from HELP_topics.txt + HELP.DBS and presence in the oracle as
a later-dialect feature. **No IV-ism is smuggled into a III+ claim.**

**Error catalog (DBASE.MSG, code = 1-based line)  --  all re-confirmed exact:**
10 `Syntax error.`, 35 `Unrecognized phrase/keyword in command.`,
36 `Not a Logical expression.`, 87 `Operation with Logical field invalid.`,
90 `No PARAMETER statement found.`, 91 `Wrong number of parameters.`,
93 `Mismatched DO WHILE and ENDDO.`, 100 `DOs nested too deep.`;
type name `Logical` at 257; Logical literals `False`/`True` at 180/181.

**Sample idioms  --  all re-confirmed at the cited lines:** `CBMENU.PRG`
37-39/52-77/80-95/117-122(`CASE "Aa" -> DO Check`)/176(`RETURN`);
`RECONCIL.PRG` 44-49/175-191/234-246; `CHECK.PRG` 115-124/161-165/197/287;
`NUMWORDS.PRG:8` (`PARAMETERS mamt`); `RPRTPRO.PRG` PROCEDURE lines
9/126/158/217/268/275/313/354/416 + `PARAMETERS row,pr` at 417;
`REPORTS.PRG` 15/63/65.

**Semantic cross-check (guard-must-be-Logical, by-ref vs by-value):** the
oracle `oracles/xharbour/doc/en/statement.txt` IF entry states "`<lCondition>`
is a logical control expression", corroborating error 36 and the section5 no-coercion
rule. The by-reference-for-bare-name / by-value-for-expression rule is confirmed
by `oracles/harbour/doc/en/var.txt` `hb_PIsByRef()` (883-917): bare names -> `.T.`,
expressions -> `.F.`. The `(x)`-forces-by-value idiom remains correctly tagged
`[oracle-resolves]` for III+ 1.1 specifically.

**Corrections applied during this pass (4):**

1. section1 (line ~66): the III+-delta tag previously cited
   `oracles/xharbour/doc/en/statement.txt` "lines 215,89" for `ELSEIF` and
   `BEGIN SEQUENCE`. Line 215 does carry `[ELSEIF <lCondition2>]` (correct), but
   line 89 is `$SEEALSO$ WHILE,FOR EACH,IF`  --  it does **not** document
   `BEGIN SEQUENCE`. `BEGIN SEQUENCE` is documented in
   `oracles/xharbour/doc/en/hvm.txt` 321-342. Citation corrected; tag raised to
   `[verified]` (two mined sources + oracle).
2. section2 LOOP semantics (line ~121): citation `oracles/harbour/doc/en/statement.txt`
   is a **nonexistent path**  --  there is no `statement.txt` anywhere under
   `oracles/harbour/`. The quoted "most recently executed FOR or DO WHILE"
   text lives in `oracles/xharbour/doc/en/statement.txt` line 37. Path
   corrected.
3. section6.2 nesting rule (line ~481): same nonexistent
   `oracles/harbour/doc/en/statement.txt` path corrected to
   `oracles/xharbour/doc/en/statement.txt` lines 63-66 (FOR entry).
4. Sources footer: removed the nonexistent `oracles/harbour/doc/en/statement.txt`
   source line; folded its (real, xharbour) content into the existing xharbour
   entry and added the `hvm.txt` `BEGIN SEQUENCE` source.

The `oracles/harbour/doc/en/var.txt` citation (the one *real* harbour-tree
reference) was confirmed present and accurate.

**Residual `[oracle-resolves]` items (unchanged; need the binary):** the seven
Open-questions items (DO-name precedence, 32-per-file cap + 33rd-procedure
error, procedure-name significant length, construct nesting cap, stray
`ENDIF`/`ENDCASE` error code, unmatched-parameter init value, `(x)` by-value
confirmation) are correctly scoped and not over-claimed.

**Verdict: PASS (after 4 citation corrections).** All construct syntax,
error codes, and sample idioms are accurate to the local ground truth; the only
defects were oracle-path citations (a fabricated `oracles/harbour/.../statement.txt`
path and a mis-numbered `BEGIN SEQUENCE` line), now fixed to point at the real
`oracles/xharbour/` files. Code blocks are ASCII-clean; non-ASCII is confined to
prose ` -- `/`section`.
