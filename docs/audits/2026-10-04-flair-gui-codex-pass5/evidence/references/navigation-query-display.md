# Navigation, Query & Display Commands  --  dBASE III PLUS 1.1

Full reference for the commands that move the record pointer, query/search the
active database, and produce output: `GO`/`GOTO`, `SKIP`, `LOCATE`/`CONTINUE`,
`SEEK`, `FIND`, `SET FILTER TO`, `SET DELETED`, `SET RELATION TO ... INTO`,
`SET FIELDS`, the `LIST`/`DISPLAY` family (`STRUCTURE`/`STATUS`/`MEMORY`/
`HISTORY`/`FILES`), `?`/`??`, `REPORT FORM`, `LABEL FORM`, and the aggregates
`SUM`/`AVERAGE`/`COUNT`. For each command: full syntax (every clause), exact
semantics, defaults, record-pointer effects, and the interaction with
`FOUND()`/`EOF()`/`BOF()`/`RECNO()`/`DELETED()`.

## Status / confidence

- **Target:** dBASE III PLUS 1.1 (1986). The period authority for *which*
  commands/clauses exist and their exact written syntax is the mined help text
  `archive/golden-mined/HELP.DBS.strings.txt` (the dot-prompt HELP system) and
  `archive/golden-mined/ASSIST.HLP.strings.txt` (the ASSIST menu-mode help).
  These two are the same product, two front ends; HELP.DBS carries the fuller
  command syntax, ASSIST.HLP the menu-context phrasing.
- **Semantic oracle (NOT period-exact syntax):** Harbour/xHarbour `doc/en`
  (`rddmisc.txt`, `set.txt`, `rdddb.txt`)  --  the Clipper/xBase lineage that
  preserves III+ runtime behavior of pointer/search/EOF semantics. Used to
  cross-check semantics, never to import IV+/Clipper-only syntax.
- **Modern dbase.com help** (`archive/dbase-com-help/`) is used only for the
  `SET EXACT` / comparison-operator core. Two of its claims are **IV+/Plus-era
  and WRONG for III+** and are tagged as deltas below: (1) the `==` operator
  does not exist in III+; (2) automatic stringification of a `C + N` (or any
  type-mismatched) expression does not happen in III+  --  `C + N` is a
  **type-mismatch error** (`DBASE.MSG` "Data type mismatch.").
- Confidence tags: `[verified: A + B]`, `[documented: src]`, `[oracle-resolves]`,
  `[inferred]`. Anything that needs the real interpreter under DOSBox to settle
  is in **Open questions** and goes to `GAPS.md`.

### Completeness against the 406-topic oracle (`HELP_topics.txt`)

Items in this document's scope and where covered:

| Topic (oracle) | Section |
|---|---|
| GO/GOTO, GOTO | [GO / GOTO](#go--goto) |
| SKIP | [SKIP](#skip) |
| LOCATE | [LOCATE](#locate) |
| CONTINUE | [CONTINUE](#continue) |
| SEEK | [SEEK](#seek) |
| FIND | [FIND](#find) |
| SET FILTER | [SET FILTER TO](#set-filter-to) |
| SET DELETED | [SET DELETED](#set-deleted) |
| SET RELATION | [SET RELATION TO ... INTO](#set-relation-to--into) |
| SET FIELDS | [SET FIELDS](#set-fields) |
| DISPLAY, LIST | [LIST / DISPLAY](#list--display-records) |
| DISPLAY/LIST STRUCTURE | [LIST/DISPLAY STRUCTURE](#listdisplay-structure) |
| DISPLAY/LIST STATUS | [LIST/DISPLAY STATUS](#listdisplay-status) |
| DISPLAY/LIST MEMORY | [LIST/DISPLAY MEMORY](#listdisplay-memory) |
| DISPLAY/LIST HISTORY | [LIST/DISPLAY HISTORY](#listdisplay-history) |
| DISPLAY/LIST FILES | [LIST/DISPLAY FILES](#listdisplay-files) |
| ?/?? | [? / ??](#---output) |
| REPORT (FORM) | [REPORT FORM](#report-form) |
| LABEL (FORM) | [LABEL FORM](#label-form) |
| AVERAGE, SUM, COUNT | [Aggregates](#aggregates-sum--average--count) |
| SCOPE | [Scope, FOR, WHILE](#scope-for-and-while-the-shared-grammar) |
| FOUND(), EOF(), BOF(), RECNO(), DELETED() | [Pointer-state functions](#pointer-state-functions) |
| FILE LIST, CONDITION | [Scope, FOR, WHILE](#scope-for-and-while-the-shared-grammar) |

Out of scope here (other specs own them, cross-linked where touched):
USE/SELECT/CLOSE/work areas (runtime), INDEX/SET INDEX/SET ORDER/REINDEX/SORT
(index/sort spec), DELETE/RECALL/PACK/ZAP/REPLACE/APPEND/EDIT (data-modify
spec), `.frm`/`.lbl` **file layout** (reports-labels spec  --  only the FORM
*invocation* command is here), full-screen BROWSE/CHANGE (full-screen spec),
the `?`/`??` PICTURE/TRANSFORM formatting detail (functions spec). `TYPE <file>`
is text-file output, not a query command, and is covered with file utilities.

---

## Scope, FOR, and WHILE: the shared grammar

Most commands in this file take the same record-selection grammar. III+ HELP
defines `<scope>` as exactly four phrases [verified: HELP.DBS `_Scope` + ASSIST]:

```
   RECORD <n>     A single record (record number n).
   NEXT <n>       <n> records starting at the CURRENT record.
   REST           Current record through the bottom record.
   ALL            Every record in the database file.
```

Defaults: "Each command with the scope option has a default scope of either the
current database record or all database records." [documented: HELP.DBS `_Scope`].
The per-command default is stated in each section below.

- `FOR <condition>`  --  process every record in scope whose `<condition>` is `.T.`
  The scan visits every record in scope (it does NOT stop at the first false).
- `WHILE <condition>`  --  process records in scope only **while** `<condition>` is
  `.T.`; processing **stops at the first record where it is false**. WHILE is
  almost always paired with an ordered (indexed/`SEEK`ed) starting position.
- A `<condition>` is a logical expression built from fields/memvars/constants/
  functions and the logical operators `.AND. .OR. .NOT.` and relational
  operators `< > = <> # <= >= $` [verified: HELP.DBS `_Conditions` + `_Relational
  Operators`]. Note III+ has **only these** relational operators: the equality
  test is the single `=` (no `==`), `<>` and `#` are the two not-equal forms, and
  `$` is the substring-comparison operator ("`<expC1> $ <expC2>` is `.T.` if the
  first string is identical to or contained within the second") [verified:
  HELP.DBS `_Relational Operators`: list is exactly `< > = <> # <= >= $`; **no
  `==`**  --  the Clipper/IV+ exact-equality `==` operator is NOT in the III+
  surface]. Mathematical operators are `+ - * /` and `**`/`^` (exponentiation)
  [verified: HELP.DBS `_Mathematical Operators`].
- **String operators (`+` and `-`):** both concatenate two character strings
  [verified: HELP.DBS `_String Operators`]. `+` joins them as-is. `-` is the
  **III+ "ragged-blank" concatenation**: it joins the two strings but moves all
  trailing blanks of the first string to the **end** of the combined result
  ("If `-` is used, all trailing blanks are moved to the end of the combined
  string." [verified: HELP.DBS `_String Operators`]). Example:
  `"AB   " - "CD"` yields `"ABCD   "`, whereas `"AB   " + "CD"` yields
  `"AB   CD"`. Both operands of `+`/`-` must be character  --  this is the same
  same-type rule, so `<expC> + <expN>` is a "Data type mismatch.", not an
  auto-conversion (see the type rule below).
- `<expression list>` (a.k.a. field list when bare field names)  --  comma-separated
  expressions controlling which columns are shown/operated on. Default is the
  whole record's fields (subject to `SET FIELDS`).

**Evaluation order within a scoped command** (matters for FOR vs WHILE and
pointer position) [verified: HELP.DBS semantics + Harbour rdd behavior;
oracle-resolves the exact tie-breaks]:

1. Establish the start record: with an explicit `RECORD n`, that record; with
   `NEXT`/`REST`/no scope, the current record; with `ALL`, the top record
   (commands with default scope ALL implicitly `GO TOP` first  --  LIST, SUM,
   AVERAGE, COUNT, REPORT, LABEL).
2. Walk forward (in controlling-index order if an index is master, else record
   order). For each record evaluate `WHILE` first; if `.F.`, stop. Then evaluate
   `FOR`; if `.F.`, skip this record but continue the walk. Honour `SET DELETED`
   and `SET FILTER` (a deleted-and-hidden or filtered-out record is invisible to
   the walk entirely).
3. After the command, the pointer is left **past** the last record processed  -- 
   typically at EOF for an ALL-scope command (see each command's "Pointer
   effect").

**III+-specific type rule (the single biggest hazard):** every operand of an
operator must be the same type, with the sole documented exception of date+/-
number arithmetic [verified: HELP.DBS `_Expressions`: "Each part of an expression
(except the operator) should be of the same type. The exception ... is the use
of dates and numbers with some of the operators."]. Therefore a `<condition>`
or `<expression list>` element like `Amount + Name` (N + C) raises
**"Data type mismatch."** (`DBASE.MSG` line 9) at evaluation time. **There is no
auto-stringification** in III+. The modern dbase.com `+` operator doc that
"converts the other data type to its display representation" describes
dBASE Plus/IV behavior and is a **delta  --  do not implement for III+.**
[verified: HELP.DBS `_Expressions` + `DBASE.MSG` "Data type mismatch." vs.
`archive/dbase-com-help/OPS_PLUS.md` (Plus-era, contradicted)].

---

## GO / GOTO

Absolute positioning of the record pointer.

**Syntax** [verified: HELP.DBS `_GO/GOTO` + ASSIST `_GOTO`]:

```
[GO | GOTO] [RECORD] <expN>
GO | GOTO BOTTOM
GO | GOTO TOP
```

- `GO` and `GOTO` are exact synonyms.
- `RECORD` is an optional noise keyword: `GO 5`, `GOTO 5`, `GO RECORD 5`,
  `GOTO RECORD 5` are identical.
- At the dot prompt a bare number with no verb (`5`) also positions to record 5
  (the verb is implied) [documented: HELP.DBS `_GO/GOTO` form `[GO/GOTO [RECORD]]
  <expN>`]. In a program file you should write `GO <expN>` explicitly.
- `<expN>` is any numeric expression; non-integer values are truncated
  [inferred from numeric handling].

**Semantics:**

- `GO TOP`  --  move to the first record **in the current order**. If a controlling
  index is active, that is the first key in index order; otherwise record 1.
- `GO BOTTOM`  --  move to the last record in the current order.
- `GO <expN>`  --  move to the **physical record number** `<expN>`, independent of
  any index order. (`GO` always means the physical record number, even with an
  index open  --  only `TOP`/`BOTTOM` follow index order.) [verified: HELP.DBS +
  Harbour `dbGoto()` "RecNo()-based positioning"].

**Pointer / state effects:**

- `RECNO()` becomes `<expN>` (or the top/bottom record's number).
- `GO TOP` on a non-empty file: `BOF()` and `EOF()` both `.F.`
- `GO <expN>` with `<expN>` outside `1..RECCOUNT()`: raises **"Record is out of
  range."** (`DBASE.MSG` line 5) [documented: DBASE.MSG]. `GO 0` is likewise out
  of range. `[oracle-resolves]` whether `GO` to a record hidden by `SET DELETED`/
  `SET FILTER` is allowed (Clipper allows it and leaves the pointer there; III+
  behavior under DOSBox to confirm).
- On an **empty** database (`RECCOUNT()=0`): `GO TOP`/`GO BOTTOM` leave the
  pointer at the phantom EOF record, `EOF()` `.T.`, `RECNO()` returns 1
  (`LASTREC()+1`) [oracle-resolves; Harbour: `RECNO()==1` at EOF of empty file].
- `GO`/`GOTO` does **not** affect `FOUND()`. `FOUND()` reflects only the last
  search command (`SEEK`/`FIND`/`LOCATE`/`CONTINUE`) [verified: Harbour
  `Found()` "tests previous SEEK, LOCATE, CONTINUE, or FIND"  --  GO not listed].

**Real III+ idiom**  --  reset to top before a forward scan
(`Sample_Programs_and_Utilities/RECONCIL.PRG:172`, `CHECK.PRG:236`):

```
GO TOP
DO WHILE .NOT. EOF()
   ... process ...
   SKIP
ENDDO
```

`See also:` SKIP, RECNO(), SET RELATION (relations re-position child files).

---

## SKIP

Relative positioning of the record pointer.

**Syntax** [verified: HELP.DBS `_SKIP` + ASSIST]:

```
SKIP [<expN>]
SKIP [<expN>] [IN <alias>]      && IN-alias form: see note
```

- `<expN>`  --  number of records to move. Positive = forward, negative = backward.
  Default (no argument) is **+1** (forward one record) [verified: HELP.DBS "The
  default is forward one record."].
- The `IN <alias>` clause (skip a *named, unselected* work area without changing
  the SELECTed area) is a documented xBase form. III+ supports moving the pointer
  in related child areas implicitly via `SET RELATION`; whether the explicit
  `SKIP <n> IN <alias>` syntax is accepted by the III+ parser is
  `[oracle-resolves]`  --  HELP.DBS lists only `SKIP [<expN>]`. Treat `IN <alias>`
  as **not part of the III+ surface** unless the binary proves otherwise.

**Semantics & pointer effects:**

- Moves the pointer `<expN>` records in the **current order** (index order if a
  controlling index is active; physical order otherwise).
- Honours `SET DELETED` and `SET FILTER`: hidden records are skipped over and do
  not count toward `<expN>` [verified: Harbour `dbSkip()` honours deleted/filter].
- **Forward past the last record:** pointer lands on the phantom EOF record,
  `EOF()` becomes `.T.`, `RECNO()` returns `RECCOUNT()+1` (= `LASTREC()+1`).
  Once at EOF, `SKIP` (positive) is a no-op / re-asserts EOF. Issuing a forward
  `SKIP` while already at EOF raises **"End of file encountered."**
  (`DBASE.MSG` line 4) in some contexts `[oracle-resolves: whether SKIP at EOF
  errors or silently stays]`.
- **Backward past the first record:** pointer lands on the first record and
  `BOF()` becomes `.T.`; `RECNO()` stays 1. A further backward `SKIP` keeps
  `BOF()` `.T.` Issuing `SKIP -1` at BOF can raise **"Beginning of file
  encountered."** (`DBASE.MSG` line 37) `[oracle-resolves]`.
- A successful `SKIP` that lands on a valid record clears `BOF()`/`EOF()` to
  `.F.` [verified: Harbour BOF/EOF examples loop on `! Bof()` / `! Eof()`].
- `SKIP` does **not** change `FOUND()`.
- On an empty file, `SKIP` leaves pointer at EOF (`EOF()` `.T.`).

**Real III+ idiom** (`Sample_Programs_and_Utilities/CHECK.PRG:244`,
`PROG.PRG:9`): forward walk with `SKIP` at the bottom of a `DO WHILE .NOT.
EOF()` loop. Backward walk uses `SKIP -1` guarded by `.NOT. BOF()`.

`See also:` GO/GOTO, RECNO(), EOF(), BOF().

---

## LOCATE

Sequential (record-by-record) search for the first record matching a condition.
Works on any file, indexed or not; HELP notes it "works faster on a database
file that is not indexed" [verified: ASSIST `_LOCATE`].

**Syntax** [verified: HELP.DBS `_LOCATE` + ASSIST]:

```
LOCATE [<scope>] [FOR <condition>] [WHILE <condition>]
```

- **Default scope is `ALL`**  --  LOCATE implicitly starts from the top of the
  file (in current order) unless an explicit scope restricts it
  [verified: HELP.DBS `_Scope` default + Harbour LOCATE semantics].
- `FOR <condition>` is the primary search predicate. A bare `LOCATE FOR <cond>`
  scans from the top for the first record where `<cond>` is `.T.`
- `WHILE <condition>` bounds the scan: it stops at the first record where the
  WHILE is `.F.` (used to limit a LOCATE within an ordered group).
- LOCATE with no FOR/WHILE simply moves to the first record of the scope.

**Semantics & pointer effects:**

- On a **match**: pointer is left on the matching record, `FOUND()` is `.T.`,
  `EOF()` is `.F.`, `RECNO()` is the match's record number.
- On **no match** (scan exhausted): pointer is at EOF, `FOUND()` is `.F.`,
  `EOF()` is `.T.` The dot prompt prints **"End of LOCATE scope"**
  (`DBASE.MSG` line 157) when the scope is exhausted without a (further) match.
- Honours `SET DELETED` (deleted records hidden when `SET DELETED ON`) and
  `SET FILTER` automatically.
- Each work area has its **own** LOCATE condition and `FOUND()` flag; a LOCATE in
  one area does not disturb another [verified: Harbour `Found()` "each work area
  has its own Found() flag"].

**Real III+ idiom** (`Sample_Programs_and_Utilities/CLRDEP.PRG:50,99,154`):

```
LOCATE FOR Num=cntr
... test FOUND() ...
```

`See also:` CONTINUE, FOUND(), SEEK/FIND (for indexed search instead).

---

## CONTINUE

Resume the most-recent `LOCATE` in the current work area, finding the *next*
record that satisfies it.

**Syntax** [verified: HELP.DBS `_CONTINUE` + ASSIST]:

```
CONTINUE
```

No clauses. CONTINUE takes none.

**Semantics  --  the critical III+ detail:** CONTINUE searches forward from the
current record for "the next record that meets the condition of the LOCATE
command most recently executed **in that work area**" [verified: HELP.DBS
`_CONTINUE`]. CONTINUE re-applies **only the `FOR` condition** of the prior
LOCATE. It does **not** re-apply the original `<scope>` and does **not**
re-apply the `WHILE` clause  --  once the original LOCATE's WHILE/scope bound was
established, CONTINUE simply keeps walking forward under the FOR predicate until
the FOR matches again or EOF is reached. `[verified: HELP.DBS wording "next
record that meets the condition" (singular, = the FOR) + Harbour CONTINUE
semantics; the scope/WHILE non-reapplication is oracle-resolves for the exact
edge  --  flagged in Open questions]`.

**Pointer / state effects:**

- Match: pointer on next matching record, `FOUND()` `.T.`
- No further match: pointer at EOF, `FOUND()` `.F.`, prints "End of LOCATE
  scope".
- `CONTINUE` with no prior `LOCATE` in this work area raises **"CONTINUE without
  LOCATE."** (`DBASE.MSG` line 40) [documented: DBASE.MSG].

**Real III+ idiom**  --  the canonical LOCATE/CONTINUE loop
(`Sample_Programs_and_Utilities/CLRDEP.PRG:154`):

```
LOCATE FOR Num > 0 .AND. .NOT. Clear
DO WHILE FOUND()
   cntr = cntr + 1
   REPLACE Num WITH cntr
   CONTINUE
ENDDO
```

This is the idiom the reimplementation must reproduce exactly: the loop tests
`FOUND()`, does work, then `CONTINUE` re-applies the same FOR predicate from the
current record onward.

`See also:` LOCATE, FOUND().

---

## SEEK

Fast indexed search for the first record whose **controlling index key** matches
an expression value.

**Syntax** [verified: HELP.DBS `_SEEK` + ASSIST]:

```
SEEK <expression>
```

- `<expression>` is evaluated, and its value is searched for in the master
  (controlling) index. The expression's **type must match the index key's type**
  (character index -> `<expC>`, numeric index -> `<expN>`, date index -> `<expD>`).
- The database **must be indexed** with a controlling index set; otherwise raises
  **"Database is not indexed."** (`DBASE.MSG` line 26) [documented: DBASE.MSG].

**Semantics & pointer effects:**

- Match: pointer on the first record whose key matches; `FOUND()` `.T.`,
  `EOF()` `.F.`
- No match: pointer at EOF, `FOUND()` `.F.`, `EOF()` `.T.` (III+ does not have
  Clipper's `SET SOFTSEEK`  --  `SOFTSEEK` is a later/xHarbour extension and is NOT
  in the III+ surface; a failed III+ SEEK always lands at EOF). [verified:
  HELP.DBS has no SOFTSEEK + DBASE.MSG; Harbour `_SET_SOFTSEEK` flagged as
  later].
- **Character SEEK + `SET EXACT`:** with `SET EXACT OFF` (the III+ default), a
  character SEEK does a **"begins with"** partial match  --  `SEEK "S"` finds the
  first key beginning with `S`. With `SET EXACT ON`, the key must match the
  search value exactly except trailing blanks are ignored. [verified:
  `archive/dbase-com-help/SET_EXACT.md` (III+-stable core) + Harbour
  `_SET_EXACT`]. Note the index-key comparison length matters: a `SEEK` value
  longer than the key, or shorter, follows the begins-with rule when EXACT is
  OFF.
- Each work area has its own `FOUND()` flag.

**Real III+ idiom** (`Sample_Programs_and_Utilities/CHECK.PRG:68`  --  duplicate-key
test, `EDITVOID.PRG:39`  --  find-or-fail):

```
SEEK mchkno
IF FOUND()
   @ 18,15 SAY "Check number already exists.  Please reenter"
ENDIF
```

```
SEEK mcheck
IF .NOT. FOUND()
   @ 20,26 SAY "Check "+LTRIM(STR(mcheck,4))+" cannot be found."
   ...
ENDIF
```

`See also:` FIND, FOUND(), SET ORDER, SET INDEX.

---

## FIND

The older, character-string form of indexed search (predates SEEK; both exist in
III+).

**Syntax** [verified: HELP.DBS `_FIND` + ASSIST]:

```
FIND <character string>
```

- `<character string>` is a **literal** typed directly after `FIND`, **without
  quotes**  --  "The character string does not have to be in quotes." [verified:
  HELP.DBS `_FIND`]. Leading blanks in the typed string are significant only as
  written; to search for a value held in a memory variable you macro-substitute
  it: `FIND &mvar`.
- FIND searches the controlling index only (numeric indexes: FIND with a numeric
  literal works too, but SEEK is the idiomatic form for non-character keys).

**Semantics & pointer effects:** identical to SEEK except for the literal-string
argument convention:

- Match: pointer on first key match, `FOUND()` `.T.`
- No match: pointer at EOF, `FOUND()` `.F.`, `EOF()` `.T.` III+ prints
  **"No find."** (`DBASE.MSG` line 14) / **"** Not Found **"** at the dot prompt.
- Requires an indexed file (else "Database is not indexed.").
- Partial / begins-with matching under `SET EXACT OFF` applies exactly as for
  SEEK.

**Real III+ idiom**  --  FIND with a macro-substituted memvar
(`Sample_Programs_and_Utilities/SHOW_FLT.PRG:48`, `FLT_RSVE.PRG:63`). Note these
samples test `EOF()` instead of `FOUND()` after FIND  --  both are valid; on a
failed FIND, `EOF()` is `.T.` and `FOUND()` is `.F.`:

```
SET INDEX TO Flt_No, Aval_flt
FIND &answer
IF EOF()
   @ 21,4 SAY "*** This is not the correct flight number, try again ***"
   ...
ENDIF
```

`See also:` SEEK, FOUND().

**SEEK vs FIND summary:** `SEEK` takes a typed *expression* (use for numeric,
date, computed, or quoted-string keys; the modern preferred form); `FIND` takes a
bare *literal character string* (legacy convenience; for memvars use `FIND
&mvar`). Behavior of the search and all pointer/`FOUND()` effects are identical.

---

## SET FILTER TO

Make the active database appear to contain only the records meeting a condition.

**Syntax** [verified: HELP.DBS `_SET FILTER` + ASSIST]:

```
SET FILTER TO [<condition>]
SET FILTER TO FILE <query file>      && .qry; ? lists cataloged query files
SET FILTER TO FILE ?
```

- `SET FILTER TO <condition>`  --  installs the logical `<condition>` as a filter on
  the **currently SELECTed work area**.
- `SET FILTER TO FILE <file>`  --  activates the filter condition stored in a query
  (`.qry`) file [verified: ASSIST `_SET FILTER TO`]. The `.qry` is the
  ASSIST/`CREATE QUERY` artifact.
- `SET FILTER TO` with no argument **turns the filter off** for the active area
  [verified: HELP.DBS "SET FILTER TO turns off the filter"].

**Semantics & pointer effects (III+-critical):**

- The filter is **lazy / not retroactive**: setting a filter does **not** move
  the record pointer, and the *current* record may be one the filter excludes
  until the pointer next moves. **You must `GO TOP` (or otherwise move the
  pointer) after `SET FILTER TO` to land on the first record that passes the
  filter.** This is a well-known III+ gotcha the reimplementation must replicate
  [verified: HELP.DBS wording "appear to contain only the records meeting the
  condition" + Harbour filter semantics; the must-GO-TOP behavior is
  oracle-resolves for the exact pointer state immediately after SET FILTER].
- Once active, every navigation/scan command (`SKIP`, `GO TOP`/`BOTTOM`,
  `LOCATE`, `LIST`, `SUM`, scoped commands) sees only passing records; excluded
  records are invisible to the walk. `EOF()`/`BOF()` reflect the *filtered* view.
- `RECNO()` still returns the **physical** record number of the current
  (passing) record.
- The filter `<condition>` is re-evaluated per record during navigation. Type
  rules apply: a mismatched-type condition raises "Data type mismatch." or "Not a
  Logical expression." (`DBASE.MSG` lines 9, 36).
- Each work area has its own filter; switching work areas does not clear it.
- `SET DELETED` and `SET FILTER` compose: a record is visible only if it passes
  the filter AND is not hidden by `SET DELETED ON`.

`See also:` MODIFY QUERY, SET DELETED, SET FIELDS.

---

## SET DELETED

Control whether records marked for deletion are visible to commands.

**Syntax** [verified: HELP.DBS `_SET ON/OFF 2` + ASSIST table]:

```
SET DELETED ON | OFF       && default OFF
```

**Semantics:**

- **Default is OFF** [verified: HELP.DBS lists default as lowercase `on/OFF` ->
  OFF is default; Harbour `_SET_DELETED` "disabled, which is the default"].
  Note the HELP one-liner phrasing "Ignores all records marked for deletion"
  describes what ON does.
- `SET DELETED ON`  --  records marked for deletion (via `DELETE`) are **ignored**:
  they become invisible to navigation (`SKIP`, `GO TOP/BOTTOM`), scans (`LIST`,
  `LOCATE`, `SUM`, `AVERAGE`, `COUNT`, `REPORT`, `LABEL`), and `RECCOUNT()`-bounded
  walks. The pointer skips over them.
- `SET DELETED OFF` (default)  --  deleted records are **included** in all
  operations; they appear in `LIST`/`DISPLAY` output with a leading `*` and
  `DELETED()` returns `.T.` for them [verified: HELP.DBS `_DELETE`: "an asterisk
  (*) preceding DISPLAY/LIST output, and by testing with the DELETED() function"].

**III+ delta / interaction notes:**

- `SET DELETED` never affects **direct** `GO <expN>` to a physical record number
  in Clipper/III+  --  `GO` can land on a deleted-and-hidden record even with
  `SET DELETED ON` (it is direct positioning, not a scan). `[oracle-resolves]`
  whether III+ matches Clipper here.
- `DELETED()` reflects the deletion flag of the **current** record regardless of
  the `SET DELETED` switch [verified: Harbour `Deleted()`]. With `SET DELETED ON`
  you normally never land on a deleted record via scans, so `DELETED()` is
  usually `.F.` during a filtered walk.

`See also:` DELETE, RECALL, PACK, DELETED().

---

## SET RELATION TO ... INTO

Link the active (parent) database to another open (child) database so that
moving the parent pointer automatically repositions the child.

**Syntax** [verified: HELP.DBS `_SET RELATION`]:

```
SET RELATION TO [<key expression> | RECNO() | <expN>] INTO <alias>
SET RELATION TO                                    && removes relation(s)
```

- `<alias>`  --  the child file, identified by its alias / work-area name. The child
  must already be open in another work area.
- **Keyed relation:** `SET RELATION TO <key expression> INTO <alias>`  --  the child
  **must be indexed** on a key compatible with `<key expression>`; the link is by
  index key. Each time the parent pointer moves, III+ evaluates `<key
  expression>` in the parent and `SEEK`s it in the child's controlling index,
  positioning the child to the first matching key (or to EOF / `FOUND()` `.F.` in
  the child if no match).
- **Record-number relation:** `SET RELATION TO RECNO() INTO <alias>` or
  `SET RELATION TO <expN> INTO <alias>`  --  the child is positioned by **record
  number** (the value of `RECNO()`/`<expN>` from the parent). The child need NOT
  be indexed in this case [verified: HELP.DBS "must be indexed unless RECNO() or
  another numeric expression is used. In this case, the files are linked by record
  number."].
- `SET RELATION TO` with nothing after `TO` **removes the relation(s)** from the
  currently SELECTed (parent) work area [verified: HELP.DBS].

**Semantics & pointer effects:**

- The relation is **one-directional, parent -> child.** Moving the parent pointer
  (`SKIP`, `GO`, `LOCATE`, `SEEK`, scan in the parent) re-positions the child.
  Moving the child pointer directly does NOT move the parent.
- Multiple relations from one parent into several children are allowed; relations
  may chain (parent -> child -> grandchild). A relation that loops back raises
  **"Cyclic relation."** (`DBASE.MSG` line 42) [documented: DBASE.MSG].
- After a parent move with a keyed relation, the child's `FOUND()`/`EOF()`
  reflect whether the key was found in the child. Test `<alias>->FOUND()` (alias
  arrow) or `SELECT`-then-test for child match. Reading an unmatched child field
  yields a blank/empty value [oracle-resolves: exact unmatched-child field value].
- `SET RELATION` is the documented exception to "commands move only the active
  file's pointer"  --  relations propagate the move [verified: HELP.DBS `_SELECT`
  NOTE: "commands that move the record pointer ... work only on the currently
  SELECTed ... database file, **unless a RELATION is SET.**"].

`See also:` SELECT, USE, SET FIELDS, SET VIEW (a `.vue` can establish relations).

---

## SET FIELDS

Define a restricted, possibly multi-file, list of accessible fields.

**Syntax** [verified: HELP.DBS `_SET FIELDS` + ASSIST]:

```
SET FIELDS TO [<field list> | ALL]
SET FIELDS ON | OFF           && default OFF
```

- `SET FIELDS TO <field list>`  --  restricts the default field set (the columns
  that `LIST`/`DISPLAY`/`EDIT`/`BROWSE` show and that bare expressions resolve to)
  to the named fields. The list may name fields **from more than one open file**
  (qualify with `alias->field`), redefining the per-file default FIELDS list
  [verified: HELP.DBS "Defines a list of fields from one or more files"].
- `SET FIELDS TO ALL`  --  include all fields of the active database's structure.
- `SET FIELDS TO` (no list)  --  empties the SET FIELDS list (see also `CLEAR
  FIELDS`).
- Consecutive `SET FIELDS TO <list>` are **additive**  --  each adds to the existing
  list rather than replacing it [verified: HELP.DBS "Consecutive SET FIELDS TO are
  done in an additive fashion."]. To clear, use `SET FIELDS TO` / `CLEAR FIELDS`.
- `SET FIELDS ON | OFF`  --  toggles whether the field list is **respected (ON)** or
  **ignored (OFF, default)** [verified: HELP.DBS "determines whether or not the
  field list is respected or ignored"]. Note: `SET FIELDS TO <list>` defines the
  list but you typically also need `SET FIELDS ON` for it to take effect (the TO
  form may turn it ON implicitly  --  `[oracle-resolves]` whether `SET FIELDS TO
  <list>` auto-enables the ON switch in III+).

**Semantics:** affects which fields are the implicit/default set for commands
that omit an explicit field list, and is a key building block of `SET VIEW`
environments. Does not move the pointer.

`See also:` CLEAR FIELDS, SET VIEW, SET RELATION.

---

## LIST / DISPLAY (records)

`LIST` and `DISPLAY` produce the same selective record output; they differ only
in **paging**: `DISPLAY` pauses periodically and prompts for a key press; `LIST`
scrolls continuously without pausing [verified: ASSIST `_DISPLAY` "pauses
periodically", `_LIST` "does not pause"; HELP.DBS `_DISPLAY` "lists with periodic
pauses"].

**Syntax** [verified: HELP.DBS `_LIST`/`_DISPLAY` + ASSIST]:

```
LIST    [<scope>] [<expression list>] [FOR <condition>] [WHILE <condition>] [OFF] [TO PRINT]
DISPLAY [<scope>] [<expression list>] [FOR <condition>] [WHILE <condition>] [OFF] [TO PRINT]
```

- `<expression list>`  --  comma-separated expressions (field names and/or computed
  expressions such as `Cost * Rate`) selecting the columns. Default = all fields
  of the current record (subject to `SET FIELDS`). Memo fields are printed at
  `SET MEMOWIDTH` width (default 50) [verified: HELP.DBS `_SET MEMOWIDTH`].
- `OFF`  --  suppress the leading record-number column [verified: HELP.DBS].
- `TO PRINT`  --  echo output to the printer as well as the screen.
- `SET HEADINGS ON` (default ON) prints field-name/expression column headings;
  `SET HEADINGS OFF` suppresses them [verified: HELP.DBS `_SET ON/OFF 3`].

**Default scope  --  the key difference:**

- **`LIST` defaults to scope `ALL`**  --  bare `LIST` lists every record (and does
  an implicit `GO TOP` first). "Used alone, it displays all records."
  [verified: HELP.DBS `_LIST`].
- **`DISPLAY` defaults to the CURRENT record only**  --  bare `DISPLAY` shows just
  the current record. "Lists the current record." [verified: HELP.DBS `_DISPLAY`].
  Use an explicit scope (`DISPLAY ALL`, `DISPLAY NEXT 10`) to widen it.

**Pointer / state effects:**

- A bare `DISPLAY` (current record) does **not** move the pointer.
- `LIST` / `DISPLAY ALL` (or any multi-record scope) walks to the end and leaves
  the pointer at **EOF** (`EOF()` `.T.`) [verified: scoped-walk semantics +
  Harbour]. A `DISPLAY NEXT n` / `LIST WHILE` leaves the pointer just past the
  last record shown.
- Deleted records show a leading `*` when `SET DELETED OFF`; hidden when ON.
- Neither command changes `FOUND()`.

**Output presentation** [documented: HELP.DBS + DBASE.MSG row labels]: record
numbers are right-aligned under a `Record#`-style heading; logical fields print
as `T`/`F`; dates in `SET DATE` format; numerics right-aligned at field width.

---

## LIST/DISPLAY STRUCTURE

**Syntax** [verified: HELP.DBS `_LIST STRUCTURE`/`_DISPLAY STRUCTURE` + ASSIST]:

```
LIST STRUCTURE    [TO PRINT]
DISPLAY STRUCTURE [TO PRINT]
```

Shows the structure of the active database file: file name, number of records,
date of last update, complete field descriptions (name, type, width, dec), and
the total bytes per record [verified: ASSIST `_LIST STRUCTURE`]. `DISPLAY
STRUCTURE` pauses periodically; `LIST STRUCTURE` does not. Does not move the
pointer. Reported record length is the on-disk record length **including** the
1-byte deletion flag (so it is sum-of-field-widths + 1) [inferred from DBF
layout; confirm against a golden `.dbf` header rlen].

---

## LIST/DISPLAY STATUS

**Syntax** [verified: HELP.DBS `_LIST STATUS`/`_DISPLAY STATUS` + ASSIST]:

```
LIST STATUS    [TO PRINT]
DISPLAY STATUS [TO PRINT]
```

Lists information about open files and system parameters: for each work area the
open database, its alias, open index files and key expressions, the master
index, the active filter, relations, plus the current SET states, default drive,
SET DATE/DECIMALS, function-key assignments, etc. [verified: HELP.DBS "open files
and system parameters"; exact column/layout is `[oracle-resolves]`  --  needs a
golden capture under DOSBox]. `DISPLAY STATUS` pages; `LIST STATUS` does not.

---

## LIST/DISPLAY MEMORY

**Syntax** [verified: HELP.DBS `_LIST MEMORY`/`_DISPLAY MEMORY` + ASSIST]:

```
LIST MEMORY    [TO PRINT]
DISPLAY MEMORY [TO PRINT]
```

Lists the **name, type, and size** of the active memory variables [verified:
HELP.DBS]. Distinguishes PUBLIC / PRIVATE scope and shows the value. III+ has a
fixed memvar pool (see "Out of memory variable memory."/"...slots." in
`DBASE.MSG` lines 21-22). `DISPLAY MEMORY` pages; `LIST MEMORY` does not.

---

## LIST/DISPLAY HISTORY

**Syntax** [verified: HELP.DBS `_LIST HISTORY`/`_DISPLAY HISTORY` + ASSIST]:

```
LIST HISTORY    [LAST <expN>] [TO PRINT]
DISPLAY HISTORY [LAST <expN>] [TO PRINT]
```

Lists the commands stored in HISTORY mode in chronological order. `LAST <expN>`
limits output to the most recent `<expN>` lines; without it, all retained lines
are listed [verified: HELP.DBS]. The retained count is governed by `SET HISTORY
TO <n>` (default 20) and capture is gated by `SET HISTORY ON`/`SET DOHISTORY`
[verified: HELP.DBS `_SET HISTORY`]. `DISPLAY HISTORY` pages; `LIST HISTORY` does
not.

---

## LIST/DISPLAY FILES

**Syntax** [verified: HELP.DBS `_LIST FILES`/`_DISPLAY FILES` + ASSIST]:

```
LIST FILES    [LIKE <skeleton>] [TO PRINT]
DISPLAY FILES [LIKE <skeleton>] [TO PRINT]
```

Lists files on the directory. `LIKE <skeleton>` filters by a DOS-style wildcard
skeleton (`?` = any single char, `*` = any run) [verified: HELP.DBS + `_Skeleton`].
**Default (no LIKE): only database (`.dbf`) files are shown** [verified: ASSIST
`_DIR` "Unless otherwise specified by the skeleton, only database files are
shown."]. `DISPLAY FILES` pages; `LIST FILES` does not. Related: the standalone
`DIR [<drive>:] [<path>] [<skeleton>]` command (HELP.DBS `_DIR`).

---

## ? / ?? (output)

The general expression-output command  --  "the way to ask dBASE a question and have
the answer displayed" [verified: HELP.DBS `_?/??`].

**Syntax** [verified: HELP.DBS `_?/??`]:

```
?  [<expression list>]
?? [<expression list>]
```

- `?` outputs a **carriage-return/line-feed first**, then the evaluated
  expression list, advancing to a new line  --  i.e. it prints on the next line.
- `??` outputs the expression list **at the current cursor/print position** with
  no leading CR/LF.
- `<expression list>`  --  comma-separated expressions of **any** type. Multiple
  expressions are separated on output by a single space [oracle-resolves: exact
  inter-expression separator width]. Each expression is converted to its display
  form (numbers right-formatted per `SET DECIMALS`/`SET FIXED`, dates in `SET
  DATE` format, logicals as `.T.`/`.F.`, character as-is).
- A bare `?` with no expression list outputs a single blank line (CR/LF).
- Output honours `SET PRINT ON` (also to printer), `SET CONSOLE`, `SET DEVICE`,
  `SET MARGIN`, and `SET ALTERNATE` (captured to the alternate file).

**III+ type rule (delta):** `?` does **not** auto-stringify a type-mismatched
expression. `? "Total: " + Amount` where `Amount` is numeric is a **"Data type
mismatch."** error in III+  --  you must write `? "Total: " + STR(Amount,10,2)`.
The dbase.com/Plus auto-conversion behavior must NOT be implemented. [verified:
HELP.DBS `_Expressions` type rule + DBASE.MSG vs OPS_PLUS.md delta].

**Real III+ idiom** (`Sample_Programs_and_Utilities/FLT_INFO.PRG:17`,
`NUMWORDS.PRG`): explicit string-build with `STR()`/`LTRIM()`/`+` between same
types; never `C + N`.

`See also:` @...SAY (positioned output), TEXT...ENDTEXT, SET PRINT, SET DEVICE.

---

## REPORT FORM

Run a pre-designed columnar report from a `.frm` report-form file. (The `.frm`
*file layout* is owned by the reports-labels spec; this section documents the
*command* that runs it.)

**Syntax** [verified: HELP.DBS `_REPORT` + ASSIST `_REPORT FORM`]:

```
REPORT FORM <form file> | ?
   [<scope>]
   [FOR <condition>]
   [WHILE <condition>]
   [PLAIN]
   [HEADING <expC>]
   [NOEJECT]
   [TO PRINT]
   [TO FILE <file>]
   [SUMMARY]
```

- `<form file>`  --  the `.frm` report form (default extension `.frm`); `?` lists
  cataloged report forms [verified: HELP.DBS "Use ? to display a list of
  available form files."].
- **Default scope is `ALL`** (implicit `GO TOP`, all records) [verified: ASSIST +
  scope default]. Restrict with `<scope>`/`FOR`/`WHILE`.
- `PLAIN`  --  suppress page numbers, the date, and the repeating column headings on
  each page (a continuous plain listing) [documented: HELP.DBS].
- `HEADING <expC>`  --  an extra character heading printed alongside the form's own
  page title [documented: HELP.DBS/ASSIST].
- `NOEJECT`  --  suppress the initial form-feed/page eject before the first page
  [documented: ASSIST].
- `TO PRINT`  --  send to the printer.
- `TO FILE <file>`  --  send report text to a disk file (default extension `.txt`)
  [documented: ASSIST/HELP.DBS].
- `SUMMARY`  --  print only group subtotals/totals, omitting the detail lines
  [documented: HELP.DBS `_REPORT` syntax includes `[SUMMARY]`]. (Note: ASSIST
  menu help omits SUMMARY; HELP.DBS includes it  --  HELP.DBS is the dot-prompt
  authority, so SUMMARY is valid at the dot prompt.)

**Pointer / state effects:** walks the scope to the end, leaving the pointer at
**EOF** [verified: ALL-scope walk semantics]. Honours `SET DELETED`, `SET FILTER`,
`SET FIELDS`, and any active controlling index (records are reported in index
order). `FOR`/`WHILE` and group breaks (defined in the `.frm`) interact: totals
break on the form's group key. Does not change `FOUND()`.

`See also:` CREATE/MODIFY REPORT, LABEL FORM, the reports-labels `.frm` spec.

---

## LABEL FORM

Run a pre-designed mailing-label layout from a `.lbl` label file.

**Syntax** [verified: HELP.DBS `_LABEL` + ASSIST `_LABEL FORM`]:

```
LABEL FORM <label file> | ?
   [<scope>]
   [FOR <condition>]
   [WHILE <condition>]
   [SAMPLE]
   [TO PRINT]
   [TO FILE <file>]
```

- `<label file>`  --  the `.lbl` label form (default extension `.lbl`); `?` lists
  cataloged label forms [verified: HELP.DBS `_LABEL`].
- **Default scope is `ALL`** (implicit `GO TOP`) [verified: scope default].
- `SAMPLE`  --  print a row of sample labels (rows of `*`/test labels) so the
  operator can align the printer before the real run [documented: ASSIST `_LABEL
  FORM` `[SAMPLE]`].
- `TO PRINT`  --  send to printer. `TO FILE <file>`  --  send label text to a disk file
  [verified: HELP.DBS].

**Pointer / state effects:** identical walk semantics to REPORT FORM  --  runs the
scope to EOF, honours `SET DELETED`/`SET FILTER`/`SET FIELDS`/index order, does
not change `FOUND()`.

`See also:` CREATE/MODIFY LABEL, REPORT FORM, the reports-labels `.lbl` spec.

---

## Aggregates: SUM / AVERAGE / COUNT

Conditional aggregation over the active database. All three are scoped scan
commands sharing the FOR/WHILE/scope grammar; all default scope **ALL** (implicit
`GO TOP`) and leave the pointer at **EOF** after running.

### SUM

**Syntax** [verified: HELP.DBS `_SUM` + ASSIST]:

```
SUM [<expN list>] [<scope>] [FOR <condition>] [WHILE <condition>] [TO <memvar list>]
```

- `<expN list>`  --  the numeric expressions/fields to total. **Default: all numeric
  fields** of the active database are summed [verified: HELP.DBS/ASSIST "Totals
  expressions involving numeric fields"].
- `TO <memvar list>`  --  store the resulting totals into memory variables
  (one memvar per summed expression, in order) [verified: HELP.DBS].
- Each summed expression must be numeric; a non-numeric expression raises "Not a
  numeric expression." (`DBASE.MSG` line 27). Numeric overflow during summation
  raises "Numeric overflow (data was lost)." (`DBASE.MSG` line 38).
- With `SET TALK ON` (default ON), the totals are echoed to the screen as
  `summed` results [documented: HELP.DBS + DBASE.MSG verb "summed" line 199].

### AVERAGE

**Syntax** [verified: HELP.DBS `_AVERAGE` + ASSIST]:

```
AVERAGE [<expN list>] [<scope>] [FOR <condition>] [WHILE <condition>] [TO <memvar list>]
```

- Computes the **arithmetic mean** of numeric expressions. **Default: all numeric
  fields** of the active database are averaged unless limited by `<expN list>`
  [verified: HELP.DBS `_AVERAGE`].
- `TO <memvar list>` stores the means into memvars [verified: HELP.DBS].
- Average is over the count of records *in scope that passed FOR/WHILE/filter/
  deletion*  --  not `RECCOUNT()` [inferred from aggregate semantics; standard xBase].
- TALK verb "averaged" (DBASE.MSG line 192).

### COUNT

**Syntax** [verified: HELP.DBS `_COUNT` + ASSIST]:

```
COUNT [<scope>] [FOR <condition>] [WHILE <condition>] [TO <memvar>]
```

- Counts the number of in-scope records meeting the condition. (COUNT takes no
  expression list  --  it counts records, not field values.)
- `TO <memvar>`  --  store the tally into a single memory variable [verified:
  HELP.DBS].
- Counts only records visible under `SET DELETED`/`SET FILTER` that pass
  FOR/WHILE.

**All three:** honour `SET DELETED`, `SET FILTER`, controlling index order; do
not alter `FOUND()`; leave pointer at EOF after an ALL-scope run.

`See also:` TOTAL (creates a summary `.dbf`; data-modify spec), the functions
spec for STR()/numeric formatting of stored results.

---

## Pointer-state functions

These functions report the result of the navigation/search commands above. Each
work area maintains its own copies; precede with `<alias>->` to test an
unselected area [verified: Harbour `Bof()`/`Found()` "applies to the currently
selected database unless preceded by an alias"].

| Function | Returns | Set/cleared by |
|---|---|---|
| `RECNO()` | physical record number of current record; `LASTREC()+1` at EOF; on empty file returns 1 | every pointer move (GO/SKIP/LOCATE/SEEK/FIND/append) |
| `EOF()` | `.T.` when pointer is **after** the last logical record | SKIP-past-end, failed SEEK/FIND/LOCATE, end of a scan; cleared by any move landing on a valid record |
| `BOF()` | `.T.` when pointer is **before** the first logical record (attempted) | backward SKIP past record 1; cleared by forward move onto a valid record |
| `FOUND()` | `.T.` if the **last** `SEEK`/`FIND`/`LOCATE`/`CONTINUE` succeeded | set only by those four commands; **not** changed by GO/SKIP/DISPLAY/LIST/aggregates |
| `DELETED()` | `.T.` if the current record's deletion flag is set | DELETE / RECALL on the current record; independent of `SET DELETED` |
| `RECCOUNT()` | total records in the active `.dbf` (ignores filter/deleted) | append/PACK/ZAP |

[verified: HELP.DBS `_Database Functions` + Harbour `rddmisc.txt` Bof/Eof/Found/
RecNo/Deleted descriptions]

**Key invariants the reimplementation must honour:**

- A **successful** search (`SEEK`/`FIND`/`LOCATE`/`CONTINUE` with a match):
  `FOUND()` `.T.`, `EOF()` `.F.`, pointer on the match.
- A **failed** search: `FOUND()` `.F.`, `EOF()` `.T.`, pointer at EOF.
- `EOF()` and `FOUND()` are **distinct**: after a failed search both `EOF()`
  `.T.` and `FOUND()` `.F.`; but after a normal `LIST ALL` walk to the end,
  `EOF()` is `.T.` while `FOUND()` retains whatever the last search left.
  The III+ samples test either after a search interchangeably because both are
  conclusive on failure (`CHECK.PRG` tests `FOUND()`; `SHOW_FLT.PRG`/`FLT_RSVE.PRG`
  test `EOF()` after `FIND`).
- On an **empty** file: `EOF()` and `BOF()` are both `.T.`; `RECNO()` returns 1.

---

## Open questions (need the real interpreter under DOSBox) -> GAPS.md

1. **SKIP at EOF / BOF:** does a forward `SKIP` while already `EOF()` raise "End
   of file encountered." (DBASE.MSG 4) or silently no-op? Same for `SKIP -1` at
   `BOF()` -> "Beginning of file encountered." (DBASE.MSG 37). `[oracle-resolves]`
2. **GO to a hidden record:** with `SET DELETED ON` or an active `SET FILTER`,
   does `GO <expN>` land on the excluded record (Clipper does) and what are
   `EOF()`/`DELETED()` immediately after? `[oracle-resolves]`
3. **SET FILTER pointer state:** exact pointer position immediately after
   `SET FILTER TO <cond>` when the current record fails the filter  --  does it stay
   put (must `GO TOP`) or auto-advance? `[oracle-resolves]`
4. **CONTINUE and WHILE/scope:** confirm CONTINUE re-applies ONLY the FOR and
   neither the original `<scope>` nor `WHILE` (the prompt's stated rule). HELP.DBS
   wording supports FOR-only; confirm the WHILE non-reapplication and what happens
   if the original LOCATE used `NEXT n`/`REST`. `[oracle-resolves]`
5. **SET FIELDS TO auto-enable:** does `SET FIELDS TO <list>` implicitly turn the
   ON switch on, or must you also issue `SET FIELDS ON`? `[oracle-resolves]`
6. **`?` inter-expression separator:** exact spacing between multiple
   expressions in one `?`/`??` list. `[oracle-resolves]`
7. **DISPLAY STATUS layout:** exact field/column layout of LIST/DISPLAY STATUS
   output (capture a golden under DOSBox). `[oracle-resolves]`
8. **SKIP ... IN <alias>:** whether the III+ parser accepts the `IN <alias>`
   clause on SKIP (HELP.DBS lists only `SKIP [<expN>]`). `[oracle-resolves]`
9. **Empty-file RECNO():** confirm `RECNO()` returns 1 (not 0) at EOF of an empty
   file in III+. `[oracle-resolves]`
10. **Keyed-relation unmatched child:** field values read from a related child
    when the parent key has no child match  --  blanks vs. last-positioned record.
    `[oracle-resolves]`

---

## Sources

- `archive/golden-mined/HELP.DBS.strings.txt`  --  III+ dot-prompt HELP system
  (period-exact command syntax `5...`, descriptions `6...`, see-also `9...`).
  Primary syntax authority. Sections cited by `_<topic>` marker.
- `archive/golden-mined/ASSIST.HLP.strings.txt`  --  III+ ASSIST menu-mode help
  (period-exact, `#...#` highlight markup). Cross-checks command formats.
- `archive/golden-mined/HELP_topics.txt`  --  406-topic completeness oracle.
- `archive/golden-mined/DBASE.MSG.strings.txt`  --  error/message catalog (cited by
  line number, e.g. line 5 "Record is out of range.", line 40 "CONTINUE without
  LOCATE.", line 157 "End of LOCATE scope").
- `archive/golden-mined/FIXTURE_MAP.txt`  --  golden `.dbf` structures used by the
  sample programs (CHECKS, AVAL_FLT, CUSTOMER, etc.).
- `archive/dbase-com-help/SET_EXACT.md`, `OPS_COMPARISON.md`, `OPS_PLUS.md`  -- 
  comparison/`SET EXACT` core (III+-stable) AND the IV+/Plus deltas explicitly
  flagged wrong for III+ (`==`, auto-stringification of `C+N`).
- `oracles/harbour/doc/en/rddmisc.txt`  --  semantic oracle for `Bof()`/`Eof()`/
  `Found()`/`RecNo()`/`Deleted()`/`dbGoto()`/`dbSkip()` (Clipper lineage; NOT
  period-exact syntax).
- `oracles/harbour/doc/en/set.txt`  --  semantic oracle for `_SET_DELETED`,
  `_SET_EXACT`, `_SET_SOFTSEEK` (SOFTSEEK flagged as a later/non-III+ extension).
- Real III+ sample programs (idioms cited inline):
  `goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/`
  `CHECK.PRG` (SEEK+FOUND, GO TOP/SKIP/EOF loop),
  `EDITVOID.PRG` (SEEK + `.NOT. FOUND()`),
  `CLRDEP.PRG` (LOCATE/CONTINUE/FOUND loop),
  `SHOW_FLT.PRG` / `FLT_RSVE.PRG` (FIND `&macro`, test EOF()),
  `RECONCIL.PRG` / `ADD.PRG` / `PROG.PRG` / `TAXCODES.PRG` / `CLEANUP.PRG`
  (GO TOP / SKIP walks),
  `FLT_INFO.PRG` (REPORT FORM invocation).

---

## Verification

Adversarial cross-check performed 2026-06-16 against the period oracles
(`HELP.DBS.strings.txt`, `ASSIST.HLP.strings.txt`, `DBASE.MSG.strings.txt`,
`HELP_topics.txt`) and the Harbour semantic oracle
(`oracles/harbour/doc/en/rddmisc.txt`, `set.txt`, `rdddb.txt`).

**Mechanical claims (author summary)  --  confirmed:**
- 58 code fences, even/balanced.
- Zero non-ASCII characters **inside any code fence** (verified by an
  awk fence-state scan). The em-dashes/`->` that the byte scan flags are all in
  Markdown prose, which is correct and intended.

**Syntax cross-check vs HELP.DBS `5...` command-format lines  --  all match:**
GO/GOTO (`[GO/GOTO [RECORD]] <expN>` / `GO/GOTO BOTTOM/TOP`), SKIP
(`SKIP [<expN>]`  --  HELP shows no `IN <alias>`, matching the doc's "not in the
III+ surface" caveat), LOCATE, CONTINUE (no clauses), SEEK (`<expression>`),
FIND (`<character string>`, no quotes), SET FILTER
(`[<condition>]/[FILE <file name>/?]`), SET DELETED (`on/OFF`, default OFF),
SET RELATION (`[<key expression>/RECNO()/<expN> INTO <alias>]`), SET FIELDS
(`TO [<field list>/ALL]` + `on/OFF`, additive), LIST/DISPLAY and the
STRUCTURE/STATUS/MEMORY/HISTORY/FILES sub-forms, ?/?? (`? <expression list>`),
REPORT FORM (`...[PLAIN] [HEADING <expC>] [NOEJECT] [TO PRINT] [TO FILE <file>]
[SUMMARY]`), LABEL FORM, SUM/AVERAGE/COUNT. The LIST-vs-DISPLAY paging/default-
scope split is corroborated by both HELP.DBS (`_LIST` "displays all records" vs
`_DISPLAY` "Lists the current record." / "lists with periodic pauses") and
ASSIST (`_LIST` "does not pause periodically" vs `_DISPLAY` "pauses
periodically").

**DBASE.MSG line numbers  --  all verified exact:** 4 "End of file encountered.",
5 "Record is out of range.", 9 "Data type mismatch.", 14 "No find.", 21-22
out-of-memvar messages, 26 "Database is not indexed.", 27 "Not a numeric
expression.", 36 "Not a Logical expression.", 37 "Beginning of file
encountered.", 38 "Numeric overflow (data was lost).", 40 "CONTINUE without
LOCATE.", 42 "Cyclic relation.", 157 "End of LOCATE scope", and the TALK verbs
192 "averaged" / 199 "summed".

**Semantics cross-check (Harbour)  --  confirmed:** `Found()` "test if the
previous SEEK, LOCATE, CONTINUE, or FIND operation was successful. Each work
area has its own Found() flag" (rddmisc.txt)  --  exactly the doc's FOUND() rule
(GO/SKIP do not touch it). `dbGoto(<recno>)` is record-number positioning
(supports "GO = physical record number"). `_SET_DELETED` default disabled
(= OFF). `_SET_SOFTSEEK` exists in Harbour but is **absent from the 406-topic
III+ index**  --  corroborates the doc's "SOFTSEEK is not in the III+ surface"
delta. `SET EXACT` IS topic 212 in the index (its use is legitimate).

**III+-vs-later deltas  --  verified correct as written:**
1. `==` does **not** exist in III+: HELP.DBS `_Relational Operators` lists exactly
   `< > = <> # <= >= $` with no `==`. (Doc's claim FIXED/confirmed.)
2. No `C + N` auto-stringification: HELP.DBS `_Expressions` requires same-type
   operands (sole exception date+/-number); a type clash is "Data type mismatch."
   (line 9). (Confirmed.)
No dBASE IV/5/PLUS-only construct (arrays/`DECLARE`, IV-only clauses) appears in
the document; nothing to remove.

**Fix applied (1 completeness fix):** The 406-topic index lists `STRING OP`
(topics 160/388) and HELP.DBS `_String Operators` documents the `+` **and `-`**
character-concatenation operators (with the III+-specific rule that `-` moves
the first string's trailing blanks to the end of the result). The original
"shared grammar" section enumerated relational and logical operators but
**omitted the string operators entirely**  --  a genuine in-scope gap given the
doc's own `<expression list>`/type-mismatch focus. Added a full String/Math/
Relational operator paragraph to the shared-grammar section, including a worked
`"AB   " - "CD"` -> `"ABCD   "` example and an explicit "no `==`" note.

**Completeness vs the help index:** With the string-operator addition, every
in-scope topic from `HELP_topics.txt` (GO/GOTO, GOTO, SKIP, LOCATE, CONTINUE,
SEEK, FIND, SET FILTER, SET DELETED, SET RELATION, SET FIELDS, DISPLAY, LIST,
DISPLAY/LIST FILES/HISTORY/MEMORY/STATUS/STRUCTURE, ?/??, REPORT, LABEL, SUM,
AVERAGE, COUNT, SCOPE, CONDITION, FIELD LIST, RELATIONAL OP, STRING OP) is
covered. Out-of-scope topics (USE/SELECT, INDEX/SORT, DELETE/RECALL/PACK/
REPLACE/APPEND/EDIT, BROWSE/CHANGE, .frm/.lbl layout) remain correctly
delegated to sibling specs.

**Residual `[oracle-resolves]` gaps (need the real interpreter under DOSBox;
tracked in Open questions -> GAPS.md):** SKIP-at-EOF/BOF error vs no-op; GO to a
hidden record; exact pointer state after SET FILTER TO; CONTINUE WHILE/scope
non-reapplication edge with NEXT/REST; SET FIELDS TO auto-enable of the ON
switch; `?` inter-expression separator width; DISPLAY STATUS column layout;
SKIP ... IN <alias> parser acceptance; empty-file RECNO()==1; keyed-relation
unmatched-child field value. None of these block the in-scope reference; they
are behavioral edge confirmations only.
