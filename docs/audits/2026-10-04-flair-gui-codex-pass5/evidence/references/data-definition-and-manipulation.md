# Data-Definition and Manipulation Commands (dBASE III PLUS 1.1)

Complete reference for the commands that define database structure and add /
change / remove / reorganize records, including the aggregate commands and the
index commands. Each entry gives full period-exact syntax (every clause),
semantics, record-pointer and index effects, defaults, and III+-specific
behavior.

## Status / confidence

- **Target = dBASE III PLUS 1.1 (1986).** Syntax in each entry is quoted from the
  mined III+ HELP corpus (the period authority), not from memory. The single
  biggest III-vs-later hazard appears below and is flagged inline:
  **`C + N` (character + numeric) is a *Data type mismatch* error in III+**, NOT
  the auto-stringification that the modern dbase.com `+` doc describes. Never
  import the modern `+` semantics into a III+ implementation.
- Two period sources agree on the command surface:
  - `archive/golden-mined/HELP.DBS.strings.txt`  --  the on-line HELP database. Its
    `` `5 `` lines are the **command-format authority** (every clause, every
    option). This is the most complete period source and is cited as
    `[HELP.DBS:<line>]`.
  - `archive/golden-mined/ASSIST.HLP.strings.txt`  --  the ASSIST (menu) command
    help, `#...#` = reverse-video highlight in the original. Cited as
    `[ASSIST.HLP:<line>]`. ASSIST exposes a *subset* (no JOIN/UPDATE/TOTAL/
    INSERT/ZAP/SEEK-via-menu), so where ASSIST and HELP.DBS agree the claim is
    `[verified: HELP.DBS + ASSIST.HLP]`.
- Semantics that the period help only summarizes (pointer motion, what PACK does
  to indexes, SDF/DELIMITED layout, soft-seek, etc.) are corroborated against the
  Clipper-lineage oracle `oracles/harbour/doc/en/*.txt`  --  **semantic, NOT
  period-exact-syntax** (Harbour drops UPDATE/JOIN/TOTAL as commands and adds
  `==`, arrays, etc.). Tagged `[verified: HELP.DBS + Harbour <file>]`.
- Idioms are cross-cited to the real III+ sample programs under
  `goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/`.
- `[oracle-resolves]` marks anything that needs the real DBASE.EXE under DOSBox to
  settle; collected in **Open questions**.

### Coverage against the 406-item HELP oracle (`HELP_topics.txt`)

In-scope items fully covered here: **APPEND** (incl. APPEND BLANK / APPEND FROM),
**APPEND FROM**, **AVERAGE**, **BROWSE** (brief; cross-link), **CHANGE**,
**COPY**, **COPY FILE**, **COUNT**, **CREATE**, **CREATE EXAMPLE**, **DELETE**,
**EDIT**, **EXPORT**, **IMPORT**, **INDEX**, **INSERT**, **JOIN**, **MODIFY
STRUCTURE**, **PACK**, **RECALL**, **REINDEX**, **REPLACE**, **SET INDEX**, **SET
ORDER**, **SORT**, **SUM**, **TOTAL**, **UPDATE**, **USE**, **ZAP**. Concept items
in scope and addressed inline: **FIELD NAME / FIELD TYPE / FIELD SIZE /
DATABASE NAME / DEFINING / TYPES** (under CREATE), **RECORD COPY** (COPY),
**RECORD DELETE** (DELETE/RECALL/PACK/ZAP), **INDEX/SORT** (INDEX/SORT),
**KEY FIELD** (INDEX), **SCOPE / FIELD LIST / CONDITION** (the shared clause
grammar, defined once below).

Out of scope (covered by sibling specs, cross-linked, not duplicated here):
SEEK/FIND/LOCATE/CONTINUE/GO/SKIP (navigation), DISPLAY/LIST/REPORT/LABEL
(retrieval/output), CREATE SCREEN/REPORT/LABEL/QUERY/VIEW + MODIFY * (designers),
all `SET` switches except the handful that change the semantics of commands here
(SET DELETED, SET SAFETY, SET CARRY, SET UNIQUE, SET EXACT, SET FIELDS, SET
FILTER, SET RELATION), CLOSE / SELECT / work areas. **Not fully documented here**
-> see GAPS.md: exact byte effects on the .DBF header (record count, last-update
date) for each mutator; these belong to the DBF-format spec.

---

## 0. Shared clause grammar (scope / FIELDS / FOR / WHILE / TO)

These clauses recur in almost every command below; defined once.

### `<scope>`
Selects which records the command visits. Exactly one of:

```
ALL                  all records in the file (the whole file)
NEXT <expN>          the current record and the next expN-1 (expN records total)
RECORD <expN>        only record number expN
REST                 the current record through end-of-file
```
`[verified: HELP.DBS:1842-1851 (SCOPE narrative) "RECORD <n> A single record. /
NEXT <n> <n> records beginning with the current record. / REST The records
beginning with the current record and ending with the bottom record. / ALL All of
the records in the database file."]`

- **Default scope is command-specific.** The period SCOPE help: "Each command with
  the scope option has a default scope of either the current database record or all
  database records." `[verified: HELP.DBS:1852-1853]`. The pattern in III+:
  full-file commands (LIST natural, COPY, SORT, COUNT, SUM, AVERAGE,
  DELETE-with-FOR) default to **ALL**; record-at-a-time commands
  (DELETE/RECALL/REPLACE with no FOR, EDIT, CHANGE) default to the **current record
  only**. `[verified: HELP.DBS:1852-1853 + HELP.DBS:1023]` "Only the current record
  is affected unless the scope and/or FOR/WHILE options are used."
- `NEXT <expN>` and `REST` start at the *current* record; `ALL` and a bare FOR
  (no explicit scope) effectively rewind to the top first. `[verified: Harbour
  rddmisc.txt scope semantics + HELP.DBS]`

### `FIELDS <field list>`
Comma-separated field names limiting which fields the command touches /
copies / edits. Default = all fields of the active structure (subject to a `SET
FIELDS TO` list if one is active). `[HELP.DBS:1442]`

### `FOR <condition>`
A logical expression evaluated per record; the command acts only on records for
which it is `.T.`. With `FOR` and no explicit scope the scope becomes ALL.

### `WHILE <condition>`
Logical expression evaluated per record; processing **stops at the first record
where it is `.F.`** (it does not skip and continue). WHILE is normally paired
with an index + SEEK/FIND to walk a contiguous key range. `[verified: HELP.DBS +
Harbour rddmisc.txt]`

### `TO <memvar list>` / `TO PRINT` / `TO FILE <file>`
Result destination  --  meaning is command-specific (memory variables for the
aggregates; printer/file for the output commands).

### III+ expression-type rule that governs all `WITH`/FOR/condition uses
- Operands must be **type-compatible**. The period EXPRESSION help is explicit:
  "Each part of an expression (except the operator) should be of the same type. The
  exception to this rule is the use of dates and numbers with some of the
  operators." `[verified: HELP.DBS:1733-1737]`  --  i.e. the ONLY cross-type
  exceptions are `date - date -> number` and `date +/- number -> date`; there is no
  `character +/- number` exception. `C + N` (character concatenated with a number)
  therefore raises **`Data type mismatch.`** `[verified: HELP.DBS:1733-1737 +
  DBASE.MSG line 9 + CLAUDE.md Law 3]`. To build a string from a number you must
  call `STR()`
  explicitly: real idiom `file1="Dep"+LTRIM(STR(myear,4,0))`
  `[YEAREND.PRG:52]`. **Do NOT** implement the modern dbase.com `+` rule (number
  -> string auto-conversion)  --  that is a dBASE PLUS/Visual-era behavior, wrong for
  III+. `[verified: archive/dbase-com-help/OPS_PLUS.md (modern, flagged wrong) vs
  CLAUDE.md Law 3]`
- `C + C` concatenates (the period STRING OP help: "+ and - Concatenation
  operators ... Concatenation is the act of joining two strings"); overflow raises
  `+ : Concatenated string too large.` `[verified: HELP.DBS:1816-1821 + DBASE.MSG
  line 74]`
- `C - C` concatenates "with all trailing blanks moved to the end of the combined
  string" (the classic dBASE "minus"); overflow -> `- : Concatenated string too
  large.` `[verified: HELP.DBS:1818-1821 + DBASE.MSG line 73 +
  archive/dbase-com-help/OPS_MINUS.md]`

---

## 1. CREATE  --  define a new database structure

```
CREATE <new file> [FROM <structure extended file>]
```
`[verified: HELP.DBS:548 + ASSIST.HLP:39]`

Defines the structure of a new `.DBF` and adds it to the directory (and to the
catalog if one is open). Interactive `CREATE <file>` enters the full-screen
structure editor; you define each field as **field name / field type / field
length / decimal places**, then III+ offers *"Input data records now? (Y/N)"*
`[documented: DBASE.MSG line 171]`.

### Database file name (DATABASE NAME concept)
- The `.DBF` **file name** is **up to 8 characters**, must begin with a letter, and
  may include letters, numbers, and the underscore (`_`); it may not contain
  blanks. (The `.DBF` extension is supplied automatically.) `[verified:
  HELP.DBS:1911-1917 "A FILE NAME may have up to 8 characters. It must begin with a
  letter and may include numbers and the underscore character (_). It may not
  contain blank spaces."]` This is the DOS 8.3 file-name limit; do not confuse it
  with the 10-character *field*-name limit below.

### Field definition rules (FIELD NAME / FIELD TYPE / FIELD SIZE)
- **Field name**: **up to 10 characters**, must begin with a letter, may include
  letters, numbers, and the underscore (`_`), and may not contain blanks; stored
  uppercased. `[verified: HELP.DBS:1906-1909 "A FIELD NAME may have up to 10
  characters. It must begin with a letter and may include letters, numbers, and the
  underscore character (_). It may not contain blank spaces."; oracle-resolves the
  exact validation message]`
- **Field types**  --  the III+ set is exactly **five**: Character, Numeric, Date,
  Logical, Memo (note: **no Float, no General/OLE, no autoincrement**  --  those are
  dBASE IV/5/PLUS):
  - `C` Character  --  "any printable character that can be entered from the keyboard,
    including blanks"; length 1..254.
  - `N` Numeric  --  "only digits, a decimal point and a sign"; total width incl. sign
    and decimal point, max 19; decimals 0..15. The FIELD WIDTH "includes the number
    of decimal places plus the decimal point and sign." `[HELP.DBS:1938-1939]`
  - `D` Date  --  date values "stored as numbers to allow calculations," displayed in
    the `SET DATE` format (default AMERICAN mm/dd/yy); fixed display width 8, stored
    `CCYYMMDD` on disk.
  - `L` Logical  --  true/false; "T and Y represent true, and F and N represent
    false"; fixed width 1.
  - `M` Memo  --  "free-form character data ... stored in a separate file" (the
    `.DBT`); fixed 10-byte block pointer in the `.DBF`. `[verified:
    HELP.DBS:1918-1930 (FIELD TYPE narrative) + DBASE.MSG lines 255-259
    "Character/Date/Logical/Memo/Numeric" + HELP.DBS:1921-1939 + DBF-format spec]`
- **Maximum record length** and **field count** are bounded; exceeding raises
  `Maximum record length exceeded.` `[documented: DBASE.MSG line 133]`. Exact
  caps (128 fields / 4000-byte record in III+) -> see DBF-format spec;
  `[oracle-resolves]` here.
- A structure with no valid fields raises `Structure invalid.` `[documented:
  DBASE.MSG line 32]`.

### CREATE FROM (structure-extended)
`CREATE <new file> FROM <sx file>` builds a structure from a *structure-extended*
database (the four-field file produced by `COPY STRUCTURE EXTENDED`, with fields
`FIELD_NAME`, `FIELD_TYPE`, `FIELD_LEN`, `FIELD_DEC`). `[verified: HELP.DBS:550 +
COPY STRUCTURE EXTENDED below + DBASE.MSG lines 244-247 field names]`. This is the
programmatic way to build a .DBF whose layout is computed at run time.

### Effects
- The new file becomes the **active database in the current work area** (USE'd
  exclusively). Record pointer is at EOF on a 0-record file. No index is created.
- `SET SAFETY ON` (the default) prompts before overwriting an existing .DBF of the
  same name: *"... already exists, overwrite it? (Y/N)"* `[documented: DBASE.MSG
  line 170 + HELP.DBS:1623]`.

`CREATE EXAMPLE` (HELP topic) is the tutorial walk-through of the above, not a
separate command.

---

## 2. MODIFY STRUCTURE  --  edit an existing structure

```
MODIFY STRUCTURE
```
`[verified: HELP.DBS (MODIFY CMDS group) + ASSIST.HLP:226]`

Opens the structure editor on the **currently active** database. Add / delete /
rename fields and change types and widths.

**III+ data-preservation hazard (period-documented):**
> "Information is automatically restored after changes are saved, but **some
> information is lost if changes to field names and field lengths are done at
> once. Do these changes separately.**" `[ASSIST.HLP:223]`

Mechanism: MODIFY STRUCTURE copies the old records into the rebuilt structure by
matching **field name**. If you rename a field *and* change its width in the same
save, the matcher cannot pair old->new and that column's data is dropped. The
safe idiom is two passes (rename, save; then re-enter and resize, save).
`[verified: ASSIST.HLP:223 + period A-T documentation behavior]`. MODIFY
STRUCTURE requires the file open **exclusively**; open indexes must be rebuilt
afterward (do `REINDEX` or re-`INDEX`). `[inferred from index-match invariant;
oracle-resolves the auto-reindex question]`.

---

## 3. USE  --  open / close the active database (+ its indexes)

```
USE [<database file>/?] [INDEX <index file list>] [ALIAS <alias name>]
```
`[verified: HELP.DBS:1185 + ASSIST.HLP:6]`

- Opens `<database file>.DBF` in the **currently SELECTed work area**, closing
  whatever database was already open there. `[HELP.DBS:1187]`
- `INDEX <index file list>` opens up to **seven** index (`.NDX`) files for this
  database; **the first one listed becomes the master (controlling) index** and
  determines record order and SEEK/FIND target. `[verified: HELP.DBS:1492 "Opens
  up to seven index files" + Harbour rddord.txt ordSetFocus semantics]`. Real
  idiom: `USE Aval_Flt INDEX Aval_Flt, Flt_No` `[FLIGHT.PRG:13]`, and the same
  file opened with a *different* master by reordering the list:
  `USE Aval_Flt INDEX Flt_no, Aval_Flt` `[FLT_RSVE.PRG:75]`.
- `ALIAS <name>` sets the work-area alias used in `alias->field` references and
  SELECT; default alias is the file name. Re-using an in-use alias raises
  `ALIAS name already in use.` `[documented: DBASE.MSG line 24]`.
- `USE` with **no file name** closes the active database (and its indexes/format)
  in the current work area. Real idiom: bare `USE` `[FLIGHT.PRG:16, MAINT.PRG]`.
- `USE ?` lists cataloged database files for selection (catalog open).
- Opening a non-.DBF raises `Not a dBASE database.`; opening with a stale index
  raises `Index file does not match database.` `[documented: DBASE.MSG lines
  15,19]`.
- Pointer after USE: **record 1** (or EOF if 0 records). With a master index,
  record 1 = first record in index order.
- Network: an exclusive open is required for ZAP / PACK / MODIFY STRUCTURE / INDEX
  / REINDEX; failing that raises `Exclusive open of file is required.` `[documented:
  DBASE.MSG line 107]`. **Caveat:** the mined III+ HELP corpus (`HELP.DBS`,
  `ASSIST.HLP`, `HELP_topics`) shows **no `EXCLUSIVE` keyword on `USE` and no `SET
  EXCLUSIVE`**  --  the `USE` syntax here is exactly `USE [<file>/?] [INDEX ...] [ALIAS
  ...]` with no EXCLUSIVE clause `[HELP.DBS:1185-1186]`. Historically III+
  networking (the dBASE ADMINISTRATOR / multi-user pack) added both `USE ...
  EXCLUSIVE` and `SET EXCLUSIVE ON`, but the single-user HELP corpus does not
  document them. `[oracle-resolves the exact USE...EXCLUSIVE token vs SET EXCLUSIVE
  in 1.1.]`

Cross-link: SELECT and the ten work areas -> see the navigation/work-area spec.

---

## 4. APPEND family  --  add records at end of file

```
APPEND [BLANK]
APPEND FROM <file> [FOR <condition>]
       [[TYPE] <file type>] /
       [DELIMITED [WITH BLANK/<delimiter>]]
```
`[verified: HELP.DBS:433 + ASSIST.HLP:77]`

### APPEND (interactive)
Adds one new record at the end and drops into the full-screen data-entry editor
(the same screen as EDIT / the active `.FMT` format file). On reaching the last
GET it offers another blank record; Ctrl-End / Esc exits. `[verified: HELP.DBS:437
+ ASSIST.HLP:75]`. With `SET CARRY ON`, the new record is pre-filled with the
previous record's field values. `[documented: HELP.DBS:1590 + HELP.DBS:750
"APPEND, SET CARRY"]`.

### APPEND BLANK (programmatic)
Adds **one** empty record at end-of-file, **does not** enter the editor, and
**moves the record pointer to the new record**. The universal III+ insert idiom is
`APPEND BLANK` then a series of `REPLACE`:

```
APPEND BLANK
REPLACE Date    WITH mdate
REPLACE Amt     WITH -mamt
REPLACE Paidfrom WITH option
```
`[CASH.PRG:103-106]` and `[CHECK.PRG:208-214]`. A blank record holds: C/M ->
spaces, N -> blank (treated as 0 by arithmetic), D -> blank date, L -> false.
`[verified: sample idiom + Harbour rdddb.txt dbAppend()]`.

### APPEND FROM  --  import records from another file
Copies records from `<file>` onto the end of the active database, matching by
**field name** (only fields whose names match the active structure are imported;
others ignored). `[verified: HELP.DBS:437 + Harbour dbsdf.txt/dbdelim.txt]`.

- `FOR <condition>`  --  append only source records satisfying the condition. (No
  `<scope>`/`WHILE` on APPEND FROM in III+.) `[HELP.DBS:434]`
- `[TYPE] <file type>`  --  the source is a **foreign format**, not a .DBF. III+
  TYPEs include `SDF`, `DELIMITED`, `DIF`, `SYLK`, `WKS`, `PFS`. The keyword
  `TYPE` is optional (just naming the type works). `[documented: HELP.DBS:435 +
  DBASE.MSG lines 112-118 DIF/SYLK error catalog]`.
- `SDF` (System Data Format): fixed-width text, one record per line, fields butted
  with no separators, lines terminated CR/LF, width taken from the structure.
  `[verified: Harbour dbsdf.txt]`.
- `DELIMITED` (the default text shape): each field comma-separated; character
  fields wrapped in `"..."`; leading/trailing spaces trimmed.
  - `DELIMITED WITH <delimiter>` uses `<delimiter>` instead of `"` around
    character fields (still comma-separated).
  - `DELIMITED WITH BLANK` uses a single space as the field separator and no
    string quoting. `[verified: HELP.DBS:436 + ASSIST.HLP:79 + Harbour
    dbdelim.txt]`.
- A type/structure mismatch on a DIF/SYLK source surfaces the DIF/SYLK error
  catalog entries. Pointer: ends on the last appended record (EOF semantics like
  APPEND BLANK on each). Open indexes ARE updated as records are appended.
  `[verified: HELP.DBS + Harbour]`.

Cross-link: PFS specifically -> IMPORT/EXPORT below.

---

## 5. INSERT  --  add a record at the current position

```
INSERT [BEFORE] [BLANK]
```
`[verified: HELP.DBS:743 + HELP_topics line 48]`  *(not exposed in ASSIST menus)*

Adds a record **before or after the current record** (default = after), shifting
the physical record numbers of all following records down by one, then enters the
full-screen editor. `INSERT BLANK` adds the blank record *without* entering the
editor. `[HELP.DBS:743]`.

III+ specifics / hazards:
- INSERT physically renumbers records (expensive on large files)  --  unlike APPEND
  which only touches EOF.
- INSERT is **invalid / refused when a master index is active in a way that
  conflicts with physical order**: the error `Cannot append in column order.`
  `[documented: DBASE.MSG line 143]` and `Record is not inserted.`
  `[DBASE.MSG line 25]` are the period failure modes. Practical III+ programs
  almost always use APPEND BLANK instead; INSERT is rare. `[inferred from sample
  corpus (zero INSERT uses) + DBASE.MSG; oracle-resolves exact index-active
  behavior of INSERT]`.
- `SET CARRY` affects INSERT the same way as APPEND. `[HELP.DBS:750]`.

---

## 6. BROWSE  --  full-screen multi-record editor (brief; cross-link)

```
BROWSE [FIELDS <field list>] [LOCK <expN>] [WIDTH <expN>]
       [FREEZE <field>] [NOFOLLOW] [NOAPPEND] [NOMENU]
```
`[verified: HELP.DBS:460 + ASSIST.HLP:96]`

Spreadsheet-style edit window over many records of the active database/view.
Clauses: `FIELDS` = which columns; `LOCK <expN>` = number of leftmost columns
that stay frozen while panning; `WIDTH <expN>` = display width cap for character
fields; `FREEZE <field>` = restrict editing to one field; `NOFOLLOW` = when an
indexed key field is edited, stay on the screen row rather than chasing the record
to its new indexed position; `NOAPPEND` = disallow adding records; `NOMENU` =
suppress the Ctrl-Home option menu. `[verified: HELP.DBS:463 + ASSIST.HLP:96]`.

Full semantics, key map, and the editor's interaction with indexes/SET DELETED
-> see the **full-screen UI / editor** spec. (BROWSE shares the editing key map
shown in `DBASE.MSG.strings.txt` lines 281-323.)

---

## 7. EDIT / CHANGE  --  full-screen single-record editor (brief)

```
EDIT   [<scope>] [FIELDS <field list>] [FOR <condition>] [WHILE <condition>]
CHANGE [<scope>] [FIELDS <field list>] [FOR <condition>] [WHILE <condition>]
```
`[verified: HELP.DBS:659 (EDIT), HELP.DBS:486 (CHANGE) + ASSIST.HLP:84]`

`EDIT` and `CHANGE` are the **same command** (synonyms) in III+: full-screen
field editor starting at the current (or scoped/conditioned) record, one record
per screen, PgUp/PgDn between records. `[verified: HELP.DBS:665 "CHANGE" see-also
EDIT + HELP.DBS:491]`. Default scope = current record; with FOR/WHILE it walks the
matching records. Editing a key field while a master index is active can re-sort
the record (cf. BROWSE NOFOLLOW). Full key map / format-file (`SET FORMAT`)
interaction -> the **full-screen UI / editor** spec.

---

## 8. REPLACE  --  change field values without the editor

```
REPLACE [<scope>] <field1> WITH <exp1>
        [, <field2> WITH <exp2> ...] [FOR <condition>] [WHILE <condition>]
```
`[verified: HELP.DBS:1019 + ASSIST.HLP:103]`

Programmatic field assignment. **Only the current record** is changed unless a
`<scope>` and/or FOR/WHILE is given. `[HELP.DBS:1023]`. Multiple field/expression
pairs in one statement, comma-separated. `<expN>` must be **type-compatible** with
the target field (C->C, N->N, etc.); a mismatch raises `Data type mismatch.`
(again: `REPLACE Cfield WITH 5` is an error, not auto-stringified).
`[verified: HELP.DBS + DBASE.MSG line 9 + CLAUDE.md Law 3]`.

Real idioms  --  note III+ programs frequently write **one field per REPLACE
statement** even when several change together:
```
REPLACE Aseat_avl WITH Aseat_avl + mseats          && in-place arithmetic update
REPLACE Cno_seats WITH Cno_seats - mseats
```
`[FLT_RSVE.PRG:148,154]`; multi-field single statement is also valid (the comma
form). Mass update: `REPLACE ALL Amt WITH 0.00` `[RECONCIL.PRG:226]`.

### Critical III+ index hazard (period-documented)
> "NOTE: **Do not use the scope and FOR/WHILE options if the field being replaced
> is the key in an active index.** Please see the written documentation for more
> information." `[HELP.DBS:1024]`

Why: a scoped/conditional REPLACE walks records in **index order**; changing the
key value re-sorts the record mid-walk, so the cursor can skip records or process
records twice. The safe III+ pattern when you must rewrite a key field over many
records is to `SET ORDER TO 0` (drop to natural order) or close the index, do the
REPLACE ALL, then `REINDEX`. `[verified: HELP.DBS:1024 + index-invariant
reasoning; oracle-resolves the exact double/skip behavior]`.

- Open indexes whose key references a replaced field ARE updated on the single-
  record (default-scope) form.
- REPLACE does **not** move the record pointer beyond the records it processes
  (single-record form leaves you on the same record).
- See-also: `UPDATE` (batch cross-file REPLACE). `[HELP.DBS:1028]`.

---

## 9. DELETE / RECALL  --  mark and unmark records for deletion

```
DELETE [<scope>] [FOR <condition>] [WHILE <condition>]
RECALL [<scope>] [FOR <condition>] [WHILE <condition>]
```
`[verified: HELP.DBS:555 (DELETE), HELP.DBS:988 (RECALL) + ASSIST.HLP:112,118]`

`DELETE` sets the **deletion flag** (the leading byte of the record: `*` =
deleted, space = live). It does **not** remove data. `RECALL` clears the flag.
`[verified: HELP.DBS:556 + ASSIST.HLP:107]`.

- Default scope: **current record only** (no FOR). With FOR/WHILE the scope is the
  whole file unless restricted. Real idioms: `DELETE` (current) `[FLT_RSVE.PRG:156,
  EDITVOID.PRG:140]`; `DELETE ALL FOR YEAR(Date)=myear` `[YEAREND.PRG:66]`;
  `DELETE REST` `[CASH.PRG:339]`.
- A deleted record is recognized by `Del` in full-screen mode, by a `*` preceding
  DISPLAY/LIST output, and by the **`DELETED()`** function. `[HELP.DBS:559]`.
- **`SET DELETED ON`** makes deleted records *invisible* to subsequent record-
  visiting commands (LIST, REPLACE ALL, COPY, SUM, etc., and the record pointer
  skips them)  --  the default is `SET DELETED OFF`, so deleted records are still
  processed. `[verified: HELP.DBS:1603 + Harbour set.txt]`. This is the standard
  "soft delete then ignore" pattern.
- Record pointer: the single-record form does not move; scoped forms end past the
  last processed record.
- Deleted records still occupy disk and are still in the index until **PACK** (or
  ZAP). RECALL only works while the record physically exists. See-also DELETED(),
  PACK, RECALL/DELETE. `[HELP.DBS:563,994]`.

(Do not confuse with **DELETE FILE** / **ERASE**, which remove a *file* from
disk  --  those are file-management commands, cross-linked, not record deletion.)

---

## 10. PACK  --  physically remove deleted records

```
PACK
```
`[verified: HELP.DBS:935 + ASSIST.HLP:121]`

Permanently removes every record currently marked for deletion from the active
database, compacting the file, **and rebuilds/adjusts all open index files** so
they stay consistent. `[verified: ASSIST.HLP:120 "...and adjusts all open index
files" + HELP.DBS:936]`.

- Requires the file open **exclusively** (network). Failure -> `Exclusive open of
  file is required.` `[DBASE.MSG line 107]`.
- After PACK the record count in the .DBF header is reduced and the record pointer
  goes to the top (record 1). `[inferred + Harbour; oracle-resolves exact final
  pointer]`.
- Real idiom  --  the canonical archive-then-purge sequence:
  ```
  COPY TO &file1 FOR YEAR(Date)=myear     && save the year's rows
  DELETE ALL FOR YEAR(Date)=myear         && mark them
  PACK                                     && physically remove
  ```
  `[YEAREND.PRG:57,66,68]`.
- See-also DELETE, DELETED(), RECALL, **ZAP**. `[HELP.DBS:942]`.

---

## 11. ZAP  --  empty the file

```
ZAP
```
`[verified: HELP.DBS:1204 + HELP_topics line 327 (@ZAP)]`  *(not in ASSIST menu)*

Removes **all** records from the active database in one shot. Period definition:
> "Functionally, ZAP is equivalent to **DELETE ALL followed by PACK**."
> `[HELP.DBS:1206]`

- Truncates the .DBF to zero records and **empties all open index files**.
- `SET SAFETY ON` (default) prompts for confirmation before zapping. `[inferred
  from SAFETY semantics HELP.DBS:1623; oracle-resolves the exact ZAP prompt
  text]`.
- Requires exclusive open. Far faster than DELETE ALL + PACK because it does not
  scan/compact. Real idiom  --  reset the demo data files:
  ```
  USE Checks
  ZAP
  INDEX ON Chkno TO Chkno
  ```
  `[REINIT.PRG:31-33]`.

---

## 12. COPY family  --  duplicate data and structure

```
COPY TO <new file> [<scope>] [FIELDS <field list>] [FOR <cond>]
     [WHILE <cond>] [[TYPE] <file type>] /
     [DELIMITED [WITH BLANK/<delimiter>]]
COPY STRUCTURE [EXTENDED] TO <new file> [FIELDS <field list>]
```
`[verified: HELP.DBS:521 + ASSIST.HLP:217]`

### COPY TO  --  duplicate records
Writes all-or-part of the active database to `<new file>`. Default scope = ALL.
`[HELP.DBS:525]`.
- `FIELDS <list>` restricts copied columns; `FOR`/`WHILE`/`<scope>` restrict
  rows. Real idiom: `COPY TO &file1 FOR YEAR(Date)=myear` `[YEAREND.PRG:57]` (note
  macro `&file1` builds the target name at run time).
- With no TYPE/DELIMITED, the target is a new **.DBF** (a true structural copy of
  the selected fields). **A memo field is only copied to another .DBF**; copying
  to a text type drops/serializes memos.
- `[TYPE] <file type>` / `DELIMITED ...` export to a foreign/text file with the
  same SDF / DELIMITED / DIF / SYLK / WKS / PFS shapes described under APPEND FROM
  (COPY TO is the inverse of APPEND FROM). `[verified: HELP.DBS:526 + Harbour
  dbsdf.txt/dbdelim.txt]`. Example (Harbour-doc form, identical III+ syntax):
  `COPY TO overdue DELIMITED FOR ! Empty(accounts->duedate)` `[Harbour
  dbdelim.txt:56]`.
- `SET DELETED ON` excludes deleted rows from the copy; otherwise they are copied
  *including* their deletion flag. `[verified: HELP.DBS:1603 + Harbour]`.
- If nothing matches: `No fields were found to copy.` / `No fields to process.`
  `[documented: DBASE.MSG lines 134,45]`.
- Pointer: COPY TO does not change the source pointer's record number but ends at
  EOF after the scan; the new file is **not** opened (the source stays active).

### COPY STRUCTURE TO  --  empty clone
Creates a new **empty** .DBF with the same structure (optionally only `FIELDS
<list>`). `[verified: HELP.DBS:528 + Harbour dbstrux.txt:71]`. Use to spin up a
working/scratch file with the same layout.

### COPY STRUCTURE EXTENDED TO  --  structure as data
Creates a 4-field database whose **records describe** the active file's fields:
`FIELD_NAME` (C/10..11), `FIELD_TYPE` (C/1), `FIELD_LEN` (N), `FIELD_DEC` (N)  -- 
one record per source field. `[verified: HELP.DBS:529 + DBASE.MSG lines 244-247
"FIELD_NAME/FIELD_TYPE/FIELD_LEN/FIELD_DEC" + Harbour dbstrux.txt:121]`. Round-
trips with `CREATE <file> FROM <sx file>` to build/alter structures
programmatically. Note III+ uses `EXTENDED` *after* STRUCTURE; the field-name set
is the III+ form (`FIELD_LEN`/`FIELD_DEC`, not the IV `FIELD_DECIMAL`).

### COPY FILE  --  byte copy of any file
```
COPY FILE <source file> TO <target file>
```
`[verified: HELP.DBS:534 + ASSIST.HLP:260]`
Duplicates any file of any type at the OS level. **Both names must include the
extension.** Cannot copy a file that is open -> `Cannot erase a file which is
open.`-class refusal. `[HELP.DBS:535 + DBASE.MSG line 86]`. Used to copy
.NDX/.DBT/.FMT alongside a .DBF.

---

## 13. IMPORT / EXPORT  --  PFS:FILE bridge

```
IMPORT FROM <file> [TYPE] PFS
EXPORT TO   <file> [TYPE] PFS
```
`[verified: HELP.DBS:753 (IMPORT), HELP.DBS:688 (EXPORT) + ASSIST.HLP:282,286]`

- **IMPORT FROM ... PFS** reads a PFS:FILE database, **creates the corresponding
  dBASE III files** (a .DBF, plus a .FMT screen and a .VUE view matching the PFS
  form) and appends the data. `[verified: ASSIST.HLP:280 + HELP.DBS:754]`.
- **EXPORT TO ... PFS** writes the active database (using its current format file)
  out to a PFS:FILE. `[verified: ASSIST.HLP:284 + HELP.DBS:689]`.
- `PFS` is the only documented IMPORT/EXPORT type in III+ help; the keyword `TYPE`
  is optional. A bad source -> `Not a valid PFS file.` `[documented: DBASE.MSG
  line 136]`.
- These are distinct from `COPY TO ... TYPE`/`APPEND FROM ... TYPE`: IMPORT/EXPORT
  also generate the screen/view companions, whereas COPY/APPEND move data only.
  `[verified: ASSIST.HLP + HELP.DBS]`.

---

## 14. SORT  --  write a physically reordered copy

```
SORT TO <new file> ON <field1> [/A] [/C] [/D]
     [, <field2> [/A] [/C] [/D] ...] [<scope>]
     [FOR <condition>] [WHILE <condition>]
```
`[verified: HELP.DBS:1110 + ASSIST.HLP:211]`

Copies all-or-part of the active database to a **new physically ordered .DBF**
(SORT does not order in place; it produces a sorted copy). `[HELP.DBS:1113]`.

- `ON <field>` sort key(s); multiple fields = nested sort (first is primary).
- Per-field flags: **`/A`** ascending (default), **`/D`** descending, **`/C`**
  case-insensitive (ignore upper/lower). `/A` and `/C` may combine
  (`/AC`/`/A/C`). `[verified: HELP.DBS:1117 + ASSIST.HLP:212]`.
- `<scope>` / `FOR` / `WHILE` restrict which records are written; default = ALL.
- SORT keys are **field names only**, NOT arbitrary expressions (that is the key
  difference from INDEX, which takes a full expression). `[verified: HELP.DBS
  syntax "ON <field>" vs INDEX "ON <key expression>"]`.
- ASSIST always sorts ascending; the `/D` and `/C` flags are only reachable from
  the dot prompt. `[ASSIST.HLP:207]`.
- The new file is created but **not** opened; do `USE <new file>` to work with it
  `[ASSIST.HLP:208]`. Catalog (if open) is updated. `[HELP.DBS:1119]`.
- SORT physically renumbers records, so it is the right tool when you need a
  permanently ordered file for sequential/`TOTAL` processing; INDEX is preferred
  for live ordered *access* without rewriting the file.

---

## 15. INDEX ON / REINDEX / SET INDEX / SET ORDER  --  index management

### INDEX ON  --  create/open an index
```
INDEX ON <key expression> TO <index file> [UNIQUE]
```
`[verified: HELP.DBS:726 + ASSIST.HLP:204]`

- Builds an `.NDX` ordering the database by `<key expression>` and **leaves it
  open as the master index** of the current work area. `[HELP.DBS:727]`.
- `<key expression>` is a full dBASE expression (not just a field): e.g.
  `INDEX ON Chkno TO Chkno` `[CLEANUP.PRG, REINIT.PRG]`. Multi-key compound
  indexes are built by concatenating into one **character** expression with
  `STR()`/`DTOC()` (III+ has no multi-field index syntax). The key TYPE may be
  C, N, or D; a logical key is not allowed.
- **`UNIQUE`** keeps only the **first** record of each set of equal keys in the
  index (others still in the .DBF, just absent from this index). Equivalent to
  `SET UNIQUE ON` then INDEX. `[verified: HELP.DBS:729 + HELP.DBS:1635]`.
- **Index key length cap  --  source disagreement (`oracle-resolves`):** the error
  catalog contains BOTH `Index is too big (100 char maximum).` `[DBASE.MSG line
  23]` AND `Index expression is too big (220 char maximum).` `[DBASE.MSG line
  109]`. The 100 is the **evaluated key value** length cap; the 220 is the
  **key-expression source-text** length cap. This split needs the real
  interpreter to confirm. See Open questions.
- Interruptible build: `Index interrupted. Index will be deleted if not
  completed.` and a damaged index reports `Index damaged. REINDEX should be done
  before using data.` `[documented: DBASE.MSG lines 110,111]`.
- Max open indexes per work area = **7**; exceeding -> `Too many indices.`
  `[documented: DBASE.MSG lines 28 + HELP.DBS:1493]`.
- After INDEX, the record pointer is at the **top in index order**. The active
  database is required (`Database is not indexed.` is raised by SEEK/FIND with no
  index `[DBASE.MSG line 26]`).
- See-also REINDEX, SET INDEX, SET ORDER, SET UNIQUE. `[HELP.DBS:732]`.

### REINDEX  --  rebuild open indexes
```
REINDEX
```
`[verified: HELP.DBS:997 + HELP_topics line 79]`
Rebuilds **all open index files in the current work area** from their original key
expressions (the keys are stored in the .NDX header). `[HELP.DBS:998]`. Use after
bulk REPLACE of key fields, after restoring data, or when an index is reported
damaged. Real idiom: `REINDEX` `[ERRORPRG.PRG:37,39]`. See-also INDEX, SET INDEX,
SET UNIQUE. `[HELP.DBS:1001]`.

### SET INDEX TO  --  open existing indexes for the active database
```
SET INDEX TO [<index file list>/?]
```
`[verified: HELP.DBS:1492 + HELP_topics line 127]`
Opens up to **seven** existing `.NDX` files for the active database; the **first**
listed becomes the master/controlling index. `SET INDEX TO` with **no list closes
all index files** in the current work area (database stays open in natural order).
`SET INDEX TO ?` lists cataloged index files. `[HELP.DBS:1493]`. This is the
runtime equivalent of the `USE ... INDEX ...` clause. See-also CLOSE, SET ORDER,
USE. `[HELP.DBS:1497]`.

### SET ORDER TO  --  pick the controlling index without reopening
```
SET ORDER TO [<expN>]
```
`[verified: HELP.DBS:1519 + HELP_topics line 131]`
`<expN>` is the **1-based position** of an already-open index in the `USE ...
INDEX` / `SET INDEX TO` list; it becomes the master, **without** closing/reopening
the other indexes (all stay open and are still maintained). `SET ORDER TO 0`, or
`SET ORDER TO` with no number, drops to **natural (record-number) order** while
keeping every index open and updated. `[HELP.DBS:1526]`. See-also SET INDEX.

### Index maintenance invariant (applies to all mutators above)
While an index is open, **all** record changes (APPEND, REPLACE, DELETE/RECALL,
PACK, INSERT, BROWSE/EDIT edits) automatically maintain it. If a database is
changed while its index is **closed**, the index is stale and `USE` will reject it
with `Index file does not match database.`; fix with REINDEX (or re-INDEX).
`[verified: ASSIST.HLP:200 "dBASE III maintains the file in indexed order provided
that the index file is open whenever changes are made" + DBASE.MSG line 19]`.

---

## 16. The aggregates  --  SUM / AVERAGE / COUNT

```
SUM     [<expN list>] [<scope>] [FOR <condition>] [WHILE <condition>] [TO <memvar list>]
AVERAGE [<expN list>] [<scope>] [FOR <condition>] [WHILE <condition>] [TO <memvar list>]
COUNT   [<scope>] [FOR <condition>] [WHILE <condition>] [TO <memvar>]
```
`[verified: HELP.DBS:1132 (SUM), :450 (AVERAGE), :540 (COUNT) + ASSIST.HLP:183,
189, 195]`

- **SUM** totals numeric expressions over the scope; **AVERAGE** computes their
  arithmetic mean; **COUNT** tallies the matching records.
- Default scope = **ALL**; default `<expN list>` for SUM/AVERAGE = **every numeric
  field** in the structure. `[HELP.DBS:453 "All numeric fields ... are averaged
  unless limited by the numeric expression list."]`.
- `TO <memvar list>` stores the results into memory variables (one memvar per
  expression for SUM/AVERAGE; a single memvar for COUNT). Without TO, results
  print (subject to SET TALK). The TO memvars are **created if absent**.
- Real idioms:
  ```
  SUM Amt TO mamt1                              && one numeric field
  SUM Amt TO outstand FOR .NOT. Can             && conditional sum
  SUM Amt TO notclear FOR Num > 0
  ```
  `[CLEANUP.PRG:13, RECONCIL.PRG:22,26]`.
- `SET DELETED ON` excludes deleted records from all three. `SET FILTER`/`SET
  FIELDS` also constrain the records/fields seen. `[verified: HELP.DBS:1603 +
  Harbour]`.
- Non-numeric SUM/AVERAGE expression -> `Not a numeric expression.` `[documented:
  DBASE.MSG line 27]`. Numeric overflow during accumulation -> `Numeric overflow
  (data was lost).` `[documented: DBASE.MSG line 38]`.
- Pointer: each ends at EOF after the scan (full-file default).

---

## 17. TOTAL  --  summarize a sorted/indexed file into a new file

```
TOTAL ON <key field> TO <new file> [<scope>]
      [FIELDS <field list>] [FOR <condition>] [WHILE <condition>]
```
`[verified: HELP.DBS:1156 + HELP_topics line 96]`  *(not in ASSIST menu)*

Creates a **new .DBF** with one record per distinct value of `<key field>`; in
each output record the numeric `FIELDS` are **summed** across the group, and all
other fields take the value from the **first** record of the group. `[verified:
HELP.DBS:1159 + Harbour]`.

III+ requirements / hazards (period-documented):
- **The source MUST be SORTed or INDEXed on `<key field>` first**  --  TOTAL groups
  *consecutive equal keys*; an unordered file produces wrong groupings.
  `[HELP.DBS:1162]`.
- **The numeric fields must be wide enough to hold the group sums**  --  TOTAL does
  not widen them; an undersized field overflows silently / errors `Numeric
  overflow.` `[HELP.DBS:1163 + DBASE.MSG line 162]`.
- `FIELDS <list>` limits which numeric fields are summed (default = all numeric
  fields). `<scope>`/`FOR`/`WHILE` limit which source records contribute.
- Catalog (if open) updated with the new file. `[HELP.DBS:1164]`. The new file is
  created, not opened.

---

## 18. JOIN  --  Cartesian/conditional combine of two files

```
JOIN WITH <alias> TO <new file> FOR <condition> [FIELDS <field list>]
```
`[verified: HELP.DBS:759 + HELP_topics line 50]`  *(not in ASSIST menu)*

Builds a **new .DBF** by combining the active database with the database open in
work area `<alias>`. `[HELP.DBS:761]`.

Semantics (period + oracle):
- For **each** record of the active file, JOIN scans **every** record of
  `<alias>`; whenever `FOR <condition>` is `.T.`, it writes a combined record to
  `<new file>`. This is effectively a filtered Cartesian product (O(n*m))  --  it can
  produce a very large file. `[verified: HELP.DBS:761 + Harbour join semantics]`.
- `FIELDS <field list>` chooses the output columns (qualify cross-file names as
  `alias->field`); without it, fields from both files are included up to the .DBF
  field/width limits (`Maximum record length exceeded.` if too wide). `[verified:
  HELP.DBS:760 + DBASE.MSG line 133]`.
- **You cannot JOIN a file with itself** -> `Cannot JOIN a file with itself.`
  `[documented: DBASE.MSG line 135]`.
- Pointer: the active file's pointer advances through all its records (ends EOF);
  the `<alias>` pointer is moved internally. The new file is created, not opened.
- Catalog updated if open. `[HELP.DBS:763]`.
- In modern III+ code, `SET RELATION` + a report/COPY is usually preferred over
  JOIN's blow-up. `[inferred from corpus; JOIN unused in samples]`.

---

## 19. UPDATE  --  batch cross-file REPLACE by matching key

```
UPDATE ON <key field> FROM <alias>
       REPLACE <field> WITH <expression>
       [, <field2> WITH <exp2> ...] [RANDOM]
```
`[verified: HELP.DBS:1175 + HELP_topics line 98]`  *(not in ASSIST menu)*

Uses records from the database in work area `<alias>` (the "transaction" file) to
update the **active** database (the "master"). Whenever the `<key field>` values
match between the two files, the listed `REPLACE`s are applied to the active
file's record. `[HELP.DBS:1178]`.

III+ requirements (period-documented):
- **Before UPDATE you must SORT or INDEX BOTH files on `<key field>`.**
  `[HELP.DBS:1181]`. Without `RANDOM`, UPDATE walks both files in sorted order in a
  single merge pass (each transaction key matched once).
- **`RANDOM`**: the **active** (master) file must be indexed on `<key field>`, but
  the `FROM` (transaction) file may be in any order  --  UPDATE SEEKs each
  transaction record into the master. `[verified: HELP.DBS:1177 + Harbour update
  semantics]`.
- The `<expression>`s commonly reference the `<alias>` fields, e.g.
  `UPDATE ON Acct FROM Trans REPLACE Bal WITH Bal + Trans->Amt`. Expressions obey
  the III+ type rules (no C+N). `[inferred from REPLACE rules; oracle-resolves
  whether duplicate transaction keys accumulate or only the last applies]`.
- Pointer ends at EOF on the active file. See-also REPLACE. `[HELP.DBS:1182]`.

---

## Cross-cutting: record-pointer & index summary table

| Command            | Default scope     | Pointer after          | Open indexes |
|--------------------|-------------------|------------------------|--------------|
| CREATE             | n/a               | EOF (empty file)       | none created |
| USE <f>            | n/a               | rec 1 (or index top)   | opened/master = 1st |
| APPEND BLANK       | n/a               | the new record         | maintained   |
| APPEND FROM        | matched src rows  | last appended (EOF)    | maintained   |
| INSERT             | current+1         | the new record         | conflict possible |
| REPLACE (no FOR)   | current           | unchanged              | maintained   |
| REPLACE scoped     | per scope/FOR     | past last processed    | maintained (HAZARD if key) |
| DELETE/RECALL      | current (no FOR)  | unchanged / past last  | maintained (flag only) |
| PACK               | n/a               | top                    | rebuilt      |
| ZAP                | n/a               | EOF (empty)            | emptied      |
| COPY TO            | ALL               | EOF (source)           | source unchanged |
| SORT TO            | ALL               | EOF (source)           | new file unordered-open |
| INDEX ON           | n/a (full scan)   | index top              | new = master |
| REINDEX            | n/a               | (rebuilt) top          | rebuilt      |
| SUM/AVG/COUNT      | ALL               | EOF                    | read-only    |
| TOTAL/JOIN/UPDATE  | ALL/per FOR       | EOF (active)           | read; new file unopened |

`[verified where cited above; rows marked from HELP.DBS + Harbour; exact final
pointer for PACK/ZAP is oracle-resolves]`

---

## Open questions (need the real DBASE.EXE under DOSBox)

1. **Index key length: 100 vs 220.** `DBASE.MSG` carries both `Index is too big
   (100 char maximum).` and `Index expression is too big (220 char maximum).`
   Confirm that 100 = evaluated key-value length and 220 = source-expression
   length (and which one fires first). `[oracle-resolves; GAPS.md]`
2. **REPLACE on an active key field, scoped.** Reproduce the documented "do not
   scope a REPLACE on a key field" hazard: does the cursor skip records, process
   some twice, or error? Determine the exact failure. `[oracle-resolves]`
3. **INSERT with an active master index.** Does INSERT raise `Cannot append in
   column order.`, silently fall back to APPEND, or renumber-and-reindex? Confirm
   the `Record is not inserted.` trigger. `[oracle-resolves]`
4. **Final record pointer** after PACK and after ZAP (top vs EOF vs rec 1 on now-
   empty file). `[oracle-resolves]`
5. **MODIFY STRUCTURE auto-reindex**: does saving rebuild open indexes
   automatically, or must the user REINDEX? Also confirm the precise "rename +
   resize in one save loses data" boundary. `[oracle-resolves]`
6. **ZAP / CREATE-overwrite confirmation prompt text** under SET SAFETY ON (exact
   wording vs the generic `... already exists, overwrite it? (Y/N)`).
   `[oracle-resolves]`
7. **UPDATE with duplicate transaction keys** (no RANDOM): are repeated matches
   accumulated or only one applied? `[oracle-resolves]`
8. **APPEND FROM TYPE WKS/DIF/SYLK** exact dialects/limits in III+ 1.1 (the help
   names PFS for IMPORT/EXPORT but the error catalog implies DIF/SYLK appender
   support). `[oracle-resolves]`
9. **Exact field/record caps** (max fields per .DBF, max record length) that fire
   `Maximum record length exceeded.` / `Structure invalid.` -> belongs to DBF spec
   but referenced here. `[oracle-resolves; GAPS.md]`
10. **`USE ... EXCLUSIVE` / `SET EXCLUSIVE` in 1.1.** Neither token appears anywhere
   in the mined HELP corpus (`HELP.DBS`/`ASSIST.HLP`/`HELP_topics`), yet
   `Exclusive open of file is required.` is in `DBASE.MSG` and III+ networking
   historically had both. Confirm whether the 1.1 single-user `DBASE.EXE` accepts
   the EXCLUSIVE clause / SET, and which command actually requests the lock for
   ZAP/PACK/MODIFY STRUCTURE/INDEX/REINDEX. `[oracle-resolves]`

---

## Sources

- `archive/golden-mined/HELP.DBS.strings.txt`  --  on-line HELP database; the
  command-format (`` `5 ``) lines are the period syntax authority. Primary for
  every command here (line refs cited inline).
- `archive/golden-mined/ASSIST.HLP.strings.txt`  --  ASSIST menu command help
  (`#...#` = highlight); period-exact for the subset ASSIST exposes; corroborates
  USE/APPEND/COPY/DELETE/RECALL/PACK/INDEX/SORT/SUM/AVERAGE/COUNT/CREATE/MODIFY
  STRUCTURE/IMPORT/EXPORT.
- `archive/golden-mined/HELP_topics.txt`  --  the 406-item completeness oracle.
- `archive/golden-mined/DBASE.MSG.strings.txt`  --  error/message catalog (type
  mismatch, index caps, JOIN/INSERT/overflow failures, foreign-format errors).
- `archive/dbase-com-help/OPS_PLUS.md`, `OPS_MINUS.md`, `SET_EXACT.md`  --  modern
  dbase.com operator/SET docs; used ONLY to define `+`/`-`/SET EXACT, with the `+`
  auto-stringification rule explicitly flagged WRONG for III+.
- `oracles/harbour/doc/en/dbstrux.txt` (COPY STRUCTURE / EXTENDED / CREATE FROM),
  `dbsdf.txt` + `dbdelim.txt` (SDF / DELIMITED layout for APPEND FROM & COPY TO),
  `rdddb.txt` (dbAppend/dbGoto/dbSeek/dbGoTop pointer semantics), `rddord.txt`
  (INDEX / SET INDEX / SET ORDER / ordSetFocus), `set.txt`  --  semantic oracle
  (Clipper lineage; NOT period-exact syntax).
- Real III+ programs in
  `goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/`:
  `YEAREND.PRG` (COPY/DELETE ALL FOR/PACK; STR+LTRIM idiom), `CLEANUP.PRG`
  (USE/SUM/INDEX ON/GO BOTT), `REINIT.PRG` (ZAP/INDEX ON), `CHECK.PRG` &
  `CASH.PRG` (APPEND BLANK + REPLACE), `FLT_RSVE.PRG`/`FLIGHT.PRG` (USE ... INDEX
  multi-index, DELETE, in-place REPLACE arithmetic), `RECONCIL.PRG`
  (SUM ... FOR, REPLACE ALL), `ERRORPRG.PRG` (REINDEX), `EDITVOID.PRG` (DELETE).
- `CLAUDE.md` Laws (III+ target; `C+N` is a type mismatch, not stringification).

---

## Verification (adversarial pass, 2026-06-16)

Verifier cross-checked every entry against the full text of
`archive/golden-mined/HELP.DBS.strings.txt` (1982 lines), `ASSIST.HLP.strings.txt`
(286 lines), `DBASE.MSG.strings.txt` (432 lines), the 406-line `HELP_topics.txt`
completeness index, and `oracles/harbour/doc/en/dbsdf.txt` + `dbdelim.txt`.

### Syntax / line-citation audit  --  PASS
Every cited `` `5 `` command-format line was confirmed against the actual file
offsets: APPEND (433-436), AVERAGE (450-451), BROWSE (460-462), CHANGE (486-487),
COPY (521-524), COPY FILE (534), COUNT (540-541), CREATE (548), DELETE (555),
EDIT (659-660), EXPORT (688), IMPORT (753), INDEX (726), INSERT (743), JOIN
(759-760), MODIFY STRUCTURE (894), PACK (935), RECALL (988), REINDEX (997),
REPLACE (1019-1027), SET INDEX (1492), SET ORDER (1519), SORT (1110-1112), SUM
(1132), TOTAL (1156), UPDATE (1175), USE (1185-1186), ZAP (1204). All match. The
period authority's syntax surface is reproduced faithfully (every clause present).

### III+ correctness  --  PASS, no IV/PLUS contamination found
- Searched the doc for IV+ markers (`==`, `DECLARE`/arrays, `FIELD_DECIMAL`,
  Float/General/autoincrement, SCAN, CALCULATE, FLOCK/RLOCK, CONVERT). The only
  hits are the doc *warning against* these  --  no leakage into the III+ rules.
- The central `C + N -> Data type mismatch.` claim is now upgraded to
  `[verified]` against the period EXPRESSION help `HELP.DBS:1733-1737`, which
  states the ONLY cross-type exception is dates with numbers  --  strongly
  corroborating that `C + N` has no auto-stringification in III+. The modern
  dbase.com `+` rule remains correctly flagged WRONG.
- Relational operators confirmed against `HELP.DBS:1792-1801`: III+ uses `=`,
  `<>`/`#`, `$`  --  there is no `==`. The doc never uses `==`. Good.
- Field-type set confirmed = exactly five (C/N/D/L/M) per `HELP.DBS:1918-1930`;
  no Float/General.

### Fixes applied (Edits to this file)
1. **Added the DATABASE NAME rule** (8-char `.DBF` file-name limit, letter-first,
   underscore allowed, no blanks)  --  was a missing in-scope concept item; now
   `[verified: HELP.DBS:1911-1917]`. Disambiguated from the 10-char field name.
2. **Corrected the field-type citation** from the spurious `HELP.DBS:1217ff`
   (that range is the Database *Functions* list) to the real FIELD TYPE narrative
   `HELP.DBS:1918-1939`, and expanded each type with the period wording. Upgraded
   the **field-name** rule from `[inferred]` to `[verified: HELP.DBS:1906-1909]`
   (it is period-documented, not merely inferred from DBF bytes).
3. **Tightened the `USE ... EXCLUSIVE` claim.** The mined III+ HELP corpus contains
   **no `EXCLUSIVE` keyword on USE and no `SET EXCLUSIVE`**; the prose previously
   read as if the token were established. Reworded to state EXCLUSIVE is absent
   from this corpus (historically present in the III+ multi-user/ADMINISTRATOR
   pack), with the `Exclusive open of file is required.` error kept as the
   documented failure mode. Remains `[oracle-resolves]`.
4. **Upgraded the `<scope>` grammar** to `[verified: HELP.DBS:1842-1851]` (verbatim
   SCOPE narrative) and the default-scope statement to `[verified:
   HELP.DBS:1852-1853]`.
5. **Upgraded `C + C` / `C - C`** concatenation semantics to `[verified:
   HELP.DBS:1816-1821]` (period STRING OP help), beyond just the DBASE.MSG overflow
   errors.

### Completeness vs the 406-item HELP index
All in-scope command and concept items are present. After fix (1), the
DATABASE NAME concept is now substantively covered rather than merely claimed. No
in-scope HELP topic is missing. The out-of-scope boundary (navigation, retrieval/
output, designers, most SETs, work areas) is honored and cross-linked, consistent
with sibling specs. ASSIST exposes a documented subset (no JOIN/UPDATE/TOTAL/
INSERT/ZAP/SEEK)  --  confirmed: those five are absent from `ASSIST.HLP`.

### Residual [oracle-resolves] gaps (unchanged; need DBASE.EXE under DOSBox)
The nine Open-questions items stand: index key 100-vs-220 cap; scoped REPLACE on an
active key field; INSERT under an active master index; final pointer after
PACK/ZAP; MODIFY STRUCTURE auto-reindex + the rename-and-resize data-loss boundary;
exact ZAP/CREATE-overwrite SAFETY prompt text; UPDATE duplicate-key accumulation;
APPEND FROM WKS/DIF/SYLK dialect limits; exact field/record caps. Added to that
set: confirm whether III+ 1.1 accepts `USE ... EXCLUSIVE` / `SET EXCLUSIVE`
(absent from the HELP corpus but historically part of III+ networking).
