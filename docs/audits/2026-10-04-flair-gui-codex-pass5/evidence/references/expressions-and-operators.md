# Expressions and Operators (dBASE III PLUS 1.1)

## Status / confidence

Target: **dBASE III PLUS 1.1 (1986)**. This document specifies the expression
grammar and the complete operator set: arithmetic, relational, logical, and
string operators, with precedence, parentheses, result typing, and the
III+-specific behaviors that diverge from later dBASE (IV / 5 / dBASE PLUS).

The **period-exact authority** for *which operators exist and their syntax* is
the mined III+ HELP text
(`archive/golden-mined/HELP.DBS.strings.txt`, topics `OPERATOR`,
`MATHEMATICAL OP`, `RELATIONAL OP`, `LOGICAL OP`, `STRING OP`, `EXPRESSION`,
`EXP 2`, `CONDITION`) and the III+ error catalog
(`archive/golden-mined/DBASE.MSG.strings.txt`). The `dbase.com` OPS pages
(`archive/dbase-com-help/OPS_*.md`, `SET_EXACT.md`) document a *much later*
product (dBASE PLUS) and are used **only** for the III+-stable core; every
later-era addition (the `==` operator, automatic C+N stringification, the `%`
modulo operator, `null`, object/Date/Function operands) is flagged as a delta
and is **NOT** part of III+.

The single biggest hazard, repeated throughout: in III+, **`C + N` (mixing a
character expression and a numeric expression with an operator) is a `Data type
mismatch` error**, NOT auto-stringification. The modern `+`/comparison doc that
says "the other operand is converted to its display representation" is **wrong
for III+** and must not be imported.

Confidence tags: `[verified: A + B]` two independent sources; `[documented: src]`
one source; `[oracle-resolves]` needs the real interpreter under DOSBox to
settle; `[inferred]` reasoned but unconfirmed.

Completeness against the 406-item HELP oracle
(`archive/golden-mined/HELP_topics.txt`)  --  this document fully covers the
in-scope items:

| # | Topic | Covered in |
|---|-------|------------|
| 151 | EXPRESSION | [Expressions](#1-expressions) |
| 152 | EXP 2 | [Expressions](#1-expressions) |
| 153 | CONDITION | [Conditions (FOR/WHILE)](#7-conditions-forwhile-clauses) |
| 155 | KEY FIELD | [Index/key expressions](#8-index-and-key-expressions) |
| 156 | OPERATOR | [Operator taxonomy](#2-operator-taxonomy-the-four-classes) |
| 157 | MATHEMATICAL OP | [Arithmetic operators](#3-arithmetic-mathematical-operators) |
| 158 | RELATIONAL OP | [Relational operators](#4-relational-comparison-operators) |
| 159 | LOGICAL OP | [Logical operators](#5-logical-operators) |
| 160 | STRING OP | [String operators](#6-string-operators-concatenation) |

Out of this document's scope (covered by sibling specs, cross-linked where
relevant): the function catalog (`@FUNCTIONS` family, e.g. `MOD()`, `SUBSTR()`,
`STR()`, `VAL()`, `CTOD()`, `DTOC()`), data types and literals
(`MEMORY VARIABLE`, `FIELD TYPE`), and `SET EXACT`/`SET DECIMALS`/`SET FIXED`
command semantics (only the comparison-affecting behavior is summarized here).

---

## 1. Expressions

> "An expression consists of one or more fields, memory variables, constants,
> functions, and operators. The expression type is the type of value that it
> returns. Possible types are date, character, numeric, and logical."
>  --  `HELP.DBS.strings.txt:1727-1732` (topic `@EXPRESSION`)
> `[documented: mined HELP III+]`

An expression is a syntactic combination of **operands** joined by
**operators** and grouped by **parentheses**, that evaluates to a single value
of one of the four III+ data types:

- **Character (C)**  --  a string (`'CA'`, `"Glendale"`, a character field/memvar,
  or a function returning C such as `SUBSTR()`/`TRIM()`/`STR()`).
- **Numeric (N)**  --  a number (`123.34`, `0`, a numeric field/memvar, or
  `VAL()`/`LEN()`/`RECNO()`/`MOD()` ...).
- **Date (D)**  --  produced only by date fields, date memvars, `DATE()`,
  `CTOD()`; there are **no date literals** in III+ (you build a date with
  `CTOD('01/01/85')`, `HELP.DBS.strings.txt:1832`).
- **Logical (L)**  --  `.T.`/`.F.` (the only dotted logical literals in the mined
  surface; the single chars `T`/`Y`, `F`/`N` are accepted as logical-field
  *input*, see 5.2), a logical field/memvar, the result of any
  relational/logical operation, or a logical-returning function
  `EOF()`/`BOF()`/`DELETED()`/`FOUND()`/`FILE()`/`ISCOLOR()`/`ISALPHA()` etc.
  (NB: `ISCANCEL` in the mined HELP is a *logical field name* in a sample DBF,
  `HELP.DBS:1958`, **not** a III+ built-in function.)

### 1.1 The type-agreement rule (the core III+ constraint)

> "Each part of an expression (except the operator) should be of the same type.
> The exception to this rule is the use of dates and numbers with some of the
> operators. For example, the result of the subtraction of two dates is a
> number, not a date. The result of adding a number to a date is a date."
>  --  `HELP.DBS.strings.txt:1733-1737` (topic `@EXPRESSION`)
> `[documented: mined HELP III+]`

This is the load-bearing rule. **Operands of an operator must be the same type**,
with the *only* sanctioned cross-type combinations being the date+number /
date-date arithmetic forms (see 3.4). There is **no implicit numeric->character
or character->numeric coercion in III+**. Any disallowed mix raises a runtime
error:

- `Data type mismatch.`  --  `DBASE.MSG.strings.txt:9` `[documented: mined error catalog]`

Concrete consequences (all III+-specific, all contradict the modern OPS pages):

| Expression | III+ result |
|------------|-------------|
| `'A' + 1` | `Data type mismatch.` (NOT `"A1"`) |
| `1 + 'A'` | `Data type mismatch.` |
| `"Total: " + 5` | `Data type mismatch.` (you must write `"Total: " + STR(5,...)`) |
| `5 = 'x'` | `Data type mismatch.` |
| `Amt + Name` | `Data type mismatch.` (numeric field + char field) |

To join a number into text you must explicitly stringify with `STR()` (and to
parse text to a number, `VAL()`). The III+ samples do exactly this  --  see
the idiom in 6.3.

`[verified: mined HELP type rule (HELP.DBS:1733) + mined error "Data type mismatch." (DBASE.MSG:9)]`

> **Delta vs dBASE PLUS / IV.** The modern `dbase.com` `+`/comparison pages
> (`OPS_PLUS.md:31-43`, `OPS_COMPARISON.md:7-37`) say the non-string operand is
> "converted into its display representation" / "converted to a number". That
> auto-stringification / auto-numification was added **after III+**. Do **not**
> implement it for III+. `[verified: dbase.com OPS_PLUS.md:31 (modern) contradicted by mined HELP III+ type rule]`

### 1.2 Examples from the HELP surface

```
City                       && a single character field -> C
Address + City             && C + C concatenation -> C
Price * (1 + Cost)         && N arithmetic with parentheses -> N
```
 --  `HELP.DBS.strings.txt:1741-1743` (topic `@EXP 2`)

### 1.3 Expression list `<exp list>`

> "An expression list (`<exp list>`) is one or more expressions separated by
> commas: `City, Price * (1 + Cost), Price`."
>  --  `HELP.DBS.strings.txt:1747-1748` `[documented: mined HELP III+]`

Used by `?`/`??`, `LIST`/`DISPLAY <expression list>`, `SUM`/`AVERAGE
[<expression list>]`, `REPLACE field WITH <exp>, field2 WITH <exp2>`, etc.

### 1.4 Typed-expression notations used in command syntax

The HELP system tags an operand by the type a command requires
(`HELP.DBS.strings.txt:1744-1746`):

- `<expN>`  --  numeric expression
- `<expC>`  --  character expression
- `<expD>`  --  date expression
- `<condition>` / `<expL>`  --  logical expression

### 1.5 Line length limit

A single command/expression line is limited:

- `Line exceeds maximum of 254 characters.`  --  `DBASE.MSG.strings.txt:18`
  `[documented: mined error catalog]`

So the maximum source-line length (after `;` continuation is resolved) is **254
characters**. (Continuation in `.prg` source uses a trailing `;`  --  see the
sample idioms; this is a lexical, not operator, feature.)

---

## 2. Operator taxonomy: the four classes

> "Operators define the operation performed on two items. There are four types
> of operators:
> 1 - Mathematical
> 2 - Relational
> 3 - Logical
> 4 - String"
>  --  `HELP.DBS.strings.txt:1776-1783` (topic `@OPERATOR`)
> `[documented: mined HELP III+]`

This taxonomy is the III+ canon. Note that III+ groups `+`/`-` twice: as
**Mathematical** operators (numeric/date) and as **String** operators
(concatenation). The actual token dispatched on depends entirely on operand
type (see 3 and 6). There is **no separate "membership"/`$` class** in the HELP
taxonomy  --  `$` is listed under **Relational** operators.

Invalid/unknown operator tokens raise:

- `Invalid operator.`  --  `DBASE.MSG.strings.txt:104` `[documented: mined error catalog]`
- `Syntax error.`  --  `DBASE.MSG.strings.txt:10`
- `Unbalanced parenthesis.`  --  `DBASE.MSG.strings.txt:8`

---

## 3. Arithmetic (mathematical) operators

> "Mathematical operators generate numeric results:
> `+`  Addition
> `-`  Subtraction
> `*`  Multiplication
> `/`  Division
> `**` or `^`  Exponentiation"
>  --  `HELP.DBS.strings.txt:1784-1791` (topic `@MATHEMATICAL OP`)
> `[documented: mined HELP III+]`

The **complete** III+ arithmetic operator set, with operand and result types:

| Token | Operation | Operands | Result | Notes |
|-------|-----------|----------|--------|-------|
| `+`   | addition (binary) | N + N | N | also unary `+` (no-op) and date arithmetic (3.4) |
| `-`   | subtraction (binary) | N - N | N | also unary minus (negation) and date arithmetic (3.4) |
| `*`   | multiplication | N * N | N | |
| `/`   | division | N / N | N | division-by-zero -> see 3.3 |
| `**`  | exponentiation | N ** N | N | identical to `^` |
| `^`   | exponentiation | N ^ N | N | identical to `**` |
| (unary) `-` | negation | -N | N | sign change |
| (unary) `+` | identity | +N | N | no sign change; rarely used |

`[verified: mined HELP MATHEMATICAL OP (HELP.DBS:1784) + Harbour const-fold rule "<nConst1> ^ <nConst2> => <nConst>" (cmpopt.txt:144)]`

### 3.1 III+ has NO `%` (modulo) operator

The III+ `MATHEMATICAL OP` HELP page lists **exactly five** arithmetic
operators (`+ - * / **`/`^`). There is **no `%` modulo operator** token. The
remainder operation is available **only as the `MOD()` function**:

> `MOD(<expN1>, <expN2>)  The remainder of <expN1> divided by <expN2>.`
>  --  `HELP.DBS.strings.txt:1306` (topic `@NUM FUNC 2`)

```
nRemainder = MOD(17, 5)     && -> 2.   III+ has NO  17 % 5  operator form.
```

`[verified: mined HELP MATHEMATICAL OP lists no % (HELP.DBS:1784-1791) + MOD() is the documented remainder mechanism (HELP.DBS:1306)]`

> **Delta vs dBASE IV+.** The `%` modulo operator is a later (dBASE IV /
> dBASE PLUS) addition. The modern `dbase.com` numeric-operators page documents
> `%`; do **not** add it to III+. In III+, the token `%` outside a `PICTURE`
> string is not an arithmetic operator. `[inferred from mined HELP omission; oracle-resolves whether '%' parses as Invalid operator vs Syntax error]`

### 3.2 `**` vs `^`

`**` and `^` are exact synonyms for exponentiation. Both appear in the same
HELP line (`HELP.DBS.strings.txt:1791`). **Chained `^` is LEFT-associative:**
`2 ^ 3 ^ 2` = `(2^3)^2` = **64** (not `2^(3^2)` = 512). And **unary minus binds
TIGHTER than `^`**: `-2 ^ 2` = `(-2)^2` = **4** (not `-(2^2)` = -4).
`[verified: minted 2026-06-16, re/mint-results-002.md -- ? STR(2^3^2,..)=64, STR(-2^2,..)=4]`

### 3.3 Numeric edge cases (errors)

- Negative base with a fractional exponent:
  `^ or ** : Negative base, fractional exponent.`  --  `DBASE.MSG.strings.txt:75`
- Overflow: `Numeric overflow (data was lost).`  --  `DBASE.MSG.strings.txt:38`
- Square root / log domain errors are at the function level
  (`SQRT() : Negative.` `DBASE.MSG.strings.txt:59`; `LOG() : Zero or negative.`
  `:56`), not operator level.
- **Division by zero (`1/0`) does NOT raise an error**  --  it produces a numeric
  **overflow value rendered as `*`-fill** (e.g. `?? 1/0` -> `*****`), the same
  treatment as numeric overflow (not a trapped condition).
  `[verified: minted 2026-06-16, re/mint-results-002.md]`
- **All cross-type operator mismatches raise the SAME error: #9
  `Data type mismatch.`** (`"A"+1`, `1+"A"`, `D+D`, `"abc"-5`, `DATE()<"x"`,
  `"x"$5` all -> #9). `[verified: minted C1.TXT 2026-06-16]`

`[documented: mined error catalog; div-by-zero + mismatch token minted-verified]`

### 3.4 Date arithmetic (the sanctioned cross-type exception)

The only operators that legally mix types are `+` and `-` with dates:

| Form | Result type | Semantics |
|------|-------------|-----------|
| `D + N` | D | the date N days later (N negative = earlier) |
| `N + D` | D | same (commutative) |
| `D - N` | D | the date N days earlier |
| `D - D` | N | number of days between the two dates |

 --  `HELP.DBS.strings.txt:1734-1737` ("subtraction of two dates is a number ...
adding a number to a date is a date") and the `dbase.com` minus page
(`OPS_MINUS.md:25`: "subtract one date from another date; the result is the
number of days between the two dates").

`[verified: mined HELP III+ (HELP.DBS:1734) + dbase.com OPS_MINUS.md:25]`

`D + D` (date plus date) and `N - D` (number minus date) are **not** sanctioned
-> `Data type mismatch.` `[inferred from the type-agreement rule; oracle-resolves the exact error token]`

> Blank-date handling ("adding any number to a blank date gives a blank date";
> "subtracting a blank date gives zero") is documented in the *modern*
> `OPS_PLUS.md:27` / `OPS_MINUS.md:25` and is plausibly III+-stable but is
> **not** in the mined III+ surface. `[documented: dbase.com (modern); oracle-resolves for III+]`

---

## 4. Relational (comparison) operators

> "Relational operators generate logical results (true or false):
> `<`  Less than
> `>`  Greater than
> `=`  Equal
> `<>` or `#`  Not equal
> `<=`  Less than or equal
> `>=`  Greater than or equal
> `$`  Substring comparison
> Substring comparison returns a true if the first string is either identical
> to or contained within the second string."
>  --  `HELP.DBS.strings.txt:1792-1803` (topic `@RELATIONAL OP`)
> `[documented: mined HELP III+]`

The **complete** III+ relational operator set:

| Token | Meaning | Operand types | Result |
|-------|---------|---------------|--------|
| `<`   | less than | C<C, N<N, D<D, (L<L) | L |
| `>`   | greater than | C>C, N>N, D>D, (L>L) | L |
| `=`   | equal (string match governed by SET EXACT) | C=C, N=N, D=D, L=L | L |
| `<>`  | not equal | same as `=` | L |
| `#`   | not equal (synonym of `<>`) | same as `=` | L |
| `<=`  | less than or equal | C/N/D | L |
| `>=`  | greater than or equal | C/N/D | L |
| `$`   | substring (left contained in / equal to right) | C $ C | L |

### 4.1 The result of every comparison is Logical

A relational expression *always* yields type **L** (`.T.`/`.F.`). This is why a
comparison can be used directly as a `<condition>` (see 7) and why comparison
expressions are the building blocks of `IF`/`DO WHILE`/`FOR`/`WHILE`. Comparing
two logicals is legal but pointless ("redundant; use logical operators
instead", `OPS_COMPARISON.md:5`).

`[verified: mined HELP "Relational operators generate logical results" (HELP.DBS:1794) + dbase.com OPS_COMPARISON.md:5]`

### 4.2 `#` and `<>` are the same operator; there is **NO `==`** in III+

`#` is a synonym for `<>` (both "not equal"). The III+ samples use both forms
interchangeably:

```
DO WHILE lockcount # 0     && FLT_CNCL.PRG:118  -> "#" not-equal
IF counter <> VAL(choice2) && MAINT.PRG:37      -> "<>" not-equal
```
 --  `goldens/.../Sample_Programs_and_Utilities/FLT_CNCL.PRG:118` (also
`SHOW_FLT.PRG:64`), `MAINT.PRG:37` (also `CASH.PRG:46`)
`[verified: real III+ sample, sed-confirmed]`

> **Delta vs dBASE IV+ / dBASE PLUS.** The `==` ("exactly equal") operator does
> **NOT** exist in III+. It is absent from the `RELATIONAL OP` HELP page (which
> lists exactly `< > = <> # <= >= $`). `==` was introduced in dBASE IV. In
> III+, the only equality operator is `=`, and its string behavior is governed
> by `SET EXACT` (4.4). The `dbase.com OPS_COMPARISON.md:44` table that lists
> `==` is documenting the modern product. `[verified: mined HELP RELATIONAL OP has no '==' (HELP.DBS:1792-1803) + dbase.com OPS_COMPARISON.md:44 lists '==' (modern delta)]`

### 4.3 Operand types and ordering

- **Numeric**: ordinary numeric comparison.
- **Date**: chronological comparison (earlier date < later date).
- **Character**: byte-by-byte comparison using the machine collating sequence
  (effectively ASCII / the active code page; III+ does not have language-driver
  secondary weights  --  that is a dBASE PLUS feature, `SET_EXACT.md:25`). String
  equality (`=`) is **case-sensitive** and is governed by `SET EXACT` (4.4).
- Mixed types (e.g. `C < N`, `D = C`) -> `Data type mismatch.`
  (`DBASE.MSG.strings.txt:9`). There is **no** auto-conversion in III+
  (contrast `OPS_COMPARISON.md:7-37`, modern). `[verified: mined type rule + mined error]`

The `?`/`??` and command parser also surface, for malformed comparison
operands: `Not a Logical expression.` (`DBASE.MSG.strings.txt:36`),
`Not a numeric expression.` (`:27`), `Not a Character expression.` (`:43`)  -- 
raised when a clause requires a specific type and the expression yields
another. `[documented: mined error catalog]`

### 4.4 The `=` operator on strings, and SET EXACT (the directionality rule)

For **character** operands, the `=` operator is *directional* and its meaning
depends on `SET EXACT`. This is the most subtle III+ comparison behavior.

> `SET EXACT on/OFF  Requires an exact match for character string equality.`
>  --  `HELP.DBS.strings.txt:1609` (topic list) `[documented: mined HELP III+]`

The default is **`SET EXACT OFF`** (note the capitalization convention in the
mined index: the default value is shown uppercase). Behavior:

- **`SET EXACT OFF` (default):** `left = right` is true iff **`left` begins
  with `right`**  --  i.e. the right-hand string is matched as a prefix of the
  left-hand string, comparing only `LEN(right)` characters. The comparison
  stops at the length of the *right* operand.
  - `"Smith" = "S"`  -> `.T.`  (left begins with "S")
  - `"Smith" = "Sm"` -> `.T.`
  - `"S" = "Smith"`  -> `.F.`  (left "S" does not begin with "Smith")
  - `"Smith" = ""`   -> `.T.`  (everything begins with the empty string)
  - The empty string on the right always matches -> the canonical
    "is-empty" test puts the empty string on the **left**: `"" = x`.

- **`SET EXACT ON`:** the two strings must match **exactly**, except that
  **trailing blanks are ignored** in both operands.
  - `"Smith" = "Smith "` -> `.T.` (trailing blank ignored)
  - `"Smith" = "S"`      -> `.F.`

`[verified: dbase.com SET_EXACT.md:19-23 + OPS_COMPARISON.md:57 (III+-stable core); cross-checked against mined SET EXACT topic HELP.DBS:1609]`

Cross-links:
- `SET EXACT` command full spec: `../commands/set-commands.md` (to be authored).
- The same prefix-match logic governs `SEEK`/`FIND` against an index
  (`SEEK("S")` finds the first key beginning with "S" when EXACT is OFF) and
  `LOCATE FOR field = literal`. See `../commands/navigation-query-display.md`, `../commands/navigation-query-display.md`.

Notes on `<>`/`#` for strings: with EXACT OFF they are the negation of the
"begins with" test ("does not begin with"); with EXACT ON they are
strict-not-equal modulo trailing blanks. `[documented: dbase.com OPS_COMPARISON.md:57]`

The relational operators `<`, `>`, `<=`, `>=` on strings do a full
lexicographic comparison and are **not** affected by `SET EXACT`
(EXACT governs only equality-class operators `=`, `<>`, `#`). `[inferred from
SET EXACT scope wording in SET_EXACT.md:17; oracle-resolves whether < / > on
strings of unequal length pad or stop short]`

> **Delta / hazard:** the modern docs note a `CHR(0)`-on-the-right
> compatibility quirk and the `==` "always EXACT" operator
> (`OPS_COMPARISON.md:57,63`). The `==` part is **not** III+. The `CHR(0)`
> quirk is plausibly inherited but unverified for III+ -> `[oracle-resolves]`.

### 4.5 `$`  --  substring containment

> "Substring comparison returns a true if the first string is either identical
> to or contained within the second string."  --  `HELP.DBS.strings.txt:1802-1803`

```
result = leftC $ rightC      && .T. iff leftC occurs anywhere within rightC
```

- Operands: both must be **Character**; result is **L**.
- `"mit" $ "Smith"` -> `.T.` (contained)
- `"Smith" $ "Smith"` -> `.T.` (identical counts as contained)
- `"x" $ "Smith"` -> `.F.`
- The search is **not** anchored (unlike `=` with EXACT OFF, which is
  prefix-only); `$` matches at any position. `$` is **case-sensitive** and is
  **not** affected by `SET EXACT`. `[verified: mined HELP $ definition (HELP.DBS:1802) + dbase.com OPS_COMPARISON.md:65]`
- `N $ N` or `C $ N` etc. -> `Data type mismatch.`

Real III+ idiom (the classic single-key menu validation): test a typed
character against a set of allowed characters with `$`:

```
DO WHILE .NOT. UPPER(choice)$'ABCE'
   ...                         && loop until the user types A, B, C, or E
ENDDO
```
 --  `goldens/.../Sample_Programs_and_Utilities/FLIGHT.PRG:40`
`[verified: real III+ sample, sed-confirmed]`

```
DO WHILE .NOT. ans$"YyNn"     && loop until a Yes/No keystroke
```
 --  `CHECK.PRG:135` `[verified: real III+ sample, sed-confirmed]`

> Empty-string behavior of `$` ("an empty string is not contained in another
> string", `OPS_COMPARISON.md:65`) is from the modern doc; III+ behavior of
> `'' $ x` is `[oracle-resolves]`.

---

## 5. Logical operators

> "Logical operators generate a logical result (true or false):
> `.AND.`  Results in true if both expressions are true.
> `.OR.`   Results in true if either expression is true.
> `.NOT.`  Changes the truth value of the expression to the opposite value."
>  --  `HELP.DBS.strings.txt:1804-1809` (topic `@LOGICAL OP`)
> `[documented: mined HELP III+]`

| Token | Arity | Operands | Result | Truth |
|-------|-------|----------|--------|-------|
| `.AND.` | binary | L .AND. L | L | `.T.` iff both operands `.T.` |
| `.OR.`  | binary | L .OR. L | L | `.T.` iff either operand `.T.` |
| `.NOT.` | unary (prefix) | `.NOT.` L | L | inverts the operand |

### 5.1 The dotted syntax is mandatory

III+ logical operators are written **with leading and trailing dots**:
`.AND.`, `.OR.`, `.NOT.`. The bare words `AND`/`OR`/`NOT` are **not** logical
operators (they would be parsed as identifiers / cause a syntax error). The
dots are part of the token. `[verified: mined HELP (HELP.DBS:1807-1809) + every III+ sample uses the dotted form, e.g. ADD.PRG:114 (`.NOT. ... .AND. .NOT.`), YEAREND.PRG:28 (`.NOT. ... .AND.`), sed-confirmed]`

### 5.2 Logical literals `.T.` / `.F.` (and the `T`/`Y`, `F`/`N` input chars)

The logical constants are dotted: `.T.` (true) and `.F.` (false). These are the
only logical literal tokens the mined HELP surface documents. The mined HELP
`FIELD TYPE` text adds that, **for logical-field data entry**, the *single
characters* `T`/`Y` represent true and `F`/`N` represent false
(`HELP.DBS.strings.txt:1923-1924`: "T and Y represent true, and F and N
represent false")  --  i.e. this is about which keystrokes a logical field
*accepts as input*, **not** about extra dotted literal tokens. For source-code
logical literals, write `.T.`/`.F.`.

```
STORE .T. TO pass            && HELP.DBS:1831 -> stores .T. to logical memvar
```
`[verified: mined HELP MEMORY VARIABLE example (HELP.DBS:1831) + FIELD TYPE input-char note (HELP.DBS:1923)]`

> **Caveat.** Xbase products (dBASE/Clipper) also accept `.Y.`/`.N.` as dotted
> logical-literal synonyms for `.T.`/`.F.`, but **`.Y.`/`.N.` do not appear
> anywhere in the mined III+ HELP surface** (verified: zero hits for `.Y.`/`.N.`
> in `HELP.DBS.strings.txt`). Whether III+ actually parses `.Y.`/`.N.` as
> literals is unconfirmed from local sources. `[oracle-resolves]`

### 5.3 Operands must be logical

`.AND.`/`.OR.`/`.NOT.` require Logical operands. A non-logical operand raises:

- `Not a Logical expression.`  --  `DBASE.MSG.strings.txt:36` `[documented: mined error catalog]`

There is **no** truthiness coercion of N or C to L (e.g. `5 .AND. .T.` is an
error, not "true"). `[inferred from the type-agreement rule + the existence of
the dedicated error; oracle-resolves the exact trigger]`

### 5.4 Short-circuit evaluation

dBASE III PLUS evaluates `.AND.`/`.OR.` with **left-to-right short-circuit**:
for `a .AND. b`, if `a` is `.F.` then `b` is not evaluated; for `a .OR. b`, if
`a` is `.T.` then `b` is not evaluated. This is the Xbase-lineage behavior. Two
independent Clipper-lineage oracles document it:

1. The xHarbour operator reference annotates the short-circuit cases of `.AND.`
   and `.OR.` explicitly as `(shortcut)` (a **runtime** evaluation property,
   "full clipper compatible"):
   ```
   ? .F. .AND. .T.   // Result: .F.   (shortcut)
   ? .T. .OR.  .T.   // Result: .T.   (shortcut)
   ```
    --  `oracles/xharbour/doc/en/operator.txt:80-81,146-147`
2. The Harbour compiler additionally const-folds these at compile time (the
   `-z`-disableable optimization tier  --  a *compile-time* analogue, evidence but
   not proof of the runtime behavior):
   ```
   .F. .AND. <expr>   => .F.        <expr> .AND. .F.   => .F.
   .T. .OR.  <expr>   => .T.        <expr> .OR.  .T.   => .T.
   .T. .AND. <expr>   => <expr>     .F. .OR.  <expr>   => <expr>
   ```
    --  `oracles/harbour/doc/cmpopt.txt:133-140`
`[verified: xharbour operator.txt:80 runtime "(shortcut)" + harbour cmpopt.txt:133 const-fold; both Clipper-lineage oracles, NOT period-exact III+]`

The III+ samples *rely* on left-to-right ordering, e.g. guarding a function
call by a cheaper test first:

```
DO WHILE .NOT. ans$"YyNn" .AND. .NOT. special   && ADD.PRG:114
```
`[verified: Harbour short-circuit semantics (cmpopt.txt:133-140) + III+ sample reliance (ADD.PRG:114, sed-confirmed); oracle-resolves whether III+ short-circuits identically to Clipper, or always evaluates both operands. NB cmpopt's rules are a compile-time const-fold optimization (disableable via -z), so they evidence but do not prove runtime short-circuit]`

---

## 6. String operators (concatenation)

> "String operators generate character results:
> `+` and `-`  Concatenation operators
> Concatenation is the act of joining two strings. If `-` is used, all trailing
> blanks are moved to the end of the combined string."
>  --  `HELP.DBS.strings.txt:1815-1821` (topic `@STRING OP`)
> `[documented: mined HELP III+]`

Both operands must be **Character**; the result is **Character**.

### 6.1 `+`  --  simple concatenation

`a + b` produces the bytes of `a` immediately followed by the bytes of `b`.
Trailing blanks in `a` are preserved in place (between `a` and `b`).

```
"AB " + "CD"   ->  "AB CD"      (5 chars; the blank stays between)
```

Real idiom (build a label from a string literal and a stringified number  --  note
the explicit `STR()`, because `C + N` is illegal in III+, see 1.1):

```
@  1,60 SAY "LAST CHECK # "+LTRIM(STR(lastchk,4,0))
```
 --  `goldens/.../Sample_Programs_and_Utilities/CHECK.PRG:19`
`[documented: real III+ sample]`

### 6.2 `-`  --  concatenation with trailing-blank relocation

`a - b` concatenates `a` and `b` but **removes the trailing blanks from `a`**,
joins the trimmed `a` to `b`, and **appends the removed blanks to the end** of
the whole result. The result has the **same total length** as `a + b`; only the
position of `a`'s trailing blanks changes (they migrate to the very end).

```
"AB  " - "CD"  ->  "ABCD  "     (same 6-char length as "AB  "+"CD" = "AB  CD",
                                 but the two blanks are moved to the end)
```

This is the period-exact behavior: "all trailing blanks are moved to the end of
the combined string" (`HELP.DBS.strings.txt:1819-1821`), confirmed by the
modern minus page: "the trailing blanks from the first operand are removed
before the concatenation, and placed at the end of the result ... results in a
string with the same length" (`OPS_MINUS.md:29`).

`[verified: mined HELP STRING OP (HELP.DBS:1819) + dbase.com OPS_MINUS.md:29]`

Primary use: building fixed-width keys / display strings where you want the
visible tokens adjacent (no embedded gaps) but the field to keep its fixed
width  --  and especially **building trimmed multi-field index keys** (see 8):

```
INDEX ON Last - First TO names    && key = trimmed Last + First, blanks at end
```
The modern minus page makes this explicit: "If you want to trim field values
when creating an expression index for a DBF table, use the minus operator."
(`OPS_MINUS.md:31`). `[documented: dbase.com OPS_MINUS.md:31; III+-stable]`

### 6.3 Concatenation operands must both be Character

`C + N`, `N + C`, `C - N`, `D + C`, etc. are **`Data type mismatch.`** in III+.
The only way to merge a number into character output is to convert with `STR()`
first (and `DTOC()` for a date). This is why the samples are full of
`...+STR(...)`, `...+LTRIM(STR(...))`, `...+RTRIM(SUBSTR(...))`:

```
camt=RTRIM(camt)+" AND "+cents+"/100 DOLLARS"   && NUMWORDS.PRG:69
```
 --  `goldens/.../Sample_Programs_and_Utilities/NUMWORDS.PRG:69`
`[documented: real III+ sample]`

`[verified: mined type rule (HELP.DBS:1733) + mined error "Data type mismatch." (DBASE.MSG:9); sample idiom NUMWORDS.PRG:69]`

> **Delta vs dBASE PLUS.** The modern `+` page (`OPS_PLUS.md:31-43`) auto-converts
> the non-string operand to its display form, and `-` likewise. That is **NOT**
> III+; do not import it. `[verified: dbase.com OPS_PLUS.md:31 (modern) vs mined III+ type rule]`

### 6.4 Result-length limit

The concatenated result has a maximum length. Exceeding it raises:

- `+ : Concatenated string too large.`  --  `DBASE.MSG.strings.txt:74`
- `- : Concatenated string too large.`  --  `DBASE.MSG.strings.txt:73`

(A III+ character expression result cannot exceed the maximum string length;
the related line limit is 254 chars, `DBASE.MSG.strings.txt:18`.) The exact
maximum concatenation length value is not stated in the mined surface ->
[Open questions]. `[documented: mined error catalog]`

---

## 7. Conditions (FOR/WHILE clauses)

> "A condition is a logical expression that defines the criteria for choosing
> specified records. It is part of the FOR or WHILE clause of a command.
> A condition consists of one or more logical expressions connected by logical
> operators."
>  --  `HELP.DBS.strings.txt:1749-1755` (topic `@CONDITION`)
> `[documented: mined HELP III+]`

A `<condition>` is just a Logical-typed expression (type L). It appears in
`FOR <condition>`, `WHILE <condition>` (on `LIST`, `DISPLAY`, `LOCATE`,
`COUNT`, `SUM`, `AVERAGE`, `DELETE`, `RECALL`, `REPLACE`, `COPY`, `REPORT`,
`LABEL`, `JOIN`, ...) and as the test of `IF`/`DO WHILE`.

HELP examples (`HELP.DBS.strings.txt:1756-1757`, `1812-1814`):

```
DISPLAY FOR Zip > '90000'
LIST WHILE Zip > '90000' .AND. City <> 'Glendale'
DISPLAY FOR City = 'Glendale' .AND. State = 'CA'
DISPLAY FOR State = 'NY' .OR. State = 'MO'
DISPLAY FOR .NOT. (City = 'Glendale')
```

Real III+ conditions from samples (note operator precedence in action  --  see 9):

```
LOCATE FOR .NOT. CLEAR .AND. YEAR(Date) = myear        && YEAREND.PRG:28
IF Amt>0 .AND. Num>mlastdep                            && CLEANUP.PRG:28
IF Payto = "VOID" .AND. Amt = 0.00                     && EDITVOID.PRG:66
IF Date >= begin .AND. Date <= end                     && RPRTPRO.PRG:192
IF balance<>mbalance .OR. lastchk<>mlastchk .OR. ;     && CBMENU.PRG:93 (continued line)
   ...
```
 --  `YEAREND.PRG:28`, `CLEANUP.PRG:28`, `EDITVOID.PRG:66`, `RPRTPRO.PRG:192`,
`CBMENU.PRG:93` `[verified: real III+ samples, sed-confirmed]`

Note the use of `;` to continue a long condition onto the next physical line
(lexical continuation, resolved before the 254-char limit applies).

Cross-links: `../commands/navigation-query-display.md` (a `SET FILTER TO <condition>` is the
same Logical-expression grammar); `../commands/navigation-query-display.md`,
`../commands/data-definition-and-manipulation.md`.

---

## 8. Index and key expressions (KEY FIELD)

> "A key field is a field or expression by which a database file is SORTed,
> INDEXed, JOINed, UPDATEd, or TOTALed."
>  --  `HELP.DBS.strings.txt:1767-1771` (topic `@KEY FIELD`)
> `[documented: mined HELP III+]`

`INDEX ON <key expression> TO <ndx>` (`ASSIST.HLP.strings.txt:204`) takes a
**single expression** whose value is the sort/search key. Key expressions are
ordinary expressions but with important constraints specific to indexing:

- The key expression must yield a **single, fixed-width** value. To index on
  more than one field you **concatenate** them into one Character expression,
  typically with the trailing-blank-relocating `-` to keep keys tidy (6.2):
  ```
  INDEX ON Last - First TO names
  INDEX ON STR(Amount,10,2) + Cust TO byamt    && stringify N to keep C key
  ```
- Numeric and date keys are allowed (single field/expression); mixing types in
  one key requires explicit conversion (`STR()`, `DTOC()`), because `C + N` is a
  type mismatch (1.1, 6.3).
- Index key **length limits** (the parser enforces these):
  - `Index is too big (100 char maximum).`  --  `DBASE.MSG.strings.txt:23`
  - `Index expression is too big (220 char maximum).`  --  `DBASE.MSG.strings.txt:109`

  i.e. the *evaluated key* is capped at 100 characters while the *source
  expression text* is capped at 220 characters. `[verified: two distinct mined
  error strings DBASE.MSG:23 + :109]`
- The `UNIQUE` clause and `SET UNIQUE` interact with keys but are a `SET`/command
  concern (cross-link `../commands/data-definition-and-manipulation.md`).

`[documented: mined HELP KEY FIELD (HELP.DBS:1767) + mined INDEX syntax (ASSIST.HLP:204) + mined index-size errors]`

---

## 9. Operator precedence and grouping

The mined III+ HELP system does **not** publish a single precedence table; it
documents the operator classes (sections 3-6) and shows precedence implicitly
through examples (`Price * (1 + Cost)`, `.NOT. (City = 'Glendale')`,
`.NOT. CLEAR .AND. YEAR(Date) = myear`). The precedence below is the standard
Xbase precedence, reconstructed from those examples plus the Clipper-lineage
oracle; the relative ordering of the arithmetic, then relational, then logical
tiers is confirmed by every sample idiom.

### 9.1 Precedence (highest binds first) within and across classes

| Tier | Operators | Assoc. | Result | Confidence |
|------|-----------|--------|--------|------------|
| 1 | `( )` grouping; function call | n/a | per inner | `[documented: HELP examples]` |
| 2/3 | unary `-`, unary `+` **and** `**`/`^` (exponentiation)  --  relative order disputed, see note | prefix / (assoc TBD) | N | `[oracle-resolves: unary-vs-power order]` |
| 4 | `*`, `/` | left | N | `[verified: HELP example Price*(1+Cost) requires * over +]` |
| 5 | binary `+`, binary `-` (arithmetic AND string concat) | left | N or C | `[verified: HELP examples]` |
| 6 | relational/comparison: `< > = <> # <= >= $` | (non-assoc / left) | **L** | `[verified: HELP CONDITION examples]` |
| 7 | `.NOT.` (logical negation) | prefix | L | `[verified: sample `.NOT. CLEAR .AND. ...`]` |
| 8 | `.AND.` | left | L | `[verified: HELP/sample]` |
| 9 | `.OR.` | left | L | `[verified: HELP/sample]` |

Key consequences (all observable in the samples / HELP):

- **Arithmetic binds tighter than relational binds tighter than logical.**
  In `LOCATE FOR Num > 0 .AND. .NOT. Clear` (`CLRDEP.PRG:154`), this parses as
  `((Num > 0) .AND. ((.NOT. Clear)))`  --  the comparison and the negation both
  resolve before `.AND.`. `[verified: sample CLRDEP.PRG:154, sed-confirmed + standard Xbase precedence]`
- `.NOT.` binds tighter than `.AND.`/`.OR.`. In
  `.NOT. CLEAR .AND. YEAR(Date) = myear` (`YEAREND.PRG:28`), this is
  `((.NOT. CLEAR) .AND. (YEAR(Date) = myear))`, not `.NOT. (CLEAR .AND. ...)`.
  `[verified: sample YEAREND.PRG:28]`
- `.AND.` binds tighter than `.OR.`. In
  `a <> b .OR. c <> d .OR. e` the `.OR.`s chain at the lowest precedence; any
  `.AND.` sub-clause groups first. `[documented: standard Xbase precedence; cross-checked vs Harbour]`
- The unary minus / exponentiation interaction (`-2 ^ 2`) and exponentiation
  associativity (`2 ^ 3 ^ 2`) are **not pinned** by local sources ->
  [Open questions]. Note that *standard* Xbase/Clipper actually binds `^`
  **tighter** than unary minus, so `-2 ^ 2` evaluates to `-(2^2) = -4` (not
  `(-2)^2 = 4`); the tier-2/3 grouping above is therefore deliberately left
  unresolved rather than committed to one ordering. `[inferred; oracle-resolves]`

### 9.2 Parentheses

Parentheses force grouping and override precedence; they are the recommended
way to make intent explicit (the HELP `.NOT. (City = 'Glendale')` example uses
them even where not strictly required). Unbalanced parentheses raise:

- `Unbalanced parenthesis.`  --  `DBASE.MSG.strings.txt:8` `[documented: mined error catalog]`

Parentheses are also used for function-argument lists and have nothing to do
with arrays  --  **III+ has no `[...]` array subscript operator** (arrays via
`DECLARE` and the `[]` operator are a dBASE IV / Clipper feature). `[verified:
no array facility in the HELP_topics oracle; '[ ]' in HELP syntax means
"optional", HELP.DBS:1692]`

### 9.3 Same-type-operand rule applies after precedence

Precedence determines *grouping*, but each operator still enforces the
type-agreement rule (1.1). So `Amt + "x" > 0` first tries `Amt + "x"` (N + C)
and fails with `Data type mismatch.` before any comparison happens. `[inferred
from type rule + left-to-right binding]`

---

## 10. Quick reference: complete III+ operator inventory

| Operator | Class | Operands -> Result | III+ note |
|----------|-------|--------------------|-----------|
| `+` | arith / string | N+N->N, D+N->D, N+D->D, C+C->C | C+N is an ERROR |
| `-` | arith / string | N-N->N, D-N->D, D-D->N, C-C->C; unary -N->N | C-N error; `-` concat moves trailing blanks to end |
| `*` | arith | N*N->N | |
| `/` | arith | N/N->N | div-by-zero behavior `[oracle-resolves]` |
| `**` | arith | N**N->N | synonym of `^` |
| `^` | arith | N^N->N | synonym of `**` |
| `<` | relational | C/N/D -> L | |
| `>` | relational | C/N/D -> L | |
| `=` | relational | C/N/D/L -> L | string match governed by SET EXACT (directional) |
| `<>` | relational | C/N/D/L -> L | |
| `#` | relational | C/N/D/L -> L | exact synonym of `<>` |
| `<=` | relational | C/N/D -> L | |
| `>=` | relational | C/N/D -> L | |
| `$` | relational | C $ C -> L | substring containment (unanchored) |
| `.AND.` | logical | L .AND. L -> L | dotted; short-circuit |
| `.OR.` | logical | L .OR. L -> L | dotted; short-circuit |
| `.NOT.` | logical | .NOT. L -> L | dotted; prefix |

**NOT in III+** (later-era additions, do not implement): `==` (dBASE IV),
`%` modulo operator (dBASE IV; use `MOD()`), automatic C<->N / display-form
coercion in `+`/`-`/comparisons (dBASE PLUS), `[]` array subscript (dBASE IV /
Clipper), `null` operand semantics (dBASE PLUS), object/Date/Function operands
(dBASE PLUS), bare-word `AND`/`OR`/`NOT`.

---

## Open questions

These need the real III+ interpreter (`DBASE.EXE` + `DBASE.OVL`) under DOSBox
to settle. Each goes to `GAPS.md`.

1. **Exponentiation associativity and unary-minus interaction.** Is `2 ^ 3 ^ 2`
   right- or left-associative? Does `-2 ^ 2` parse as `-(2^2) = -4` or
   `(-2)^2 = 4`? Local sources don't pin this. `[oracle-resolves]`
2. **Division by zero.** Does `5 / 0` raise an error (which message?) or return
   a value? No mined error string names it specifically. `[oracle-resolves]`
3. **Short-circuit guarantee.** Confirm III+ short-circuits `.AND.`/`.OR.`
   left-to-right (does NOT evaluate the right operand when the result is
   already determined), matching the Clipper-lineage const-fold rule
   (`cmpopt.txt:133-140`). `[oracle-resolves]`
4. **String `<`/`>`/`<=`/`>=` of unequal length.** Does the lexicographic
   comparison pad the shorter operand with blanks, or stop at the shorter
   length? And confirm `SET EXACT` does NOT affect the inequality operators.
   `[oracle-resolves]`
5. **`'' $ x` (empty left operand of `$`).** Modern doc says empty string is
   "not contained"; confirm III+ returns `.F.` (and `x $ ''`). `[oracle-resolves]`
6. **The `CHR(0)`-on-the-right `=` quirk** (`OPS_COMPARISON.md:63`): does III+
   exhibit the "right operand begins with CHR(0) and EXACT OFF -> always true"
   behavior? `[oracle-resolves]`
7. **Exact maximum concatenation / character-expression length** (the value
   behind `+/- : Concatenated string too large.`). Likely 254 (line limit) or a
   distinct internal cap. `[oracle-resolves]`
8. **Date blank-date arithmetic** (`D + N` with a blank `D`, `D - D` with a
   blank operand): confirm "blank date + N = blank date" and "blank date
   subtracted = 0" hold in III+ as in the modern doc. `[oracle-resolves]`
9. **Exact error token for disallowed mixes**  --  does `'A' + 1` give
   `Data type mismatch.` (DBASE.MSG:9) vs `Syntax error.` (:10) vs
   `Invalid operator.` (:104)? And does `5 .AND. .T.` give
   `Not a Logical expression.` (:36)? `[oracle-resolves]`
10. **Does `%` parse at all?** Confirm `5 % 2` yields `Invalid operator.` /
    `Syntax error.` (i.e. is genuinely absent), not a silently-accepted modulo.
    `[oracle-resolves]`

---

## Sources

Local ground truth (cited inline above):

- `archive/golden-mined/HELP.DBS.strings.txt`  --  period-exact III+ HELP text.
  Operator topics: `@OPERATOR` (1776), `@MATHEMATICAL OP` (1784),
  `@RELATIONAL OP` (1792), `@LOGICAL OP` (1804), `@STRING OP` (1815);
  expression topics: `@EXPRESSION` (1727), `@EXP 2` (1738), `@CONDITION`
  (1749), `@KEY FIELD` (1767); `SET EXACT` index line (1609); logical literals
  (1831, 1923); `MOD()` (1306). **Primary III+ syntax authority.**
- `archive/golden-mined/DBASE.MSG.strings.txt`  --  III+ error catalog. Cited:
  `Data type mismatch.` (9), `Syntax error.` (10), `Unbalanced parenthesis.`
  (8), `Line exceeds maximum of 254 characters.` (18), `Not a numeric
  expression.` (27), `Not a Logical expression.` (36), `Not a Character
  expression.` (43), `Invalid operator.` (104), `+/- : Concatenated string too
  large.` (73-74), `^ or ** : Negative base, fractional exponent.` (75),
  `Numeric overflow (data was lost).` (38), `Index is too big (100 char
  maximum).` (23), `Index expression is too big (220 char maximum).` (109).
- `archive/golden-mined/HELP_topics.txt`  --  the 406-item completeness oracle
  (topics 151-160 + 155, this document's scope).
- `archive/golden-mined/ASSIST.HLP.strings.txt`  --  command syntax (e.g.
  `INDEX ON <key expression> TO <index file name>`, line 204;
  `SEEK <expression>`, line 128).
- `archive/dbase-com-help/OPS_COMPARISON.md`, `OPS_PLUS.md`, `OPS_MINUS.md`,
  `SET_EXACT.md`  --  **modern dBASE PLUS** operator pages. Used only for the
  III+-stable core (string `-` blank relocation, `=` prefix/EXACT semantics,
  date arithmetic). Every later-era element (`==`, auto-stringification, `%`,
  `null`, objects, language-driver weights) is flagged as a delta and excluded
  from III+.
- `oracles/harbour/doc/cmpopt.txt:124-147`  --  Clipper/Harbour compile-time
  const-fold (logical short-circuit folding at 133-140, `^` numeric const-fold
  at 144). Semantic oracle only, NOT period-exact syntax.
- `oracles/xharbour/doc/en/operator.txt:60-159`  --  Clipper-lineage operator
  reference. Confirms `.AND.`/`.OR.` **runtime** short-circuit (the `(shortcut)`
  annotations, lines 80-81/146-147), `.AND./.OR./.NOT.` truth tables, "full
  clipper compatible". Semantic oracle only; its `!` synonym for `.NOT.` is a
  Clipper extension NOT claimed for III+.
- Real III+ sample programs in
  `goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/`
  (all line/file refs re-verified with `sed` during verification):
  `CHECK.PRG` (`+STR()` concat :19, `$ "YyNn"` menu validation :135),
  `FLIGHT.PRG` (`$'ABCE'` menu validation :40), `ADD.PRG` (`.AND.` short-circuit
  guard `ans$"YyNn" .AND. .NOT. special` :114), `CLEANUP.PRG` (`.AND.` numeric
  conditions `Amt>0 .AND. Num>mlastdep` :28), `MAINT.PRG` (`<>` not-equal
  `counter <> VAL(choice2)` :37; also `CASH.PRG:46`), `FLT_CNCL.PRG` (`#`
  not-equal `lockcount # 0` :118; also `SHOW_FLT.PRG:64`), `YEAREND.PRG`
  (`.NOT. ... .AND.` precedence :28), `EDITVOID.PRG` (`= ... .AND. = 0.00` :66),
  `CBMENU.PRG` (chained `<> ... .OR. ...` continued line :93), `RPRTPRO.PRG`
  (range test `Date >= begin .AND. Date <= end` :192), `CLRDEP.PRG`
  (`Num > 0 .AND. .NOT. Clear` precedence :154), `NUMWORDS.PRG` (heavy
  `+RTRIM(SUBSTR())` concat idioms, :69).

Cross-links (sibling specs): `../commands/set-commands.md`,
`../commands/navigation-query-display.md`, `../commands/navigation-query-display.md`, `../commands/data-definition-and-manipulation.md`,
`../commands/navigation-query-display.md`, `./data-types-and-literals.md`,
`./functions-*.md` (`MOD`, `STR`, `VAL`, `SUBSTR`, `CTOD`, `DTOC`, `TRIM`).

---

## Verification

Adversarial second-pass verification (2026-06-16). Every citation re-derived
against the actual local sources; the substantive III+ semantics held up well,
but a cluster of sample-program citations was scrambled and two function/literal
claims overstated their evidence.

### Re-derived and CONFIRMED (independent cross-check)

- **Mined HELP operator/expression text** (`HELP.DBS.strings.txt`): the
  `@OPERATOR` taxonomy (1776-1783), `@MATHEMATICAL OP` (1784-1791, five ops, no
  `%`), `@RELATIONAL OP` (1792-1803, eight ops incl. `$`, no `==`),
  `@LOGICAL OP` (1804-1809), `@STRING OP` (1815-1821, `+`/`-` concat with
  trailing-blank relocation), `@EXPRESSION`/`@EXP 2`/`@CONDITION`/`@KEY FIELD`,
  the type-agreement rule (1733-1737), `MOD()` (1306), `SET EXACT on/OFF` (1609),
  `CTOD('01/01/85')` no-date-literal (1832), `FIELD TYPE` logical input chars
  (1923-1924)  --  all read verbatim and match the doc's quotes. (Minor: the doc's
  in-block line ranges are occasionally off by 1-2 lines but always inside the
  correct topic; left as-is.)
- **Error catalog** (`DBASE.MSG.strings.txt`): all 16 cited line numbers exact  -- 
  8, 9, 10, 18, 23, 27, 36, 38, 43, 56, 59, 73, 74, 75, 104, 109.
- **Completeness oracle** (`HELP_topics.txt`): topics 151-160 confirmed; 154 =
  FIELD LIST is correctly out of scope; the coverage table is accurate.
- **ASSIST syntax**: `INDEX ON <key expression> TO ...` (204), `SEEK` (124-128).
- **dbase.com modern pages**: `OPS_COMPARISON.md` (auto-conversion 7-37, `==` 44,
  EXACT directionality + `==`-always-EXACT 57, CHR(0) quirk 63, `$` empty 65),
  `OPS_PLUS.md` (auto-stringify 31, blank-date 27, unary plus 49),
  `OPS_MINUS.md` (date-date 25, blank-relocation 29, index-trim 31),
  `SET_EXACT.md` (partial/exact 19-23, language-driver weights 25)  --  all exact.
  Every later-era element correctly flagged as a delta and excluded from III+.
- **Harbour `cmpopt.txt`**: short-circuit const-fold 133-140, `^` const-fold 144
   --  exact. **NEW second oracle added:** `xharbour/doc/en/operator.txt:80-81,
  146-147` annotates `.AND.`/`.OR.` short-circuit as `(shortcut)` at *runtime*,
  strengthening section5.4 (was single-source compile-time fold; now two independent
  oracles, one of them runtime).
- **III+-correctness deltas all hold:** `C+N` = `Data type mismatch` (not
  auto-stringification); no `==`; no `%` (only `MOD()`); no `[]`/arrays/`null`/
  objects; dotted `.AND./.OR./.NOT.` mandatory. None are III+-incorrect imports.

### FIXED

1. **Scrambled sample-program file attributions (9 citations).** The quoted
   idiom text was correct everywhere, but file names were wrong (line numbers
   mostly right). `sed`-confirmed corrections:
   - `$'ABCE'` menu loop: CHECK.PRG:40 -> **FLIGHT.PRG:40**.
   - `ans$"YyNn"` loop: MAINT.PRG:135 -> **CHECK.PRG:135**.
   - short-circuit `ans$"YyNn" .AND. .NOT. special`: CASH.PRG:114 ->
     **ADD.PRG:114** (load-bearing section5.4 evidence).
   - `Amt>0 .AND. Num>mlastdep`: ADD.PRG:28 -> **CLEANUP.PRG:28**.
   - `counter <> VAL(choice2)`: ADD.PRG:37 -> **MAINT.PRG:37** (also CASH.PRG:46).
   - `lockcount # 0`: FAST.PRG:118 -> **FLT_CNCL.PRG:118** (also SHOW_FLT.PRG:64).
   - chained `<> ... .OR.`: CHECK.PRG:93 -> **CBMENU.PRG:93**.
   - range test `Date >= begin .AND. Date <= end`: -> **RPRTPRO.PRG:192**.
   - `Num > 0 .AND. .NOT. Clear`: FLT_INFO.PRG:154 -> **CLRDEP.PRG:154**.
   Sources block and section4.2/4.5/5.4/7/9.1 updated; tags raised to `[verified:
   sed-confirmed]`.
2. **`ISCANCEL()` is not a III+ function** (section1). It appears in the mined HELP
   only as a *logical field name* in a sample DBF (`HELP.DBS:1958`), not a
   built-in. Replaced with real III+ logical functions (`ISCOLOR()`/`ISALPHA()`)
   and added a clarifying note.
3. **`.Y.`/`.N.` dotted logical literals overstated** (section1, section5.2). The cited
   sources (1831, 1923) document only `.T.`/`.F.` plus the `T`/`Y`,`F`/`N`
   logical-field *input chars*  --  **not** `.Y.`/`.N.` literal tokens (zero hits
   for `.Y.`/`.N.` in `HELP.DBS.strings.txt`). Reworded; the `.Y.`/`.N.` claim
   demoted to a `[oracle-resolves]` caveat; `[verified]` tag corrected.
4. **Precedence tier 2 vs 3 over-committed** (section9.1). The table placed unary
   `-`/`+` strictly above `**`/`^` as `[documented]`, but no local source pins
   this and *standard* Xbase binds `^` tighter than unary minus (`-2^2 = -4`).
   Merged into one disputed tier `2/3`, tagged `[oracle-resolves]`, with a note
   on the standard-Xbase ordering.
5. **Short-circuit framing tightened** (section5.4): clarified that Harbour's
   `cmpopt` rules are a *disableable compile-time* optimization (evidence, not
   proof of runtime short-circuit), and added the runtime xHarbour oracle.

### Completeness vs the HELP index

Full coverage of the in-scope operator/expression topics 151-153, 155-160
(FIELD LIST 154 correctly excluded). The operator inventory matches the mined
`@*OP` pages exactly (5 arithmetic, 8 relational, 3 logical, `+`/`-` string).
No III+ operator is missing; no later-era operator is wrongly included.

### Residual `[oracle-resolves]` gaps (unchanged  --  need real interpreter)

Exponentiation associativity & unary-minus interaction; division-by-zero
behavior; runtime short-circuit confirmation for III+ specifically; string
`<`/`>` unequal-length padding; `'' $ x`; CHR(0)-on-right `=` quirk; max
concat length value; blank-date arithmetic; exact error token for disallowed
mixes; whether `%` parses at all; **(new)** whether III+ parses `.Y.`/`.N.` as
logical literals.

`[verified: full second-pass cross-check of mined HELP + DBASE.MSG +
HELP_topics + ASSIST + dbase.com OPS pages + harbour/xharbour oracles + all
sample-PRG line/file refs sed-confirmed]`
