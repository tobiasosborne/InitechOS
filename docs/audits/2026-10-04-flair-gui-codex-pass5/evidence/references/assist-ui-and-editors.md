# Full-Screen UI: the ASSIST menu front-end and the interactive editors (dBASE III PLUS 1.1)

**Status / confidence.** TARGET = dBASE III PLUS 1.1 (1986). This document specifies
the two interlocking pieces of the III+ full-screen surface:

1. **ASSIST**  --  the cursor-menu front-end (Set Up / Create / Update / Position /
   Retrieve / Organize / Modify / Tools) that the golden `CONFIG.DB` launches by
   default (`COMMAND=ASSIST`), and which is itself a dBASE program that emits the
   ordinary commands documented below.
2. **The full-screen editors** that ASSIST and the dot prompt both invoke:
   `APPEND` / `EDIT` / `CHANGE` / `BROWSE` field-edit mode, plus the interactive
   `CREATE`/`MODIFY` designers for STRUCTURE, SCREEN, REPORT, LABEL, VIEW, QUERY.

The **command syntax** of each editor entry-point is **byte-exact** from
`ASSIST.HLP.strings.txt` (the period ASSIST help text, with `#...#` highlight
markup) and cross-checked against `HELP.DBS.strings.txt` (the 406-topic HELP
database). The **key model and status-line layouts** are reconstructed from the
verbatim navigation/status strings embedded in `DBASE.MSG.strings.txt` (lines
281-432 are the literal scoreboard/status-line and menu-prompt strings the
interpreter prints). Pixel-exact menu *geometry* (row/column placement, color
attributes, which option sits where) is **not** in any local text source and is
tagged `[oracle-resolves]` against the real interpreter under DOSBox.

Confidence tags: `[verified: A + B]` two independent sources; `[documented: src]`
one source; `[oracle-resolves]` needs the binary; `[inferred]` reasoned.

Cross-refs: `../file-formats/scr-and-fmt.md` (.SCR/.FMT byte layout),
`../file-formats/frm.md`, `../file-formats/lbl.md`, `../file-formats/qry.md`,
`../file-formats/vue.md`, `../reports-labels/` (report/label designer semantics),
`../commands/data-definition-and-manipulation.md` (CREATE/APPEND/MODIFY STRUCTURE),
`../commands/navigation-query-display.md` (BROWSE/EDIT/scope), `../language/
expressions-and-operators.md` (the C+N mismatch rule used by GET validation).

---

## 0. Completeness against the oracle (HELP_topics.txt)

This file owns the **full-screen / interactive-UI** slice of the 406-item index.
Items fully covered here:

`ASSIST` (the menu shell); the editor entry-points `APPEND`, `EDIT`, `CHANGE`,
`BROWSE`, `CREATE`, `MODIFY STRUCTURE`, `MODIFY SCREEN`/`CREATE SCREEN` +
`SCREEN`, `MODIFY QUERY`/`CREATE QUERY` + `QUERY`, `MODIFY REPORT`/`CREATE
REPORT`, `MODIFY LABEL`/`CREATE LABEL`, `MODIFY VIEW`/`CREATE VIEW` + `VIEW`,
`MODIFY FILE`, `MODIFY COMMAND` (full-screen text editor, key model shared);
the HELP-system navigation topics `USING HELP`, `HELP KEYS`, `STARTING`,
`SYNTAX`, `MAIN MENU`; the CREATE-designer concept topics `CREATING`, `DEFINING`,
`CREATE EXAMPLE`, `NARRATIVE`, `TYPES`, `FIELD NAME`, `FIELD TYPE`, `FIELD SIZE`,
`DATABASE NAME`, `EDITING`, `USING A DATABASE`, `SAVING`, `SKELETON`, `RECORD`,
`SCOPE` (as they manifest in the editors).

The ASSIST menu tree maps to these commands; the eight ASSIST menu PADs (Set Up,
Create, Update, Position, Retrieve, Organize, Modify, Tools) are described in
Section 2. Items in HELP_topics that belong to **other** specs and are only
referenced here: the report/label *format internals* (`../reports-labels/`),
SET commands (`../environment/`), `@...GET`/`@...SAY`/`PICTURE`/`READ`
(the format-file primitives  --  `../commands/programming-and-io.md` and
`scr-and-fmt.md`).

Items in scope I could **not** fully resolve from local text (-> GAPS.md):
exact on-screen geometry/coordinates of every ASSIST menu and the precise
option list under each PAD; the exact color attribute bytes; the precise
behavior of the ASSIST "action line"/prompt-pad expression builder (F10 field
menu). These are `[oracle-resolves]`. See Section 9.

---

## 1. The ASSIST shell: what it is and how it starts

`ASSIST` invokes "a menu-driven aid for using dBASE III PLUS commands."
[verified: HELP.DBS `_ASSIST` line 444-447 "Invokes a menu-driven aid for using
`8 commands" + ASSIST.HLP whole file is the per-command help ASSIST shows].

Key facts:

- **ASSIST is itself a dBASE program**, not a separate mode. Every menu choice
  composes and executes one ordinary command (`USE`, `CREATE`, `BROWSE`, ...).
  The ASSIST.HLP text is literally the command-format help shown for each menu
  pick, and every entry is one of the documented verbs. [verified: ASSIST.HLP
  command-format blocks + HELP_topics `ASSIST`]
- **Default-on via CONFIG.DB.** The golden `CONFIG.DB` (all three copies:
  `goldens/dbase-iii-plus-1.1/extracted/disk1/CONFIG.DB`,
  `goldens/dbase-iii-plus-1.0/extracted/disk1/CONFIG.DB`,
  `goldens/dbase-iii-plus-1.1-pristine/files/System_1/CONFIG.DB`) contains exactly:
  ```
  STATUS=ON
  COMMAND=ASSIST
  ```
  `COMMAND=<cmd>` runs `<cmd>` immediately after dBASE boots (after the license
  screen). With the shipped default, **a fresh dBASE III PLUS starts in ASSIST,
  not at the dot prompt.** [verified: golden CONFIG.DB bytes + HELP.DBS `STARTING`
  topic "When you exit ASSIST, there is a dot at the bottom of the screen, which is
  the DOT PROMPT"]. `STATUS=ON` forces the status line on (Section 5).
- **Leaving ASSIST -> dot prompt.** Pressing `Esc` from the top menu drops to the
  dot prompt; typing `ASSIST` (or selecting it) returns. [verified: HELP `STARTING`
  + DBASE.MSG "Type a command (or ASSIST) and press the ENTER key" line 402].

### 1.1 The ASSIST navigation model (cursor menus)

ASSIST is a row of **menu PADs** across the top; selecting a PAD drops a **popup
menu** of options; selecting an option may drop a further **submenu** or open a
**prompt pad / action line** asking for a file name or expression. The literal
navigation prompts the shell prints on the message line are in DBASE.MSG
(reconstructed templates; the interpreter fills the `^` placeholders with the
arrow/Enter glyphs at runtime):

| Verbatim DBASE.MSG string (line) | Role |
|---|---|
| `Position selection bar - . Select - . Leave menu - . Exit - Esc.` (356-359) | standard popup-menu footer |
| `Position selection bar - . Select - . Previous menu - F10. Exit - Esc.` (380-382) | submenu footer, F10 backs up one level |
| `Position selection bar - . Select - . Exit with Esc or enter a command.` (377-379) | top action line |
| `Previous screen - PgUp. Previous menu - F10. Exit with Esc or enter a command.` (383) | multi-screen menu |
| `Enter the name of a menu option. Finish with .` (384) | type-ahead menu pick |
| `Position selection bar - . Select - . Close files - Esc. Leave menu - .` (360-362) | Set Up menu footer (Esc closes files) |
| `Enter a file name. Finish with .` (344) | file prompt pad |
| `Reading disk Directory.` (335) | "?" file-picker is reading the dir |
| `Enter a field name or an expression. F10 for a field menu. Finish with .` (345) | expression action line, F10 pops a field menu |
| `Enter an expression. F10 for a field menu. Finish with .` (346) | expression action line |
| `Enter an integer. ... to increase or decrease current value. Finish with .` (336-340) | numeric spinner (e.g. SORT direction, decimals) |
| `Press ... to select new default drive.` (338-339) | SET DEFAULT picker |

[verified: DBASE.MSG.strings.txt lines 332-432 are these literal strings].

The universal key bindings inside ASSIST:

| Key | Action |
|---|---|
| Up / Down arrow | move the selection bar within a popup menu |
| Left / Right arrow | move between top PADs (and between option columns) |
| Enter (`Return`/`<CR>`) | select the highlighted option / accept |
| `Esc` | back out one level; at the top menu, exit ASSIST to dot prompt; in Set Up it closes files |
| `F10` | go to the **previous menu** (back up one level); on an expression action line it pops the **field menu** |
| `F1` | HELP |
| PgUp / PgDn | previous / next screen of a multi-screen list |
| Letter key | type-ahead select an option whose name starts with that letter |

`[verified: DBASE.MSG nav strings + HELP.DBS HELP KEYS]`. Note: `F10` has **two**
meanings  --  "previous menu" inside menus, and "field menu" on an expression line.
[verified: DBASE.MSG line 345/393 "The F10 key displays the previous Menu screen"
+ "F10 for a field menu"].

---

## 2. The ASSIST menu tree (PAD -> options -> command)

The eight top-row PADs and the command each option emits. Option lists are
reconstructed by matching every ASSIST.HLP command-format block to its natural
PAD; the **exact on-screen ordering/labels** are `[oracle-resolves]`, but the
**set of commands reachable from each PAD** is `[verified: ASSIST.HLP]` because
ASSIST.HLP is precisely the per-option help for these menus.

### 2.1 Set Up
Selects the working environment. Footer offers "Close files - Esc."
| Option | Emits | Help (ASSIST.HLP) |
|---|---|---|
| Database file | `USE [<dbf>/?] [INDEX <ndx>] [ALIAS <alias>]` | "selects the active database file and its index files" |
| Format for Screen | `SET FORMAT TO [<fmt>/?]` | "custom screen format (.fmt) file ... for full-screen editing" |
| Query | `SET FILTER TO [FILE <qry>/?]/[<condition>]` | "activates the filter condition stored in an existing query (.qry) file" |
| Catalog | `SET CATALOG TO [<cat>/?]` | "only files listed in the active catalog are presented" |
| View | `SET VIEW TO [<vue>/?]` | "defines the operating environment using a view (.vue) file" |
| Quit dBASE III PLUS | `QUIT` | "closes all open files, ends this session ... returns to the OS" |
[verified: ASSIST.HLP lines 1-35]

### 2.2 Create
Each option drops into the corresponding interactive designer (Sections 6-8).
| Option | Emits | Designer |
|---|---|---|
| Database file | `CREATE <dbf> [FROM <structure-extended>]` | STRUCTURE designer (S.6) |
| Format (screen) | `CREATE SCREEN <scr>/?` | SCREEN painter; also writes `.FMT` (S.7) |
| View | `CREATE VIEW <vue>/?/FROM ENVIRONMENT` | VIEW setup (S.7) |
| Query | `CREATE QUERY <qry>/?` | QUERY/filter form (S.7) |
| Report | `CREATE REPORT <frm>/?` | REPORT designer (S.8) |
| Label | `CREATE LABEL <lbl>/?` | LABEL designer (S.8) |
[verified: ASSIST.HLP lines 36-73]

### 2.3 Update
Record-modifying commands; APPEND/EDIT/BROWSE drop into full-screen edit (S.4-5).
| Option | Emits |
|---|---|
| Append | `APPEND [BLANK]/FROM <file> [FOR <cond>] [[TYPE] <type>]/[DELIMITED [WITH BLANK/<delim>]]` |
| Edit | `EDIT [<scope>] [FIELDS <list>] [FOR <cond>] [WHILE <cond>]` |
| Display | `DISPLAY [<scope>] [<exp list>] [FOR <cond>] [WHILE <cond>] [OFF] [TO PRINT]` |
| Browse | `BROWSE [FIELDS <list>] [WIDTH <expN>] [LOCK <expN>] [FREEZE <field>] [NOFOLLOW] [NOAPPEND] [NOMENU]` |
| Replace | `REPLACE [<scope>] <f1> WITH <e1> [,<f2> WITH <e2>]... [FOR <cond>] [WHILE <cond>]` |
| Delete | `DELETE [<scope>] [FOR <cond>] [WHILE <cond>]` |
| Recall | `RECALL [<scope>] [FOR <cond>] [WHILE <cond>]` |
| Pack | `PACK` |
[verified: ASSIST.HLP lines 74-123]

### 2.4 Position
Move the record pointer / search.
| Option | Emits |
|---|---|
| Seek | `SEEK <expression>` (index required) |
| Locate | `LOCATE [<scope>] [FOR <cond>] [WHILE <cond>]` |
| Continue | `CONTINUE` |
| Skip | `SKIP [<expN>]` |
| Goto Record | `GOTO [RECORD] <expN> / BOTTOM / TOP` |
[verified: ASSIST.HLP lines 124-147]

### 2.5 Retrieve
Output / reporting (these are the *use* side of the Create-menu designers).
| Option | Emits |
|---|---|
| List | `LIST [<scope>] [<exp list>] [FOR <cond>] [WHILE <cond>] [TO PRINT] [OFF]` |
| Display | `DISPLAY [<scope>] [<exp list>] [FOR <cond>] [WHILE <cond>] [OFF] [TO PRINT]` |
| Report | `REPORT FORM <name>/? [<scope>] [FOR <cond>] [WHILE <cond>] [TO PRINT] [PLAIN] [NOEJECT] [HEADING <string>] [TO FILE <file>]` |
| Label | `LABEL FORM <name>/? [<scope>] [FOR <cond>] [WHILE <cond>] [TO PRINT] [SAMPLE] [TO FILE <file>]` |
| Sum | `SUM [<exp list>] [<scope>] [FOR <cond>] [WHILE <cond>] [TO <memvar list>]` |
| Average | `AVERAGE [<exp list>] [<scope>] [FOR <cond>] [WHILE <cond>] [TO <memvar list>]` |
| Count | `COUNT [<scope>] [FOR <cond>] [WHILE <cond>] [TO <memvar>]` |
[verified: ASSIST.HLP lines 148-196]

### 2.6 Organize
Index / sort / copy / structure operations.
| Option | Emits |
|---|---|
| Index | `INDEX ON <key expression> TO <ndx> [UNIQUE]` |
| Sort | `SORT [<scope>] TO <new file> ON <f1> [/A]/[/C]/[/D] [,<f2>] [/A]/[/C]/[/D]... [FOR <cond>] [WHILE <cond>]` (ASSIST sort is always ascending) |
| Copy | `COPY TO <file> [<scope>] [FIELDS <list>] [FOR <cond>] [WHILE <cond>] [[[TYPE] <type>]/DELIMITED [WITH BLANK/<delim>]]` |
[verified: ASSIST.HLP lines 197-220]

### 2.7 Modify
Re-enter a designer on an existing object.
| Option | Emits | Designer |
|---|---|---|
| Database file | `MODIFY STRUCTURE` | STRUCTURE (S.6) |
| Format (screen) | `MODIFY SCREEN <scr>/?` | SCREEN painter (S.7) |
| View | `MODIFY VIEW <vue>/?` | VIEW (S.7) |
| Query | `MODIFY QUERY <qry>/?` | QUERY (S.7) |
| Report | `MODIFY REPORT <frm>/?` | REPORT (S.8) |
| Label | `MODIFY LABEL <lbl>/?` | LABEL (S.8) |
[verified: ASSIST.HLP lines 221-251]

`MODIFY STRUCTURE` warning (period-exact): "Information is automatically restored
after changes are saved, but some information is lost if changes to field names
and field lengths are done at once. Do these changes separately."
[verified: ASSIST.HLP lines 221-226]. (HELP.DBS `_MODIFY STRUCTURE` 893-898 only
gives the syntax + `^End`-save/`Esc`-abandon keys; the data-loss caveat lives in
ASSIST.HLP, NOT in HELP.DBS.) Engine rule: III+ repopulates the new file by
matching **old field name -> new field name**; if you both rename a field and
change its length in one pass it cannot match the column and the data for that
field is lost. [documented: ASSIST.HLP 221-226; `[inferred]` mechanism].

### 2.8 Tools
File/OS utilities + structure listing.
| Option | Emits |
|---|---|
| Set Drive (Default) | `SET DEFAULT TO <drive>` (note: ASSIST.HLP labels this "SET DRIVE" but the command is `SET DEFAULT`; changes the dBASE search drive, NOT the DOS logged drive) |
| Copy File | `COPY FILE <source> TO <target>` (extensions required) |
| Directory | `DIR [[LIKE] <path>] [<skeleton>] / [ON <drive>:] [TO PRINT]` |
| Rename | `RENAME <current> TO <new>` (extensions required; updates catalog) |
| Erase | `ERASE <file>/?` (extensions required; updates catalog) |
| List Structure | `LIST STRUCTURE` (file name, #records, last-update date, field descriptions, bytes/record) |
| Import | `IMPORT FROM <file> [TYPE] <type>` (PFS) |
| Export | `EXPORT TO <file> [TYPE] <type>` (PFS) |
[verified: ASSIST.HLP lines 252-287]

> The `?` token in any `<file>/?` slot opens the **interactive file picker**
> (a popup of files of that type, filtered by the active catalog if one is set).
> DBASE.MSG "Reading disk Directory." (335) is the picker's busy message, and the
> "Enter file name:" family (226-238) are the typed prompts. [verified: ASSIST.HLP
> "/?" + DBASE.MSG].

---

## 3. The HELP system (F1) overlay

The HELP system is a second cursor-menu overlay reachable by `F1` anywhere, or by
`HELP [<keyword>]` at the dot prompt. It shares ASSIST's key model.

- `HELP` -> main menu; `HELP <command>` -> that command's screen. [verified:
  HELP.DBS `STARTING` lines 1659-1669].
- Two screen kinds: **Menu screens** and **Information screens**. [verified:
  HELP.DBS `USING HELP` 1670-1674].
- Keys (verbatim from HELP.DBS `HELP KEYS` 1675-1685, glyph placeholders resolved):
  - Up/Down arrow position the highlight on a menu option or on the `ENTER >`
    prompt.
  - `Enter` selects the highlighted option.
  - `Esc` exits HELP back to the dot prompt.
  - PgUp/PgDn show previous/next screen.
  - At the `ENTER >` prompt (bottom of every HELP screen) you may type a command,
    a **screen name** (shown upper-right of the screen), or a menu item number,
    then `Enter`.
- Information-screen layout uses these labeled sections (DBASE.MSG 386-391):
  ` Syntax : `, ` Description : `, ` Example : `, ` See also : `, with the
  product banner `dBASE III PLUS`. [verified: DBASE.MSG + HELP.DBS body which uses
  `_` (title), `` `5 `` (syntax), `` `6 `` (description), `` `9 `` (see also)
  markers].
- `SET HELP OFF` suppresses the "Do you want some help? (Y/N)" auto-prompt
  (DBASE.MSG 169) that III+ pops on certain errors. [verified: DBASE.MSG + CBMENU.PRG
  which does `SET HELP OFF` line 22].

The `SYNTAX` topic defines the notation used throughout (and which this spec
mirrors): `< >` user-supplied; `[ ]` optional; `...` repetition; `/` choice.
[verified: HELP.DBS `SYNTAX` 1686-1696].

---

## 4. Full-screen record editing: APPEND / EDIT / CHANGE / READ

These three commands plus `READ` (over a `.FMT`) all enter the same **field-edit
mode**  --  one record on screen, fields laid out either by the default
"stacked" layout or by the active format file.

### 4.1 Entry points and their differences

```
APPEND                                   && add BLANK records at end, full-screen
APPEND BLANK                             && add ONE blank record, NO full-screen (programmatic)
EDIT   [<scope>] [FIELDS <list>] [FOR <cond>] [WHILE <cond>]
CHANGE [<scope>] [FIELDS <list>] [FOR <cond>] [WHILE <cond>]
READ                                     && edits the @...GET fields posted by the active .FMT / pending GETs
```

- `APPEND` (no BLANK) opens full-screen entry positioned on a **new blank record**
  appended at end of file; pressing PgDn past the last field appends another blank
  record. [verified: ASSIST.HLP 74-79 "addition of new records to the end" +
  HELP.DBS `_APPEND` 431-443 "full-screen data entry mode to add a BLANK record"].
- `APPEND BLANK` (the programmatic form) adds exactly one empty record and does
  **not** enter full-screen mode. [verified: HELP.DBS `_APPEND`].
- `EDIT` edits existing records starting at the current record; you may walk
  forward/back with the paging keys; `FIELDS`/`FOR`/`WHILE`/`<scope>` restrict
  which fields/records are visited. [verified: ASSIST.HLP 80-85 + HELP.DBS
  `_EDIT` 657-665].
- `CHANGE` is functionally `EDIT`  --  "A full-screen editing command you use to
  alter the contents of the specified fields and records." HELP cross-references
  `CHANGE <-> EDIT`. The only difference is historical/alias. [verified: ASSIST has
  no CHANGE block but HELP.DBS `_CHANGE` 484-491 + `See also EDIT`]. `[inferred]`
  that they are byte-identical in behavior; `[oracle-resolves]` whether `CHANGE`
  defaults to a different layout.
- `EDIT RECORD <n>` jumps directly to record n. `EDIT` with no scope edits from
  the current record. [verified: ASSIST.HLP + HELP `SCOPE` 1842-1853].

### 4.2 The field-edit key model (status line + bindings)

The interpreter prints, at the bottom of the EDIT/APPEND screen, this verbatim
status-key panel (DBASE.MSG lines 300-307  --  the `Memo: ^Home` / `Exit/Save: ^End`
panel):

```
1------------------2---------------------2--------------2--------------------3
  CURSOR   <-- -->           UP   DOWN        DELETE      Insert Mode:  Ins
   Char:
    Field:
      Char:   Del    Exit/Save:   ^End
   Word:  Home End   Page:  PgUp  PgDn     Field:  ^Y     Abort:        Esc
                     Help:   F1            Record: ^U     Memo:        ^Home
```

(DBASE.MSG also carries a sibling panel at lines 281-289 that reads `Set Options:
^Home`, `Exit: ^End`, `Field: Home End`, and a `Pan: ^` segment  --  a different
editor variant; **which** full-screen mode each panel drives  --  APPEND vs. the
SCREEN/format painter vs. MODIFY STRUCTURE  --  is not pinned by the local text and
is `[oracle-resolves]`. The panel reproduced here is the 300-307 EDIT/APPEND one.)

Reconstructed binding table (the segments above pair a label with its key):

| Action | Key | Notes |
|---|---|---|
| Move left / right one char | Left / Right arrow | |
| Move word left / right | Home / End | "Word: Home End" |
| Up one field / Down one field | Up / Down arrow | "UP DOWN" |
| Previous / next record (page) | PgUp / PgDn | from last field, PgDn appends in APPEND |
| Toggle Insert/Overwrite | Ins | status shows "Insert Mode: Ins" |
| Delete char under cursor | Del | |
| Delete (blank) the whole field | `^Y` | "Field: ^Y" (Ctrl-Y) |
| Delete/clear the record (mark/blank) | `^U` | "Record: ^U" (Ctrl-U)  --  toggles the deletion mark in EDIT |
| Open / edit a Memo field | `^Home` | "Memo: ^Home" enters the memo full-screen editor |
| Save and exit | `^End` | "Exit/Save: ^End" (the panel shows only `^End`; `^W` also works as a WordStar save but is **not** in this EDIT panel  --  it is the `Save: ^W` of the MODIFY COMMAND text editor, section9) |
| Abandon (no save) | `Esc` | "Abort: Esc" |
| Help | `F1` | |

[verified: DBASE.MSG lines 300-307 are the literal EDIT/APPEND panel strings +
HELP.DBS `_DELETE` "recognized as marked for deletion by the word Del in
full-screen mode"].

III+-specific edit semantics:
- **Field validation on exit-from-field.** A `GET` carrying `PICTURE`/`RANGE`
  validates when you leave the field. `RANGE` rejects out-of-range numeric/date
  with "Range value must be numeric. Press any key to continue." (DBASE.MSG 432)
  for a non-numeric range; out-of-range numeric shows `^--- Out of range.`
  (DBASE.MSG 72). [verified: DBASE.MSG + HELP `@...GET` 379-387].
- **C + N is a TYPE-MISMATCH error.** When the active `.FMT`'s `GET` expressions
  or any expression the editor evaluates mix a character and a numeric value with
  `+`, III+ raises **"Data type mismatch."** (DBASE.MSG 9). It does **NOT**
  auto-stringify. The dbase.com `+` help (OPS_PLUS.md line 31: "The other data type
  is converted into its display representation") documents *later* dBASE behavior
  and is **wrong for III+**. [verified: DBASE.MSG "Data type mismatch" + III-vs-
  later delta; see `../language/expressions-and-operators.md`].
- **SET CONFIRM** (OFF by default): with CONFIRM OFF, filling a fixed-width field
  auto-advances to the next field; with CONFIRM ON you must press Enter to advance.
  [documented: HELP_topics `SET CONFIRM`; behavior `[oracle-resolves]` in editor].
- **SET CARRY** (OFF by default): with CARRY ON, `APPEND` copies the previous
  record's field values into each new blank record. [documented: HELP_topics
  `SET CARRY`].
- **The deletion mark "Del"** appears in the status area when the current record is
  marked for deletion; `^U` toggles it. [verified: HELP.DBS `_DELETE` 553-562 "by
  the word Del in full-screen mode" + DBASE.MSG "Del" tokens 189-190].
- **Read-only / locked records** (network/admin): the editor shows `Read Only`,
  `Record Locked`, `File Locked`, `Exclusive` (DBASE.MSG 214-218). [documented:
  DBASE.MSG].

### 4.3 Format-file driven layout (.FMT)

If `SET FORMAT TO <fmt>` is active (or a `.FMT` was generated by CREATE SCREEN),
`APPEND`/`EDIT`/`READ` lay the record out per the `.FMT` instead of the default
stacked list. A `.FMT` is a plain dBASE program of `@ r,c SAY/GET/TO` lines. Real
golden example (`CLIENTS.FMT`, verbatim):

```
@  4, 21  SAY "CLIENTS ADDRESS AND PHONE ENTRY"
@  9,  1  SAY "FIRST NAME:"
@  9, 14  GET  CLIENTS->FIRSTNAME
@  9, 37  SAY "LAST NAME:"
@  9, 50  GET  CLIENTS->LASTNAME
@ 11,  1  SAY "PHONE NUMBER:"
@ 11, 16  GET  CLIENTS->PHONE
@  8,  0  TO 12, 79
@ 14,  0  TO 18, 79
```
[verified: golden `Sample_Programs_and_Utilities/CLIENTS.FMT` bytes]. The
field-edit key model (4.2) is identical; only the layout changes. The status panel
for a format-driven EDIT is the EDIT panel above. See `../file-formats/scr-and-fmt.md`
for the `.SCR`/`.FMT` byte format and `../commands/programming-and-io.md` for
`@...GET`/`READ`.

---

## 5. BROWSE: the spreadsheet-style multi-record editor

`BROWSE` shows many records as rows and fields as columns in a scrolling window.

### 5.1 Full syntax (period-exact)

```
BROWSE [FIELDS <field list>] [WIDTH <expN>]
       [LOCK <expN>] [FREEZE <field>]
       [NOFOLLOW] [NOAPPEND] [NOMENU]
```
[verified: ASSIST.HLP 92-98 + HELP.DBS `_BROWSE` 458-470]. (HELP.DBS lists the
clauses in a slightly different order: `BROWSE [FIELDS <list>] [LOCK <expN>]
[WIDTH <expN>] [FREEZE <field>] [NOFOLLOW] [NOAPPEND] [NOMENU]`  --  same set.)

Clause semantics (verbatim HELP.DBS `_BROWSE`):
- `FIELDS <list>`  --  which fields (columns) to edit, in this order. Default = all
  fields in structure order (or the `SET FIELDS` list).
- `WIDTH <expN>`  --  "defines editing width of character fields" (truncates the
  on-screen column to expN; the full value is still stored/scrollable).
- `LOCK <expN>`  --  "the number of leftmost fields which remain stationary while
  panning." (Frozen leftmost columns, like a spreadsheet freeze-panes.)
- `FREEZE <field>`  --  restricts editing to that single named field (cursor stays in
  one column; the others are display-only).
- `NOFOLLOW`  --  "displays the current indexed record by replacing the record with
  the altered key field." i.e. when you change an indexed key value, the cursor
  normally *follows* the record to its new sorted position; NOFOLLOW keeps the
  cursor in place instead.
- `NOAPPEND`  --  prevents adding new records (disables the append-past-bottom row).
- `NOMENU`  --  turns off the BROWSE option menu (the F1-toggle menu, 5.3).

### 5.2 BROWSE column layout and status

- Top status line shows the file/record context (DBASE.MSG `Rec:` 249, `Field:`
  250, `Column:` 252 tokens). Records are rows; fields are columns separated by
  column rules.
- Bottom: the BROWSE help line. The "added" message (DBASE.MSG 191) confirms an
  appended record.
- Character columns wider than the window pan horizontally; `LOCK` keeps the
  leftmost N columns pinned during pan. `[verified: ASSIST.HLP/HELP "panning" +
  DBASE.MSG]`.

### 5.3 The BROWSE menu (top-of-screen, F1 toggles it)

"Access many of the BROWSE command options from an optional menu at the top of
the display." [verified: ASSIST.HLP 92-95]. The menu's options come straight from
the DBASE.MSG single-word strings 201-207 (the literal menu-pad labels):

| Label (DBASE.MSG) | Action |
|---|---|
| `Top` (207) | go to top record |
| `Bottom` (201) | go to bottom record |
| `Lock` (205) | set the number of locked leftmost columns interactively |
| `Freeze` (203) | choose a single field to freeze editing on |
| `None` (206) | clear Freeze/Lock |
| `Help` (204) | help |
| `Abandon` (202) | abandon (discard the record's edits) |

[verified: DBASE.MSG 201-207 are exactly these words, in the editor's word table].
`F1` toggles this menu on/off (NOMENU disables it). [verified: ASSIST.HLP "optional
menu" + DBASE.MSG "Toggle menu: F1" 293].

### 5.4 BROWSE key model

Reuses the field-edit keys (4.2) plus column navigation. The literal panel for
the table editors that BROWSE shares (DBASE.MSG 290-294) is:

```
1------------------2------------------2---------------------2-----------------3
 CURSOR:   <-- -->   Delete char: Del   Insert row:      ^N   Insert:    Ins
   Char:
    Delete word:  ^T   Toggle menu:     F1   Zoom in:  ^PgDn
   Word:  Home End   Delete row:   ^U   Abandon:        Esc   Zoom out: ^PgUp
```

| Action | Key | Source |
|---|---|---|
| Char left / right | Left / Right arrow | panel `CURSOR: <-- -->` |
| Next / previous field (column) | (Right at column end / Left at column start; also Tab in some builds) | `[inferred]` |
| Word left/right within a field | Home / End | panel `Word: Home End` |
| Up / down a record (row) | Up / Down arrow | `[inferred]` (no UP/DOWN label in this panel) |
| Page of records | PgUp / PgDn | `[inferred]` |
| Delete char | Del | panel `Delete char: Del` |
| Delete word | `^T` | panel `Delete word: ^T` |
| Delete (mark) the record / row | `^U` | panel `Delete row: ^U` |
| Insert row | `^N` | panel `Insert row: ^N` |
| Open / toggle the BROWSE menu | `F1` | panel `Toggle menu: F1` |
| Abandon current record edit | `Esc` | panel `Abandon: Esc` |
| Memo field zoom in / out | `^PgDn` / `^PgUp` | panel `Zoom in/out` |
| Toggle Insert | Ins | panel `Insert: Ins` |

[verified: DBASE.MSG 290-294 are exactly this BROWSE-table panel]. `^PgDn`/`^PgUp`
"Zoom in/out" enter and leave the memo full-screen editor for a memo column.
Note the BROWSE panel does **not** print a `^End` save key, a `^Y` field-blank, or
a `Pan: ^` segment (those belong to the EDIT/section4.2 and 281-289 panels); `^End`-save
and column panning in BROWSE are `[inferred]` from the EDIT key family, not from
this panel.

---

## 6. CREATE / MODIFY STRUCTURE  --  the field-definition table editor

`CREATE <dbf> [FROM <structure-extended>]` and `MODIFY STRUCTURE` open the same
table-style editor where each row defines one field.

### 6.1 Layout (verbatim from HELP.DBS `CREATE EXAMPLE` 1970-1980)

```
A:DEMO.DBF                                      Bytes remaining:        3977
                                        Fields defined:            2
     Field Name  Type     Width  Dec      Field Name  Type     Width  Dec
  1  PAIDTO       Character   20
  2  NUMBER       Character    3
```
[verified: HELP.DBS `_CREATE EXAMPLE`]. Two-column field grid; header shows
filename, "Bytes remaining:" (record-length budget toward the 4000-byte / 128-field
limit) and "Fields defined:" counters. Column labels are exactly
`Field Name`, `Type`, `Width`, `Dec`. The DBASE.MSG editor word table confirms the
struct-edit field names `FIELD_NAME`, `FIELD_TYPE`, `FIELD_LEN`, `FIELD_DEC`
(DBASE.MSG 244-247) and the prompt `Field:` (250).

### 6.2 Field-definition rules (verbatim HELP.DBS field topics)

- **Field Name** (`FIELD NAME` 1901-1909): up to **10 characters**; must begin with
  a letter; letters, digits, underscore `_`; no blanks. [verified: HELP.DBS].
- **Database/File Name** (`DATABASE NAME` 1910-1917): up to **8 characters**; begins
  with a letter; digits and `_` allowed; no blanks. [verified: HELP.DBS].
- **Field Type** (`FIELD TYPE` 1918-1930): the **five** III+ types  -- 
  `Character`, `Logical` (T/Y=true, F/N=false), `Numeric` (digits, one decimal
  point, sign), `Memo` (free-form, stored in `.DBT`), `Date` (mm/dd/yy, stored as a
  number). The type prompt accepts the initial letter (C/N/L/M/D). [verified:
  HELP.DBS + DBASE.MSG type words `Character/Date/Logical/Memo/Numeric` 255-259].
  **III+ has NO Float type** (that is dBASE IV) and no `+`-autocoercion. `[III-vs-IV
  delta]`.
- **Field Size** (`FIELD SIZE` 1931-1939): two parts, `FIELD WIDTH` and
  `FIELD DEC`. For Numeric, WIDTH **includes** the decimal places, the decimal
  point, and the sign. `Dec` only applies to Numeric. [verified: HELP.DBS].
  Memo width is fixed at 10 (the 10-byte `.DBT` block pointer); Logical fixed at 1;
  Date fixed at 8. [verified: FIXTURE_MAP field descriptors e.g. `NOTE:M10.0`,
  `PAID:L1.0`, `DATE:D8.0`].

### 6.3 Keys and save/abandon

The MODIFY STRUCTURE editor uses the EDIT-class key panel (4.2). Exit:
- **Save**: `^End` (HELP.DBS `_MODIFY STRUCTURE` 892-898 `^End to save`); also
  pressing `Enter` in the **first space of a new (empty) field row** saves
  (HELP.DBS `CREATE EXAMPLE` 1981-1982: "press the Enter key in the first space of
  a new field or ^End anywhere").
- **Abandon**: `Esc` (discards changes). [verified: HELP.DBS].

On save, III+ asks "Input data records now? (Y/N)" (DBASE.MSG 171) after a
`CREATE`, letting you drop straight into APPEND. [verified: DBASE.MSG].
`MODIFY STRUCTURE` restores data by old->new field-name match (see 2.7 warning).

`COPY STRUCTURE EXTENDED` produces the structure-extended file (one record per
field, columns `FIELD_NAME/FIELD_TYPE/FIELD_LEN/FIELD_DEC`) that `CREATE ... FROM`
reads back. [verified: ASSIST.HLP `CREATE ... FROM <structure extended file>` +
HELP.DBS `_CREATE` 546-552 + DBASE.MSG field words 244-247].

---

## 7. The SCREEN / VIEW / QUERY designers

### 7.1 CREATE/MODIFY SCREEN (the screen painter)

```
CREATE SCREEN <scr>/?      MODIFY SCREEN <scr>/?
```
Designs a full-screen form for a database. Writes **two** files: the `.SCR`
worktable and a generated `.FMT` format file; the new `.fmt` becomes the active
format until another is selected in Set Up. [verified: ASSIST.HLP 41-46 + HELP.DBS
`_CREATE/MODIFY SCREEN` 884-891 "generation of a format file"]. Full `.SCR`/`.FMT`
byte layout: `../file-formats/scr-and-fmt.md`.

Painter mode is a WYSIWYG "blackboard". DBASE.MSG carries its literal action-line
prompts:
- `Enter text. Drag field or box under cursor with . F10 for menu.` (411-412)
- `Move field with . Complete with . Exit drag with Esc.` (413-415)
- `Enter or update picture value. Complete with . Exit with Esc.` (416-417)
- `Enter range value. Complete with . Exit with Esc.` (418-419)
- `Position cursor to box corner with . Complete with . Exit with Esc.` (420-422)
- `Position cursor to other corner with ...` (423-425)  --  drawing a `@...TO` box.
- `Enter a picture template without using quotes. Finish with .` (426)
- `Enter one or more function symbols without using quotes. Finish with .` (427)
- Footer: `Position selection bar - . Select - . Leave menu - . Blackboard - F10.`
  (407-410) and the change variant (428-431).
[verified: DBASE.MSG 407-432]. `F10` here toggles the **Blackboard** (the live
screen) vs. the menu. Keys reuse the EDIT panel; box-drawing is `@...TO` /
`@...TO ... DOUBLE`. The picture/function symbols are the PICTURE template set
(`9 # * $ , .` numeric/money; `A N X !` char; `Y L` logical; `D E` date  -- 
HELP.DBS `PICTURE` 411-422), see `scr-and-fmt.md`.

### 7.2 CREATE/MODIFY VIEW

```
CREATE VIEW <vue>/?/FROM ENVIRONMENT       MODIFY VIEW <vue>/?
```
Defines the **operating environment**: the set of database files, their index
files, their relationships, the active fields from each, and optionally a format
and a filter. "Views provide a means for using more than one database file in
ASSIST." Remains in use until another view or database is selected in Set Up.
`CREATE VIEW FROM ENVIRONMENT` snapshots the *current* open environment into a
`.vue`. [verified: ASSIST.HLP 47-54 / 232-236 + HELP.DBS `_MODIFY VIEW` 899-907].
The VIEW setup screen is a multi-pane selector (databases, indexes, relations,
fields). Exact panes `[oracle-resolves]`. `.VUE` byte layout: `../file-formats/
vue.md`. Real golden: `Sample_Programs_and_Utilities/RESERVE.VUE`.

### 7.3 CREATE/MODIFY QUERY (the filter form)

```
CREATE QUERY <qry>/?       MODIFY QUERY <qry>/?
```
Builds a filter condition via a query form, saves it in a `.qry`, and activates it
(equivalent to `SET FILTER TO FILE <qry>`). Records not meeting the condition are
ignored until another filter is selected in Set Up. [verified: ASSIST.HLP 55-61 /
237-241 + HELP.DBS `_CREATE/MODIFY QUERY` 867-874]. The query form prompt-pad
footer (DBASE.MSG 368-371): `Select - . Leave prompt pad - .` and
`Next/Previous record - PgDn/PgUp. Toggle query form - F1. Leave option - .` and
`Leave prompt pad - .`. The expression action line uses `Enter a field name or an
expression. F10 for a field menu.` (DBASE.MSG 345). [verified: DBASE.MSG].
`.QRY` byte layout: `../file-formats/qry.md`. Real golden:
`Sample_Programs_and_Utilities/NOTPAID.QRY`.

---

## 8. The REPORT and LABEL designers

### 8.1 CREATE/MODIFY REPORT

```
CREATE REPORT <frm>/?       MODIFY REPORT <frm>/?
```
Designs a **columnar report** (titles, totals, format controls) and saves it in a
`.FRM` form file; produce output later via the Retrieve-menu `REPORT FORM`.
[verified: ASSIST.HLP 62-67 / 242-246 + HELP.DBS `_CREATE/MODIFY REPORT` 875-882].
The report designer is a multi-page menu (Options / Groups / Columns / Locate).
Its literal prompts (DBASE.MSG): `Enter report title. Exit - Ctrl-End.` (363),
`Enter column heading. Exit - Ctrl-End.` (364), and the column footer `Position
selection bar - . Select - . Prev/Next column - PgUp/PgDn.` (365-367). [verified:
DBASE.MSG]. The status panel for the report/label table editor (DBASE.MSG 295-299):
```
1------------------2--------------------2-------------------2-----------------3
  CURSOR   <-- -->   Delete char:   Del   Insert column: ^N   Insert:   Ins
    Delete word:    ^T   Report format: F1   Zoom in:  ^PgDn
   Word:  Home End   Delete column:  ^U   Abandon:      Esc   Zoom out: ^PgUp
```
`F1` toggles the "Report format" preview. `^N` inserts a column, `^U` deletes a
column. [verified: DBASE.MSG 295-299]. `.FRM` layout: `../file-formats/frm.md` and
`../reports-labels/`. Real golden: `TRAVELER.FRM`, `TRIP.FRM`, `FLT_INFO.FRM`.

### 8.2 CREATE/MODIFY LABEL

```
CREATE LABEL <lbl>/?       MODIFY LABEL <lbl>/?
```
Designs a mailing-label layout, saves it in a `.LBL`; produce output via the
Retrieve-menu `LABEL FORM ... [SAMPLE]`. [verified: ASSIST.HLP 68-73 / 247-251 +
HELP.DBS `_CREATE/MODIFY LABEL` 859-866]. The designer offers predefined label
dimensions (lines/label, width, columns across, spacing) and a content area for
field expressions. Keys reuse the EDIT/table panel. `.LBL` layout:
`../file-formats/lbl.md`. Real golden: `ADDRESS.LBL`.

---

## 9. The MODIFY COMMAND / MODIFY FILE full-screen text editor

`MODIFY COMMAND <prg>` and `MODIFY FILE <file>` open the built-in **full-screen
text editor** (wordwrap-off line editor). Its verbatim key panel (DBASE.MSG
315-323; the panel at 308-314 with `Up a field:`/`Down a field:`/`Field: ^N`/
`Field: ^U`/`Pan: ^` is a *different* editor variant, not the text editor):

```
1------------------2--------------------2-------------------2-----------------3
  CURSOR:  <-- -->           UP  DOWN      DELETE      Insert Mode:    Ins
     Line:
      Char:    Del   Insert line:   ^N
   Word:  Home End   Page: PgUp  PgDn   Word:    ^T    Save: ^W  Abort:Esc
    Find:   ^KF        Line:    ^Y    Read file:     ^KR
   Reformat: ^KB     Refind: ^KL                       Write file:    ^KW
```

| Action | Key |
|---|---|
| Char left/right | Left/Right arrow |
| Word left/right | Home / End |
| Up/Down line | Up / Down arrow |
| Page up/down | PgUp / PgDn |
| Delete char | Del |
| Delete word | `^T` |
| Delete line | `^Y` |
| Insert line | `^N` |
| Toggle Insert | Ins |
| Find | `^KF` ; Refind `^KL` |
| Reformat paragraph | `^KB` |
| Read file into buffer | `^KR` ; Write block to file `^KW` |
| Save & exit | `^W` (also `^End`) |
| Abandon | `Esc` |

[verified: DBASE.MSG 308-323]. These are WordStar-style `^K` control diamonds  -- 
the same family used in all III+ memo/text editors. The 254-char line limit
applies ("Line exceeds maximum of 254 characters." DBASE.MSG 18). `MODIFY COMMAND`
requires no extension (defaults `.prg`); `MODIFY FILE` requires the extension.
[verified: HELP.DBS `MODIFY FILE` notes 857-858].

---

## 10. Status / scoreboard line (SET STATUS / SET SCOREBOARD)

With `STATUS=ON` (the golden CONFIG.DB default), III+ paints a status line at the
bottom (typically row 22) of every full-screen mode. Its literal token strings are
in DBASE.MSG:

- File / record context: `<dbf name>`, `Rec: ` (249), record-of-count via
  `record`/`records`/`to` (208-210), `<READY>` (186), `EOF` (254).
- Lock/insert indicators on the right: `Num`/`Caps`/`NumCaps` (183-185),
  `Ins`/`Del`/`InsDel` (188-190).
- `Record No.` (149), `Record = ` (150), `Field #` (151).

[verified: DBASE.MSG 149-190, 248-254]. `SET STATUS ON/OFF` toggles the status
line; `SET SCOREBOARD ON/OFF` toggles the top-line message area (Ins/range/error
flashes). `CBMENU.PRG` (a real III+ program) sets `SET STATUS OFF`, `SET SCOREBOARD`
is implied off, demonstrating that a finished app turns the scoreboard off for a
clean screen. [verified: golden `Sample_Programs_and_Utilities/CBMENU.PRG` lines
16-24 `SET STATUS OFF`, `SET HELP OFF`, `SET MENU OFF`]. Related: SET MENU ON/OFF
controls whether full-screen editors show their help/menu line.

A real III+ app builds its own ASSIST-like menus with `@...SAY`/`@...TO` boxes
rather than relying on ASSIST; the shipped sample `MENUMASK.PRG` draws nested
double-line boxes with `@ r1,c1 TO r2,c2 DOUBLE` and shadow characters
`CHR(176)`, exactly the idiom CREATE SCREEN emits. [verified: golden
`Sample_Programs_and_Utilities/MENUMASK.PRG` lines 7-22].

---

## 11. III+ vs. later deltas (do not import IV behavior)

| Topic | III+ (this target) | dBASE IV / later (do NOT use) |
|---|---|---|
| `C + N` in a GET/expr | **"Data type mismatch."** error | auto-stringify (OPS_PLUS.md is IV-era) |
| Field types in CREATE | 5: C, N, L, M, D | + Float (F), + binary/OLE later |
| Menu front-end | ASSIST (this menu tree) | dBASE IV "Control Center" (different tree/labels) |
| `==` exact-equal operator | does **not** exist | added in IV |
| Arrays (`DECLARE`) | do **not** exist | added in IV |
| BROWSE menu | top-line toggle menu (F1), labels Top/Bottom/Lock/Freeze/None/Help/Abandon | richer in IV |

[verified: DBASE.MSG error catalog + ASSIST.HLP surface + HELP_topics index
(neither `==`, `DECLARE`, `Float`, nor `Control Center` appear) + OPS_PLUS.md as a
counter-example. `[III-vs-IV delta]`].

---

## 12. Open questions ([oracle-resolves]  --  needs DBASE.EXE/OVL under DOSBox)

1. **Exact ASSIST menu geometry**: row/column of the PAD row, popup origin, the
   precise option ordering and labels under each of the 8 PADs, and the color
   attribute bytes. Local text gives the *command set* per PAD, not pixel layout.
2. **CHANGE vs EDIT default layout**: are they byte-identical, or does CHANGE pick
   a different default field set? HELP only says "see also EDIT."
3. **CONFIRM/CARRY in-editor behavior**: confirm the exact auto-advance and
   value-carry rules in the live APPEND/EDIT loop.
4. **F10 field menu builder**: the exact contents/behavior of the F10 "field menu"
   on an expression action line (which fields, how chosen, how inserted).
5. **VIEW designer panes**: the exact multi-pane layout (databases / indexes /
   relations / fields) and how relations are entered.
6. **Report/label designer page set**: exact menu pages (Options/Groups/Columns/
   Locate for REPORT; dimensions page for LABEL) and field order.
7. **Status-line exact column positions** (row, and column of each token); the
   `1---2---3` panel templates show *segments* but not absolute columns.
8. **Memo zoom editor**: confirm `^Home` (from EDIT) vs `^PgDn` (from BROWSE)
   both reach the same memo full-screen editor.
9. **Which row the status line occupies** and the scoreboard's exact top-line
   format string (`<READY>` placement, range echo).

---

## Sources

- `archive/golden-mined/ASSIST.HLP.strings.txt`  --  period-exact ASSIST command-help
  text (the literal help shown for each menu pick); primary for every command
  format in Sections 2, 4-8. [verified]
- `archive/golden-mined/HELP.DBS.strings.txt`  --  the 406-topic HELP database body;
  primary for the field-definition rules (S.6), HELP-system keys (S.3), BROWSE
  clause semantics (S.5), CREATE EXAMPLE screen (S.6), MODIFY STRUCTURE save/abandon.
- `archive/golden-mined/DBASE.MSG.strings.txt`  --  error/message/word catalog;
  **primary for all status-line and key-panel layouts** (lines 149-432) and the
  BROWSE menu word list (201-207).
- `archive/golden-mined/HELP_topics.txt`  --  the 406-item completeness oracle.
- `archive/golden-mined/FIXTURE_MAP.txt`  --  field descriptors confirming fixed type
  widths (M10, L1, D8).
- `goldens/.../System_1/CONFIG.DB` (and disk1 copies)  --  `COMMAND=ASSIST`,
  `STATUS=ON`. [verified bytes]
- `goldens/.../Sample_Programs_and_Utilities/`: `CLIENTS.FMT`, `CLIENTS.SCR`
  (format-file/screen evidence, S.4.3); `MENUMASK.PRG`, `CBMENU.PRG`
  (real menu/SET idioms, S.10). [verified bytes]
- `archive/dbase-com-help/OPS_PLUS.md`  --  modern `+` doc; used as the **counter-
  example** proving III+ does NOT auto-stringify (S.4.2, S.11).
- Cross-spec: `../file-formats/scr-and-fmt.md`, `frm.md`, `lbl.md`, `qry.md`,
  `vue.md`; `../commands/data-definition-and-manipulation.md`,
  `../commands/navigation-query-display.md`; `../language/
  expressions-and-operators.md`.

---

## Verification

Adversarial re-derivation from a second independent source pass (2026-06-16).
Each key claim was re-checked against a source *other* than the one the author
cited, using the byte-exact mined help as the III+ authority and the
Clipper-lineage / dbase.com docs as the semantic oracle.

**Re-derived and CONFIRMED (independent corroboration):**
- ASSIST command set per PAD  --  every command-format block in Section 2 matches
  `ASSIST.HLP.strings.txt` verbatim (re-read lines 1-287); the eight-PAD command
  set is complete and byte-exact (USE/SET FORMAT/SET FILTER/SET CATALOG/SET VIEW/
  QUIT; CREATE family; APPEND/EDIT/DISPLAY/BROWSE/REPLACE/DELETE/RECALL/PACK;
  SEEK/LOCATE/CONTINUE/SKIP/GOTO; LIST/DISPLAY/REPORT FORM/LABEL FORM/SUM/AVERAGE/
  COUNT; INDEX/SORT/COPY; MODIFY family; SET DEFAULT/COPY FILE/DIR/RENAME/ERASE/
  LIST STRUCTURE/IMPORT/EXPORT). [verified: ASSIST.HLP]
- `CHANGE` IS documented in HELP.DBS with its own syntax block (`CHANGE [<scope>]
  [FIELDS <list>] [FOR][WHILE]`, 486-491, `See also EDIT`)  --  the spec's CHANGE~EDIT
  claim holds; HELP.DBS `_EDIT` cross-refs CHANGE in turn. [verified: HELP.DBS
  484-491 + 657-665]
- Field-definition rules: 10-char field names, 8-char file names, the **five**
  types (Character/Logical/Numeric/Memo/Date), Numeric WIDTH includes dec+point+
  sign  --  all verbatim in HELP.DBS `FIELD NAME`/`DATABASE NAME`/`FIELD TYPE`/`FIELD
  SIZE` (1901-1939). CREATE EXAMPLE screen byte-exact (1970-1982), including the
  "press Enter in the first space of a new field or ^End anywhere" save rule.
  [verified: HELP.DBS]
- BROWSE syntax + clause semantics  --  both clause orders (ASSIST.HLP 96-98 and
  HELP.DBS `_BROWSE` 460-470) verified; FIELDS/LOCK/WIDTH/FREEZE/NOFOLLOW/NOAPPEND/
  NOMENU semantics quoted verbatim. BROWSE menu word list (Bottom/Abandon/Freeze/
  Help/Lock/None/Top) is DBASE.MSG 201-207 exactly. [verified]
- CONFIG.DB: re-read two golden copies; both contain exactly `STATUS=ON` /
  `COMMAND=ASSIST`. [verified bytes]
- C+N => **"Data type mismatch."** (NOT auto-stringify)  --  corroborated three ways:
  DBASE.MSG:9, the sibling `expressions-and-operators.md` (which documents the same
  rule + a coercion table at lines 22-110), and OPS_PLUS.md (dbase.com) explicitly
  framing auto-coercion as the behavior of *later* dBASE. The III-vs-later delta is
  correct. [verified: DBASE.MSG + sibling spec + dbase.com counter-example]
- No `Float`, no `DECLARE`/arrays, no `==` operator anywhere in HELP.DBS /
  HELP_topics / ASSIST.HLP (grepped)  --  Section 11's III-vs-IV deltas are sound.
  [verified: absence-grep over all three mined sources]
- HELP-system nav (STARTING/USING HELP/HELP KEYS/SYNTAX), DELETE "Del in
  full-screen mode", MODIFY COMMAND `.prg` default + MODIFY FILE extension rule,
  and every cited DBASE.MSG line number (9/18/72/432, 201-218, 244-260, 295-299,
  300-307, 315-323, 363-371, 345/393) verified at the byte. [verified]

**CORRECTED (author over-cited or mis-attributed; all III+-internal, none were
imported IV behavior):**
1. section4.2  --  the EDIT/APPEND key panel was attributed to "DBASE.MSG 281-289 **and**
   300-307, one for EDIT/APPEND and one for MODIFY STRUCTURE." Only 300-307 is the
   reproduced `Memo: ^Home`/`Exit/Save: ^End` panel; 281-289 is a *distinct*
   variant (`Set Options: ^Home`, `Exit: ^End`, `Field: Home End`, `Pan: ^`) whose
   owning mode the local text does not pin. Fixed the citation and demoted the
   281-289\u2194MODIFY-STRUCTURE mapping to `[oracle-resolves]`.
2. section4.2  --  "Save and exit `^End` (or `^W`)" overclaimed: the EDIT panel prints only
   `^End`; `Save: ^W` is the MODIFY COMMAND text-editor panel (315-323), not EDIT.
   Annotated.
3. section5.4  --  the BROWSE binding table listed `^End` save, `^Y` field-blank, and a
   `Pan: ^` segment as `[verified: DBASE.MSG 290-307]`, but the BROWSE panel
   (290-294) prints none of those. Re-tagged each row to its actual panel token and
   marked `^End`/pan as `[inferred]`. Also restored the omitted `Char:` continuation
   line in the panel block.
4. section2.7  --  the MODIFY STRUCTURE rename+resize data-loss caveat was cited as
   "[verified: ASSIST.HLP + HELP.DBS `_MODIFY STRUCTURE`]"; HELP.DBS `_MODIFY
   STRUCTURE` (893-898) carries only the syntax + save/abandon keys  --  the caveat is
   ASSIST.HLP-only. Re-attributed; the engine mechanism is `[inferred]`.
5. section9  --  text-editor panel re-cited to its true range (DBASE.MSG **315-323**); noted
   that 308-314 is a separate `Up a field:/Down a field:/Field: ^N/^U/Pan: ^`
   variant.

**Completeness vs the help index.** The full-screen / interactive-UI slice of the
406-topic HELP index is fully covered: every editor entry-point, every MODIFY/CREATE
designer, the HELP-nav topics, and the CREATE concept topics are present and now
correctly sourced. No in-scope HELP topic is missing.

**Residual `[oracle-resolves]` gaps (unchanged  --  need DBASE.EXE/OVL under DOSBox):**
exact ASSIST menu geometry/colors/option ordering; which full-screen mode drives
the 281-289 vs 300-307 vs 308-314 panel; CHANGE-vs-EDIT default field set;
CONFIRM/CARRY in-editor loop behavior; the F10 field-menu builder; the VIEW designer
pane layout; the REPORT/LABEL designer page set; absolute status-line column/row
positions; and confirmation that EDIT-`^Home` and BROWSE-`^PgDn` reach the same memo
editor. These remain flagged in section12 and GAPS.md.
