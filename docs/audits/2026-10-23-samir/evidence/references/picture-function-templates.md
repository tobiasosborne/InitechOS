# PICTURE Templates, @-FUNCTION Codes, RANGE, and TRANSFORM() (dBASE III PLUS 1.1)

## Status / confidence

Scope: the PICTURE template language, the FUNCTION (`@`-prefix) code set, the
`RANGE` clause of `@...GET`, the negative-number sign / parentheses /
DB-CR formats, and the `TRANSFORM()` function. These cover the period HELP
oracle items **`@...GET`** (line 7), **`@...SAY`** (line 8), and **`PICTURE`**
(line 11) of `archive/golden-mined/HELP_topics.txt`, plus their `@`-mirror
entries (lines 179, 180, 234).

Completeness note: `TRANSFORM` and `RANGE` are **not** standalone topics in the
406-item index. `RANGE` is a clause of `@...GET` (covered here); `TRANSFORM()`
is a numeric/character function. The III+ surface evidence for `TRANSFORM()`'s
*existence* is weak (see Open questions) -> a GAPS entry is warranted. The
PICTURE/FUNCTION clause itself is solidly attested by both the mined CREATE
SCREEN help text and 12+ real sample `.prg` programs.

Confidence summary:

- The PICTURE template chars actually used in III+ samples (`9`, `#`, `!`, `A`,
  `N`, `X`, `Y`, `L`, `.`, `,`, `$`, `*`) and the `@`-function set: behavior is
  **[verified: dbase.com function/template help + Harbour/xHarbour transfrm.txt]**
  for the cross-Xbase-stable core, but several FUNCTION letters documented by
  dbase.com (`I`, `J`, `T`, `^`, `M`, `S<n>` scroll, `R` insert-on-GET) are
  **dBASE-IV-or-later additions** and are tagged as deltas below; do not assume
  III+ honors them.
- The single biggest III+ hazard (`C + N` is a **type-mismatch error**, not
  auto-stringification) is reaffirmed where TRANSFORM/STR interplay matters.
- Items needing the real DBASE.EXE/DBASE.OVL under DOSBox to settle are tagged
  `[oracle-resolves]` and listed in **Open questions**.

Cross-refs: this file documents the *template grammar*. The host commands
`@...SAY...GET` and `READ` (cursor model, edit keys, GET stack, full-screen
mechanics) belong to the `@...GET`/`@...SAY` command spec; only the PICTURE /
FUNCTION / RANGE clauses are detailed here.

---

## 1. Where templates are used

A template (a PICTURE clause, an optional FUNCTION clause, and for numeric GETs
a RANGE clause) can appear in four places in III+:

1. `@ <row>,<col> SAY <exp> [PICTURE <expC>] [FUNCTION <expC>]`
   -- formats a value for **display** (output mask).
2. `@ <row>,<col> [SAY <exp>] GET <var> [PICTURE <expC>] [FUNCTION <expC>]`
   `[RANGE <low>,<high>] [VALID ...]` -- formats a field/memvar for **editing**
   (input mask), then activated by a subsequent `READ`. (VALID is **not** III+;
   see delta below.)
3. `TRANSFORM(<exp>, <expC-picture>)` -- returns the formatted value as a
   character string, for use in `?`, `REPORT`, `LABEL`, `LIST`, etc.
4. CREATE SCREEN / CREATE REPORT / CREATE LABEL form designers, which store a
   PICTURE string and a FUNCTION-symbol string into the generated `.fmt`/`.frm`/
   `.lbl` and emit the corresponding `@...SAY...GET ... PICTURE ...` code.

The mined CREATE-SCREEN ("blackboard") prompts confirm the two-field design:
`"Enter or update picture value."` and, separately,
`"Enter one or more function symbols without using quotes."`
[verified: archive/golden-mined/DBASE.MSG.strings.txt lines 416, 427]. So in
the designer the PICTURE *template* and the FUNCTION *symbols* are entered into
**two distinct fields**; in source code they may be merged by writing the
function letters after a leading `@` at the front of the PICTURE string.

### 1.1 PICTURE vs FUNCTION -- the two-part model

- **FUNCTION** = a directive that applies to the **whole** value (alignment,
  uppercasing, currency, sign style). Written as a run of letters, each
  optionally preceded conceptually by `@`. In a PICTURE string the function run
  is introduced by a single leading `@`, e.g. `PICTURE "@!R 999-9999"`.
- **PICTURE** (template proper) = a string of **per-position** characters, one
  per character of the value, controlling each position individually.

In III+ source code essentially everyone writes only a PICTURE clause and folds
any function codes into it with a leading `@`. Real samples use `@!` to force
uppercase: `@ 10,37 GET answer PICTURE "@!"`
[verified: goldens/.../REINIT.PRG:15 + YEAREND.PRG:18]. The separate `FUNCTION`
keyword exists but is rarely seen in the sample corpus (none of the 30 sample
`.prg` files use the standalone `FUNCTION` keyword; all fold codes into PICTURE)
[verified: grep over Sample_Programs_and_Utilities/*.PRG -- 0 standalone FUNCTION].

Equivalence: `... PICTURE "@!"` is the same as `... FUNCTION "!"` with no
per-position template. When both a FUNCTION clause and a PICTURE clause are
given, the FUNCTION applies to the whole value and the PICTURE applies
per-position [documented: dbase.com function/picture help].

---

## 2. PICTURE template characters (per-position)

Each template character governs the value position aligned with it. Characters
not in the table below are treated as **literals**: on SAY they are inserted
into the output; on GET they are displayed but skipped over (the user cannot
type into a literal position), and -- under the default `@R`-less behavior --
literals *are* stored back into the variable (see 2.3, the `99/99/85` idiom).

### 2.1 Core template characters (III+ confirmed)

| Char | Applies to | SAY (display) | GET (input mask) |
|------|-----------|---------------|------------------|
| `9` | N, C | digit position; for numerics shows a digit, leading positions space-padded; sign allowed in numerics | accepts a digit; for numerics also accepts `-`/`+`/`.`/space |
| `#` | N, C | digit/sign/space/decimal-point position (looser than `9`) | accepts digit, sign, space, or the SET POINT char |
| `.` | N | decimal point: fixes where the fractional part begins | same; anchors the decimal column |
| `,` | N | grouping comma: printed only if a digit precedes it, else a blank | display-only separator (skipped on input) |
| `$` | N | currency/leading-`$` fill: leading blanks become `$` | fill char in the displayed field |
| `*` | N | asterisk ("check protection") fill: leading blanks become `*` | fill char in the displayed field |
| `A` | C | letters only [A-Za-z] | accepts only alphabetic chars |
| `N` | C | letters and digits | accepts letters and digits |
| `X` | C | any character | accepts any character |
| `!` | C | converts the letter at this position to uppercase | input forced to uppercase at this position |
| `L` | L | logical: shows `T`/`F` | accepts `T`/`t`/`F`/`f`/`Y`/`y`/`N`/`n` -> stored T/F |
| `Y` | L | logical: shows `Y`/`N` | accepts `Y`/`y`/`N`/`n` (and T/F) -> shown/stored as Y/N |

[verified: dbase.com template + function help + xharbour/doc/transfrm.txt lines
25-31 (numbers: `9 # . $ * ,`), 46-50 (logicals: `Y # L`), 58-61 (strings:
`! # 9` and `A N X`)]

Notes and III+ specifics:

- **`9` vs `#`**: both are digit positions for numerics. `9` is the everyday
  digit placeholder; `#` additionally tolerates sign and space and is the
  "numeric/sign/space" position. For character data `9` and `#` both restrict a
  position to a digit [verified: transfrm.txt:26-27 + transfrm.txt:60].
- **`A` / `N` / `X`** are the character-class templates: alphabetic-only,
  alphanumeric, any. `X` is the most permissive and is what you get by default
  for a character GET with no PICTURE [documented: transfrm.txt:61 + dbase.com].
- **`!`** uppercases only the position it occupies. To uppercase the whole field
  use the FUNCTION form `@!` (see 3) -- which is the dominant idiom:
  `PICTURE "@!"` and `PICTURE '!!!'` both appear in samples
  [verified: goldens/.../REINIT.PRG:15 (`"@!"`) + FLT_RSVE.PRG:44 (`'!!!'`)].
- **`L` / `Y`** are logical templates. `Y` displays/accepts `Y`/`N`; `L`
  displays/accepts `T`/`F`. Both accept Y/N/T/F on input
  [verified: transfrm.txt:46-50 + dbase.com]. III+ logical GETs frequently use
  no PICTURE at all (`@ 20,61 GET ans` then test `IF ans`)
  [documented: goldens/.../CHECK.PRG:137].
- **`$` and `*` fills**: leading blank (unused) integer positions are replaced
  by `$` or `*` respectively. These are *fill* characters, not literal `$`/`*`
  at a fixed spot. `*` is the classic check-protection fill
  [verified: transfrm.txt:29-30 + dbase.com `$` row].
- **`,`** prints a grouping comma only when at least one significant digit
  precedes it; otherwise the comma column shows a blank (so `5` formatted with
  `9,999` shows `    5`, not `   ,5`). Real numeric money masks combine these:
  `PICTURE "999,999.99"` and `PICTURE "9999999.99"`
  [verified: goldens/.../CHECK.PRG:112 + EDITVOID.PRG:130 + ADD.PRG (many)].

### 2.2 Worked display examples

```
? TRANSFORM(1234.5,  "9,999.99")      &&  "1,234.50"
? TRANSFORM(5,       "9,999")          &&  "    5"      (comma suppressed)
? TRANSFORM(1234.5,  "$$,$$$.99")      &&  "$1,234.50"  ($ fills leading blanks)
? TRANSFORM(1234.5,  "**,***.99")      &&  "*1,234.50"  (* fills leading blanks)
@ 8,66 GET mamt PICTURE "999,999.99"   &&  money entry up to 999,999.99
@ 9,31 GET dep_city PICTURE '!!!'      &&  3-char uppercase airport code
```

[examples: dbase.com TRANSFORM phone example + goldens/.../CHECK.PRG,
FLT_RSVE.PRG]

### 2.3 Literals in the template (the `99/99/85` idiom) -- III+ behavior

Non-template characters embedded in a PICTURE are literals. In the real sample
`@ 10,35 GET mdate PICTURE '99/99/85'` the `/` and the literal `85` form a date
mask: the user types four digits, and `/`...`/85` are shown as fixed literals
[verified: goldens/.../FLT_RSVE.PRG:46].

CRITICAL III+ delta on literal storage:

- In **III+ (and Clipper)**, with no insert function, literal characters that
  appear in the PICTURE **are stored back into the variable** along with the
  typed digits (the variable here is a character field receiving `12/31/85`).
  This is why `mdate` is a *character* field, not a date.
- The **`R` ("insert template literals, do not store them")** behavior -- where
  `(999) 999-9999` shows punctuation but stores only the 10 digits -- is the
  **dbase.com / dBASE IV+ model** documented as
  `@R (999) 999-9999` -> stores `8008675309`, displays `(800) 867-5309`
  [documented: dbase.com TRANSFORM phone example].
  In III+ the `R` function exists but its precise store-vs-keep semantics for
  GET differ from the modern doc -> **[oracle-resolves]** (see Open questions).
  For a III+ reimplementation, default to: literals are part of the stored value
  unless an insert/`R` function strips them.

---

## 3. FUNCTION codes (`@`-prefix, whole-value directives)

Function codes apply to the entire value. In a PICTURE string they are written
as a leading `@` followed by one or more code letters and then a space before
the per-position template, e.g. `PICTURE "@C 99,999.99"` or `PICTURE "@!"`.
In the CREATE SCREEN designer they go in the separate function-symbols field
(entered "without using quotes")
[verified: archive/golden-mined/DBASE.MSG.strings.txt:427].

### 3.1 The III+ FUNCTION set

The codes the Xbase lineage agrees on and that the III+ manual documents:

| Code | Applies to | Effect |
|------|-----------|--------|
| `@!` | C | uppercase the entire value |
| `@A` | C | restrict entry to alphabetic characters |
| `@B` | N | **left-justify** the number within its width (default is right-justified) |
| `@C` | N | append **`CR`** (credit) after a **positive** number |
| `@X` | N | append **`DB`** (debit) after a **negative** number |
| `@(` | N | enclose **negative** numbers in **parentheses**, no minus sign |
| `@D` | D/N | use the current `SET DATE` format |
| `@E` | D/N | use **European/British** date format (DD/MM/YY) and exchange `.`/`,` |
| `@R` | C | display literal template chars (the "insert" function; see 2.3 delta) |
| `@S<n>` | C | limit displayed width to `<n>` and horizontally scroll the field |
| `@Z` | N | display zero values as **blanks** |

[verified: xharbour/doc/transfrm.txt lines 13-19 (`@X`=`DB`, `@(`=parens,
`@B`=left-justify, `@C`=`CR`, `@E`=exchange `.`/`,`, `@Z`=blank-if-zero) +
dbase.com function table (`B`,`C`,`X`,`Z`,`(`,`A`,`D`,`E`,`R`,`S<n>`)]

The `@C`/`@X` pairing: `@C` puts `CR` after positives, `@X` puts `DB` after
negatives. Both are accounting conventions; you typically use one. dbase.com
states `C` = "Displays CR after a positive number", `X` = "Displays DB after a
negative number" [verified: dbase.com function table + transfrm.txt:13,17].

### 3.2 The negative-number sign format (default sign + the parens/DB/CR forms)

A trailing-minus would be the format where a negative number shows the minus
sign **after** the digits (e.g. `570-` instead of `-570`). **Caution -- this is
NOT confirmed for III+ and contradicts both independent oracles:**

- The Clipper/Harbour implementation oracle (`oracles/harbour/src/rtl/transfrm.c`)
  places the sign **leading** for the default/no-function case and uses
  asterisk-fill on overflow; it contains **no trailing-minus logic** at all
  [verified: transfrm.c lines 451-633 -- only PF_DEBIT/PF_PARNEG/PF_CREDIT and a
  leading `-` from `hb_itemStrBuf`].
- dbase.com's function help documents **no** trailing-minus format; negative
  numbers are handled only by `@(` (parentheses) or `@X` (DB)
  [verified: dbase.com function table -- "No trailing minus sign format is
  mentioned"].
- The "Office Space 570-" rendering is a COBOL/accounting-report convention, not
  an attested dBASE III+ default. Treat a default trailing-minus as a **possible
  misremembered/imported convention** until the real interpreter is checked.

  -> **[RESOLVED 2026-06-16 -- minted]** Confirmed against real DBASE.EXE
  (re/mint-results-002.md): **there is NO native trailing-minus format in III+.**
  `STR(-570,4)` -> `-570` (leading minus); `TRANSFORM(-570,"9999")` -> `-570`;
  `TRANSFORM(-570,"@( 9999")` -> `( 570)`; `TRANSFORM(-570,"@X 9999")` -> `570 DB`;
  `TRANSFORM(570,"@C 9999")` -> `570 CR`. A literal `-` in a template
  (`TRANSFORM(-570,"999-")` -> `***-`) is just a LITERAL character (and the value
  overflowed the 3 nines -> `***`). So dBASE's negative/accounting forms are:
  leading minus (default), `@(` parentheses, `@X` trailing ` DB`, `@C` trailing
  ` CR` -- NOT a bare trailing minus. **The Office Space "570-" is an
  application-level render, not a dBASE PICTURE output** (closest native: `( 570)`
  or `570 DB`). Do NOT implement a native trailing-minus.
- Explicit, unambiguous negative formats via FUNCTION codes:
  - `@(`  -> negatives in parentheses: `-570` -> `(570)`
    [verified: transfrm.txt:14 + dbase.com `(` row].
  - `@X`  -> `DB` suffix on negatives: `-570` -> `570 DB`
    [verified: transfrm.txt:13 + dbase.com `X` row].
  - `@C`  -> `CR` suffix on positives: `570` -> `570 CR`
    [verified: transfrm.txt:17 + dbase.com `C` row].

Practical guidance for the reimplementation: model numeric display as
(1) format magnitude into the digit/comma/decimal template, (2) apply the sign
representation chosen by FUNCTION (`@(` parens, `@X` DB), and (3) for the
default/no-function negative case emit a **leading** `-` (matching `STR()` and
both oracles), and fill with `*` on overflow. Sign style is a *numeric display*
feature realized through the PICTURE/FUNCTION subsystem, not a separate command.
(Do not emit a trailing minus by default -- see the caution above; that form is
unverified for III+.)

### 3.3 SET-driven formatting that interacts with templates

- `SET POINT TO` -- decimal-point character (default `.`). The `#` position and
  `.` template char honor it [documented: transfrm.txt:28].
- `SET CURRENCY` / currency symbol -- the `$` function/template fill uses the
  configured currency symbol; `SET CURRENCY LEFT|RIGHT` controls side
  [documented: dbase.com `$` row]. (III+ has `SET CURRENCY` for the symbol; the
  LEFT/RIGHT placement keyword is a later addition -> delta.)
- `SET DATE` / `SET CENTURY` -- date templates and `@D`/`@E` honor these
  [verified: transfrm.txt:61-66 + dbase.com `D`/`E` rows].
- `SET DECIMALS` -- affects how many fractional digits an unmastered numeric
  shows [documented: transfrm.txt:33-38].

---

## 4. RANGE clause (numeric and date GET bounds)

Syntax (clause of `@...GET`):

```
@ <row>,<col> [SAY ...] GET <var> [PICTURE <expC>] RANGE [<low>] , [<high>]
```

Semantics:

- Constrains the value entered into `<var>` to lie within `<low>..<high>`
  inclusive. Applies to **numeric and date** GETs only (the bounds must match
  the variable type). The mined error
  `"Range value must be numeric.  Press any key to continue."` confirms the
  designer enforces numeric bounds [verified: DBASE.MSG.strings.txt:432].
- On a failed READ the interpreter rejects the entry, redisplays the allowed
  range using the runtime prompt `"RANGE is "` followed by the low/high bounds,
  and re-edits the field [verified: DBASE.MSG.strings.txt:158 (`RANGE is `);
  designer prompt `"Enter range value."` line 418].
- **One-sided ranges are legal**: either bound may be omitted. The real samples
  use an *open-ended upper bound* by writing the comma with nothing after it:
  `GET mcheck PICTURE "9999" RANGE 0,`  -- "at least 0, no upper limit"
  [verified: goldens/.../EDITVOID.PRG:30 + CANCL.PRG:44]. Symmetrically
  `RANGE ,100` would mean "at most 100." (A lone `RANGE 0,` with nothing after
  the comma is idiomatic and valid in III+.)
- RANGE only fires when the user actually edits and the value is out of bounds;
  an unchanged in-range default passes. RANGE is enforced at `READ` time, per
  GET, not at `@...GET` definition time.

[verified: DBASE.MSG.strings.txt:158,418,432 + goldens samples EDITVOID.PRG,
CANCL.PRG]

---

## 5. TRANSFORM()

```
TRANSFORM(<exp>, <expC>)  ->  Character string
```

Returns `<exp>` formatted according to the PICTURE/FUNCTION string `<expC>`,
using exactly the same template grammar as `@...SAY`. It lets you obtain the
formatted text for use anywhere a string is expected (`?`, report/label fields,
`STR()`-free money formatting, building screen strings). `<expC>` may begin with
an `@`-function run followed by a per-position template, e.g.
`TRANSFORM(amt, "@( 999,999.99")`.

Type rules:

- `<exp>` may be numeric, character, date, or logical; the template must be
  appropriate to the type (numeric templates for numbers, etc.). Mismatch yields
  a data-type error (see 6).
- The result is **always character**.

[documented: oracles/harbour/doc/transfrm.txt (the function is the doc's whole
subject) + dbase.com TRANSFORM phone example (`"(800) 867-5309"`)]

III+ existence caveat: `TRANSFORM` is **not** in the 406-item HELP index
(`HELP_topics.txt`). The Xbase function is well attested from Clipper onward, and
dBASE documents it, but whether III+ 1.1 ships `TRANSFORM()` as a built-in
function needs confirmation against the real interpreter -> **[oracle-resolves]**
(GAPS). If III+ lacks `TRANSFORM()`, the same formatting is reachable only via
`@...SAY ... PICTURE` to a window/`SET DEVICE TO PRINT`, which the samples favor.

### 5.1 Clipper/Harbour deltas in TRANSFORM (do NOT import blindly)

The Harbour `transfrm.txt` oracle lists several behaviors that are **Clipper/
Harbour extensions**, not III+:

- `@0` / `@L` "zero-pad the number into a string" -- **Clipper addition**, flagged
  "New addition" in the oracle [delta: transfrm.txt:21-23].
- `@)` "quotes around negative numbers without leading spaces" -- Clipper
  [delta: transfrm.txt:15].
- `@R` "Show remainder" for logicals/strings and `@!` upper for strings as
  *function* forms -- Clipper framing [delta: transfrm.txt:44,55-56].
- `@E` exchanging `.`/`,` "Not in US/UK manual, does work" -- the oracle itself
  flags it as undocumented in the period manual [delta: transfrm.txt:18].
- Harbour vs Clipper disagree on whether `@E` reverses MM/DD regardless of
  `SET DATE` -- explicitly an unresolved oracle question
  [delta/open: transfrm.txt:70-77].

---

## 6. The C + N hazard and template/type interaction (III+ CRITICAL)

This subsystem is where the biggest III+-vs-modern trap lives. Templates do NOT
coerce types, and III+ does NOT auto-stringify across `+`:

- In III+ (and Clipper), `"Total: " + amount` where `amount` is **numeric** is a
  **Data type mismatch** error, NOT auto-stringification
  [verified: archive/golden-mined/DBASE.MSG.strings.txt:9 ("Data type mismatch.")
  + archive/dbase-com-help/OPS_PLUS.md is the *modern* (IV+/dBASE PLUS) behavior
  that auto-converts -- WRONG for III+].
- The dbase.com `+` doc explicitly says "concatenate any other data type to a
  string ... converted into its display representation ... Numbers become
  strings with no leading spaces" (OPS_PLUS.md:31-33). **This is dBASE PLUS /
  IV+ behavior and must be flagged wrong for III+.**
- Correct III+ idiom: convert explicitly. The samples never rely on `C + N`; they
  call `STR()` / `LTRIM(STR(...))`:
  `@ 19,9 SAY "Number of seats available: " + STR(Aseat_avl,3)`
  [verified: goldens/.../SHOW_FLT.PRG:94] and
  `"There are " + LTRIM(STR(lastwth,4,0)) + ...`
  [verified: goldens/.../CLRCASH.PRG:152].
- Consequence for templates: to put a *formatted number* into a concatenated
  string in III+ you must turn it into a string first -- via `STR()` (and a
  PICTURE for the `@...SAY` form) or, if III+ has it, `TRANSFORM()`. You cannot
  hand a numeric to a character concatenation and expect a string.

Other relevant errors the template engine raises:

- `"Illegal value."` (DBASE.MSG.strings.txt:44) -- bad GET value vs template.
- `"Not a Character expression."` (line 43) -- PICTURE/FUNCTION argument must be
  character.
- `"Range value must be numeric."` (line 432) -- RANGE bound type check.
- `"<value> out of range"` family for numeric overflow against the template
  width (`"Numeric overflow."` line 162) -> a too-wide number for the template
  shows asterisks / overflow indication -> **[oracle-resolves]** for exact III+
  overflow rendering.

---

## 7. SAY vs GET semantics summary

- **SAY** (display): the template is an *output* mask. Numerics are
  right-justified within the template width (unless `@B`); leading positions are
  spaces, or `$`/`*` if those fills are used; signs/`CR`/`DB`/parentheses per
  FUNCTION. Character `!`/`@!` uppercases. No input occurs; literals are emitted
  verbatim.
- **GET** (input): the template is an *input mask* for the subsequent `READ`.
  Each per-position template char restricts what the user may type there
  (`9`->digit, `A`->letter, `!`->forced uppercase, etc.). Literal positions are
  skipped by the cursor but rendered. RANGE bounds (numeric/date) are checked at
  READ time. With `SET CONFIRM`/`SET BELL` and the GET delimiter (`SET
  DELIMITERS`) the field framing changes but the template grammar is identical.
  On completion the (possibly literal-bearing) value is stored back into `<var>`
  (see 2.3 for the literal-storage III+ rule and the `R`/insert delta).

---

## 8. III+ vs dBASE IV / dBASE PLUS / Clipper deltas (consolidated)

| Feature | III+ 1.1 | Later (IV / PLUS / Clipper) |
|---------|----------|------------------------------|
| `C + N` (char + numeric) | **type-mismatch error** [verified] | auto-stringifies (dbase.com OPS_PLUS) |
| `@R` insert / store-only-core | exists; exact store rules uncertain [oracle-resolves] | well-defined: store digits, show literals |
| `@I` center, `@J` right-align, `@T` trim | **not in III+** [delta] | dbase.com function table (IV+) |
| `@^` exponential, `@M <list>` multiple-choice | **not in III+** [delta] | dbase.com / IV+ |
| `@S<n>` horizontal scroll | likely not III+ [oracle-resolves] | dbase.com (IV+) |
| `@0`/`@L` zero-pad/leading-zeros, `@)` no-space parens | not III+ [delta] | `@L` also in dbase.com IV+ ("leading zeros"); `@0`/`@)` Clipper [transfrm.txt:21-23,15] |
| `TRANSFORM()` builtin | **unconfirmed for III+** [oracle-resolves] | present IV+ / Clipper |
| `VALID <cond>` clause on GET | **not III+** (IV+ feature) [delta] | dBASE IV+ |
| `SET CURRENCY LEFT/RIGHT`, multi-char currency | symbol only in III+; placement keyword later [delta] | IV+ |
| `==` exact operator, `DECLARE` arrays | **not III+** (out of scope, noted per Laws) | IV+ |

---

## Open questions ([oracle-resolves] -- need DBASE.EXE/DBASE.OVL under DOSBox)

1. **Does III+ 1.1 ship `TRANSFORM()` as a built-in?** Not in the 406-topic HELP
   index. Confirm by `? TRANSFORM(1234.5,"9,999.99")` in the real interpreter.
   -> GAPS.md.
2. **Does III+ ever use a trailing minus at all?** Both oracles
   (Harbour transfrm.c, dbase.com) show a **leading** sign and asterisk-overflow,
   with NO trailing-minus form. The "570-" trailing-minus is currently
   *unverified for III+* and may be an imported COBOL/accounting convention.
   Confirm with `? STR(-570,4)`, `? TRANSFORM(-570,"999")`, and
   `@ r,c SAY -570 PICTURE "999"`; only document trailing-minus if DBASE.EXE
   actually emits it.
3. **`@R` GET store semantics in III+**: are PICTURE literals stored back into
   the variable (the `99/99/85` idiom suggests yes for plain templates) or
   stripped under `@R`? Confirm by GETting through `@R (999) 999-9999` into a
   10-char field and checking the stored bytes.
4. **Numeric overflow rendering**: what does a too-wide value show against a
   narrow template -- asterisks, truncation, or `"Numeric overflow."` error?
5. **Which FUNCTION letters does III+ 1.1 actually honor?** Confirm `@B @C @X @(
   @Z @E @D @! @A @R` are recognized and reject/ignore `@I @J @T @^ @M @S`.
6. **`@E` date behavior**: does III+ `@E` reverse MM/DD regardless of `SET DATE`,
   or just select European format? (Harbour/Clipper disagree -- transfrm.txt
   :70-77.)
7. **`SET POINT`/`SET CURRENCY` interaction** with `$`/`#`/`.` templates: confirm
   the fill character and decimal char track these SETs in III+ 1.1.

---

## Sources

- `archive/golden-mined/HELP_topics.txt` -- 406-item completeness oracle;
  topics `@...GET` (7), `@...SAY` (8), `PICTURE` (11), and `@`-mirrors
  (179, 180, 234). No `TRANSFORM`/`RANGE` topic (clause/function respectively).
- `archive/golden-mined/DBASE.MSG.strings.txt` -- runtime/designer text:
  `"RANGE is "` (158), `"Enter or update picture value."` (416),
  `"Enter range value."` (418),
  `"Enter one or more function symbols without using quotes."` (427),
  `"Range value must be numeric."` (432), `"Data type mismatch."` (9),
  `"Illegal value."` (44), `"Not a Character expression."` (43),
  `"Numeric overflow."` (162).
- `archive/golden-mined/ASSIST.HLP.strings.txt` -- CREATE SCREEN / SET FORMAT
  context (the `.fmt` designer that emits PICTURE/FUNCTION).
- `archive/dbase-com-help/OPS_PLUS.md` -- modern `+` operator (auto-stringify);
  cited as the **wrong-for-III+** baseline for the `C + N` hazard.
- `oracles/harbour/doc/transfrm.txt` + `oracles/xharbour/doc/transfrm.txt` --
  semantic oracle for numeric/logical/string/date template chars and `@`-function
  codes (`@X @( @) @B @C @E @Z`, `@0/@L`, `@R`, `@!`), with Clipper-delta flags.
- dbase.com help (WebFetch): function-code table
  `https://www.dbase.com/help/2019_0/Form_Objects/IDH_FORM_function.htm`
  (`( ! # ^ $ A B C D E I J L R S<n> T X Z`); picture/`@R` phone example
  `https://www.dbase.com/help/2_62/Form_Objects/IDH_FORM_TEMPLATE_EX.htm`
  (`@R (999) 999-9999` -> stores `8008675309`, shows `(800) 867-5309`).
  Modern (IV+/PLUS); IV-era codes flagged as deltas.
- Real III+ idioms (`goldens/dbase-iii-plus-1.1-pristine/files/
  Sample_Programs_and_Utilities/`):
  `SHOW_FLT.PRG:39,94` (`PICTURE '999'`, `+ STR()`),
  `CHECK.PRG:41,112` (`PICTURE "9999"`, `"999,999.99"`),
  `EDITVOID.PRG:30,130` + `CANCL.PRG:44` (`RANGE 0,` open upper bound; money mask),
  `FLT_RSVE.PRG:44,46,47` (`'!!!'`, `'99/99/85'` literal-embed, 25x `!`),
  `REINIT.PRG:15` + `YEAREND.PRG:18` (`PICTURE "@!"`),
  `CLRCASH.PRG:152` (`LTRIM(STR(...))`), `ADD.PRG`/`RECONCIL.PRG` (`"9999999.99"`).

---

## Verification

Adversarial re-derivation performed 2026-06-16 against a second independent
source set: the Clipper-lineage implementation oracle
`oracles/harbour/src/rtl/transfrm.c` (the actual TRANSFORM/picture engine), the
Harbour/xHarbour `transfrm.txt` docs, the live dbase.com function & template
help pages (WebFetch), the mined `HELP_topics.txt` / `DBASE.MSG.strings.txt`, and
the 30 III+ sample `.prg` files.

What was cross-checked and CONFIRMED:

- **`@C` / `@X` / `@(` semantics** -- `@C` emits ` CR` only when value > 0,
  `@X` emits ` DB` only when value < 0, `@(` parenthesizes negatives without a
  minus. Verified against transfrm.c lines 451, 621-633, 616-618 and dbase.com
  ("Displays CR after a positive number" / "DB after a negative number" /
  "Encloses negative numbers in parentheses without the minus sign"). The doc's
  table (3.1) and worked forms (3.2) are correct and internally consistent.
- **PICTURE per-position chars** (`9 # . , $ * A N X ! L Y`) -- match
  transfrm.txt:26-31/48-50/59-61 and transfrm.c case tables (lines 224-285,
  522-559). Comma-suppression and `$`/`*` leading-fill confirmed in transfrm.c
  522-557.
- **`@`-function set vs deltas** -- dbase.com confirms `( ! # ^ $ A B C D E I J L
  R S<n> T V<n> X Z`. The doc's III+ set (`! A B C X ( D E R S Z`) and its
  IV+-only deltas (`@I @J @T @^ @M`, `V<n>` wrap) are correctly partitioned.
- **HELP/MSG line citations** -- all spot-checked citations resolve exactly:
  HELP topics 7/8/11/179/180/234; DBASE.MSG 9/43/44/158/162/416/418/427/432.
  TRANSFORM and RANGE are confirmed ABSENT from the 406-item index (the doc's
  existence caveat for `TRANSFORM()` is well-founded).
- **Sample `.prg` citations** -- REINIT:15, YEAREND:18, FLT_RSVE:44/46/47,
  CHECK:41/112/137, EDITVOID:30/130, CANCL:44, SHOW_FLT:39/94, CLRCASH:152 all
  match verbatim. `RANGE 0,` open-upper-bound idiom confirmed in EDITVOID.PRG:30
  and CANCL.PRG:44.
- **`C + N` hazard** -- the only `+` concatenations in the samples join a string
  literal with a *character* var (`option`), never string+numeric; numeric
  interpolation always goes through `STR()`/`LTRIM(STR())`. The III+
  type-mismatch claim (vs the wrong-for-III+ OPS_PLUS auto-stringify) holds.

What was FIXED (corrections applied):

1. **Trailing-minus "570-" overclaim (3.2, 3.2-guidance, Open-Q #2).** The doc
   asserted a default trailing-minus as "the classic dBASE/Xbase numeric
   display." NEITHER independent oracle supports this: transfrm.c uses a
   **leading** sign and `*`-overflow with no trailing-minus path, and dbase.com
   documents no trailing-minus form. Rewrote 3.2 to flag the form as
   **unverified / possibly an imported COBOL-accounting convention**, corrected
   the reimplementation guidance to emit a leading minus by default, and
   rewrote Open-Q #2 accordingly. This was the chief III+-correctness risk.
2. **`@0`/`@L` delta nuance (8).** The doc called `@L` Clipper-only; dbase.com
   also documents `L` ("leading zeros") for IV+. Re-scoped the delta row so only
   `@0`/`@)` are Clipper-specific.
3. **Sample count.** "29 sample `.prg` files" -> 30 (verified `ls`), and added
   the grep-verified "0 standalone FUNCTION keyword" attestation.
4. **Stray unmatched ``` code fence** at end of Sources removed (formatting).

Completeness vs the HELP index: all three in-scope oracle items (`@...GET`,
`@...SAY`, `PICTURE`, plus their `@`-mirror entries) are covered. The PICTURE
template-char table and `@`-function set are exhaustive against both the mined
text and the implementation oracle.

Residual [oracle-resolves] gaps (require real DBASE.EXE/DBASE.OVL under DOSBox;
all GAPS-worthy, none blocking the spec's correctness):

- Whether III+ 1.1 ships `TRANSFORM()` as a built-in at all (absent from HELP
  index; well-attested only from Clipper/IV onward).
- Whether III+ ever emits a trailing minus (current evidence: no -- leading sign
  only). DOWNGRADED from asserted to open by this review.
- `@R` GET store-vs-keep semantics for literal placeholders in III+.
- Numeric-overflow rendering (asterisks vs `"Numeric overflow."` error).
- Exact set of FUNCTION letters III+ 1.1 honors vs ignores/rejects.
- `@E` date MM/DD-reversal-regardless-of-SET-DATE question (Harbour vs Clipper
  disagree; transfrm.txt:70-77).
- `SET POINT` / `SET CURRENCY` interaction with `$`/`#`/`.` templates in III+.

Confidence: HIGH on the cross-Xbase-stable core (template chars, `@`-function
effects, RANGE, C+N hazard); MEDIUM on the III+-specific items left as
[oracle-resolves]. The one prior semantic defect (default trailing-minus) is
corrected.
