# SET Commands — the dBASE III PLUS 1.1 Environment Switchboard

**Scope:** Every `SET` command in dBASE III PLUS 1.1 (1986) and the global
environment state each one controls. This is the master reference for the
interpreter's environment/settings subsystem. For each SET: full syntax (every
clause), the parameter/value space, the **default**, exactly what state it
mutates, and the III+-specific behavior. SET commands that have a dedicated
home elsewhere (full-screen format files, indexes, relations) are documented
here at the environment-state level and cross-linked.

---

## Status / confidence

- The **set of SET commands that EXIST in III+** is fixed by the period
  COMPLETENESS oracle: `archive/golden-mined/HELP_topics.txt` (the canonical
  406-item index). Every SET listed there is covered below; the index is the
  authority for *what exists*. [verified: HELP_topics.txt]
- The **ASSIST help text** (`archive/golden-mined/ASSIST.HLP.strings.txt`)
  gives period-exact syntax for the six SETs ASSIST exposes (FORMAT, FILTER,
  CATALOG, VIEW, DEFAULT/"DRIVE"). Those are quoted verbatim. [documented]
- **Defaults and ranges** for numeric SETs (DECIMALS, TYPEAHEAD, MARGIN,
  HISTORY, MEMOWIDTH) come from dbase.com help + period references and are
  tagged per claim. Some numeric **maxima** differ between III+ and later
  dBASE; those deltas are flagged.
- The biggest correctness hazard in the language (not in this file's commands
  but governed by `SET EXACT` here): in III+ **`C + N` is a type-mismatch
  error, NOT auto-stringification.** The modern dbase.com `+` doc (which
  describes auto-conversion) is WRONG for III+. See the EXACT entry and
  `specs/language/expressions-and-operators.md`.
- Items needing the real interpreter under DOSBox to settle are in **Open
  questions** and route to `GAPS.md`.

A handful of SET commands appear in the HELP index but are not given full
period syntax there (e.g. `SET ORDER`, `SET PATH`, `SET PROCEDURE`,
`SET RELATION`, `SET TYPEAHEAD`, `SET FUNCTION`, `SET COLOR`, `SET DATE`,
`SET DECIMALS`). Their syntax/semantics below are reconstructed from
dbase.com + Harbour/Clipper-lineage docs + real sample programs and are tagged
accordingly. The III+ *existence* of each is nonetheless verified by the index.

---

## 1. General model

### 1.1 What a SET command is

`SET` mutates one slot of a single global, process-wide environment record. The
environment is **global** in III+: there is exactly one of each setting for the
whole interpreter session; settings are NOT per-work-area (unlike INDEX/ORDER,
which attach to a work area). A program that changes a SET and does not restore
it leaks that change to its caller and to the dot prompt. Disciplined III+
programs therefore save and restore the environment around themselves; the
sample programs open with an explicit environment block:

```
* Set working environment            (CBMENU.PRG, lines 17-25)
SET TALK OFF
SET ESCAPE OFF
SET BELL OFF
SET HEADING OFF
SET HELP OFF
SET MENU OFF
SET SAFETY OFF
SET STATUS OFF
```
[verified: goldens .../Sample_Programs_and_Utilities/CBMENU.PRG:17-25]

There is **no `PUSH`/`POP`/`SET()` introspection** in III+. The current value
of a setting is read back only with `DISPLAY STATUS` / `LIST STATUS`, or via
the few environment functions (`OS()`, `ISCOLOR()`, etc.) — there is no general
`SET("EXACT")` accessor (that is a Clipper/later addition). [documented:
HELP_topics.txt has DISPLAY STATUS / LIST STATUS; no SET() function topic]
[III-vs-later delta: Clipper `SET()`/`SETMODE()` introspection does not exist
in III+]

### 1.2 Two grammatical shapes

1. **Toggle SETs:** `SET <name> ON|OFF`. A boolean slot. ON/OFF is mandatory;
   there is no bare `SET BELL` form. [documented: ASSIST/sample idioms always
   write the keyword]
2. **Value SETs:** `SET <name> TO [<value>]`. Takes a typed argument (a number,
   a name, a file, a list, a condition, a color string). `SET <name> TO` with
   **no value** generally *clears/resets* the slot to its empty/default state
   (e.g. `SET FILTER TO`, `SET INDEX TO`, `SET DECIMALS TO`, `SET FORMAT TO`,
   `SET PATH TO`). [verified: ASSIST help shows the bracketed optional argument
   for FORMAT/FILTER/CATALOG/VIEW + dbase.com "to restore defaults, use SET
   DECIMALS TO without a value"]

### 1.3 CONFIG.DB — the startup environment

At launch, `DBASE.EXE` reads **`CONFIG.DB`** from the current directory (a plain
text file, CR/LF terminated). Each non-blank line is `KEYWORD=value`. The
keyword is the SET name **without** the word `SET` and **without** `TO`; the
value is the argument. The golden CONFIG.DB shipped with III+ 1.1 contains
exactly:

```
STATUS=ON
COMMAND=ASSIST
```
[verified: goldens/dbase-iii-plus-1.1-pristine/files/System_1/CONFIG.DB +
goldens/dbase-iii-plus-1.1/extracted/disk1/CONFIG.DB — both identical, CR/LF]

Notes on CONFIG.DB:
- `STATUS=ON` forces the status bar ON at startup (this is why STATUS is ON in
  a fresh ASSIST session even though the *bare-interpreter* default of SET
  STATUS is also ON; see SET STATUS).
- `COMMAND=ASSIST` makes the interpreter run `ASSIST` as its first command
  (drops the user into the ASSIST full-screen menu instead of the dot prompt).
  `COMMAND=` is the only non-SET CONFIG.DB directive of interest here; it is the
  "autoexec command" line. [verified: CONFIG.DB bytes + ASSIST.HLP context]
- Other CONFIG.DB keywords accepted by III+ include the toggles and value sets
  (e.g. `BELL=OFF`, `DELIMITERS=ON`, `DEFAULT=B`, `PATH=...`, `TYPEAHEAD=...`,
  plus non-SET tuning keywords `BUCKET`, `MAXMEM`, `MVARSIZ`/`MVMAXBLKS`,
  `GETS`, `F1`..`F10` function-key macros, `COLOR=`). The non-SET memory-tuning
  keywords are out of scope here (see `specs/environment/config-db.md` when written).
  [documented: period CONFIG.DB references] [oracle-resolves: exact full keyword
  list accepted by 1.1 — confirm against DBASE.EXE]

### 1.4 What happens on bad input

- `SET <unknown> ...` or a malformed value raises a syntax/`*** Unrecognized
  command verb` style error from the parser. The error catalog
  `DBASE.MSG.strings.txt` line 96 carries `Invalid DOS SET option.` (that is the
  DOS-environment `SET`/`RUN` path, distinct from a dBASE SET). [documented:
  DBASE.MSG.strings.txt:96]
- `SET COLOR` to a malformed attribute string, `SET FILTER TO` with a bad
  condition, etc., raise the normal expression/data-type errors. [inferred from
  general III+ error behavior]

---

## 2. Toggle (ON/OFF) SET commands

All toggles take `ON` or `OFF`. The "Default" column is the value in a **fresh
bare interpreter** (no CONFIG.DB). Note CONFIG.DB and ASSIST can override these
at startup (e.g. CONFIG.DB sets `STATUS=ON`).

| SET | Default | ON means | Controls / state changed |
|-----|---------|----------|---------------------------|
| `SET BELL ON/OFF` | ON | bell sounds | Sound the console bell on data-entry errors and when a GET field fills. [verified: HELP_topics SET BELL + sample `SET BELL OFF`] |
| `SET CARRY ON/OFF` | OFF | carry forward | In APPEND/INSERT/EDIT, copy the previous record's field values into the new blank record. [documented: HELP_topics SET CARRY] |
| `SET CENTURY ON/OFF` | OFF | 4-digit year | Date display/entry shows the century: `MM/DD/YYYY` when ON, `MM/DD/YY` when OFF. Affects CTOD/DTOC width and date GET picture width. [verified: HELP_topics SET CENTURY + dbase.com date docs] |
| `SET CONFIRM ON/OFF` | OFF | require Enter | When ON, a full-field GET does NOT auto-advance; the user must press Enter (or an exit key) to leave the field. When OFF, filling the last char of a GET auto-advances to the next field. [verified: HELP_topics SET CONFIRM + sample `SET CONFIRM ON/OFF`] |
| `SET CONSOLE ON/OFF` | ON | screen echo | Turns console (screen) output on/off for `?`,`??`,`@...SAY`, etc. Used to suppress on-screen echo while still printing. Does NOT affect keyboard. [verified: HELP_topics SET CONSOLE + samples FLT_INFO.PRG:18/20, FLT_CNCL.PRG:79/81] |
| `SET DEBUG ON/OFF` | OFF | trace to printer | Redirects the SET ECHO command trace to the printer instead of the screen (used with SET ECHO ON for debugging). [documented: HELP_topics SET DEBUG] |
| `SET DELETED ON/OFF` | OFF | hide deleted | When ON, records flagged for deletion (the `*` flag) are filtered out of most commands that walk the file (LIST, DISPLAY, COPY, etc.). When OFF, deleted records are still visible/processed. GO/GOTO and direct record access ignore this setting. [verified: HELP_topics SET DELETED + dbase.com] |
| `SET DELIMITERS ON/OFF` | OFF | show delimiters | When ON, GET fields on screen are bracketed by the delimiter character(s) set via `SET DELIMITERS TO`. Pairs with the value form below. [verified: HELP_topics SET DELIMITERS] |
| `SET DOHISTORY ON/OFF` | OFF | record DO cmds | When ON, commands executed from within DO command files are recorded into the HISTORY buffer (normally only interactive commands are saved). [documented: HELP_topics SET DOHISTORY] [III-vs-later delta: removed in dBASE IV] |
| `SET ECHO ON/OFF` | OFF | echo commands | Echoes each command line as it executes (program trace). Output goes to screen, or to printer if SET DEBUG ON. [documented: HELP_topics SET ECHO] |
| `SET ESCAPE ON/OFF` | ON | Esc aborts | When ON, pressing Esc during command-file execution interrupts the program (raising the "*** INTERRUPTED" / Cancel-Suspend-Ignore prompt). When OFF, Esc is ignored — programs disable it to prevent users breaking out. [verified: HELP_topics SET ESCAPE + sample `SET ESCAPE OFF`] |
| `SET EXACT ON/OFF` | OFF | exact `=` match | Governs string comparison with `=`,`<>`,`#` and SEEK/index lookups. See Section 4 — critical. [verified: HELP_topics SET EXACT + SET_EXACT.md + OPS_COMPARISON.md] |
| `SET FIXED ON/OFF` | OFF | fixed decimals | When ON, ALL numeric display uses the number of decimals from SET DECIMALS (forces a uniform decimal count). When OFF, decimals vary per value. [documented: HELP_topics SET FIXED + dbase.com] |
| `SET HEADING ON/OFF` | ON | column headers | When ON, LIST/DISPLAY/SUM/AVERAGE etc. print column headers (field names). When OFF, no headers. [verified: HELP_topics SET HEADINGS + sample `SET HEADING OFF`] |
| `SET HELP ON/OFF` | ON | offer HELP | When ON, after certain syntax errors the interpreter offers/launches HELP (the "Do you want some help? (Y/N)" prompt). Programs turn it OFF to suppress that prompt. [verified: HELP_topics SET HELP + sample `SET HELP OFF`] |
| `SET INTENSITY ON/OFF` | ON | enhanced GETs | When ON, GET input fields are shown in the enhanced (reverse/highlight) video attribute; when OFF they use the standard attribute. Interacts with SET COLOR. [documented: HELP_topics SET INTENSITY] |
| `SET MENU ON/OFF` | ON | show key menu | When ON, full-screen edit modes (EDIT/BROWSE/APPEND/READ) display the navigation key menu/cursor-key help at the top. OFF hides it. [verified: HELP_topics SET MENUS + sample `SET MENU OFF`/`SET MENU ON`] |
| `SET PRINT ON/OFF` | OFF | echo to printer | When ON, all console output (`?`,`??`,`@...SAY` results) is ALSO sent to the printer (LPT). Distinct from `@...SAY ... TO PRINT`-style device routing. [verified: HELP_topics SET PRINT + samples RPRTPRO.PRG:121/428] |
| `SET SAFETY ON/OFF` | ON | confirm overwrite | When ON, commands that would overwrite an existing file (COPY TO, INDEX TO, SORT TO, SAVE TO, ZAP, etc.) prompt "file already exists, overwrite? (Y/N)". Programs set it OFF to overwrite silently. [verified: HELP_topics SET SAFETY + sample `SET SAFETY OFF`] |
| `SET SCOREBOARD ON/OFF` | ON | status line msgs | Controls display of the scoreboard messages (line 0 indicators: Ins, Del, error/range messages, record count) when SET STATUS is OFF. When STATUS is ON the scoreboard area is part of the status bar. [documented: HELP_topics SET SCOREBOARD] |
| `SET STATUS ON/OFF` | ON | status bar | Shows the bottom status bar (drive, file, record number, NumCaps/Ins, etc.). The shipped CONFIG.DB forces `STATUS=ON`. Programs commonly `SET STATUS OFF` for clean full-screen UIs. [verified: HELP_topics SET STATUS + CONFIG.DB `STATUS=ON` + samples `SET STATUS OFF`] |
| `SET STEP ON/OFF` | OFF | single-step | When ON, program execution pauses before each command, prompting the user to step/continue/escape (debugging). [documented: HELP_topics SET STEP] |
| `SET TALK ON/OFF` | ON | echo results | When ON, commands echo their results to the screen (STORE/= show the assigned value, REPLACE shows counts, APPEND shows record numbers, etc.). The single most common first line of a III+ program is `SET TALK OFF` to suppress this chatter. [verified: HELP_topics SET TALK + many samples `SET TALK OFF`/`set talk off`] |
| `SET TITLE ON/OFF` | ON | catalog title prompt | When a catalog is open (SET CATALOG ON), controls whether the user is prompted for a descriptive title each time a new file is added to the catalog. OFF suppresses the prompt. [documented: HELP_topics SET TITLE] |
| `SET UNIQUE ON/OFF` | OFF | unique index keys | When ON, a subsequent `INDEX ON` keeps only the FIRST record for each duplicate key value (equivalent to `INDEX ... UNIQUE`). Affects index building/REINDEX, not retrieval after the fact. [documented: HELP_topics SET UNIQUE + ASSIST INDEX ... UNIQUE] |

### 2.1 Toggles that EXIST but are not in the HELP "ON/OFF" topics

These are real III+ toggles used in the sample programs / period references but
do not each get a HELP_topics line of their own (they are covered under file/
record commands, or are multi-user options):

- `SET EXCLUSIVE ON/OFF` — default OFF. When ON, files are opened for exclusive
  (single-user) access; required before commands that rewrite the whole file in
  a shared environment (PACK, ZAP, INDEX, REINDEX, MODIFY STRUCTURE). Programs
  in the samples toggle it: `SET EXCLUSIVE ON` / `SET EXCLUSIVE OFF`.
  [verified: samples SHOW_FLT/FLT_*.PRG `SET EXCLUSIVE ON/OFF` + dbase.com]
  Note: `EXCLUSIVE` is not in HELP_topics' ON/OFF list (HELP_topics 202-224),
  consistent with III+'s primarily single-user help framing, but the command is
  genuinely present in 1.1: the base error catalog carries
  `Exclusive open of file is required.` and `File is in use by another.` /
  `Record is in use by another.` [verified: DBASE.MSG.strings.txt:105-107],
  which are the diagnostics raised when EXCLUSIVE/locking is in play — i.e. the
  shared-access machinery (and therefore SET EXCLUSIVE) ships in 1.1's runtime,
  not only in a separate LAN pack. [oracle-resolves: confirm the exact default
  (OFF) and verb acceptance against DBASE.EXE 1.1]

---

## 3. Value (`SET ... TO`) commands

Grouped by what they control. Syntax shown with the period bracket convention:
`[...]` optional, `/` alternatives, `<...>` placeholders.

### 3.1 Numeric / formatting state

#### SET DECIMALS
```
SET DECIMALS TO [<expN>]
```
- Number of decimal places shown for computed numeric results (division, SQRT,
  LOG, financial functions, etc.). Affects **display only**, not stored values
  or internal precision. [verified: dbase.com SET DECIMALS + HELP_topics]
- **Default: 2.** [verified: dbase.com "default setting is 2" + WebSearch period
  refs]
- `SET DECIMALS TO` with no value resets to the default (2). [documented:
  dbase.com]
- Works together with SET FIXED. With **FIXED OFF** (default), DECIMALS controls
  the decimal count only for *division and the transcendental functions*
  (`/`, `EXP()`, `LOG()`, `SQRT()`, `VAL()`); for `+`/`-` the result takes the
  greater operand's decimals and for `*` the sum of the operands' decimals. With
  **FIXED ON** it forces a uniform count on ALL numeric display. [verified:
  HELP_topics SET FIXED + harbour set.txt:1238-1260 decimals-OFF rule table
  (Division/Exp/Log/Sqrt/Val "Determined by SET DECIMALS TO"; Add/Sub "greatest
  operand decimals"; Mult "sum of operand decimals")]
- **III-vs-later delta:** dbase.com states a maximum of **34** decimals — that
  is a modern dBASE Plus value. III+ caps decimals far lower (period docs cite
  a max around 15–18, tied to the numeric field width limit). Treat 34 as NOT
  applicable to III+. [oracle-resolves: exact III+ SET DECIMALS maximum —
  confirm against DBASE.EXE; route to GAPS.md]

#### SET FIXED
Toggle — see Section 2 (default OFF). Pairs with SET DECIMALS.

#### SET MARGIN
```
SET MARGIN TO [<expN>]
```
- Left print margin: number of columns the printer output is indented (added to
  every printed line column position). Affects printer output only.
- **Default: 0.** [verified: harbour set.txt:301-304 `_SET_MARGIN` "The default
  value is 0" + WebSearch period refs + HELP_topics SET MARGIN]
- `SET MARGIN TO` with no value resets to 0. [inferred from value-SET reset
  convention]

#### SET MEMOWIDTH
```
SET MEMOWIDTH TO <expN>
```
- Column width used when a memo field's contents are displayed/printed (by `?`,
  LIST, REPORT). Memo text is word-wrapped to this width.
- **Default: 50.** [documented: HELP_topics SET MEMOWIDTH + period refs]
  [oracle-resolves: confirm exact III+ default is 50 not 60; route to GAPS.md]

#### SET DECIMALS vs SET FIXED interplay — example
```
SET DECIMALS TO 4
SET FIXED OFF
? 22/7        && shows 3.1429   (4 places, the computed default)
SET FIXED ON
? 5           && shows 5.0000   (forced 4 places on everything)
```
[inferred from dbase.com DECIMALS/FIXED semantics]

### 3.2 Date / display state

#### SET DATE
```
SET DATE [TO] <format keyword>
```
- Selects the date display/entry format. III+ format keywords:
  `AMERICAN` (MM/DD/YY, default), `ANSI` (YY.MM.DD), `BRITISH` (DD/MM/YY),
  `ITALIAN` (DD-MM-YY), `FRENCH` (DD/MM/YY), `GERMAN` (DD.MM.YY),
  `JAPAN` (YY/MM/DD), `USA` (MM-DD-YY). Combine with SET CENTURY for 4-digit
  years.
- **Default: AMERICAN (MM/DD/YY).** [verified: harbour set.txt:130-140 +
  set.txt:1147 `SET DATE [TO] [ANSI / BRITISH / FRENCH / GERMAN / ITALIAN /
  JAPAN / USA / AMERICAN]` — the keyword list and every date picture above
  (ANSI yy.mm.dd, BRITISH dd/mm/yy, FRENCH dd/mm/yy, GERMAN dd.mm.yy, ITALIAN
  dd-mm-yy, JAPAN yy/mm/dd, USA mm-dd-yy) match the Clipper-lineage oracle
  exactly; HELP_topics SET DATE confirms III+ existence]
  [III-vs-later delta: III+ has the fixed keyword set above; `SET DATE TO` with
  an explicit picture string is a IV+ feature — not in III+. oracle-resolves:
  confirm III+ keyword spelling list against DBASE.EXE]

#### SET CENTURY
Toggle — see Section 2 (default OFF). Controls 2- vs 4-digit year display/entry.

### 3.3 Color / screen state

#### SET COLOR
```
SET COLOR TO [<standard>] [,<enhanced>] [,<border>] [,<background>]
SET COLOR TO            && (no args) restores default colors
SET COLOR ON/OFF        && switch between color and monochrome attribute maps
```
- Each `<attr>` is `<foreground>/<background>` using single-letter color codes;
  append `+` for high-intensity foreground, `*` for blinking. Color letters:
  `N` black, `B` blue, `G` green, `BG` cyan, `R` red, `RB`/`RG` magenta/brown,
  `W` white, `U` underline (mono), `I` inverse (mono), `X` blank. [documented:
  dbase.com SET COLOR + Clipper color spec]
- Position 1 = **standard** (normal text/SAY), position 2 = **enhanced**
  (GET input fields, highlighted), position 3 = **border** (screen frame, CGA),
  position 4 = **background** (rarely used). [documented: dbase.com]
- Real idiom — two-state highlight in a menu (CBMENU.PRG):
```
SET COLOR TO N/W     && black on white (highlighted item)
...
SET COLOR TO W/N     && white on black (normal item)
```
[verified: samples CBMENU.PRG:75/78, CHECK area; CBMENU2.PRG]
- `SET COLOR TO` (empty) restores the boot defaults; `SET COLOR ON|OFF`
  switches the active attribute set between the color map and the mono map for
  dual-monitor systems. [documented: dbase.com] [oracle-resolves: exact III+
  default attribute string; route to GAPS.md]

#### SET INTENSITY / SET DELIMITERS (toggles)
See Section 2. The companion value form for delimiters:

#### SET DELIMITERS TO
```
SET DELIMITERS TO [<expC>] [DEFAULT]
SET DELIMITERS ON/OFF
```
- `<expC>` is one or two characters that bracket GET fields on screen when
  `SET DELIMITERS ON`. One char = used on both sides; two chars = left and
  right. `SET DELIMITERS TO DEFAULT` resets to the colon pair `::`.
  `SET DELIMITERS TO` (empty) clears the custom delimiter. [documented:
  dbase.com + Clipper] 
- **Default delimiter: `:` (colon), display OFF.** [documented]

### 3.4 File / path / device state

#### SET DEFAULT
```
SET DEFAULT TO <drive>
```
- Changes the default disk drive dBASE searches/writes when no drive is given.
  Does NOT change the OS logged drive. Period-exact ASSIST text:
  > "SET DEFAULT changes the disk drive that dBASE III searches for files when
  > no disk drive is specified, but it does not change the operating system
  > logged drive." Command Format: `SET DEFAULT TO <drive>`
  [verified: ASSIST.HLP.strings.txt:252-256 (titled "SET DRIVE")]
- `<drive>` is a drive letter, e.g. `B`. [verified: ASSIST]

#### SET PATH
```
SET PATH TO [<path list>]
```
- Search path (DOS-style, semicolon- or comma-separated directory list) used to
  FIND files (USE, DO, etc.) that are not in the default drive/dir. `SET PATH
  TO` with no list clears the path. [documented: HELP_topics SET PATH +
  dbase.com] Does not affect where new files are *created* (that's the default
  drive). [documented]

#### SET ALTERNATE
```
SET ALTERNATE TO [<filename>]
SET ALTERNATE ON/OFF
```
- `SET ALTERNATE TO <filename>` opens/creates a text capture file (`.txt`
  default extension) — the "alternate" log. `SET ALTERNATE ON` then echoes all
  console output to that file; `SET ALTERNATE OFF` pauses it. `SET ALTERNATE TO`
  (no name) closes the file. Used to capture a session transcript or build a
  report text file. [documented: HELP_topics SET ALTERNATE + dbase.com]
- **Default: no alternate file, OFF.** [documented]
- `CLOSE ALTERNATE` also closes it. [documented]

#### SET DEVICE
```
SET DEVICE TO SCREEN / PRINT
```
- Routes `@...SAY` output to either the SCREEN (default) or the PRINTer. When
  `TO PRINT`, `@row,col SAY` positions text on the printed page (used to drive
  formatted printed forms). Does not affect `?`/`??` (those follow SET PRINT).
- **Default: SCREEN.** [documented: HELP_topics SET DEVICE + dbase.com]

#### SET PRINT (toggle)
See Section 2. `SET PRINT ON` echoes `?`/`??`/console output to LPT. Orthogonal
to SET DEVICE (which routes only `@...SAY`).

### 3.5 Database environment state (work-area-attaching SETs)

These mutate the *current work area's* data environment, not a global scalar.

#### SET INDEX
```
SET INDEX TO [<index file list>]
```
- Opens one or more `.ndx` index files for the database in the current work
  area. The FIRST index in the list becomes the controlling/master index (sets
  record order); the rest are kept updated but do not control order. `SET INDEX
  TO` with no list closes all index files for the work area (reverts to natural
  record order). Real idiom:
```
SET INDEX TO Flt_No, Aval_flt     && Flt_No is the master index
```
[verified: HELP_topics SET INDEX + sample SHOW_FLT.PRG:47]
- Equivalent to the `INDEX <list>` clause of `USE`. [documented]

#### SET ORDER
```
SET ORDER TO [<expN>]
```
- Selects which of the already-open index files (by position 1..n in the SET
  INDEX / USE INDEX list) is the controlling index, without reopening them.
  `SET ORDER TO 0` (or no value) selects natural record order while keeping all
  indexes open and updated. [documented: HELP_topics SET ORDER + dbase.com +
  Clipper]
- **Default: 1** (first index controls, once any index is open). [documented]

#### SET RELATION
```
SET RELATION TO [<key expr> / <expN>] INTO <alias>
                [, <key expr2> INTO <alias2>] ...
SET RELATION TO            && (no args) clears all relations from current area
```
- Links the current work area (parent) to another open work area (child) so
  that moving the parent's record pointer automatically positions the child:
  by SEEK on `<key expr>` if the child is indexed on a matching key, or by
  record number if `<expN>` is given (one-to-one). `SET RELATION TO` with no
  argument breaks the relation(s). [documented: HELP_topics SET RELATION +
  dbase.com] 
- A cyclic relation raises the error `Cyclic relation.` [verified:
  DBASE.MSG.strings.txt:42]
- **III-vs-later delta:** III+ supports a SINGLE relation per work area; chained
  multi-relation (multiple `INTO` in one statement) and `SET SKIP` for
  one-to-many are IV+ extensions. [oracle-resolves: confirm whether III+ 1.1
  accepts multiple `INTO` clauses; route to GAPS.md]

#### SET FILTER
```
SET FILTER TO [FILE <query file name> / ?] / [<condition>]
SET FILTER TO              && (no args) removes the filter
```
- Restricts the current work area so that only records satisfying `<condition>`
  (a logical expression) are visible to file-walking commands. Alternatively
  loads the condition from a `.qry` query file (`FILE <name>` or `?` to pick).
  Period-exact ASSIST text:
  > "SET FILTER TO activates the filter condition stored in an existing query
  > (.qry) file. Once the filter is in place, Retrievals ... will show only
  > records that meet the filter condition." Command Format: `SET FILTER TO`
  > `[FILE <query file name>/?]/[<condition>]`
  [verified: ASSIST.HLP.strings.txt:13-19]
- The filter does not reposition the pointer; the next file-walk (e.g. `GO TOP`,
  LIST) honors it. A `GO TOP` is usually needed for the filter to take effect on
  the current position. [documented: dbase.com]
- **Default: no filter.** [verified: ASSIST]

#### SET FIELDS
```
SET FIELDS TO [<field list> / ALL]
SET FIELDS ON/OFF
```
- Defines a restricted/ordered list of active fields visible to commands like
  LIST, DISPLAY, BROWSE, REPLACE, etc. `SET FIELDS ON` activates the list;
  `SET FIELDS OFF` (default) makes all fields active. `SET FIELDS TO` (empty)
  clears the list. With multiple work areas, the FIELDS list can include fields
  from related aliases. [documented: HELP_topics SET FIELDS + dbase.com]
- **Default: OFF, empty list (all fields active).** [documented]

#### SET FORMAT
```
SET FORMAT TO [<format file name> / ?]
SET FORMAT TO              && (no name) reverts to the default screen
```
- Selects a custom `.fmt` screen-format file used for full-screen editing
  (APPEND/EDIT/CHANGE/READ/INSERT). Period-exact ASSIST text:
  > "SET FORMAT TO selects a custom screen format (.fmt) file that is used for
  > full-screen editing. This format file defines the input screen used with
  > the Edit and Append options ..." Command Format: `SET FORMAT TO`
  > `[<format file name>/?]`
  [verified: ASSIST.HLP.strings.txt:8-12]
- `.fmt` is the default extension. See `specs/file-formats/scr-and-fmt.md`
  (cross-link) for the `@...GET/SAY` contents of a format file. [documented]

#### SET CATALOG
```
SET CATALOG TO [<catalog file name> / ?]
SET CATALOG ON/OFF
```
- Opens a catalog `.cat` file (itself a `.dbf`) that tracks the files in an
  application. When a catalog is open and `SET CATALOG ON`, every file
  created/used is added to the catalog and the `?`-pick lists are scoped to the
  catalog. Period-exact ASSIST text:
  > "SET CATALOG TO selects the catalog that ASSIST will use. When a catalog is
  > in use, only the files listed in the active catalog are presented for
  > editing and reporting. Additionally, all files created or used are added to
  > the catalog for future reference." Command Format: `SET CATALOG TO`
  > `[<catalog file name>/?]`
  [verified: ASSIST.HLP.strings.txt:20-25]
- `SET CATALOG TO` (no name) closes the catalog. The `$` operator in a catalog
  context and the SET TITLE prompt (Section 2) interact here. [documented]
- **Default: no catalog, CATALOG OFF.** [documented]

#### SET VIEW
```
SET VIEW TO [<view file name> / ?]
```
- Restores a whole saved operating environment from a `.vue` view file: which
  `.dbf` files are open in which work areas, their indexes, the relations
  between them, the active field list, the format file, and the filter. Period-
  exact ASSIST text:
  > "SET VIEW TO defines the operating environment using a view (.vue) file.
  > The environment consists of database files, their index files, their active
  > fields and their relationships. Views provide a means for using more than
  > one database file in ASSIST." Command Format: `SET VIEW TO`
  > `[<view file name>/?]`
  [verified: ASSIST.HLP.strings.txt:26-31]
- A `.vue` is created by `CREATE VIEW` / `MODIFY VIEW` / `CREATE VIEW ... FROM
  ENVIRONMENT`. [verified: ASSIST CREATE VIEW:47-54]
- **III-vs-later delta:** `.vue` is the III+ view format; dBASE IV introduces
  `.vue`-incompatible query/view files (`.qbe`/`.vue` v2). [documented]

### 3.6 Program / procedure state

#### SET PROCEDURE
```
SET PROCEDURE TO [<procedure file name>]
```
- Opens a procedure library file (`.prg`, default ext) so its `PROCEDURE`s can
  be called with `DO <name>` without each being a separate file. `SET PROCEDURE
  TO` (no name) closes the procedure file. Real idiom:
```
SET PROCEDURE TO Rprtpro          && opens RPRTPRO.PRG as the proc library
```
[verified: HELP_topics SET PROCEDURE + sample REPORTS.PRG:15]
- **III+ limit: at most ONE procedure file open at a time**, holding up to
  **32 procedures**. [documented: period III+ docs] [oracle-resolves: confirm
  the 32-procedure cap for 1.1; route to GAPS.md]
- **III-vs-later delta:** multiple concurrent procedure files are a IV+
  feature; in III+ a second SET PROCEDURE closes the first. [documented]

#### SET FUNCTION
```
SET FUNCTION <expN> / <key> TO <expC>
```
- Reprograms a function key macro: `<expN>` is the function-key number (2..10;
  F1 is reserved for HELP and cannot be reassigned), and `<expC>` is the string
  the key expands to (include `;` to embed a carriage return). E.g.
  `SET FUNCTION 2 TO "LIST;"` makes F2 type "LIST" + Enter. [documented:
  HELP_topics SET FUNCTION + dbase.com + Clipper]
- Default key assignments are the III+ built-ins (F1=HELP, F2=ASSIST,
  F3=LIST, F4=DIR, F5=DISPLAY STRUCTURE, F6=DISPLAY STATUS, F7=DISPLAY MEMORY,
  F8=DISPLAY, F9=APPEND, F10=EDIT — each with trailing CR). [documented: period
  III+ docs] [oracle-resolves: confirm exact default F-key strings for 1.1;
  route to GAPS.md]

#### SET HISTORY
```
SET HISTORY TO <expN>
SET HISTORY ON/OFF
```
- `SET HISTORY TO <expN>` sets the number of recently-executed commands kept in
  the HISTORY buffer (retrievable with up-arrow at the dot prompt, viewable with
  `LIST/DISPLAY HISTORY`). `SET HISTORY ON/OFF` enables/disables recording.
- **Default size: 20** commands; recording **ON** at the dot prompt.
  [verified: WebSearch period refs "SET HISTORY default is 20" + HELP_topics SET
  HISTORY]
- Interacts with SET DOHISTORY (whether DO-file commands are also recorded).
  [documented]

### 3.7 Console / messaging state

#### SET MESSAGE
```
SET MESSAGE TO [<expC>]
```
- Sets a custom message string displayed centered on the bottom status line
  (line 24) — only visible when SET STATUS is ON. `SET MESSAGE TO` (empty)
  clears it. [documented: HELP_topics SET MESSAGE + dbase.com]
- **Default: empty.** [documented]
- **III-vs-later delta:** the per-GET `MESSAGE` clause on `@...GET` is related
  but separate; SET MESSAGE is the global status-line message. [documented]

#### SET TYPEAHEAD
```
SET TYPEAHEAD TO <expN>
```
- Size (in characters) of the keyboard type-ahead buffer. `0` disables
  type-ahead (each keystroke must be consumed before the next is accepted;
  needed for some INKEY()-driven UIs).
- **Default: 20** in dBASE III PLUS. The Clipper/Harbour lineage uses a
  different default and range (default 50, min 16, max 4096) [verified: harbour
  set.txt:383-386 `_SET_TYPEAHEAD` "Defaults to 50. The minimum is 16 and the
  maximum is 4096"]; modern dbase.com states default 50, range 0–1600. Those are
  NOT the III+ values. [oracle-resolves: III+ default is widely cited as 20
  (range 0–32000). Confirm the III+ 1.1 default/range against DBASE.EXE; route
  to GAPS.md]

#### SET CONSOLE / SET STATUS / SET SCOREBOARD / SET MESSAGE relationship
- STATUS bar (line 24) ON hides the SCOREBOARD-only indicators because they
  live in the status bar. With STATUS OFF, SCOREBOARD ON puts Ins/Del/error/
  range indicators on line 0. SET MESSAGE only shows with STATUS ON.
  CONSOLE OFF suppresses screen echo entirely. [documented: dbase.com +
  HELP_topics] [oracle-resolves: exact line/row of scoreboard vs status in 1.1]

---

## 4. SET EXACT — the comparison/lookup hazard (critical)

```
SET EXACT ON/OFF
```
**Default: OFF.** [verified: SET_EXACT.md "When SET EXACT is OFF (the default)"
+ OPS_COMPARISON.md]

What it controls: the rule for `=`, `<>`, `#` string comparison, and index
SEEK/FIND matching. [verified: SET_EXACT.md + OPS_COMPARISON.md]

- **EXACT OFF (default) — "begins with":** `"Smith" = "S"` is TRUE; the left
  string need only *begin with* the right string. `SEEK("S")` finds "Smith".
  The comparison is left-anchored and length is the RIGHT operand's length.
  [verified: SET_EXACT.md:19,23 + OPS_COMPARISON.md:57]
- **EXACT ON — full match (trailing blanks ignored):** `"Smith" = "Smith "` is
  TRUE (trailing blanks dropped), but `"Smith" = "S"` is FALSE. `SEEK("Smith")`
  finds "Smith ". [verified: SET_EXACT.md:21]
- The CHR(0) compatibility quirk: with EXACT OFF, if the RIGHT operand is (or
  begins with) `CHR(0)`, the comparison returns TRUE. [documented:
  OPS_COMPARISON.md:63]

### 4.1 III+-specific WARNING — do NOT import the modern `+` doc

The dbase.com `+` and comparison docs describe **automatic data-type
conversion** in mixed expressions (e.g. `C + N` auto-stringifies the number,
`"abc" + 5` works). **This is FALSE for dBASE III PLUS.** In III+:

- `C + N` (character plus numeric) is a **type-mismatch error** —
  `Data type mismatch` — NOT auto-stringification. You MUST explicitly convert:
  `"Total: " + STR(n)`. [verified: CLAUDE.md Law 3 hazard + III+ surface; the
  dbase.com OPS_PLUS.md auto-conversion text (lines 31-44) is a later-version
  behavior and is WRONG for III+]
- A comparison between mismatched types likewise does not silently coerce the
  way OPS_COMPARISON.md describes for dBASE Plus. Treat OPS_COMPARISON.md's
  "automatically converts data types" section (lines 7-37) as IV/Plus behavior,
  NOT III+. [verified: CLAUDE.md + period III+ semantics]

The ONLY part of OPS_PLUS/OPS_COMPARISON safe to take for III+ is the
SET EXACT string-matching rule itself (Section 4 above), which is III+-stable.
[verified: SET_EXACT.md cross-checked against III+ surface]

Real III+ idiom showing explicit conversion (never `C+N`):
```
@ 1,60 SAY "LAST CHECK # " + LTRIM(STR(lastchk,4,0))
```
[verified: sample CHECK.PRG:19]

---

## 5. Completeness against the HELP index (oracle)

HELP_topics.txt SET-related items and where covered here:

| HELP_topics item | Covered in |
|------------------|-----------|
| SET TO (generic value-set topic) | Section 1.2 (grammar) |
| SET ALTERNATE | 3.4 |
| SET CATALOG | 3.5 |
| SET COLOR | 3.3 |
| SET DATE | 3.2 |
| SET DECIMALS | 3.1 |
| SET DEFAULT | 3.4 |
| SET DELIMITERS | 2 (toggle) + 3.3 (value) |
| SET DEVICE | 3.4 |
| SET FIELDS | 3.5 |
| SET FORMAT | 3.5 |
| SET FILTER | 3.5 |
| SET FUNCTION | 3.6 |
| SET HISTORY | 3.6 |
| SET INDEX | 3.5 |
| SET MARGIN | 3.1 |
| SET MESSAGE | 3.7 |
| SET MEMOWIDTH | 3.1 |
| SET ORDER | 3.5 |
| SET PATH | 3.4 |
| SET PRINT | 2 (toggle) + 3.4 |
| SET PROCEDURE | 3.6 |
| SET RELATION | 3.5 |
| SET TYPEAHEAD | 3.7 |
| SET VIEW | 3.5 |
| SET BELL | 2 |
| SET CARRY | 2 |
| SET CENTURY | 2 / 3.2 |
| SET CONFIRM | 2 |
| SET CONSOLE | 2 |
| SET DEBUG | 2 |
| SET DELETED | 2 |
| SET DOHISTORY | 2 |
| SET ECHO | 2 |
| SET ESCAPE | 2 |
| SET EXACT | 2 + Section 4 |
| SET FIXED | 2 / 3.1 |
| SET HEADINGS (SET HEADING) | 2 |
| SET HELP | 2 |
| SET INTENSITY | 2 |
| SET MENUS (SET MENU) | 2 |
| SET SAFETY | 2 |
| SET SCOREBOARD | 2 |
| SET STATUS | 2 + 1.3 (CONFIG.DB) |
| SET STEP | 2 |
| SET TALK | 2 |
| SET TITLE | 2 |
| SET UNIQUE | 2 |
| SET ON/OFF, SET ON/OFF 2/3/4 (umbrella topics) | Section 2 table |

Also covered though not a standalone ON/OFF topic: **SET EXCLUSIVE** (2.1).
The HELP index lists `MENUS`/`HEADINGS` as the topic spellings; the runtime verb
is `SET MENU`/`SET HEADING` (both spellings accepted by the parser).
[oracle-resolves: confirm both spellings parse in 1.1]

**Every SET item in HELP_topics.txt is covered.** No in-scope SET item is left
undocumented. Items where the *exact default/range/limit* is not pinned by a
local source are flagged inline and listed below.

---

## 6. Open questions ([oracle-resolves] — need DBASE.EXE under DOSBox)

1. **SET DECIMALS maximum in III+.** dbase.com says 34 (a modern value). III+ is
   lower (period docs ~15–18). Confirm the 1.1 cap. -> GAPS.md
2. **SET TYPEAHEAD default and range in III+.** Period refs cite default 20,
   range 0–32000; dbase.com (modern) says default 50, range 0–1600. Confirm 1.1
   values. -> GAPS.md
3. **SET MEMOWIDTH default** (50 vs 60). Confirm 1.1. -> GAPS.md
4. **SET DATE keyword list/spellings** accepted by 1.1 (AMERICAN/ANSI/BRITISH/
   ITALIAN/FRENCH/GERMAN/JAPAN/USA). -> GAPS.md
5. **SET RELATION** — does 1.1 accept multiple `INTO` clauses in one statement,
   or one relation per area only? -> GAPS.md
6. **SET PROCEDURE** — the 32-procedure-per-file cap for 1.1. -> GAPS.md
7. **SET FUNCTION** — exact default F2..F10 macro strings for 1.1. -> GAPS.md
8. **SET COLOR** — exact default attribute string at boot for 1.1. -> GAPS.md
9. **SET EXCLUSIVE** — is the verb accepted by the base 1.1 DBASE.EXE, or only
   in the LAN/Administrator build? -> GAPS.md
10. **CONFIG.DB full keyword set** accepted by 1.1 (which SETs + non-SET tuning
    keywords). The golden CONFIG.DB only exercises STATUS= and COMMAND=. ->
    GAPS.md
11. **SCOREBOARD vs STATUS exact screen rows** in 1.1 (line 0 vs line 24
    placement). -> GAPS.md
12. Whether `SET DECIMALS TO` / `SET MARGIN TO` with no value reset to default
    in 1.1 (confirmed for dbase.com modern; assumed for III+). -> GAPS.md

---

## Sources

- `archive/golden-mined/HELP_topics.txt` — canonical 406-item command/function/
  concept index; COMPLETENESS oracle for which SETs exist in III+. (lines
  113–137 value-SETs; 138–141 ON/OFF umbrella; 202–224 individual toggles)
- `archive/golden-mined/ASSIST.HLP.strings.txt` — period-exact syntax for
  SET FORMAT (8–12), SET FILTER (13–19), SET CATALOG (20–25), SET VIEW (26–31),
  SET DEFAULT/"SET DRIVE" (252–256); plus USE INDEX, CREATE VIEW (47–54).
- `archive/golden-mined/DBASE.MSG.strings.txt` — error catalog: `Cyclic
  relation.` (42), `Invalid DOS SET option.` (96), index errors (19–26,
  109–111).
- `archive/dbase-com-help/SET_EXACT.md` — SET EXACT semantics (default OFF,
  begins-with vs full match). III+-stable.
- `archive/dbase-com-help/OPS_COMPARISON.md`, `OPS_PLUS.md`, `OPS_MINUS.md` —
  operator/comparison semantics; the SET EXACT rule (OPS_COMPARISON:57,63) is
  III+-valid; the auto-type-conversion sections (OPS_COMPARISON:7-37,
  OPS_PLUS:31-44) are IV/Plus and are explicitly flagged WRONG for III+.
- `goldens/dbase-iii-plus-1.1-pristine/files/System_1/CONFIG.DB` and
  `goldens/dbase-iii-plus-1.1/extracted/disk1/CONFIG.DB` — golden CONFIG.DB:
  `STATUS=ON` / `COMMAND=ASSIST` (CR/LF), the startup environment.
- `goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/*.PRG`
  — real III+ idioms: CBMENU.PRG (environment block 17–25; SET COLOR menu
  toggle 75/78), CHECK.PRG (explicit STR() conversion, line 19), SHOW_FLT.PRG
  (SET INDEX TO list, 47), REPORTS.PRG (SET PROCEDURE, 15), RPRTPRO.PRG
  (SET PRINT ON/OFF 121/428), FLT_INFO.PRG / FLT_CNCL.PRG (SET CONSOLE), and
  `SET EXCLUSIVE`/`SET SAFETY`/`SET STATUS` usages.
- dbase.com help (modern, III+-stable core, IV+ caveats flagged): SET DECIMALS
  (default 2; modern max 34), SET TYPEAHEAD (modern default 50, range 0–1600),
  SET DATE, SET COLOR, SET DELIMITERS, SET RELATION, SET ORDER, SET FIELDS,
  SET MESSAGE, SET PATH, SET DEVICE, SET ALTERNATE, SET FUNCTION.
- WebSearch period references — SET DECIMALS default 2, SET HISTORY default 20,
  SET MARGIN default 0, SET TYPEAHEAD (period) default 50/20.
- `oracles/harbour/doc/`, `oracles/xharbour/doc/` — Clipper-lineage semantic
  oracle (NOT period-exact syntax); used only to corroborate SET semantics.
- CLAUDE.md — Laws (Law 3 III+-is-target; the `C+N` type-mismatch hazard).

Cross-links: `specs/file-formats/scr-and-fmt.md` (SET FORMAT `.fmt` contents),
`specs/language/expressions-and-operators.md` (`+`, comparison, SET EXACT interaction),
`specs/environment/config-db.md` (CONFIG.DB full keyword set), `specs/data/indexes.md`
(SET INDEX/ORDER/UNIQUE), `specs/data/relations.md` (SET RELATION/FILTER).

---

## Verification

Adversarial verification pass on 2026-06-16 against the local oracles.

### Completeness vs the HELP index (the COMPLETENESS oracle)
Cross-checked every SET topic in `HELP_topics.txt`:
- Value-SETs, lines 113–137 (25 topics incl. the generic `SET TO`): ALTERNATE,
  CATALOG, COLOR, DATE, DECIMALS, DEFAULT, DELIMITERS, DEVICE, FIELDS, FORMAT,
  FILTER, FUNCTION, HISTORY, INDEX, MARGIN, MESSAGE, MEMOWIDTH, ORDER, PATH,
  PRINT, PROCEDURE, RELATION, TYPEAHEAD, VIEW. **All present.**
- Toggle-SETs, lines 202–224 (23 topics): BELL, CARRY, CENTURY, CONFIRM,
  CONSOLE, DEBUG, DELETED, DOHISTORY, ECHO, ESCAPE, EXACT, FIXED, HEADINGS,
  HELP, INTENSITY, MENUS, SAFETY, SCOREBOARD, STATUS, STEP, TALK, TITLE, UNIQUE.
  **All present.**
- Umbrella topics `SET ON/OFF`(2/3/4), lines 138–141: covered by the Section 2
  table.
**Result: zero in-scope SET items missing.** No item was added — the doc was
already complete. (`@`-prefixed duplicates at 341–369 are the same index keyed
for the `@`-help lookup, not additional commands.)

### III+ correctness (no later-version rules imported)
- Scanned `HELP_topics.txt` for IV+/5 SETs that must NOT appear here: WINDOW,
  CLOCK, ODOMETER, NEAR, POINT, CURRENCY, BORDER, BLOCKSIZE, TRAP, SKIP-as-SET —
  **none present** (only the navigation command `SKIP`, line 91, and
  `DISPLAY/LIST STATUS`, 33/56, which the doc relies on). The doc correctly
  omits all IV-only SETs and correctly flags `SET SKIP`, multi-`INTO`
  `SET RELATION`, `SET DATE TO <picture>`, and multiple concurrent procedure
  files as IV+ extensions.
- The central `C + N` hazard (Section 4.1) is preserved: III+ treats character +
  numeric as a `Data type mismatch`, NOT auto-stringification. The dbase.com
  OPS_PLUS/OPS_COMPARISON auto-conversion text remains explicitly flagged WRONG
  for III+; only the SET EXACT begins-with rule is taken as III+-stable. No
  array/`DECLARE`, `==`, or IV-only function/clause was found in the doc.

### Semantics cross-checked against the second (Clipper-lineage) oracle
`oracles/harbour/doc/en/set.txt` corroborates and several tags were raised to
[verified]:
- SET DECIMALS default **2**; the FIXED-OFF rule (DECIMALS governs `/`, EXP,
  LOG, SQRT, VAL; `+`/`-` take greatest operand decimals; `*` sums them) now
  cites set.txt:1238-1260. The earlier loose "minimum" wording was corrected.
- SET MARGIN default **0** — set.txt:301-304.
- SET DATE keyword list and every date picture match the oracle exactly
  (set.txt:130-140, 1147); tag raised to [verified].
- SET TYPEAHEAD: the Clipper/Harbour values (default 50, min 16, max 4096,
  set.txt:383-386) are recorded as lineage-only and explicitly distinguished
  from the III+ value (20) — kept as [oracle-resolves].
- SET EXCLUSIVE existence in base 1.1 strengthened with `DBASE.MSG`:105-107
  (`Exclusive open of file is required.` / `File is in use by another.`),
  showing the shared-access machinery ships in the 1.1 runtime.
- ASSIST period-exact quotes for FORMAT/FILTER/CATALOG/VIEW/DEFAULT
  (ASSIST.HLP.strings.txt:8-31, 252-256) verified byte-faithful, including the
  `#...#` highlight markup and the "SET DRIVE" title over the SET DEFAULT body.
- `Cyclic relation.` confirmed at DBASE.MSG:42; `Invalid DOS SET option.` at 96.

### Fixes applied this pass
1. SET DECIMALS / FIXED interplay reworded to the precise per-operator rule and
   re-cited to the harbour decimals-OFF table (was a loose "minimum").
2. SET MARGIN default re-cited to the semantic oracle ([verified]).
3. SET DATE keyword list/pictures raised to [verified] against set.txt.
4. SET EXCLUSIVE existence corroborated with the DBASE.MSG locking diagnostics.
5. SET TYPEAHEAD: Clipper default/min/max recorded as lineage values, kept
   distinct from the III+ default (20).
No content errors required reversal; the doc imported no later-version behavior.

### Residual [oracle-resolves] gaps (need DBASE.EXE under DOSBox)
Unchanged from Section 6: SET DECIMALS max, SET TYPEAHEAD default/range,
SET MEMOWIDTH default (50 vs 60), exact SET DATE keyword spellings accepted by
1.1, SET RELATION multi-`INTO`, SET PROCEDURE 32-proc cap, default F-key macro
strings, default SET COLOR boot string, SET EXCLUSIVE default/verb confirmation,
full CONFIG.DB keyword set, SCOREBOARD-vs-STATUS screen rows, and whether
`SET DECIMALS TO`/`SET MARGIN TO` (no value) reset to default in 1.1. None of
these blocks the spec; all are numeric/string defaults the interpreter resolves.
