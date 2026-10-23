# Programming, I/O, and System Commands — dBASE III PLUS 1.1

Full reference for the procedural-language layer of dBASE III PLUS 1.1 (1986):
program control (`DO`/`PROCEDURE`/`PARAMETERS`/`RETURN`/`SET PROCEDURE`), the
console-I/O verbs (`@...SAY/GET`, `READ`, `ACCEPT`, `INPUT`, `WAIT`, `STORE`,
`TEXT...ENDTEXT`), memory-variable persistence (`SAVE`/`RESTORE`), event traps
(`ON ERROR`/`ON ESCAPE`/`ON KEY`), program-state verbs
(`SUSPEND`/`RESUME`/`CANCEL`/`QUIT`/`RETRY`), the shell-out and binary-module
verbs (`RUN`/`!`, `LOAD`/`CALL`), `EJECT`, and the I/O routing SETs
(`SET ALTERNATE`, `SET CONSOLE`, `SET PRINT`/`SET PRINTER`, `SET DEVICE`,
`SET MARGIN`).

## Status / confidence

- The **period-exact syntax authority** is the mined III+ HELP database
  `archive/golden-mined/HELP.DBS.strings.txt` (the on-disk HELP.DBS text;
  markers `_<title>`, `` `5 `` = Syntax, `` `6 `` = Description, `` `9 `` = See
  also, `` `8 `` = literal "dBASE III PLUS"). Every command in scope appears
  there; I quote each command's `` `5 `` syntax block verbatim (ASCII-folded).
- The **error catalog** is `archive/golden-mined/DBASE.MSG.strings.txt`
  (line number = error code emitted to `ERROR()` / `MESSAGE()`).
- **Real idioms** are cited from the 29 plaintext sample `.PRG` programs in
  `goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/`.
- The Clipper-lineage Harbour/xHarbour docs are a *semantic* oracle only, never
  period-exact syntax; III+-vs-later deltas are tagged inline.
- **`oracle-resolves`** items (behavior the mined text under-specifies; needs
  the real `DBASE.EXE`/`DBASE.OVL` under DOSBox) are gathered in
  **Open questions**.

Control-flow keywords (`IF/ELSE/ENDIF`, `DO CASE/CASE/OTHERWISE/ENDCASE`,
`DO WHILE/ENDDO`, `EXIT`, `LOOP`) and the full-screen `@...SAY/GET`, `READ`,
`@...TO`, `@...CLEAR`, `PICTURE`, `CLEAR` family are summarized here and
cross-linked to their dedicated specs; this file owns the procedural/I/O/system
surface.

---

## 1. Program execution model

A **program file** (`.prg`) is a sequence of commands, one per line. A logical
line may be continued onto the next physical line with a trailing semicolon
`;` (used throughout the samples, e.g. `CBMENU.PRG:93`
`IF balance<>mbalance .OR. lastchk<>mlastchk .OR. ;`). Comment lines begin with
`NOTE` or `*`; an in-line trailing comment is introduced by `&&`
[verified: HELP.DBS `_NOTE/*` + sample `PROG.PRG:2` `clear  &&  clear screen`].

```
NOTE <text>
* <text>
<command> && <text>
```

Programs are launched from the dot prompt or another program with `DO`. dBASE
maintains a **DO call stack** (program/procedure nesting). Error 100
`DOs nested too deep.` bounds it [documented: DBASE.MSG 100]. Commands are
case-insensitive; identifiers (field/memvar names) are case-insensitive. The
default extension supplied to `DO` and `MODIFY COMMAND` is `.prg`
[documented: HELP.DBS `_MODIFY COMMAND/FILE`].

### Line length limit

Error 18 `Line exceeds maximum of 254 characters.` — a single logical command
line (after `;` continuation joining) may not exceed 254 characters
[documented: DBASE.MSG 18].

---

## 2. DO / DO WITH

```
DO <program file>/<procedure name> [WITH <parameter list>]
```
[verified: HELP.DBS line 626 (the `DO` syntax block, keyed under MAIN MENU
header) + sample `RPRTPRO.PRG`].

Executes a program file (`.prg`) or a `PROCEDURE` within the currently open
procedure file. **Resolution order** when `DO <name>` is issued: dBASE first
looks for a procedure named `<name>` in the open procedure file (see
`SET PROCEDURE`), and if none is found, opens and runs a disk file `<name>.prg`
[documented: HELP.DBS `_PROCEDURE` "Identifies the beginning of each routine in
a procedure file"; `_SET PROCEDURE`; cross-checked against Clipper resolution
order in Harbour]. The exact precedence (procedure vs. disk file when both
exist) is `[oracle-resolves]`.

`WITH <parameter list>` passes a comma-separated list of expressions to the
called routine, which receives them via a matching `PARAMETERS` statement
(Section 3). Arguments may be field references, memvars, constants, or
expressions.

Real idioms:
- No-argument call: `DO Menumask` (`CBMENU.PRG:43`), `DO Check` (`:122`).
- Positional args: `DO Printer WITH newrow,pr` (`RPRTPRO.PRG:136`),
  `DO Numwords WITH mamt` (`CHECK.PRG:163`).
- A procedure call resolves to a `PROCEDURE` block in the file opened by
  `SET PROCEDURE TO Rprtpro` (`REPORTS.PRG:15`, target `RPRTPRO.PRG`).

### Parameter passing semantics (by-reference vs by-value)

When a **bare memory variable or field name** is passed in the `WITH` list, it
is passed **by reference**: changes the called routine makes to the
corresponding `PARAMETERS` variable are written back to the caller's variable.
When an **expression** (anything other than a single variable name, e.g.
`mamt+1`, `(x)`, `UPPER(s)`) is passed, it is passed **by value**
[documented: Harbour/Clipper `DO` semantics; consistent with III+ documentation
lineage] — note the III+ samples rely on this: `Printer` receives `pr` and the
caller reads back the chosen `pr` value (`RPRTPRO.PRG:417-425`,
`CBMENU`/`REPORTS` pass a bare `pr`). The exact III+ 1.1 rule for parenthesized
variables `(x)` forcing by-value is `[oracle-resolves]`.

### Errors

- 91 `Wrong number of parameters.` — `WITH` count exceeds the routine's
  `PARAMETERS` count (fewer is allowed; extras are uninitialized)
  [documented: DBASE.MSG 91].
- 90 `No PARAMETER statement found.` — `DO ... WITH` to a routine that has no
  `PARAMETERS` line [documented: DBASE.MSG 90].

See also: `PARAMETERS`, `SET PROCEDURE`, `RETURN`.

---

## 3. PARAMETERS

```
PARAMETERS <parameter list>
```
[verified: HELP.DBS `_PARAMETERS` + sample `NUMWORDS.PRG:8`,
`RPRTPRO.PRG:417`].

"Assigns local variable names to data items passed from a calling program. This
is the receiving command for the variables passed with the `DO <file name> WITH
<parameter list>`." [documented: HELP.DBS `_PARAMETERS`].

- `PARAMETERS` must be the **first executable statement** of the called program
  or `PROCEDURE` (comment/blank lines may precede it). Error 90
  `No PARAMETER statement found.` and error 92 `Valid only in programs.` guard
  misuse [documented: DBASE.MSG 90, 92].
- The named parameter variables are created as **PRIVATE** to the called
  routine (they are automatically released on `RETURN`), and they hide any
  same-named variables of higher scope for the duration of the routine
  [documented: HELP.DBS `_PRIVATE`; Clipper/Harbour semantics — III+ matches].
- The count of names in `PARAMETERS` must be `>=` the count of `WITH`
  expressions; if greater, the count must match or error 91 fires. (The mined
  text and Clipper agree that *fewer* WITH args than parameters leaves the
  trailing parameters as uninitialized logical `.F.`; the precise III+ 1.1
  initialization of unmatched parameters is `[oracle-resolves]`.)

Real idioms:
- `PARAMETERS mamt` then immediately uses `mamt` (`NUMWORDS.PRG:8-18`); caller
  is `CHECK.PRG:163` `DO Numwords WITH mamt`.
- `PARAMETERS row,pr` inside `PROCEDURE Printer` (`RPRTPRO.PRG:417`); caller
  `DO Printer WITH newrow,pr`.

See also: `DO`.

---

## 4. PROCEDURE / SET PROCEDURE TO

### PROCEDURE

```
PROCEDURE <procedure name>
```
[documented: HELP.DBS `_PROCEDURE`].

Marks the start of a named routine inside a **procedure file**. A procedure file
is an ordinary text/`.prg` file containing one or more `PROCEDURE <name>` blocks,
each terminated by `RETURN` (or by the next `PROCEDURE`, or end of file). A
procedure is invoked with `DO <name>`.

- Procedure names follow memvar/field naming rules (start with a letter; letters,
  digits, underscore; up to 10 significant characters) [documented: HELP.DBS
  `_FIELD NAME`/`_MEMORY VARIABLE`; III+ procedure names are limited to 8
  significant chars per the manual is `[oracle-resolves]`].
- A single procedure file holds at most **32 procedures** in III+
  [documented: dBASE III PLUS reference; `[oracle-resolves]` for exact 1.1 cap].

Real layout (`RPRTPRO.PRG`): a header section then
`PROCEDURE Reporta` ... `PROCEDURE Reporta1` ... `PROCEDURE Printer`
(`:9,:126,:158,...,:416`), each ending in `RETURN`.

### SET PROCEDURE TO

```
SET PROCEDURE TO [<procedure file>]
```
[verified: HELP.DBS `_SET PROCEDURE` + sample `REPORTS.PRG:15`,`:65`].

- Opens the named procedure file. **Only one procedure file may be open at a
  time.** Opening a new one closes the previous [documented: HELP.DBS
  `_SET PROCEDURE`].
- `SET PROCEDURE TO` (no file) **or** `CLOSE PROCEDURE` closes the open
  procedure file [verified: HELP.DBS `_SET PROCEDURE` + `_CLOSE`; sample
  `REPORTS.PRG:65` uses `CLOSE PROCEDURE`].
- Default extension is `.prg`.
- After opening, `DO <name>` resolves `<name>` against the procedure file's
  blocks before searching disk for `<name>.prg` (Section 2).

Real idiom: `SET PROCEDURE TO Rprtpro` ... menu loop calling `DO Reporta` ...
`CLOSE PROCEDURE` / `RETURN` (`REPORTS.PRG`).

See also: `DO`.

---

## 5. RETURN / RETRY

### RETURN

```
RETURN [TO MASTER]
```
[documented: HELP.DBS `_RETURN`].

"Closes the current program file and restores control to the calling routine or
to the dot prompt. The `TO MASTER` option returns control to the top level
routine rather than to the calling one." [verbatim, HELP.DBS `_RETURN`].

- Plain `RETURN` pops one frame off the DO stack: control resumes at the line
  **after** the `DO` that invoked the routine.
- A program/procedure also returns implicitly at end-of-file.
- `RETURN TO MASTER` unwinds the entire DO stack back to the outermost program
  (the one started from the dot prompt), discarding all intermediate frames.
- `RETURN` releases the routine's PRIVATE variables (including `PARAMETERS`
  names); PUBLIC variables persist (Section 14).
- At the dot prompt, `RETURN` is a no-op / returns to the dot prompt.

Real idiom: every sample procedure/program ends with `RETURN` (e.g.
`CLEANUP.PRG:52`, `HELP.PRG:127`, `REPORTS.PRG:66`).

### RETRY

```
RETRY
```
[verified: HELP.DBS `_RETRY` + sample `ERRORPRG.PRG:21`,`:45`].

"Returns to the calling program and reexecutes the **same** line. This is
different from `RETURN` which returns to the calling program and executes the
**next** line." [verbatim, HELP.DBS `_RETRY`].

`RETRY` is the canonical tail of an `ON ERROR` handler: fix the condition, then
re-run the offending command. Real idiom (`ERRORPRG.PRG`):

```
DO CASE
   CASE ERROR() = 108 .OR. ERROR() = 109   && file/record in use by others
        RETRY
   CASE ERROR() = 148                       && network server busy
        RETRY
   ...
ENDCASE
```

See also: `ON ERROR`, `RETURN`.

---

## 6. Control-flow keywords (cross-link summary)

Full semantics live in `../commands/control-flow-and-procedures.md` (control structures) and
`../language/expressions-and-operators.md`. Brief reference (verbatim `` `5 `` blocks):

### IF / ELSE / ENDIF
```
IF <condition>
   <commands>
[ELSE
   <commands>]
ENDIF
```
Conditional execution; the `ELSE` branch is optional [documented: HELP.DBS
`IF` block at line 714].

### DO CASE / CASE / OTHERWISE / ENDCASE
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
"Executes only one path from the set of alternative CASEs." The first true
`CASE` runs and the rest are skipped; `OTHERWISE` runs if every `CASE` is false
[verified: HELP.DBS `_DO CASE` + sample `ERRORPRG.PRG`, `NUMWORDS.PRG:26`].

### DO WHILE / ENDDO with EXIT and LOOP
```
DO WHILE <condition>
   <commands>
   [EXIT]
   [LOOP]
ENDDO
```
Repeats while `<condition>` is `.T.`. `LOOP` jumps back to the `DO WHILE` to
re-evaluate `<condition>`; `EXIT` jumps out to the first command after `ENDDO`
[verified: HELP.DBS `_DO WHILE`, `_EXIT`, `_LOOP`]. `DO WHILE .T.` is the
idiomatic "loop forever, break with EXIT" (`CBMENU.PRG:37-38` documents this in
a comment). Error 93 `Mismatched DO WHILE and ENDDO.` guards structural errors
[documented: DBASE.MSG 93].

> III-vs-later: III+ has **no** `FOR...NEXT` and **no** `DECLARE` arrays; loops
> are `DO WHILE` only. `EXIT`/`LOOP` apply only to `DO WHILE`. [verified:
> HELP_topics oracle (no FOR/NEXT/DECLARE entries) + HELP.DBS]

---

## 7. @ SAY / GET / READ / CLEAR family (cross-link summary)

Full reference: `../full-screen/say-get-read.md`, `../full-screen/picture-function-templates.md`,
`../full-screen/say-get-read.md`. Verbatim `` `5 `` syntax blocks for context:

```
@ <row>,<col> [SAY <expression> [PICTURE <template>]]
@ <row>,<col> GET <variable> [PICTURE <template>]
                            [RANGE <lower bound>, <upper bound>]
@ <row1>,<col1> TO <row2>,<col2> [DOUBLE]
@ <row1>,<col1> CLEAR [TO <row2>,<col2>]
READ [SAVE]
CLEAR [ALL/FIELDS/GETS/MEMORY/TYPEAHEAD]
```
[documented: HELP.DBS `_@...SAY`, `_@...GET`, `_@...TO`, `_@...CLEAR`, `_READ`,
`_CLEAR`].

Key facts relevant to I/O routing in this doc:
- `@...SAY` output obeys **`SET DEVICE`** (screen vs printer) — Section 18.5.
  `@ <row>,<col>` *by itself* (no `SAY`) erases from that position to end of row
  [documented: HELP.DBS `_@...SAY`].
- `@...GET` queues a pending variable for input; **`READ`** activates all
  pending GETs for full-screen entry; `READ SAVE` retains the GET set so the
  same GETs can be re-`READ` (used to build multi-page format files)
  [documented: HELP.DBS `_READ`].
- `CLEAR GETS` releases pending GETs without reading; `CLEAR` alone erases the
  screen and homes the cursor [documented: HELP.DBS `_CLEAR`].
- `READKEY()` reports which key terminated a full-screen `READ`
  [documented: HELP.DBS env funcs].

Real idiom: GET-into-loop validation:
```
choice = "5"
DO WHILE .NOT. choice $ " 01234"
   choice = " "
   @ 12,42 GET choice
   READ
ENDDO
```
(`REPORTS.PRG:33-38`).

> III-vs-later: III+ `@...GET` supports `PICTURE` and `RANGE` but **not**
> `VALID`, `WHEN`, `MESSAGE`, `COLOR`, or `FUNCTION` clauses (those are dBASE IV+
> / Clipper). [verified: HELP.DBS `_@...GET` shows only `PICTURE`/`RANGE`]
> Error 30 `Position is off the screen.` for out-of-range row/col
> [documented: DBASE.MSG 30].

---

## 8. ACCEPT

```
ACCEPT [<prompt>] TO <memvarC>
```
[verified: HELP.DBS `_ACCEPT` + Harbour `accept` semantics].

"Stores keyboard input into a **character** memory variable (`<memvarC>`). If
the prompt is used, dBASE III PLUS displays it on the screen before storing.
The prompt can be any character expression." [verbatim, HELP.DBS `_ACCEPT`].

- Reads a line of text terminated by Enter; the result is **always type C
  (character)**, even if the user types digits.
- `<prompt>` is an optional character expression shown immediately before input;
  it is displayed without a trailing newline so input follows it on the same
  line.
- Pressing Enter on an empty line stores a **null string** `""`.
- Honors `SET CONSOLE`/`SET ALTERNATE` for echo (Section 18).

See also: `INPUT`, `WAIT`.

---

## 9. INPUT

```
INPUT [<prompt>] TO <memvar>
```
[verified: HELP.DBS `_INPUT` + Harbour `input` semantics].

"Stores keyboard input into a memory variable of **any** data type. If the
prompt phrase is used, dBASE III PLUS displays the character message on the
screen. The prompt must be a character expression." [verbatim, HELP.DBS
`_INPUT`].

- Unlike `ACCEPT`, the typed text is **evaluated as a dBASE expression**, and
  the variable takes the resulting type:
  - `12.5` -> numeric; `.T.` -> logical; `"abc"` or `'abc'` -> character
    (quotes **required** for a string); `CTOD("01/01/85")` -> date.
- Because it evaluates the input, a bare word is treated as an identifier
  (memvar/field) reference, not a literal string.
- An empty Enter does **not** store a value (the prompt typically re-displays
  until a valid expression is entered); the precise III+ 1.1 empty-input
  behavior is `[oracle-resolves]`.
- Type/expression errors surface as the normal expression errors (e.g. 9
  `Data type mismatch.`, 10 `Syntax error.`).

> III-vs-later: identical surface in IV/Clipper. The III+ distinction `ACCEPT`
> (raw C) vs `INPUT` (evaluated, typed) is the canonical pair.

See also: `ACCEPT`, `WAIT`.

---

## 10. WAIT

```
WAIT [<prompt>] [TO <memvarC>]
```
[verified: HELP.DBS `_WAIT` + sample `CLEANUP.PRG:51` and many others].

"Causes all dBASE III PLUS processing to pause until **any key** is pressed. The
prompt must be a character expression. The default `<prompt>` is:
`Press any key to continue...`" [verbatim, HELP.DBS `_WAIT`].

- Waits for a **single keystroke** (not a line); no Enter required.
- `<prompt>` overrides the default message; pass `""` to suppress the prompt
  entirely.
- `TO <memvarC>` stores the pressed key as a 1-character **character** string
  (type C). A non-printing/control key (e.g. Enter, function keys) stores a
  **null string** `""` [documented: Harbour `wait` semantics; III+ exact mapping
  of control keys is `[oracle-resolves]`].
- Honors `SET CONSOLE` (no prompt echo when console off).

Real idioms (note `<prompt>` is built with concatenation, no `TO`):
- `WAIT SPACE(20)+"Press any key to return to Maintenance menu "`
  (`CLEANUP.PRG:51`)
- `WAIT SPACE(25)+"Press any key to continue  "` (`HELP.PRG:41`)
- Bare `WAIT` (default prompt) appears in `CASH.PRG:80`, `FLIGHT.PRG:19`.

See also: `ACCEPT`, `INKEY()`, `INPUT`, `ON`.

---

## 11. STORE and the `=` assignment

```
STORE <expression> TO <memvar list>
<memvar> = <expression>
```
[verified: HELP.DBS `_STORE` + sample `NUMWORDS.PRG:12`, `PROG.PRG:5`].

"Creates and initializes one or more memory variables. If more than one variable
is in the list, the value or contents of the expression are stored in all the
variables. `<memvar> = <expression>` is an alternative syntax for initializing
one memory variable at a time." [verbatim, HELP.DBS `_STORE`].

- `STORE ... TO a,b,c` assigns the same value to every name in the
  comma-separated list, creating them if needed
  (`CLEANUP.PRG:24`-style `STORE 0 TO lastdep,lastchk,lastwth`).
- The `=` form assigns exactly one variable: `mcount = 0` (`PROG.PRG:5`),
  `today=DATE()` (`CBMENU.PRG:35`). A new memvar is created PRIVATE to the
  current routine if it does not already exist at a visible scope.
- A memvar takes the **type of the assigned expression** (C/N/D/L). Type can
  change on reassignment.
- To assign **to a field** use `REPLACE`, not `=`/`STORE` (the `=`/`STORE`
  forms create/modify *memory variables*; a bare `fieldname = expr` at command
  level creates a memvar that shadows the field — it does **not** write the
  record). [documented: HELP.DBS `_STORE` "memory variables"; `_REPLACE` is the
  field writer]
- Memory limits: error 21 `Out of memory variable memory.`, error 22
  `Out of memory variable slots.` — III+ caps the number of active memvars
  (default 256, set in `CONFIG.DB` `MVARSIZ`/`MAXMEM` style parameters; exact
  1.1 default is `[oracle-resolves]`, see CONFIG.DB spec) [documented: DBASE.MSG
  21,22].
- `STORE : String too large.` (error 76) — assigned character value exceeds the
  254-char string limit [documented: DBASE.MSG 76].

Real idiom (multi-line `;`-continued STORE of a packed lookup table):
```
STORE "TEN      ELEVEN   TWELVE   THIRTEEN FOURTEEN FIFTEEN  SIXTEEN  "+;
      "SEVENTEENEIGHTEEN NINETEEN " TO teen
```
(`NUMWORDS.PRG:13-14`).

> III-vs-later: III+ has **no array assignment** (`DECLARE`/`[]` are dBASE IV+).
> The `==` exact-equality operator does **not** exist in III+ (only `=`, `<>`,
> `#`). [verified: HELP.DBS `_RELATIONAL OP` lists `= <> #` only]

See also: `RELEASE`, `REPLACE`, `RESTORE`, `SAVE`.

---

## 12. TEXT ... ENDTEXT

```
TEXT
<text characters>
ENDTEXT
```
[verified: HELP.DBS `_TEXT` + sample `HELP.PRG:16-39`].

"Provides a simple, convenient means of writing information to the screen or
printer." [verbatim, HELP.DBS `_TEXT`].

- Everything between `TEXT` and `ENDTEXT` is emitted **literally**, line by
  line, exactly as typed — no expression evaluation, no `&` macro substitution,
  no leading/trailing whitespace stripping (whitespace is preserved, which is
  why the sample help screens keep their indentation).
- Output is routed by the active I/O SETs: it goes to the screen, and when
  `SET PRINT ON` it also goes to the printer; it is captured by `SET ALTERNATE`
  when alternate recording is on; suppressed from the screen when
  `SET CONSOLE OFF` (Section 18). It is **not** affected by `SET DEVICE`
  (`SET DEVICE` only governs `@...SAY`).
- `&&` is **not** treated as a comment inside `TEXT`; the `&` macro operator is
  **not** expanded inside `TEXT` (the block is fully literal). [documented:
  HELP.DBS `_TEXT`; literal behavior corroborated by sample usage in `HELP.PRG`]
- There is no scope/condition; it is purely a literal multi-line print.

Real idiom (multi-screen help, `TEXT`/`ENDTEXT` blocks paged with `WAIT`):
```
CLEAR
TEXT

     Welcome to the Checkbook Management System.  This system is designed to
keep track of your deposits, ...

ENDTEXT
WAIT SPACE(25)+"Press any key to continue  "
```
(`HELP.PRG:16-41`).

See also: `?`/`??`.

---

## 12A. ? / ?? (display expression list)

```
? <expression list>
?? <expression list>
```
[verified: HELP.DBS `_?/??` (line 359) — `` `5 `` block; `9 See also `@...SAY`,
`TEXT`].

"`?/??` is the way to ask dBASE III PLUS a question and have the answer displayed
on the screen. `?` poses the question and the expression list tells dBASE III
PLUS in its terms exactly what should be shown. The expression list can include
any type expression." [verbatim, HELP.DBS `_?/??`].

`?`/`??` are the **primary non-`@...SAY` output verbs** — the exact stream that
Section 18's `SET PRINT` / `SET CONSOLE` / `SET ALTERNATE` route. They evaluate
a comma-separated expression list and print the formatted values.

- **`?`** emits a carriage-return/line-feed **first** (advancing to a new line),
  then prints the evaluated expression list. A bare `?` with no expression list
  prints just a blank line.
- **`??`** prints the expression list at the **current** cursor/print position
  with **no** leading newline (used to append to the line a previous `?`/`??`
  started).
- The expression list may mix types; each expression is converted to its display
  form and items are separated by a single space. Numeric values honor
  `SET DECIMALS`/`SET FIXED`; dates honor `SET DATE`/`SET CENTURY`.
- Output routing (Section 18): goes to the screen unless `SET CONSOLE OFF`; also
  to the printer when `SET PRINT ON`; captured to the alternate file when
  `SET ALTERNATE` recording is on. **Not** governed by `SET DEVICE` (that governs
  only `@...SAY`).
- Because the list is evaluated, the C+N type-mismatch rule of Section 19 applies:
  `? "Total: " + amount` (C+N) raises error 9; you must write
  `? "Total: " + STR(amount,10,2)`.

Real idioms (the printed-report stream the samples route to the printer):
- `? SPACE(42)+"TOTAL    $"+STR(amount,10,2)` (`RPRTPRO.PRG:407`) — explicit
  `STR()` conversion, the canonical III+ way to print a number after text.
- `?` (bare, blank line) and `? <expC>` appear throughout the report procedures
  in `RPRTPRO.PRG`.

> III-vs-later: III+ `?`/`??` have **no** `AT`, `STYLE`, `PICTURE`, or `FUNCTION`
> clauses (those are dBASE IV+ / xBase extensions). The III+ `` `5 `` block shows
> only `? <expression list>` / `?? <expression list>`.
> [verified: HELP.DBS `_?/??` shows no trailing clauses]

See also: `@...SAY`, `TEXT`, `SET PRINT`, `SET CONSOLE`, `SET ALTERNATE`.

---

## 13. SAVE / RESTORE (memory variables)

### SAVE

```
SAVE TO <file> [ALL LIKE/EXCEPT <skeleton>]
```
[verified: HELP.DBS `_SAVE` + sample `CBMENU.PRG:97` `SAVE TO Chkbook.mem`].

"Stores all or part of the active memory variables to a file." [verbatim,
HELP.DBS `_SAVE`]. Default extension `.mem`.

- With no `ALL LIKE/EXCEPT`, **all** current memvars are written.
- `ALL LIKE <skeleton>` saves only those whose names match the skeleton;
  `ALL EXCEPT <skeleton>` saves all but the matching ones. Skeleton wildcards:
  `?` = any one char, `*` = any run of chars (e.g. `SAVE ALL LIKE ??p?`)
  [verified: HELP.DBS `_SKELETON`].
- The `.mem` file holds typed snapshots of C/N/D/L memvars (see the `.mem`
  format spec `../file-formats/mem.md`; golden `CHKBOOK.MEM`, `MEM.MEM`).

> Note the `ALL` placement quirk: the syntax block reads
> `SAVE TO <file> [ALL LIKE/EXCEPT <skeleton>]`. In practice
> `SAVE ALL LIKE <skel> TO <file>` and `SAVE TO <file> ALL LIKE <skel>` are both
> accepted by III+; exact accepted orderings are `[oracle-resolves]`.

### RESTORE

```
RESTORE FROM <memory file> [ADDITIVE]
```
[verified: HELP.DBS `_RESTORE` + sample `CBMENU.PRG:28` `RESTORE FROM Chkbook.mem`].

"Retrieves and activates memory variables previously SAVEd to a memory file.
Without the `ADDITIVE` option, the RESTOREd memory variables **overwrite** the
current set of memory variables. With the `ADDITIVE` option, the current memory
variables are kept and the new ones are added as well." [verbatim, HELP.DBS
`_RESTORE`].

- Plain `RESTORE FROM` first **releases all current memvars**, then loads from
  the file (a clean reset).
- `ADDITIVE` merges: existing variables are kept; same-named variables from the
  file overwrite the in-memory value [documented: HELP.DBS; Clipper agrees].
- Error 53 `Memory Variable file is invalid.` for a corrupt/foreign `.mem`
  [documented: DBASE.MSG 53].

Real round-trip (`CBMENU.PRG`): startup `RESTORE FROM Chkbook.mem`; on exit, if
balances changed, `RELEASE ALL LIKE m*` / `RELEASE i,today` then
`SAVE TO Chkbook.mem` (`:28`, `:95-97`).

See also: `RELEASE`, `STORE`.

### RELEASE (memvar lifecycle — included for completeness)

```
RELEASE <memory variable list>
RELEASE ALL [LIKE/EXCEPT <skeleton>]
RELEASE MODULE <module name>
```
[verified: HELP.DBS `_RELEASE` + sample `CBMENU.PRG:95-96`].

Erases the named memvars (or all matching a skeleton), freeing variable slots;
`RELEASE MODULE` removes a `LOAD`ed binary module (Section 17). `CLEAR MEMORY`
releases *all* memvars [documented: HELP.DBS `_CLEAR`].
See also: `CALL`, `LOAD`, `RESTORE`, `SAVE`, `STORE`.

### PUBLIC / PRIVATE (scope — included for completeness)

```
PUBLIC <memory variable list>
PRIVATE ALL [LIKE/EXCEPT <skeleton>]
PRIVATE <memory variable list>
```
[documented: HELP.DBS `_PUBLIC`, `_PRIVATE`].

- `PUBLIC` declares globals visible to every routine at every level; PUBLIC
  variables survive a routine's `RETURN` (they are not auto-released). A PUBLIC
  variable is initialized to logical `.F.` on declaration [documented: HELP.DBS
  `_PUBLIC`; III+ `.F.` init is `[oracle-resolves]`].
- `PRIVATE` hides any higher-level definitions of the named variables from the
  current and lower routines; the originals are restored on `RETURN`. Variables
  created by simple assignment inside a routine are implicitly PRIVATE to it.
- Full scope rules: `../language/memory-variables.md`.

---

## 14. ON ERROR / ON ESCAPE / ON KEY

```
ON <option> [<command>]
```
"Executes any dBASE III PLUS command if the specified option occurs. The options
in order of precedence are:
- `ERROR`  - A dBASE III PLUS error has occurred.
- `ESCAPE` - The Esc key has been pressed.
- `KEY`    - Any key on the keyboard has been pressed." [verbatim, HELP.DBS
`ON` block at line 927].

The three event traps share one syntax. Each installs a single command (almost
always `DO <handler>`) executed when its event fires. Re-issuing `ON <option>`
replaces the prior trap; `ON <option>` with **no command** clears that trap.
Precedence when multiple could fire is `ERROR` > `ESCAPE` > `KEY`
[documented: HELP.DBS `ON` block].

### ON ERROR

```
ON ERROR <command>
ON ERROR                  && clears the trap
```
[verified: HELP.DBS `ON` + sample `REPORTS`/`YEAREND`-style `ON ERROR DO Errorprg`].

- Fires whenever a runtime error occurs. Inside the handler, `ERROR()` returns
  the error code (= the DBASE.MSG line number) and `MESSAGE()` returns the
  matching text [verified: HELP.DBS env funcs + DBASE.MSG indexing +
  `ERRORPRG.PRG`].
- The handler typically branches on `ERROR()` and ends in `RETRY` (re-run the
  failing line), `RETURN` (resume at the next line), `CANCEL`, or `QUIT`.
- While the handler runs, the trap is effectively suppressed to avoid recursion
  (exact III+ 1.1 re-entrancy behavior is `[oracle-resolves]`).

Canonical handler (the shipped `ERRORPRG.PRG`, installed via
`ON ERROR DO Errorprg`):
```
SET TALK OFF
DO CASE
   CASE ERROR() = 108 .OR. ERROR() = 109     && file/record in use by others
        RETRY
   CASE ERROR() = 1                          && file does not exist
        CLEAR
        @ 5,5 SAY MESSAGE()
        CANCEL
   CASE ERROR() = 26 .OR. ERROR() = 20       && db/record not indexed
        SELECT 1
        REINDEX
        SELECT 2
        REINDEX
   CASE ERROR() = 148                        && network server busy
        RETRY
   OTHERWISE
        CLEAR
        @ 23,0 SAY MESSAGE()
        CANCEL
ENDCASE
RETURN
```
(`ERRORPRG.PRG`; the comments are Ashton-Tate's own in the shipped file.)

> **`ERROR()` code vs `DBASE.MSG` line-number caveat (CORRECTED).** The
> `DBASE.MSG` ordinal lines up with the runtime `ERROR()` code for the
> low/general errors used here — line **1** "File does not exist.", line **20**
> "Record is not in index.", line **26** "Database is not indexed." — but it does
> **not** line up in the network/locking range. In `DBASE.MSG.strings.txt` the
> "in use by another" texts are at lines **105** ("File is in use by another.")
> and **106** ("Record is in use by another."), while lines **108/109** are
> "Cannot write to a read-only file." / "Index expression is too big (220 char
> maximum)." Likewise **"Network server busy." is at line 144**, not 148.
> `ERRORPRG.PRG` nonetheless tests `ERROR()=108/109` ("file/record in use") and
> `ERROR()=148` ("network server busy"), so the live `ERROR()` numbering and the
> on-disk message-table ordinal are **offset** in the high range — the exact
> `ERROR()`->message map for codes above ~100 must be confirmed against the real
> interpreter, `[oracle-resolves]`. (Treating `DBASE.MSG` line# = `ERROR()` code
> is safe only for the low/general errors; do not assume it past the
> locking/networking band.)

### ON ESCAPE

```
ON ESCAPE <command>
ON ESCAPE                 && clears the trap
```
[documented: HELP.DBS `ON` block].

- Fires when the user presses **Esc**, but only while **`SET ESCAPE ON`** (the
  default). With `SET ESCAPE OFF`, Esc is ignored and `ON ESCAPE` never fires
  [verified: HELP.DBS `ON` + `_SET ESCAPE` "Allows an executing program to be
  interrupted ... with the Esc key"].
- Typical use: install `ON ESCAPE DO bailout` to trap user interrupts during
  long loops/reports.
- Without an `ON ESCAPE` trap and with `SET ESCAPE ON`, pressing Esc during a
  program raises the `*** INTERRUPTED ***` prompt
  (`Cancel, Ignore, or Suspend? (C, I, or S)`) [documented: DBASE.MSG 154,168].

### ON KEY

```
ON KEY <command>
ON KEY                    && clears the trap
```
[documented: HELP.DBS `ON` block].

- Fires when **any** key is pressed during program execution. The pressed key
  remains in the type-ahead buffer; the handler usually reads it with `INKEY()`
  and dispatches.
- Re-arms after the handler returns. Useful for hot-key menus.

> III-vs-later: III+ `ON KEY` is the **global** form only — it has **no**
> `LABEL <keyname>` clause (`ON KEY LABEL` is a dBASE IV / Clipper extension).
> [verified: HELP.DBS `ON` block shows only `ERROR/ESCAPE/KEY`, no `LABEL`]

See also: `ERROR()`, `MESSAGE()`, `INKEY()`, `RETRY`, `SET ESCAPE`.

---

## 15. SUSPEND / RESUME / CANCEL / QUIT

### SUSPEND
```
SUSPEND
```
"Halts execution of the currently executing program file or procedure and
presents the dot prompt, making interactive command entry possible at the level
of the program file that was SUSPENDed." [verbatim, HELP.DBS `_SUSPEND`].

- The program's full environment (open files, memvars, DO stack, record
  pointers) is preserved. Status messages `Do suspended` / `Do cancelled`
  appear [documented: DBASE.MSG 160,155].
- Error 98 `Not suspended.` if `RESUME` is issued with nothing suspended
  [documented: DBASE.MSG 98].

### RESUME
```
RESUME
```
"Begins execution of the currently SUSPENDed program file or procedure at the
command line immediately following the line where it was SUSPENDed." [verbatim,
HELP.DBS `_RESUME`].

### CANCEL
```
CANCEL
```
"Stops execution of a program file, closes all open program files (including
procedures), and returns dBASE III PLUS to interactive mode." [verbatim,
HELP.DBS `_CANCEL`]. The DO stack is fully unwound to the dot prompt; open
**database** files and memvars are **not** closed/released (only program files)
— used in error handlers (`ERRORPRG.PRG:29,50`).

### QUIT
```
QUIT
```
"Closes all open files, ends this session of dBASE III PLUS, and returns you to
the operating system." [verbatim, HELP.DBS `_QUIT`]. The only clean way to exit
to DOS; flushes and closes every open file. The interactive Esc-interrupt prompt
offers Cancel/Ignore/Suspend (DBASE.MSG 168).

> Relationship: `SUSPEND` (pause, keep state) <-> `RESUME` (continue);
> `CANCEL` (abort to dot prompt) <-> `RETURN` (return one level); `QUIT`
> (exit dBASE entirely).

See also: `CANCEL`, `RESUME`, `RETURN`, `SUSPEND`.

---

## 16. RUN / ! (shell-out)

```
RUN <system command>
! <system command>
```
[verified: HELP.DBS `_RUN/!` (line 369) — "Both execute a designated external
program from within dBASE III PLUS."].

- `!` is an exact synonym for `RUN`.
- Loads a second copy of `COMMAND.COM` and passes `<system command>` to it, so
  any DOS command or external `.EXE`/`.COM`/`.BAT` can be run, then control
  returns to dBASE [documented: HELP.DBS `_RUN/!`; Clipper `RUN` semantics].
- Requires enough free DOS memory to load `COMMAND.COM` + the child program;
  error 89 `Unable to load COMMAND.COM.` if it cannot [documented: DBASE.MSG 89].
- The DOS **default/logged drive used by `RUN/!` is NOT changed by
  `SET DEFAULT`** — `SET DEFAULT` only affects dBASE's own file search, not the
  OS shell environment [verified: HELP.DBS `_SET DEFAULT` "The system default
  drive used by the RUN/! command is not changed."].
- `<system command>` is taken literally (it is the remainder of the line); to
  use a dBASE expression as the command, build a string and macro-expand it,
  e.g. `RUN &cmd`.

> Implementation note: III+ needs `COMMAND.COM` reachable (via `COMSPEC`) and
> a `CONFIG.SYS` with adequate `FILES=`; the shipped sample disk includes a
> `CONFIG.SYS`. None of the 29 sample `.PRG` use `RUN`/`!`
> (`grep` finds zero call sites), so concrete III+ idioms are documented from
> the HELP text only.

---

## 17. LOAD / CALL (binary modules) and RELEASE MODULE

### LOAD
```
LOAD <binary file>[.<extension>]
```
[verified: HELP.DBS `_LOAD` + sample `CBMENU2.PRG:31-32` `LOAD Curson` /
`LOAD Cursoff`].

"Places a binary file in memory where it can be executed with the CALL command.
The **default extension is `.BIN`**, and the file name without an extension
becomes the **module name** for the CALL command." [verbatim, HELP.DBS `_LOAD`].

- A `.BIN` module is **raw 8086 machine code** (an `.OBJ`-less flat binary),
  loaded into a dBASE-managed memory region and invoked via a far call; it must
  end with a far return (`RETF`, opcode `CB`) and preserve registers it does not
  intend to change.
- Up to a small fixed number of modules may be resident at once (the III+ cap is
  `[oracle-resolves]`; exceeding it is reported via the table-full family).
- Error 88 `File was not LOADed.` if `CALL` names a module that is not resident
  [documented: DBASE.MSG 88].

**Golden evidence** — the shipped `CURSON.BIN` / `CURSOFF.BIN` are exactly such
modules (cursor on/off via BIOS):
```
CURSOFF.BIN (12 bytes):  50 51 B4 01 B9 8F 8F CD 10 59 58 CB
  PUSH AX ; PUSH CX ; MOV AH,01 ; MOV CX,8F8F ; INT 10h ; POP CX ; POP AX ; RETF
CURSON.BIN  (21 bytes):  50 51 CD 11 24 10 B9 87 86 74 03 B9 8C 8B B4 01 CD 10 59 58 CB
  PUSH AX ; PUSH CX ; INT 11h ; AND AL,10h ; ... ; MOV AH,01 ; INT 10h ; POP CX ; POP AX ; RETF
```
[verified: `xxd CURSON.BIN`/`CURSOFF.BIN` (golden bytes) + HELP.DBS `_LOAD`].
The trailing `CB` = far `RETF` and the leading `PUSH AX/CX` ... `POP CX/AX`
register save/restore confirm the III+ CALL ABI: a far call into the module with
the program responsible for preserving registers.

### CALL
```
CALL <module name> [WITH <expC>]
```
[verified: HELP.DBS `_CALL` + sample `CBMENU2.PRG:61,65` `CALL Cursoff` /
`CALL Curson`].

"Executes a binary file (module) that you have placed in memory with the LOAD
command." [verbatim, HELP.DBS `_CALL`].

- `<module name>` is the LOADed file's base name (no extension).
- `WITH <expC>` passes a **single character expression** to the module; its
  address (the string buffer) is handed to the binary so it can read/modify it
  in place — the III+ mechanism for returning data from a module. III+ `CALL`
  takes **only one** `WITH` argument (a character string), unlike the later
  multi-arg `CALL ... WITH a,b,c` of dBASE IV+.
  [verified: HELP.DBS `_CALL` syntax `WITH <expC>` (singular); samples call with
  no argument]
- Real idiom: `CALL Cursoff` before drawing a clock, `CALL Curson` after
  (`CBMENU2.PRG:61,65`), with no `WITH`.

### RELEASE MODULE
```
RELEASE MODULE <module name>
```
[documented: HELP.DBS `_RELEASE`]. Removes the named LOADed module from memory,
freeing its region. See also: `CALL`, `LOAD`.

---

## 18. I/O routing SET commands

These four toggles/redirects control where program output goes. They compose:
`@...SAY` -> governed by `SET DEVICE`; all *other* output (`?`, `??`, `TEXT`,
`DISPLAY`/`LIST`, `TYPE`, command results) -> governed by `SET PRINT` (printer
echo), `SET CONSOLE` (screen on/off), and `SET ALTERNATE` (file capture).

### 18.1 SET ALTERNATE
```
SET ALTERNATE TO [<file>]
SET ALTERNATE on/OFF
```
(default **OFF**) [verbatim syntax + verbatim description, HELP.DBS
`_SET ALTERNATE`].

"`SET ALTERNATE TO` records all output (other than full-screen) to a text file.
`SET ALTERNATE TO` without a file name closes the text file. `SET ALTERNATE ON`
begins recording output in the target text file. `SET ALTERNATE OFF` stops
recording and leaves the target text file open." [verbatim].

- Two-step idiom: `SET ALTERNATE TO LOGFILE` (opens/creates `LOGFILE.TXT`,
  default ext `.txt`), then `SET ALTERNATE ON` to start capturing.
- Captures the same stream the screen sees (i.e. non-`@...SAY` output): `?`,
  `??`, `DISPLAY`, `LIST`, `TYPE`, etc. **Not** full-screen / `@...SAY` output.
- `SET ALTERNATE OFF` pauses capture (file stays open); `SET ALTERNATE TO`
  (no file) closes the file. `CLOSE ALTERNATE` also closes it [verified:
  HELP.DBS `_SET ALTERNATE` See also `CLOSE`; `_CLOSE` lists `ALTERNATE`].

### 18.2 SET CONSOLE
```
SET CONSOLE ON/off
```
(default **ON**) "Makes sure all information is displayed on the screen."
[verbatim, HELP.DBS SET ON/OFF table].

- `SET CONSOLE OFF` suppresses output to the **screen** while a program runs
  (it has no effect at the interactive dot prompt — you cannot turn off your own
  console there). Output still goes to the printer (if `SET PRINT ON`) and to
  the alternate file (if recording).
- Idiomatic pairing with the printer for clean hardcopy: `SET CONSOLE OFF`
  around printed sections so the report does not also scroll on screen.

Real idiom (`CASH.PRG`-family): `SET CONSOLE OFF` ... print ...
`SET CONSOLE ON` (`CANCL.PRG:18-20`, `:79-81`).

### 18.3 SET PRINT / SET PRINTER
```
SET PRINTER TO [LPT1/LPT2/LPT3] / [COM1/COM2] /
            [\\<computer name>\<device>=LPTx]
SET PRINT on/OFF
```
(default **OFF**) [verbatim syntax, HELP.DBS `_SET PRINT`].

"Does/does not direct all output **not formatted with the @...SAY command** to
the printer. `SET PRINTER TO` directs the output to a specific printer or DOS
device. The first two forms ... are for local print devices. The third is for a
network device. `SET PRINTER TO` by itself resets to the default device (both)
and ends the spool process (network)." [verbatim].

- `SET PRINT ON` echoes the non-`@...SAY` output stream (`?`, `??`, `DISPLAY`,
  `LIST`, `TYPE`, command results) to the printer **in addition to** the screen
  (unless `SET CONSOLE OFF`).
- `SET PRINTER TO <device>` selects the physical/network port. `SET PRINTER TO`
  (bare) resets to the default.
- `@...SAY` output is **not** governed by `SET PRINT`; route `@...SAY` to the
  printer with `SET DEVICE TO PRINT` instead (18.5).
- Printer errors: 120 `Invalid printer port.`, 121 `Invalid printer
  redirection.`, 122 `Printer not ready.`, 123 `Printer is either not connected
  or turned off.` [documented: DBASE.MSG 120-123].

Real idiom (`RPRTPRO.PRG:427-428`): after the user picks "P",
`IF UPPER(pr) = "P"` / `SET PRINT ON`; `REPORTS`-family toggles `SET PRINT OFF`
(`RPRTPRO.PRG:121`) when done.

### 18.4 EJECT
```
EJECT
```
[verbatim syntax, HELP.DBS `_EJECT`]. "Causes the printer to advance the paper
to the top of the next page (form feed)." Emits a form-feed to the printer and
resets the line counter; updates `PROW()`/`PCOL()`. Typically issued between
printed report sections.

### 18.5 SET DEVICE
```
SET DEVICE TO SCREEN/print
```
(default **SCREEN**) "Sends the results of executed `@...SAY` commands to the
screen or the printer." [verbatim, HELP.DBS `_SET DEVICE`].

- Governs **only `@...SAY`** output routing (full-screen formatted output).
  `SET DEVICE TO PRINT` sends `@ r,c SAY ...` to the printer at the given
  row/column (driving `PROW()`/`PCOL()`); `SET DEVICE TO SCREEN` returns it to
  the display.
- Independent of `SET PRINT` (which handles `?`/`LIST`/etc.). To print a
  formatted form you generally `SET DEVICE TO PRINT`; to print listing output
  you `SET PRINT ON`.

### 18.6 SET MARGIN
```
SET MARGIN TO <expN>
```
(default **0**) [verbatim syntax, HELP.DBS `_SET MARGIN`]. "Readjusts the left
margin on the printer. The default left printer margin is 0." Shifts all printed
output right by `<expN>` columns.

> Composition summary (which SET affects which output):
>
> | Output source            | SET DEVICE | SET PRINT | SET CONSOLE | SET ALTERNATE |
> |--------------------------|:----------:|:---------:|:-----------:|:-------------:|
> | `@...SAY`                | yes        | no        | (screen path) | no          |
> | `?` `??` `TEXT`          | no         | yes       | yes         | yes           |
> | `DISPLAY`/`LIST`/`TYPE`  | no         | yes       | yes         | yes           |
>
> [verified: HELP.DBS `_SET DEVICE`, `_SET PRINT`, `_SET ALTERNATE` +
> SET ON/OFF table; the "(screen path)" cell — whether `SET CONSOLE OFF`
> suppresses on-screen `@...SAY` — is `[oracle-resolves]`]

---

## 19. The C + N type-mismatch rule (CRITICAL III+ delta)

> **In dBASE III PLUS, `Character + Numeric` is a TYPE-MISMATCH ERROR
> (error 9 `Data type mismatch.`), NOT auto-stringification.**

The mined III+ surface documents `+`/`-` as **string concatenation operators**
that join **two character operands** ("String operators generate character
results: `+` and `-` Concatenation operators") and arithmetic operators that add
**two numeric operands** [verified: HELP.DBS `_STRING OP` + `_MATHEMATICAL OP`].
Every part of an expression (except the operator) "should be of the same type"
with the only sanctioned cross-type exceptions being date+/-number and
date-date [verified: HELP.DBS `_EXPRESSION` / `_EXP 2`]. There is **no** rule in
the III+ help that converts a number to its display string when added to a
character value.

The modern dbase.com `+` help (`archive/dbase-com-help/OPS_PLUS.md`) explicitly
says you "may concatenate any other data type to a string ... Numbers become
strings with no leading spaces ... logical values become 'true'/'false' ...
Dates ... DTOC()." **That auto-stringification rule is a later-dBASE/dBASE PLUS
behavior and is WRONG for III+ 1.1.** In III+ you must convert explicitly:
`"Total: " + STR(amount,10,2)` (real idiom, `RPRTPRO.PRG:407`
`? SPACE(42)+"TOTAL    $"+STR(amount,10,2)`), never `"Total: " + amount`.

Other III+ string-operator specifics [verified: HELP.DBS `_STRING OP`]:
- `+` concatenates two strings preserving all characters.
- `-` concatenates two strings and **moves the trailing blanks of the left
  operand to the end of the combined string** ("If '-' is used, all trailing
  blanks are moved to the end of the combined string").
- Errors 73/74 `- :`/`+ : Concatenated string too large.` if the result exceeds
  the 254-char limit [documented: DBASE.MSG 73,74].

> Cross-link: full operator/expression semantics in `../language/expressions-and-operators.md`
> and `../language/expressions-and-operators.md`.

---

## Coverage against the HELP_topics oracle (406-item index)

Items in **this document's scope** (programming + console I/O + I/O-routing
system commands), all covered:

- Program control: `DO`(+`DO WITH`), `PROCEDURE`, `SET PROCEDURE`,
  `PARAMETERS`, `RETURN`, `RETRY`, `CANCEL`, `SUSPEND`, `RESUME`, `QUIT`.
- Control-flow keywords (summary + cross-link): `DO CASE`/`CASE`/`OTHERWISE`/
  `ENDCASE`, `DO WHILE`/`ENDDO`, `EXIT`, `LOOP`, `IF`/`ELSE`/`ENDIF`.
- Console I/O verbs: `?`/`??`, `ACCEPT`, `INPUT`, `WAIT`, `STORE` (+ `=`),
  `TEXT`/`ENDTEXT`.
- Full-screen verbs (summary + cross-link): `@...SAY`, `@...GET`, `@...TO`,
  `@...CLEAR`, `READ`, `PICTURE`, `CLEAR`.
- Memvar persistence/scope (summary): `SAVE`, `RESTORE`, `RELEASE`, `PUBLIC`,
  `PRIVATE`.
- Event traps: `ON ERROR`, `ON ESCAPE`, `ON KEY` (the `ON` topic).
- Shell-out & binary modules: `RUN`/`!`, `LOAD`, `CALL` (+ `RELEASE MODULE`).
- I/O routing system: `EJECT`, `SET ALTERNATE`, `SET CONSOLE`, `SET PRINT`
  (`SET PRINTER`), `SET DEVICE`, `SET MARGIN`.
- Comments: `NOTE`/`*`/`&&`.

Out of scope (owned by sibling specs, cross-linked, not re-documented here):
data manipulation (`APPEND`/`REPLACE`/`DELETE`/...), navigation
(`GO`/`SKIP`/`SEEK`/`FIND`/`LOCATE`), indexing/files
(`USE`/`INDEX`/`SORT`/`COPY`/...), reports/labels, the remaining `SET`
parameters (color/date/relation/filter/...), and all `FUNCTIONS` topics. The
functions referenced here (`ERROR()`, `MESSAGE()`, `INKEY()`, `READKEY()`,
`PROW()`, `PCOL()`) are documented in `../functions/`.

---

## Open questions (`[oracle-resolves]` — settle with real DBASE.EXE/OVL under DOSBox)

1. **DO resolution precedence**: when a procedure and a `<name>.prg` both exist,
   which does `DO <name>` run? (Section 2)
2. **By-value forcing**: does wrapping a variable in parentheses `(x)` force
   by-value parameter passing in III+ 1.1 (as in Clipper)? (Section 2)
3. **Unmatched PARAMETERS**: when `WITH` supplies fewer args than `PARAMETERS`
   names, are the trailing parameters `.F.`, null, or unset? (Sections 2-3)
4. **Procedure-file caps**: exact max procedures per file and max significant
   chars of a procedure name in 1.1. (Section 4)
5. **PUBLIC initialization**: is a freshly-declared PUBLIC variable initialized
   to logical `.F.`? (Section 13)
6. **INPUT empty-Enter**: behavior of `INPUT` when the user presses Enter with
   no expression. (Section 9)
7. **WAIT control keys**: exact mapping of non-printing keys (Enter, F-keys,
   arrows) stored by `WAIT TO <memvarC>`. (Section 10)
8. **LOAD module cap**: maximum number of simultaneously LOADed binary modules,
   and total binary-module memory budget. (Section 17)
9. **CALL WITH write-back**: confirm `CALL <mod> WITH <expC>` passes the string
   buffer by address so the module can modify the caller's character variable
   in place. (Section 17)
10. **`SET CONSOLE OFF` vs `@...SAY`**: does console-off suppress on-screen
    `@...SAY`, or only the `?`/`LIST` stream? (Section 18, table)
11. **ON ERROR re-entrancy**: is the ERROR trap suppressed while its own handler
    runs; what happens on an error inside the handler? (Section 14)
12. **`ERROR()` vs `DBASE.MSG` index offset (high range)**: the message-table
    ordinal equals the `ERROR()` code for low/general errors but is OFFSET in the
    locking/networking band — `ERRORPRG.PRG` tests `ERROR()=108/109` ("in use")
    and `148` ("Network server busy"), whereas `DBASE.MSG` puts "in use by
    another" at lines 105/106 and "Network server busy." at line 144. Confirm the
    exact runtime `ERROR()` numbers for the high range against the live
    interpreter. (Section 14)
13. **`SAVE ALL LIKE` clause ordering**: which orderings of
    `SAVE [ALL LIKE/EXCEPT <skel>] TO <file>` does 1.1 accept? (Section 13)
14. **MVAR default cap**: the III+ 1.1 default active-memvar count and the
    CONFIG.DB parameter that sets it (cross-ref CONFIG.DB spec). (Section 11)

---

## Sources

- `archive/golden-mined/HELP.DBS.strings.txt` — period-exact III+ HELP database
  text. Primary syntax authority. Cited blocks: `_DO` (l.626), `_PARAMETERS`,
  `_PROCEDURE`, `_SET PROCEDURE`, `_RETURN`, `_RETRY`, `_ACCEPT`, `_INPUT`,
  `_?/??` (l.359), `_WAIT`, `_STORE`, `_TEXT`, `_SAVE`, `_RESTORE`, `_RELEASE`, `_PUBLIC`,
  `_PRIVATE`, `ON` block (l.927), `_SUSPEND`, `_RESUME`, `_CANCEL`, `_QUIT`,
  `_RUN/!` (l.369), `_LOAD`, `_CALL`, `_SET ALTERNATE`, `_SET PRINT`,
  `_SET DEVICE`, `_SET MARGIN`, `_EJECT`, `_SET DEFAULT`, `_CLOSE`, `_CLEAR`,
  `_@...SAY/GET/TO/CLEAR`, `_READ`, `_DO CASE`, `_DO WHILE`, `_EXIT`, `_LOOP`,
  `IF` block (l.714), `_NOTE/*`, `_STRING OP`, `_MATHEMATICAL OP`,
  `_RELATIONAL OP`, `_EXPRESSION`/`_EXP 2`, `_SKELETON`, the SET ON/OFF tables
  (CONSOLE, PRINT default off, ESCAPE default on), and the environment-function
  table (`ERROR()`, `MESSAGE()`, `INKEY()`, `READKEY()`, `PROW()`, `PCOL()`).
- `archive/golden-mined/DBASE.MSG.strings.txt` — error message table (433 lines;
  line# = `ERROR()` code only for low/general errors, OFFSET in the
  locking/networking band — see Section 14 caveat). Cited: 1, 9, 10, 18, 20, 21,
  22, 26, 30, 53, 73, 74, 76, 88, 89, 90, 91, 92, 93, 98, 100, 105/106
  (file/record in use), 120-123, 144 ("Network server busy."), 154, 155, 160,
  168.
- `archive/golden-mined/HELP_topics.txt` — 406-item completeness oracle.
- `archive/golden-mined/ASSIST.HLP.strings.txt` — ASSIST-menu command help
  (used to corroborate `QUIT`, `USE`, `SET FORMAT` surface).
- `archive/dbase-com-help/OPS_PLUS.md` — modern dbase.com `+` operator help;
  cited as the **wrong-for-III+** auto-stringification source (Section 19).
- Sample programs (`goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/`):
  `ERRORPRG.PRG` (ON ERROR / DO CASE on ERROR() / RETRY / CANCEL / MESSAGE()),
  `HELP.PRG` (TEXT/ENDTEXT, WAIT, RETURN), `CBMENU.PRG` & `CBMENU2.PRG`
  (DO WHILE .T., DO, RELEASE, SAVE/RESTORE; LOAD/CALL Curson/Cursoff),
  `RPRTPRO.PRG` (SET PROCEDURE target, PROCEDURE/PARAMETERS, DO WITH,
  SET PRINT ON, STR concatenation), `REPORTS.PRG` (SET PROCEDURE TO /
  CLOSE PROCEDURE), `NUMWORDS.PRG` (PARAMETERS, STORE, ;-continuation),
  `PROG.PRG` (=, &&, CTOD comparison), `CLEANUP.PRG` (STORE list, WAIT, CLOSE).
- Binary-module goldens: `CURSON.BIN` (21 B), `CURSOFF.BIN` (12 B) — verified by
  `od -tx1` to be raw 8086 machine code ending in far `RETF` (`CB`), proving the
  `.BIN` LOAD/CALL ABI (Section 17).
- `oracles/harbour/doc/en/` and `oracles/xharbour/doc/` — Clipper-lineage
  semantic oracle for `ACCEPT`/`INPUT`/`WAIT`/`DO`/`RUN` behavior (not
  period-exact syntax; deltas tagged).

---

## Verification

Adversarial cross-check (2026-06-16) against the period oracles. Every `` `5 ``
syntax block quoted in this doc was byte-compared to
`archive/golden-mined/HELP.DBS.strings.txt`; the error codes to
`archive/golden-mined/DBASE.MSG.strings.txt`; the completeness scope to
`archive/golden-mined/HELP_topics.txt` and `ASSIST.HLP.strings.txt`; the binary
goldens to the on-disk `.BIN` bytes.

### Syntax blocks confirmed verbatim (HELP.DBS line refs)

`_DO` (l.626), `_DO CASE` (l.633), `_DO WHILE` (l.646), `_EXIT` (l.681),
`_LOOP` (l.838), `_PARAMETERS` (l.945), `_PROCEDURE` (l.960),
`_SET PROCEDURE` (l.1551), `_RETURN` (l.1063), `_RETRY` (l.1056),
`_ACCEPT` (l.425), `_INPUT` (l.735), `_WAIT` (l.1196, default prompt
`'Press any key to continue...'` confirmed at l.1200), `_STORE` (l.1122),
`_TEXT` (l.1148), `_?/??` (l.360), `_SAVE` (l.1071), `_RESTORE` (l.1039),
`_RELEASE` (l.1004, incl. `RELEASE MODULE`), `_PUBLIC` (l.966),
`_PRIVATE` (l.953), `ON` block (l.927, precedence ERROR>ESCAPE>KEY, no
`LABEL`), `_SUSPEND` (l.1140), `_RESUME` (l.1049), `_CANCEL` (l.479),
`_QUIT` (l.973), `_RUN/!` (l.370), `_LOAD` (l.821, default `.BIN`),
`_CALL` (l.473, `WITH <expC>` singular), `_CLOSE` (l.505, lists
ALTERNATE/PROCEDURE), `_SET ALTERNATE` (l.1371), `_SET PRINT` (l.1539),
`_SET DEVICE` (l.1436), `_SET MARGIN` (l.1500), `_SET DEFAULT` (l.1420,
"system default drive used by RUN/! is not changed"), `_EJECT` (l.668),
`_READ` (l.978), `_NOTE/*` (l.919), `_@...GET/SAY/TO/CLEAR` (l.381+).
All matched the doc exactly.

### III+-vs-later correctness re-confirmed (the mined text is authority)

- **No `==`**: `_Relational Operators` (l.1793) lists only `< > = <>|# <= >= $`.
  Verified; the doc's "no `==` in III+" is correct.
- **C + N is a type mismatch (error 9)**: `_Expressions` (l.1733) "Each part of
  an expression (except the operator) should be of the same type" with the only
  cross-type exception being date+/-number; `_String Operators` (l.1817) makes
  `+`/`-` character-only concatenation, `_Mathematical Operators` (l.1786) makes
  `+ - * / ** ^` numeric-only. No auto-stringification rule exists. The doc's
  CRITICAL delta (Section 19) is correct, and the `dbase.com OPS_PLUS.md`
  WRONG-for-III+ tag stands.
- **No `ON KEY LABEL`**, no `@...GET VALID/WHEN/MESSAGE/COLOR/FUNCTION`, no
  `FOR..NEXT`, no `DECLARE`/arrays, single-arg `CALL ... WITH <expC>`: all
  re-confirmed absent from the mined `` `5 `` blocks. The doc's flags hold.
- **`?`/`??` clause set**: `_?/??` shows no `AT/STYLE/PICTURE` — the new
  Section 12A correctly tags those as IV+.

### Corrections applied this pass

1. **Added Section 12A `?`/`??`** (display expression list). This is a
   HELP_topics oracle item (`@?/??`, index l.232) and the *primary*
   non-`@...SAY` output verb that the entire Section 18 routing model depends on,
   yet the original draft documented no syntax block for it (it only appeared in
   "See also" links and the routing table). Now covered with the verbatim
   `_?/??` block, the `?` (leading CR/LF) vs `??` (no leading newline)
   distinction, routing semantics, and the IV+ clause delta. **Completeness gap
   closed.**
2. **Fixed the `ERROR()`-code vs `DBASE.MSG`-line claim (Section 14).** The
   original said error 148 ("Network server busy") is "outside the 1..146 mined
   range." That is false: `DBASE.MSG.strings.txt` has 433 lines, **"Network
   server busy." is at line 144**, and lines **108/109** are "Cannot write to a
   read-only file." / "Index expression is too big" — NOT "file/record in use"
   (those are lines **105/106**). `ERRORPRG.PRG` tests `ERROR()=108/109/148` with
   in-use/network comments, so the live `ERROR()` numbering and the message-table
   ordinal are **offset** in the high (locking/networking) range. Replaced the
   wrong note with a corrected caveat: `DBASE.MSG` line# = `ERROR()` code only
   for low/general errors; the high-range map is `[oracle-resolves]`.
3. **Fixed `CURSOFF.BIN` byte count.** `od -tx1` shows the file is **12 bytes**
   (`50 51 b4 01 b9 8f 8f cd 10 59 58 cb`), not 11 as originally labeled in two
   places. Byte sequences for both `.BIN` modules otherwise match the goldens
   exactly (trailing `CB` = far `RETF` confirmed).

### Non-ASCII / ASCII-Law check

All 31 non-ASCII bytes are em-dash U+2014, every one in prose (titles, sentence
text); none appear inside any fenced `` ``` `` code block (checked with an
in-fence scanner). `CLAUDE.md` line 41 permits "Unicode fine in prose tables;
ASCII-clean in spec source and code blocks." The Law is satisfied.

### Completeness vs HELP_topics (programming/I/O/system scope)

After adding `?`/`??`, every in-scope command in the 406-item index is covered:
`?/??`, `ACCEPT`, `CALL`, `CANCEL`, `DO`/`DO CASE`/`DO WHILE`, `EJECT`, `EXIT`,
`INPUT`, `LOAD`, `LOOP`, `PARAMETERS`, `PRIVATE`, `PROCEDURE`, `PUBLIC`, `QUIT`,
`READ`, `RELEASE`, `RESTORE`, `RESUME`, `RETRY`, `RETURN`, `SAVE`, `STORE`,
`SUSPEND`, `TEXT`/`ENDTEXT`, `WAIT`, `RUN`/`!`, `SET ALTERNATE`/`CONSOLE`/
`PRINT`/`DEVICE`/`MARGIN`, the `ON` trap, `NOTE`/`*`/`&&`, and the `@...`
cross-link family. The `MACRO` (`&<memvarC>`) operator (HELP.DBS `_Macro`,
l.844) is referenced in Sections 12/16 but is a language-operator topic owned by
`../language/expressions-and-operators.md`; not re-documented here by design.

### Residual `[oracle-resolves]` gaps (need real DBASE.EXE/DBASE.OVL under DOSBox)

The 14 Open-questions items remain unresolved by the static corpus (DO
precedence, by-value parens, unmatched PARAMETERS init, procedure-file caps,
PUBLIC `.F.` init, INPUT empty-Enter, WAIT control-key mapping, LOAD module cap,
CALL WITH write-back, `SET CONSOLE OFF` vs `@...SAY`, ON ERROR re-entrancy,
`SAVE ALL LIKE` ordering, MVAR default cap). Added this pass: the **high-range
`ERROR()`->`DBASE.MSG` index offset** (replacing the retired "148 beyond 146"
item) — confirm the exact `ERROR()` numbers for the locking/networking band
against the live interpreter.
