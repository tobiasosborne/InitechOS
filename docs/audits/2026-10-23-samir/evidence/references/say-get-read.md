# Full-Screen I/O — @ ... SAY / GET / READ, @ ... TO, @ ... CLEAR (dBASE III PLUS 1.1)

**Status / confidence.** TARGET = dBASE III PLUS 1.1 (1986). This spec covers the
`@` family of full-screen commands and the `READ` command that drives them. The
*existence and period syntax* of `@...SAY`, `@...GET`, `@...TO`, `@...CLEAR`,
`READ`, `CLEAR GETS`, `SET FORMAT TO`, and `PICTURE` are pinned to the mined III+
surface (`HELP_topics.txt` lists all of them as distinct topics; `ASSIST.HLP`
documents `SET FORMAT TO`). The *semantics* (GET list, RANGE/VALID/WHEN behaviour,
navigation keys, write-back) are corroborated by the Clipper-lineage Harbour
`sayget.txt` oracle — which is **semantically** faithful to the III+/Clipper model
but is **NOT period-exact syntax** (Harbour adds `COLOR`, code blocks, `ReadModal()`
that did NOT exist in III+; these are tagged as later-era and excluded from the
III+ surface). Every coordinate/idiom claim is cross-checked against real III+
sample programs (`CLIENTS.FMT`, `MENUMASK.PRG`, `EDITVOID.PRG`, `CANCL.PRG`,
`CHECK.PRG`). Items needing the real interpreter under DOSBox to settle are tagged
`[oracle-resolves]` and collected under *Open questions*.

Cross-links:
- PICTURE/FUNCTION template grammar: `./picture-function-templates.md` (companion
  spec; the `@`-functions and template chars are *enumerated* here for completeness
  but their exact transform semantics live there).
- `.FMT` / `.SCR` file formats and how the screen painter generates these `@`
  statements: `../file-formats/scr-and-fmt.md`.

---

## 0. Scope & completeness against the oracle

`HELP_topics.txt` (the canonical 406-item index) items in scope for this document:

| Topic line | Item | Covered in section |
|---|---|---|
| 7  | `@...GET`  | 3, 5, 6, 7, 8 |
| 8  | `@...SAY`  | 2 |
| 9  | `@...TO`   | 9 |
| 10 | `@...CLEAR`| 10 |
| 11 | `PICTURE`  | 4 (enumerated; full grammar in companion spec) |
| 77 | `READ`     | 6, 7 |
| 123| `SET FORMAT`| 11 |

Adjacent topics deliberately **out of scope** (documented elsewhere, named here so
a swarm knows where they live): `CLEAR` (plain, full-screen clear) — see commands
spec; `SET DELIMITERS`, `SET INTENSITY`, `SET CONFIRM`, `SET BELL`, `SET ESCAPE`,
`SET DEVICE`, `SET COLOR` (topic lines 120/216/205/202/211/349/344) — the
environment/SET spec; `BROWSE`/`EDIT`/`APPEND`/`CHANGE` (their own full-screen
editors that *consume* a `SET FORMAT TO` file). `CLEAR GETS` is not a separate
topic line but is a documented sub-command of the GET model and is covered in
section 7.4. The companion `picture-function-templates.md` owns the PICTURE topic
in full.

---

## 1. The coordinate system

dBASE III PLUS addresses the screen as a grid of **rows** and **columns**, both
**0-based**.

- **Rows: 0..24** (25 physical lines). Row 24 is the bottom line; rows 0 and 24
  are special in interactive mode (status/scoreboard areas) — see below.
- **Columns: 0..79** (80-column text mode). Column 79 is the rightmost.
  [verified: real forms use the full extent — `CLIENTS.FMT` draws `@ 8,0 TO 12,79`
  (col 0 to col 79) and `MENUMASK.PRG` uses `@ 4,1 TO 23,77` and writes
  `@ 22,...`; rows reach 23/24, cols reach 79 + Harbour `MaxRow()`/`MaxCol()`
  return 24/79 on a default screen].

Syntax everywhere is `@ <row>,<col>` — **row first, then column** (the opposite of
algebraic x,y). The comma is required. `<row>` and `<col>` are numeric expressions,
evaluated at run time, so `@ mrow,mcol GET ...` is legal (see `EDITVOID.PRG:211`
`@ mrow,mcol GET mtax PICTURE "9"`). Fractional values are truncated to integers.
[verified: ASSIST/Harbour syntax `@ <nRow>, <nCol>` + sample programs]

### 1.1 Special rows in interactive/full-screen mode

- **Row 0** — when a full-screen editing context is active and `SET STATUS` /
  `SET SCOREBOARD` defaults apply, row 0 holds the **scoreboard**. The GET
  RANGE-failure message **"RANGE is <lo> to <hi>"** and the Ins/Del indicators
  appear in the scoreboard at the top of the screen.
  [verified: `DBASE.MSG` string "RANGE is " (line 158, the literal III+ message
  prefix) + Harbour sayget.txt "the message ... will appear in the SCOREBOARD area
  (row = 0, col = 60)"; note Harbour's literal text is "OUT OF RANGE" (later-era,
  not III+) and its col 60 placement is Clipper's — III+ scoreboard column
  placement is `[oracle-resolves]`].
- **Row 22 / 24** — the status line lives on the bottom line when `SET STATUS ON`
  (the III+ default in interactive mode). Programs that draw full-screen forms
  typically run with `SET STATUS OFF` and use row 24 themselves; `MENUMASK.PRG`
  writes a prompt on row 22. [inferred from sample-program layout conventions].

Whether a program can safely write on rows 0 and 24 depends on `SET STATUS` /
`SET SCOREBOARD`; the safe drawable interior with status on is roughly rows 1..21.
[oracle-resolves: exact reserved-row behaviour with each SET combination].

---

## 2. @ ... SAY — display output

### 2.1 Syntax

```
@ <row>,<col> SAY <exp> [PICTURE <cPict>] [FUNCTION <cFunc>]
```

- `<exp>` — any expression (character, numeric, date, or logical). It is
  *displayed*, formatted, at `<row>,<col>`. It is read-only output; no input.
- `PICTURE <cPict>` — a character expression combining an optional `@`-function
  clause and a template string (see section 4 and the companion spec). Controls
  how `<exp>` is rendered (e.g. `"@!"` uppercases, `"999,999.99"` formats a number).
- `FUNCTION <cFunc>` — III+ permits the function clause to be written separately
  from PICTURE; `FUNCTION "@!"` is equivalent to `PICTURE "@!"` with no template.
  In practice the sample programs always fold the function into `PICTURE`
  (`PICTURE "@!"`); both forms exist on the III+ surface.
  [documented: HELP_topics lists `PICTURE` as the input-formatting topic; Harbour
  folds FUNCTION into PICTURE; the separate `FUNCTION` keyword is III+/IV — its
  standalone use on `@...SAY` is `[oracle-resolves]`].

### 2.2 Semantics

- Output goes to the **current device**. Default device is the **screen**; after
  `SET DEVICE TO PRINT` the same `@...SAY` is sent to the printer (this is the
  documented way to do positioned printed output / forms). `@...GET` is ignored
  when the device is the printer. [documented: Harbour sayget.txt "If the current
  device is set to the printer, the output will go to the printer; the default is
  for all output to go to the screen"].
- After the SAY, the cursor sits at the column immediately past the displayed
  value, so a following `GET` on the same `@` line begins one space to the right of
  the SAY text (see 3.2). [documented: Harbour sayget.txt "field variable displayed
  at Row(), Col()+1"].
- A SAY does **not** create a pending GET and is **not** affected by `READ`. It
  paints immediately when the statement executes.

### 2.3 Real III+ idioms

```
@  4, 21  SAY "CLIENTS ADDRESS AND PHONE ENTRY"        && CLIENTS.FMT:1 (literal)
@ 17,47  SAY today                                     && MENUMASK.PRG:92 (date var)
@ 20,57  SAY "$"+LTRIM(STR(balance,10,2))              && MENUMASK.PRG:95 (C+C expr)
@  8,66  SAY Amt PICTURE "9999999.99"                  && CANCL.PRG:62 (numeric+PICTURE)
@  6,5   SAY SPACE(15)                                 && MENUMASK.PRG:11 (blank a region)
@  5,2   SAY CHR(176)+CHR(176)                         && MENUMASK.PRG:22 (box chars)
```

Note `"$"+LTRIM(STR(balance,10,2))` is **C+C** (string + string) — legal. III+
requires the programmer to convert the number with `STR()` first; you may NOT
write `"$"+balance`. See section 12.1.

---

## 3. @ ... GET — declare an input field

### 3.1 Syntax (full)

```
@ <row>,<col> [SAY <exp> [PICTURE <cSayPict>] [FUNCTION <cFunc>]]
              GET <var>
                  [PICTURE <cGetPict>]
                  [FUNCTION <cFunc>]
                  [RANGE <lo>, <hi>]
                  [VALID <condition>]        && see 3.4 note
                  [WHEN <condition>]         && see 3.4 note
```

In III+ a single `@` statement may carry an optional SAY part *and* a GET part.
The canonical pattern is "label on the left, input field to the right":

```
@ 9,1 SAY "FIRST NAME:" GET CLIENTS->FIRSTNAME       && one statement, SAY + GET
```

### 3.2 Where the GET field appears

- If the statement has **only** GET (no SAY), the input field is painted starting
  at `<row>,<col>` (e.g. `@ 9,14 GET CLIENTS->FIRSTNAME` in `CLIENTS.FMT:4`).
- If the statement has **both** SAY and GET, the SAY text is painted at
  `<row>,<col>` and the GET field begins one column past the end of the SAY text
  (`Col()+1`). [documented: Harbour sayget.txt].

### 3.3 The GET variable `<var>`

- May be a **memory variable** (`mcheck`, `ans`, `answer1`) or a **database field**
  (`CLIENTS->FIRSTNAME`, `Chkno`, `Amt`, `Memo`).
- The displayed **field width** is the *length of the variable's current value /
  the field's defined width*, optionally widened/narrowed by the PICTURE template
  (a template of N characters defines an N-wide field). A character memvar must
  already hold a string of the desired width (hence the `SPACE(n)` initialisers and
  field-blank-then-GET idiom). [documented: Harbour sayget.txt + CLIENTS.FMT where
  each GET maps to a DBF field of fixed width].
- **Memo fields**: `@ r,c GET Memo` opens the memo editor window during READ; the
  sample `EDITVOID.PRG:63` does `@ 14,10 GET Memo` then edits via READ.
  [verified: EDITVOID.PRG / CANCL.PRG GET a Memo field + Harbour treats GET on memo
  as the memo editor].
- The variable/field type drives input validation: a numeric GET only accepts
  numeric input, a date GET enforces a date mask, a logical GET accepts only
  `T/F/Y/N`. See template chars in section 4.

### 3.4 Clauses — III+ vs later (IMPORTANT)

- `PICTURE`, `FUNCTION`, `RANGE` — **all present in III+** (RANGE is heavily used in
  the III+ sample programs: `EDITVOID.PRG:30` `GET mcheck PICTURE "9999" RANGE 0,`).
- `VALID` and `WHEN` — These clauses are **NOT part of the dBASE III PLUS 1.1
  `@...GET` surface.** They were introduced in **dBASE IV** (and exist in Clipper /
  Harbour, which is why `sayget.txt` documents them). III+ achieves the same effect
  *procedurally*: after `READ`, the program re-tests the entered value with `IF`
  and loops back (the universal III+ idiom — see `CHECK.PRG` "No blank checks
  allowed - please reenter", `EDITVOID.PRG` re-prompt loops).
  **Do not emit VALID/WHEN for a III+ target.**
  [verified: HELP_topics has no VALID/WHEN topic line at all (the index jumps from
  `@...GET` line 7 straight to `@...SAY`/`@...TO`/`@...CLEAR`/`PICTURE` — there is
  no VALID, WHEN, COLOR, or RANGE topic). The Harbour `sayget.txt` $SYNTAX$ DOES
  list `[WHEN <lWhen>] ... [VALID <lValid> / RANGE ...]` with code-block support —
  i.e. these are Clipper/IV additions, confirming they are *not* III+. None of the
  29 III+ sample programs use VALID or WHEN; they all use the manual IF/READ
  re-prompt loop — e.g. CHECK.PRG:90-99, EDITVOID.PRG:116-124]
  [oracle-resolves: confirm under DOSBox that III+ 1.1 raises a syntax error on
  `GET ... VALID ...`].

---

## 4. PICTURE / FUNCTION — the @ ... GET / SAY formatting clause (III+ surface)

Full grammar and exact transform semantics live in
`./picture-function-templates.md`. Enumerated here for *completeness against the
PICTURE topic* and to pin which symbols exist on the III+ surface.

A PICTURE string is `[<@-function clause>] [<template>]`. If both are present a
**space separates** the function clause from the template (`"@!S 30"` =
function `@!` + scroll-size, but `"@! AAA"` would be function then template — the
space is significant). [documented: Harbour sayget.txt "a blank space must be
included ... if there are both functions and templates"].

### 4.1 Template characters (per output position)

| Char | Meaning |
|---|---|
| `A` | Alphabetic only |
| `N` | Alphanumeric (letters + digits) |
| `X` | Any character |
| `L` | Logical, accepts `T`/`F` only |
| `Y` | Logical, accepts `Y`/`N` only |
| `9` | Digit (and sign for numerics) |
| `#` | Digit, sign, or space |
| `!` | Letter, converted to UPPERCASE |
| `$` | (numeric) `$` shown in leading-space positions |
| `*` | (numeric) `*` shown in leading-space positions |
| `.` | Decimal-point position |
| `,` | Comma (thousands) position |

[documented: Harbour sayget.txt GET templates table; III+ supports this exact
classic set]. Real use: `PICTURE "9999"` (4-digit check no., `EDITVOID.PRG:30`),
`PICTURE "999,999.99"` (money, `EDITVOID.PRG:130`), `PICTURE "9"` (single digit,
`EDITVOID.PRG:194`).

### 4.2 `@`-function clauses confirmed on the III+ surface

`@!` (uppercase the whole field), `@A` (alpha only), `@Z` (show zero as blanks),
`@B` (left-justify numerics), `@C` / `@X` (CR / DB after numbers), `@(`/`@)`
(parenthesise negatives), `@D`/`@E` (date format), `@R` (insert non-template
chars), `@S<n>` (scroll a field within an `<n>`-wide window). [documented: Harbour
sayget.txt + transfrm.txt]. Confirm the exact subset honoured by III+ 1.1 in the
companion spec; **`@K` (clear-on-first-keystroke suggested value)** is documented by
Harbour but its III+ availability is `[oracle-resolves]`.

---

## 5. RANGE — bounded numeric/date input

### 5.1 Syntax

```
@ <row>,<col> [SAY ...] GET <var> [PICTURE ...] RANGE <lo>, <hi>
```

- Applies to **numeric and date** GETs only; `<lo>`/`<hi>` must be the same type as
  `<var>`. The range is **inclusive**.
- On READ, if the entered value is outside `[lo,hi]`, dBASE refuses to leave the
  field and shows **"RANGE is <lo> to <hi>"** in the scoreboard/status area, then
  returns the cursor to the field. [verified: `DBASE.MSG.strings.txt` line 158 holds
  the literal prefix `"RANGE is "` and line 210 holds the infix `" to "` — the III+
  message is assembled as `RANGE is <lo> to <hi>`. NOTE the Harbour/Clipper oracle
  (`sayget.txt`) instead emits the literal **"OUT OF RANGE"** at row 0 col 60; that
  is the later-era wording, NOT III+'s — the period-exact `DBASE.MSG` is authority
  here. The III+ column placement of the message is `[oracle-resolves]`].

### 5.2 Open-ended ranges (real III+ idiom)

Either bound may be **omitted** by leaving the position empty. The III+ sample
programs use a **trailing comma with no upper bound** to mean "no maximum", and
rely on the `;` line-continuation:

```
@ 18,19 SAY "Enter Check Number (or 0 to exit)";          && EDITVOID.PRG:29-30
       GET mcheck PICTURE "9999" RANGE 0,                  && lower=0, no upper
```

and

```
@ 18,11 SAY "Enter Check Number to be canceled (or 0 to exit)";   && CANCL.PRG:43-44
       GET mcan PICTURE "9999" RANGE 0,
```

So `RANGE 0,` = "at least 0, no upper limit". A leading-blank lower bound
(`RANGE ,100`) means "no lower limit, at most 100". [verified: two independent
sample programs (EDITVOID.PRG, CANCL.PRG) use `RANGE 0,` + Harbour documents
RANGE]. The `;` at end of the SAY line continues the statement onto the next
physical line — this is general III+ statement continuation, shown here because the
SAY+GET+RANGE form is long.

---

## 6. READ — activate all pending GETs

### 6.1 Syntax

```
READ [SAVE]
```

(III+ READ is essentially argument-less; the only documented option is `SAVE`,
which leaves the GET list intact after the READ instead of clearing it. The bare
`READ` is what every sample program uses.) [documented: Harbour READ; III+ samples
use bare `READ` (EDITVOID.PRG:31, CHECK.PRG:42, etc.)]
[oracle-resolves: confirm III+ 1.1 accepts `READ SAVE` — Clipper does; III+ III
manual lists `READ`].

### 6.2 What READ does

1. Takes **all GETs declared since the last READ / CLEAR GETS** (the *pending GET
   list*, section 7) and enters **full-screen edit mode** over them.
2. Positions the cursor in the **first** pending GET and lets the user move among
   the fields with the navigation keys (section 8), editing each in place.
3. On termination, **writes each edited value back** into its variable/field
   (section 7.3) and **clears the pending GET list** (unless `READ SAVE`).
4. `READ` is a **no-op if there are no pending GETs**. [documented: Harbour;
   GET-list model].

The defining pattern is *declare GETs, then one READ*:

```
@ 4,71 GET Chkno                         && EDITVOID.PRG:59-64
@ 6,68 GET Date
@ 8,25 GET Payto
@ 8,66 GET Amt
@ 14,10 GET Memo
CLEAR GETS                               && (here they clear instead of READ — see 7.4)
```

vs. the more common single-field prompt:

```
@ 4,71 GET mchkno PICTURE "9999"         && CHECK.PRG:41-42
READ
```

---

## 7. The pending-GET (GET list) model

This is the heart of the full-screen input model and the part a reimplementation
most often gets wrong.

### 7.1 The list

- Every executed `@...GET` **appends an entry to an internal "pending GET list"**
  (the GETs that have not yet been consumed). Issuing more `@...GET` statements just
  grows the list; nothing is read until a `READ`.
  [verified: Harbour "adds a GET object to the reserved array ... `GetList`" + III+
  sample idiom of multiple GETs then one READ, e.g. EDITVOID.PRG:59-64].
- The order of the list is the **order the `@...GET` statements executed**, not
  screen position. During READ, Tab/Enter walk the list in that order. [documented:
  Harbour GET-list semantics].

### 7.2 `.FMT` files build the list

A `SET FORMAT TO <name>` followed by `EDIT`/`APPEND`/`READ` runs the `.FMT`
program, whose body is exactly a sequence of `@...SAY`/`@...GET`/`@...TO`
statements that builds the GET list, then the editor's implicit READ activates it.
`CLIENTS.FMT` is precisely such a file (8 GETs + SAYs + 2 boxes). See section 11
and `../file-formats/scr-and-fmt.md`.

### 7.3 Write-back

- On a normal READ termination, **each GET's edited buffer is written back to its
  variable or DBF field**. For a DBF field GET, the field in the *current record*
  is REPLACEd. For a memvar GET, the memvar is reassigned.
- If the user changed nothing in a field, its value is unchanged (and `Updated()`
  would report .F. in Clipper; III+ has no `UPDATED()` function — see open
  questions). [documented: Harbour; III+ field/memvar write-back is the documented
  EDIT/APPEND behaviour].
- If READ is aborted with **Esc** and `SET ESCAPE ON` (default), edits to the
  current/unfinished fields are abandoned; behaviour of already-confirmed earlier
  fields is `[oracle-resolves]`.

### 7.4 CLEAR GETS — discard the pending list

```
CLEAR GETS
```

- **Removes all pending GETs from the list without reading them.** After
  `CLEAR GETS`, a subsequent `READ` has nothing to do.
- Used in III+ to *display* fields with `@...GET` (so they appear in the field
  boxes) but then **suppress editing** of them — paint-only. `EDITVOID.PRG:59-64`
  GETs five fields then `CLEAR GETS` so the record is *shown* in the mask but not
  re-entered at that point; later a targeted GET+READ edits a single field.
  [verified: EDITVOID.PRG:63-64 GET Memo then CLEAR GETS; CHECK/CANCL similar].
- `CLEAR GETS` does **not** erase anything from the screen — it only empties the
  pending list. (To blank the screen use `CLEAR` / `@...CLEAR`, section 10.)
  [documented: Harbour CLEAR GETS clears the GetList array, not the display].

---

## 8. Navigation & editing keys during READ

The full-screen READ editor (and `EDIT`/`APPEND`/`BROWSE`) uses the classic dBASE
WordStar-style key map. [documented: Harbour `ReadKey()` exit-code table in
inputall.txt enumerates the navigation keys the GET editor honours; the cursor-key
set is III+/Clipper-common]. Key behaviour:

| Key(s) | Action |
|---|---|
| `Enter` / `Ctrl-X` / `Down` (Ctrl-S/Ctrl-D within field) | confirm field, advance to next GET; on last GET, terminate READ |
| `Up` (Ctrl-E) | move to previous GET |
| `Down` (Ctrl-X) | move to next GET |
| `Left` / `Right` (Ctrl-S / Ctrl-D) | move cursor within the field |
| `Home`/`End` (Ctrl-A / Ctrl-F word moves) | start/end of field |
| `PgUp` / `PgDn` (Ctrl-R / Ctrl-C) | terminate READ (in record editors, prev/next record) |
| `Ins` (Ctrl-V) | toggle insert/overwrite |
| `Del` (Ctrl-G) | delete char under cursor |
| `Backspace` | delete char left |
| `Ctrl-Y` | clear to end of field |
| `Ctrl-W` | terminate READ saving (write) |
| `Ctrl-Q` / `Esc` | terminate READ abandoning current entry (Esc obeys SET ESCAPE) |

Termination: filling the **last** GET's last position and pressing Enter, or any
explicit terminate key (PgUp/PgDn/Ctrl-W/Esc), ends the READ. [documented: Harbour
ReadKey() exit codes: Up=4, Down=5, PgUp=6, PgDn=7, etc., plus +256 if the GET
buffer was changed]. The precise III+ 1.1 key map (e.g. whether Tab is a GET-move)
should be confirmed under DOSBox — `[oracle-resolves]`.

Relevant SET commands affecting READ (documented in the environment spec; named
here because they change READ behaviour): `SET CONFIRM ON` forces an explicit Enter
to leave a full field (default OFF = auto-advance on fill); `SET ESCAPE` controls
whether Esc aborts; `SET DELIMITERS ON`/`SET DELIMITERS TO` draws delimiters around
GET fields; `SET INTENSITY` controls reverse-video on GETs; `SET BELL` controls the
error beep; `SET DATE` / `SET CENTURY` affect date GET masks. [documented:
HELP_topics lists all these as III+ topics].

---

## 9. @ ... TO — draw boxes and lines

### 9.1 Syntax

```
@ <top>,<left> TO <bottom>,<right> [DOUBLE]
```

- Draws a **rectangle** (box) whose opposite corners are `<top>,<left>` and
  `<bottom>,<right>`. [verified: HELP_topics line 9 `@...TO` + CLIENTS.FMT &
  MENUMASK.PRG box statements].
- `DOUBLE` — draw the box with **double-line** graphics characters instead of the
  default single line. [verified: MENUMASK.PRG:8 `@ 4,1 TO 23,77 DOUBLE`, and
  several single-line boxes alongside].
- A **degenerate box draws a line**: if `<top>==<bottom>` it draws a horizontal
  line; if `<left>==<right>` a vertical line. [documented: classic @...TO line
  behaviour; consistent with box semantics] `[oracle-resolves]` for exact III+
  glyph at single-coordinate spans.

### 9.2 Box-drawing characters

The default single box uses the PC line-draw glyphs (CP437 0xC4 horizontal, 0xB3
vertical, corners 0xDA/0xBF/0xC0/0xD9); DOUBLE uses the double-line glyphs
(0xCD/0xBA, corners 0xC9/0xBB/0xC8/0xBC). [inferred from CP437 + standard dBASE box
output; not byte-verified against a III+ screen capture — `[oracle-resolves]`].
Programmers also draw "boxes" manually with shaded glyphs: `MENUMASK.PRG` lays
shadow with `CHR(176)` (the light-shade block) via `REPLICATE(CHR(176),n)`.

### 9.3 Real III+ idioms

```
@  8,  0  TO 12, 79                  && CLIENTS.FMT:17  single-line full-width box
@ 14,  0  TO 18, 79                  && CLIENTS.FMT:18
@ 1,9 TO 3,69                        && MENUMASK.PRG:7   single
@ 4,1 TO 23,77 DOUBLE                && MENUMASK.PRG:8   double outer frame
@ 5,4 TO 7,20 DOUBLE                 && MENUMASK.PRG:10  small double box (label tab)
```

`@...TO` paints immediately; it is not part of the GET list and is unaffected by
READ. In a `.FMT` file the boxes are drawn each time the format is rendered.

---

## 10. @ ... CLEAR — blank a screen region

### 10.1 Syntax (two forms)

```
@ <row>,<col> CLEAR                          && clear a rectangle from r,c to 24,79
@ <row>,<col> CLEAR TO <row2>,<col2>         && clear a specific rectangle  [III+/IV]
@ <row>,<col>                                && (no keyword) clears from c to EOL on row
```

- `@ <row>,<col> CLEAR` — **erases the rectangle** from `<row>,<col>` to the
  bottom-right of the screen (24,79), filling it with blanks (current color). It
  also homes the cursor into the cleared region. [verified: HELP_topics line 10
  `@...CLEAR` + real use `EDITVOID.PRG:52` `@ 20,0 CLEAR`, `:69` `@ 18,0 CLEAR`,
  `CANCL.PRG:52/98` `@ 18,0 CLEAR` / `@ 20, 0 CLEAR`].
- `@ <row>,<col> CLEAR TO <row2>,<col2>` — clear only the bounded rectangle.
  The `CLEAR TO` extension's presence in III+ 1.1 is `[oracle-resolves]`
  (Clipper/IV have it; the III+ samples only ever use the bare `@ r,c CLEAR`).
- `@ <row>,<col>` with **no SAY/GET/TO/CLEAR keyword** — clears the rest of that
  row from `<col>` to column 79 and leaves the cursor there. `EDITVOID.PRG:56`
  `@ 18,0` blanks line 18 before re-drawing field GETs. [verified: EDITVOID.PRG:56
  bare `@ 18,0` used to wipe a status line + classic dBASE "@ r,c with nothing =
  erase to end of line"].

### 10.2 vs. plain CLEAR

Plain `CLEAR` (no coordinates) clears the **whole screen** and releases all pending
GETs; `@ r,c CLEAR` clears only the lower-right region from `r,c`. Do not confuse
them. [documented: HELP_topics has both `@...CLEAR` (line 10) and `CLEAR` (line 20)
as separate topics].

---

## 11. SET FORMAT TO — bind a .FMT format file

### 11.1 Syntax & behaviour (period-exact)

ASSIST.HLP, verbatim:
> "SET FORMAT TO selects a custom screen format (.fmt) file that is used for
> full-screen editing. This format file defines the input screen used with the Edit
> and Append options in the Retrieve menu."
> Command Format: `SET FORMAT TO [<format file name>/?]`
[verified: ASSIST.HLP.strings.txt lines 8-12]

```
SET FORMAT TO <format file name>       && activate <name>.FMT
SET FORMAT TO ?                         && pick from a list (ASSIST/interactive)
SET FORMAT TO                           && (no arg) deactivate; revert to standard editor
```

- `<name>.FMT` defaults to extension **`.FMT`** if none given. [documented:
  ASSIST.HLP "(.fmt) file" + scr-and-fmt.md].
- Once set, **`EDIT`, `APPEND`, `CHANGE`, `INSERT`, and `READ`** use the format
  file's `@...SAY/GET/TO` layout as the full-screen edit mask instead of the
  default field-stacked editor. [documented: ASSIST.HLP "used with the Edit and
  Append options" + Harbour SET FORMAT].
- The `.FMT` is a **text dBASE program**: it is loaded and its `@` statements are
  executed to build the SAY/GET layout each time the editor needs to paint a record
  (see `CLIENTS.FMT`, a real generated format file). The painter that *generates*
  `.FMT` from a `.SCR` worktable is `CREATE/MODIFY SCREEN`. Full format-file grammar
  and the `.SCR` binary: `../file-formats/scr-and-fmt.md`.
- `SET FORMAT TO` with no file name **closes** the active format file and restores
  the standard editor. [documented: ASSIST `[<file>/?]` optional bracket implies the
  no-arg deactivate form; consistent with all `SET ... TO` commands].

### 11.2 `CLIENTS.FMT` as the worked example

```
@  4, 21  SAY "CLIENTS ADDRESS AND PHONE ENTRY"     && title (SAY only)
@  9,  1  SAY "FIRST NAME:"                          && label
@  9, 14  GET  CLIENTS->FIRSTNAME                    && GET -> pending list[0]
@  9, 37  SAY "LAST NAME:"
@  9, 50  GET  CLIENTS->LASTNAME                     && pending list[1]
...
@  8,  0  TO 12, 79                                  && box around name/phone block
@ 14,  0  TO 18, 79                                  && box around address block
```

When `SET FORMAT TO CLIENTS` then `APPEND`/`EDIT` runs, dBASE executes these
statements: SAYs paint labels, the 8 GETs build the pending list (with the
`CLIENTS->` alias binding each field to the CLIENTS work area's current record),
the two `@...TO` draw the frames, and the editor's implicit READ activates all
GETs for editing. [verified: CLIENTS.FMT bytes + the SET FORMAT semantics above].

---

## 12. III+ specifics, edge cases, and gotchas

### 12.1 `C + N` is a TYPE-MISMATCH ERROR (the single biggest hazard)

In a SAY expression you may NOT mix character and numeric with `+`. III+ has **no
auto-stringification**. `@ r,c SAY "Total: " + total` (total numeric) raises
**"Data type mismatch."** (`DBASE.MSG` line 9). You must convert:
`@ r,c SAY "Total: " + STR(total,10,2)`. The real samples always convert:
`MENUMASK.PRG:95` `@ 20,57 SAY "$"+LTRIM(STR(balance,10,2))` and
`CANCL.PRG:94` `@ 20,21 SAY "Check "+LTRIM(STR(mcan,4))+" cannot be found."`.
[verified: DBASE.MSG line 9 "Data type mismatch." + every sample SAY that joins
text to a number wraps it in STR()/LTRIM(STR()); cross-checked against
`archive/dbase-com-help/OPS_PLUS.md`, whose dBASE PLUS `+` *does* auto-convert
("You may concatenate any other data type to a string ... Numbers become strings
with no leading spaces ... logical values true and false become the strings"). That
modern dbase.com `+` behaviour is **WRONG for III+** — do not import it; III+ raises
the type-mismatch error and requires an explicit STR()/DTOC()/IIF conversion].

### 12.2 RANGE/PICTURE clause ordering

The III+ samples write `PICTURE` before `RANGE` (`GET mcheck PICTURE "9999" RANGE
0,`). Whether III+ tolerates the reverse order is `[oracle-resolves]`; emit
`PICTURE ... RANGE ...` to match the samples.

### 12.3 No VALID / WHEN / COLOR / code-block GETs

Recap of section 3.4: `VALID`, `WHEN`, `COLOR`, and code-block (`{|| ...}`)
arguments are **dBASE IV / Clipper / dBASE PLUS** features absent from III+ 1.1.
The Harbour `sayget.txt` oracle documents them; they are explicitly **excluded**
from the III+ surface. III+ does validation procedurally with IF/READ loops.

### 12.4 GET on character memvar requires pre-sized string

A character GET edits in place over the *current length* of the string, so you must
initialise the memvar to the right width first (`store space(25) to ans` then
`@ r,c GET ans`). Numerics initialise to a value of the right magnitude
(`mcheck = 0`, `EDITVOID.PRG:26`). [verified: sample-program initialise-then-GET
idiom throughout].

### 12.5 SAY/GET to printer

`@...SAY` honours `SET DEVICE TO PRINT` (positioned printed output). `@...GET` is
meaningless to the printer and is ignored when DEVICE is PRINT. [documented:
Harbour sayget.txt].

### 12.6 Statement continuation

A long `@...SAY...GET...` may be split across physical lines with a trailing `;`
(`EDITVOID.PRG:29-30`, `CANCL.PRG:43-44`, `CHECK.PRG:119/129/225/250`). This is
general III+ syntax, not specific to `@`. [verified: the `;`-terminated lines in
CHECK.PRG are 119, 129, 225, 250 — each a long `@...SAY` broken before a string
concatenation or GET].

---

## Open questions (`[oracle-resolves]` — need DBASE.EXE under DOSBox)

1. **VALID/WHEN syntax error.** Confirm III+ 1.1 rejects `@...GET ... VALID <cond>`
   and `... WHEN <cond>` as syntax errors (this spec asserts they are not on the
   III+ surface based on absence from HELP_topics and all 29 sample programs).
2. **Reserved rows.** Exact drawable region and overwrite behaviour of rows 0
   (scoreboard) and 24 (status) under each combination of `SET STATUS` /
   `SET SCOREBOARD`. Where exactly does "RANGE is ..." render (column)?
3. **`READ SAVE`.** Does III+ 1.1 accept the `SAVE` option (keep the GET list after
   READ), or is READ strictly argument-less?
4. **`@ r,c CLEAR TO r2,c2`.** Is the bounded-rectangle `CLEAR TO` extension present
   in III+ 1.1, or only the bare `@ r,c CLEAR` (clear to 24,79)?
5. **Box glyphs.** Confirm the exact CP437 line-draw characters used for single vs.
   `DOUBLE` boxes, and the glyph emitted for degenerate (single-row/col) `@...TO`.
6. **Navigation key map.** The precise III+ 1.1 READ-editor key bindings (esp.
   whether Tab moves between GETs; PgUp/PgDn behaviour outside record editors;
   Ctrl-W vs. Enter-on-last-field termination distinctions).
7. **`@K`, `@S<n>` availability.** Which `@`-functions in section 4.2 are actually
   honoured by III+ 1.1 (vs. Clipper-only additions). Resolve jointly with
   `./picture-function-templates.md`.
8. **Esc abort write-back.** When READ is aborted with Esc, are earlier
   already-confirmed GETs written back, or is the whole READ rolled back?
9. **Standalone `FUNCTION` clause.** Does III+ accept `@...SAY <exp> FUNCTION
   "@!"` as a separate keyword from PICTURE, or only `PICTURE`?

---

## Sources

- `archive/golden-mined/HELP_topics.txt` — completeness oracle; topic lines 7-11
  (`@...GET`, `@...SAY`, `@...TO`, `@...CLEAR`, `PICTURE`), 77 (`READ`), 123
  (`SET FORMAT`), plus adjacent SET topics. [period-exact existence/index]
- `archive/golden-mined/ASSIST.HLP.strings.txt` — `SET FORMAT TO` / `CREATE SCREEN`
  / `MODIFY SCREEN` verbatim help (lines 8-12, 41-44, 227-231). [period-exact]
- `archive/golden-mined/DBASE.MSG.strings.txt` — error catalog: "Data type
  mismatch." (line 9), "RANGE is " (the RANGE-failure message prefix). [period-exact]
- `oracles/harbour/doc/en/sayget.txt` — `@...GET` / `@...SAY` semantic oracle
  (GET list, PICTURE/template tables, RANGE/VALID/WHEN, device). [semantic, NOT
  period syntax; later-era VALID/WHEN/COLOR/code-block flagged and excluded]
- `oracles/harbour/doc/en/inputall.txt` — `ReadKey()` exit-code table → navigation
  keys honoured by the GET editor. [semantic]
- `oracles/harbour/doc/transfrm.txt` (+ `doc/en/transfrm.txt`) — `@`-function
  enumeration for numerics/dates/strings. [semantic; full grammar -> companion spec]
- `archive/dbase-com-help/SET_EXACT.md` — `=` / SET EXACT (string compare context).
- Real III+ sample programs (`goldens/dbase-iii-plus-1.1-pristine/files/
  Sample_Programs_and_Utilities/`): `CLIENTS.FMT` (SAY/GET/TO format file),
  `MENUMASK.PRG` (boxes, DOUBLE, SAY layout, C+C with STR), `EDITVOID.PRG`
  (GET/READ/RANGE open-ended/CLEAR GETS/`@ r,c`/`@ r,c CLEAR`), `CANCL.PRG`
  (GET PICTURE RANGE, `@ r,c CLEAR`), `CHECK.PRG` (GET PICTURE/READ/re-prompt loop).
  [verified idioms]
- Companion specs: `./picture-function-templates.md` (PICTURE/FUNCTION grammar),
  `../file-formats/scr-and-fmt.md` (.FMT/.SCR formats).

---

## Verification

Adversarial cross-check, 2026-06-16, by an independent verifier. Every load-bearing
claim was re-derived from a SECOND source (mined period text + Harbour
Clipper-lineage oracle + dbase.com), not just the author's primary citation.

### Completeness vs the help index
All seven in-scope `HELP_topics.txt` items are covered and the line numbers check
out exactly: `@...GET`=7, `@...SAY`=8, `@...TO`=9, `@...CLEAR`=10, `PICTURE`=11,
`READ`=77, `SET FORMAT`=123. Confirmed the index has **no** VALID, WHEN, COLOR, or
RANGE topic line of its own — corroborating that VALID/WHEN/COLOR are off the III+
surface and RANGE rides inside `@...GET`. COMPLETE against the oracle index.

### Cross-checks that PASSED (tag raised to verified)
- **`C + N` = "Data type mismatch."** Confirmed `DBASE.MSG.strings.txt` line 9, and
  cross-checked `archive/dbase-com-help/OPS_PLUS.md` — modern dBASE PLUS `+` *does*
  auto-stringify, so the author's "WRONG for III+" flag is correct. The biggest
  semantic hazard is correctly called.
- **VALID/WHEN are dBASE IV/Clipper, not III+.** Harbour `sayget.txt` $SYNTAX$
  explicitly lists `[WHEN <lWhen>]` and `[VALID <lValid> / RANGE ...]` with
  code-block support; HELP_topics has no such topic; 0/29 samples use them. Section
  3.4 / 12.3 are correct. (DOSBox syntax-error confirmation remains an open item.)
- **RANGE-failure message.** `DBASE.MSG` line 158 `"RANGE is "` + line 210 `" to "`
  assemble the III+ "RANGE is <lo> to <hi>". The Harbour oracle's literal is
  "OUT OF RANGE" at row 0/col 60 — later-era, NOT III+; period-exact `DBASE.MSG`
  wins. (Was previously under-stated; now flagged explicitly — see fixes.)
- **ReadKey() navigation codes** (section 8): `inputall.txt` lines 361-372 confirm
  Up=4, Down=5, PgUp=6, PgDn=7, Esc=12, Ctrl-End=14, Enter=15, and "+256 if the GET
  buffer was changed" (line 355). Table is consistent with the oracle.
- **`SET FORMAT TO` text** verbatim-matches `ASSIST.HLP.strings.txt` lines 8-12.
- **Coordinate/idiom claims** re-checked byte-for-byte against the five samples:
  CLIENTS.FMT (`@ 8,0 TO 12,79`, `@ 14,0 TO 18,79`, 8 GETs), MENUMASK.PRG
  (`@ 4,1 TO 23,77 DOUBLE` :8, `@ 5,4 TO 7,20 DOUBLE` :10, `CHR(176)` :22,
  `"$"+LTRIM(STR(balance,10,2))` :95), EDITVOID.PRG (`RANGE 0,` :30, GET×5+CLEAR
  GETS :59-64, bare `@ 18,0` :56, `@ 20,0 CLEAR` :52, `@ mrow,mcol GET mtax` :211),
  CANCL.PRG (`RANGE 0,` :44, `SAY Amt PICTURE "9999999.99"` :62, `@ 18,0 CLEAR` :52),
  CHECK.PRG (`GET mchkno PICTURE "9999"` + READ :41-42). All confirmed.

### Fixes applied by the verifier
1. **§5.1 + §1.1 RANGE message:** previously implied Harbour corroborated the exact
   wording; corrected to record that Harbour's literal is "OUT OF RANGE" (a III+-vs-
   Clipper delta) and that the III+ wording is sourced from `DBASE.MSG` 158+210.
2. **§3.4 VALID/WHEN citation:** sharpened to note HELP_topics has no VALID/WHEN/
   COLOR/RANGE topic at all, and that Harbour's $SYNTAX$ block is what carries the
   later-era VALID/WHEN — making the exclusion airtight. Fixed the re-prompt-loop
   line cite from `CHECK.PRG:93-101` → `CHECK.PRG:90-99` (the actual loop body) and
   `EDITVOID.PRG:118-124` → `:116-124`.
3. **§12.6 statement-continuation cite:** `CHECK.PRG:137` was wrong (line 137 is
   `@ 20,61 GET ans`, not a `;`-continued line); corrected to the real continuation
   lines `CHECK.PRG:119/129/225/250`.
4. **§12.1:** added the explicit `OPS_PLUS.md` cross-check quote behind the
   no-auto-stringification claim.

No claim was found that imports a dBASE IV/5/PLUS-only rule into the III+ surface;
the doc was already conservative (VALID/WHEN/COLOR/code-block, `C+N`, `READ SAVE`,
`CLEAR TO`, `@K` all correctly fenced as later-era or `[oracle-resolves]`). No
arrays/DECLARE, `==`, or IV-only functions appear.

### Residual `[oracle-resolves]` gaps (need DBASE.EXE under DOSBox)
Unchanged from the doc's Open-questions list (1-9): VALID/WHEN syntax-error
confirmation; reserved-row/scoreboard column behaviour; `READ SAVE` acceptance;
`@ r,c CLEAR TO` extension; exact CP437 box glyphs + degenerate-`@...TO` glyph;
precise III+ key map (Tab/PgUp/PgDn/Ctrl-W-vs-Enter); `@K`/`@S<n>` availability;
Esc-abort write-back of earlier GETs; standalone `FUNCTION` clause on `@...SAY`.
These are not yet mirrored into `GAPS.md` (the full-screen module has no GAPS
section there yet) — recommend adding a "Full-screen I/O" block to GAPS.md so the
RE phase picks them up. Verifier did not edit GAPS.md (out of this doc's scope).
