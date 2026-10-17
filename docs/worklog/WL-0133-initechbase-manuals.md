# WL-0133 -- The InitechBase manuals: Volume One, Volume Two, Quick Reference Guide

## Context

Bead initech-ipu5.8 (one-shot writing and gating lane, branch lane/baseman,
base 4832d01). The InitechBase rows of the PUB-REF-0001 Documentation Set
(Sec 12): BAS-V1 InitechBase Volume One -- Getting Started, Learning, Using,
Index (1040-0000-10); BAS-V2 Volume Two -- Programming, Index (1040-0000-11);
BAS-QR the Quick Reference Guide (1040-0000-20). Family F-ATE; model the
dBASE III PLUS 1.1 set (docs/pubs/originals/ashton-tate-dbase3plus.md and
the scans in initech-manuals-ref/dbase3plus, read for structure only).
Ground truth (Law 1): os/samir/** -- samir_main.c (the dot prompt, the
message catalogue g_msg), cmd/*.c (the command modules), core/*.c (the
evaluator and functions), fs/*.c (the .DBF/.DBT/.NDX codecs) -- spec/samir/*,
and the reference programs harness/diff/dbf_diff/canon/*. Two Sonnet helpers
drafted the 48 function entries and the 146 message entries (with a
reachability table, scratch msg-reach.tsv); every entry, example and claim
was then built, replayed and reviewed in this lane.

## What changed

Books (pubs/manuals/):

- initechbase-volume-one/ (new, 280 pp, 122 DRAFT markers): title, AN-20
  verso with the corporate motto, volume Introduction with Conventions Used
  in This Manual, Contents. Getting Started (G): Installing InitechBase (the
  package, working copies, the sample diskette with an emulator DIR screen),
  Starting and Leaving InitechBase (an emulator screen of a whole session).
  Learning InitechBase (L): Introduction and ten AN-16 lessons (Quick Start,
  Looking at Your Data, Moving Through a Database, Finding Records, Changing
  Your Data, Calculating, Reports, Working with Several Databases, From
  Commands to Programs, Keeping Your Data Safe). Using InitechBase (U):
  Introduction; 1 Overview (types, files, specifications generated, commands
  by task); 2 Using Commands; 3 About Program Files; 4 The InitechBase
  Environment; 5 Commands -- Symbols and Conventions, Limitations (generated
  from the engine), 53 AN-04 entries (39 commands and compounds, 14 SET
  options); 6 Functions -- 48 AN-05 entries; 7 Error Messages -- 146 AN-06
  entries; Appendices A Keys, B Beginning and End of File, C File Structure
  (generated from spec/samir/*_format.h), D ASCII (computed), E Data Types
  (generated from spec/samir/xbase_coercion.json); Glossary (AN-07). Notes
  leaves at each section end, the master index with its Key to Index Page
  Numbers (both volumes' entries), colophon, three Reader's Comment Forms;
  cover (with back cover and motto) and spine. 209 replayed examples.
- initechbase-volume-two/ (new, 116 pp, 1 DRAFT marker): Programming with
  InitechBase (P): Introduction; 22 chapters building the Initech Accounting
  System on INVOICE.DBF -- from the nutshell to the menu, the month-end
  aging report (Y2KACCT.PRG) and the finance-charge posting with the
  rounding adjustment swept into the BOLTON suspense account (INTPOST.PRG),
  documented straight, the two-digit date convention described as the
  documented convention; Appendices A Commands at the Dot Prompt and in
  Programs, B The Sample Programs, C Programming Conventions; master index;
  cover and spine. 46 replayed examples, 35 program listings.
- initechbase-quick-reference/ (new, 32 pp, 5 DRAFT markers): the PK-11
  3.5 x 9 in booklet -- self cover, commands (full syntax bold over a
  description), SET options, functions in two columns, operators, data
  types, files, limits (generated), keys, the message catalogue by number
  (generated from g_msg), Notes pages, colophon, back cover with the motto.
- initechbase-replay-specimen/ removed: the real books replace it; its
  program-route example (pubs/examples/samir/salary) lives on in Volume
  Two, Chapter 1.

The Sample Programs and Utilities diskette (pubs/examples/samir/disk):
MANIFEST + .dat table/report descriptions + 34 .PRG programs, minted by
harness/pubs/samir_sample_disk.c through the SHIPPED writers (dbf_create /
dbf_append_rec, dbt_create / dbt_append, ndx_build) and a 1990-byte .FRM
writer to frm.md: STAFF.DBF (+.DBT memo, four .NDX), DEPTS.DBF (+.NDX),
INVOICE.DBF (+ two .NDX; the reference ledger values), STAFFRPT.FRM,
INVRPT.FRM.

Gates (harness/pubs/, pubs/pubs.mk; C and thin shell, Law 3):

- samir_inventory.c: the shipped InitechBase surface asked of the SHIPPED
  dot prompt (samir_repl over the host PAL) for every quoted upper-case word
  of the SAMIR sources: NAME|command (not #16), NAME|function ("? NAME()"
  not #31), NAME|set (the SET module accepts one probe operand), each probe
  without and then with a database in USE; `messages` prints g_msg.
  Calibrated against XYZZY/LIST/UPPER()/SET ORDER. 47 commands, 48
  functions, 14 SET options, 146 messages. pubs-samir-inventory.
- samir_replay.c: the session route -- samir_repl on the typed lines of a
  .dot, the cooked-console echo, CR LF as LF, a fresh copy of the sample
  diskette per example, any other control byte RED; the ipu5.6 program route
  kept. replay.sh chooses the route by .dot/.prg.
- pubs_gate.c: command kinds command/function/set; samirmentions (inline
  \cmd/\fn of the InitechBase books against the shipped surface); messages
  (GC-02: every printed message the catalogue text byte for byte, one entry
  per catalogue message in Volume One); the period-year lint no longer reads
  the decimal places of a number (18.7500) as a year.
- samir_tables.c: limits (from the engine's capacity constants), formats
  (Appendix C layouts from the spec headers), messages (QR list from g_msg),
  coercion (Appendix E from the JSON).
- samir_programs.sh, test-pubs-samir-programs (GC-11): Y2KACCT.PRG and
  INTPOST.PRG bodies equal the reference programs byte for byte, and the
  printed transcripts hold every line of the reference outputs.
- test-pubs-commands: Using InitechBase and the Quick Reference each
  checked against the shipped surface in both directions (109 = 109, 4
  DRAFT); Volume Two must register nothing; harness/pubs/samir.allow
  (empty, monotonic); mentions and messages legs.
- test-pubs-samir-disk (CP-12), pubs-examples (re-mint, a deliberate act).
- capture.sh: a `samir` diskette (SAMIR.COM + the minted sample files) for
  InitechBase screens; pubs.mk stages it.
- mutants.sh: FAKE_XBASE_ENTRY, REMOVED_XBASE_ENTRY, CHANGED_SESSION,
  CHANGED_MESSAGE, MISSING_MESSAGE, UNSHIPPED_MENTION, SAMPLE_PROGRAM_DRIFT,
  REFERENCE_OUTPUT_DRIFT, LATE_YEAR_POINT, and the inventory probe check
  (20 named mutants in all).

Class (pubs/class/): initech-xbase.sty (new) -- sections with lettered
folios (G, L, U, P; LI-1, UA-1, UG-1, X-1), section title sheets, Notes
leaves, dictionary heads on reference pages, F-ATE index locators through
makeindex (U-5-26 printed U5-26), PG-32 thumb marks, XbaseEntry (fixed
order Syntax > Defaults > Usage > Options > Tips > Examples > See Also,
thinned never permuted, registration), XAlso, XbaseMessageEntry
(verbatim text), DotExample (typed lines bold, 80-column wrap, tint panel,
breakable), ProgramListing, TIP/NOTE/WARNING boxes, GlossaryEntry, the
master index merge (MasterIndexPartner + build.sh second pass), the QR
booklet macros, the back cover. initech-manual.cls: quickref option, the
section foot title, the DRAFT key qualified by the section letter, the index
preamble hook. initech-screen.sty: F-ATE screens on a 10% panel (NT-23).
pubs/common/colophon.tex: the InitechBase books name their replay route.
pubs/build.sh: INITECH_PUBS_OUT and the MASTERINDEX second pass.

## Why

Law 1/2 and D-14: every statement was read from the shipped source and,
wherever it shows behaviour, replayed through the shipped dot prompt; where
the shipped code accepts something it does not carry out, the entry says so
inside a DRAFT marker; nothing the dot prompt refuses is documented as a
command (no INDEX ON, CREATE, COPY, SORT, COUNT/SUM, @ SAY, BROWSE, EDIT,
LABEL FORM, SET PROCEDURE ...). The extents follow from that: the shipped
surface is about a third of dBASE III PLUS, so Volume One is 280 pages
(target 450-600) and Volume Two 116 (target 250-400) without padding; the
Quick Reference meets its 32-40.

## Frictions and defects found (no bead filed by this lane; each wants one)

Shipped InitechBase (os/samir), each observed through the shipped dot prompt:

1. Engine codes leak as catalogue messages: an unknown or incomplete SET
   option, SELECT 11, APPEND (bare), STORE 1 TO, a too-long DO WITH list in a
   procedure print "7  File already exists."; LIST/DISPLAY with no database
   and DISPLAY STRUCTURE/MEMORY "3  File is already open." / "12"; IF, ELSE,
   ENDDO, LOOP at the dot prompt "5  Record is out of range."; an
   unterminated string "4  End of file encountered."; "? (1" "2  Unassigned
   file no."; INPUT of a bad expression and alias references (A->FIELD)
   "1  File does not exist."; DO recursion "8". (scratch msg-notes.md has the
   source lines.)
2. SET INDEX TO and SET RELATION TO are recorded but not acted on
   (set.c "GATED: runtime ... deferred"); SET CONSOLE and SET TALK have no
   consumer; ON ESCAPE / ON KEY are accepted and never fire.
3. REPLACE into a memo field stores nothing and empties the memo.
4. A character constant in DO ... WITH is not passed intact (memory
   variables and expressions are).
5. LIST OFF / DISPLAY OFF ignore their expression list; DISPLAY with FOR
   and no scope tests only the current record; DISPLAY at the end of the
   file shows a phantom record; LIST with fields before the scope
   (LIST LASTNAME NEXT 3) is "5  Record is out of range."
6. After SET ORDER the record pointer is not re-seated (LIST NEXT walks a
   mixed order) -- the books always GO TOP after it.
7. ZAP on a database opened with an index leaves the index stale (a
   later APPEND BLANK/SEEK misses); the books ZAP without indexes.
8. TRANSFORM: decimal pictures need a spare digit position, commas in a
   picture leave stray commas; "? CTOD('')" prints garbage; "1E3" is not
   lexed; ? prints numeric fields without their decimals.
9. A failed SEEK/FIND prints nothing (#82 never used); 112 of the 146
   catalogue messages are never displayed by any code path (DRAFT entries).
10. The status bar guidance line reads "Enter a dBASE III PLUS command."
    (core/screen.c) -- a trade name on the shipped screen; the books
    describe the status bar without quoting it.
11. Programs cannot USE/CLOSE/QUIT or DO another program file; there is no
    ; line continuation; ? output and the next dot prompt share a line.
12. The host PAL opens file names case-sensitively, so a lower-case USE
    replays differently from the FAT target; the books type names in
    capitals.

Factory and process:

13. test-pubs-screens is RED at the base for the InitechDOS books: CHKDSK now
    reports 118320 bytes of memory where the committed captures say 118448
    (initechdos-reference and initechdos-users-guide chkdsk.txt). Not this
    lane's books (no os/ change here); `make pubs-screens` on main refreshes
    them. test-pubs-mutant therefore cannot be reached through `make`
    until then; its script was run directly against this lane's gate
    outputs (all 20 mutants RED as named).
14. The period-year lint read decimal places as years (18.7500); narrowed,
    with a mutant proving ".1994" alone is still RED (PM review: this is a
    change to an existing lint).
15. Gate names that contain a deny-listed word (test-samir-canon-y2k) cannot
    be cited in \Gates{}; the canon check is cited as test-pubs-samir-programs.
16. The lint's year rule forbids every four-digit number above 1993, so the
    sample data avoids them (extensions 1400-1431, amounts and pictures
    chosen accordingly).

## Remaining DRAFT markers (128: Volume One 122, Volume Two 1, QR 5)

- 112 message entries: no command of this version raises the message.
- Verso address and permit number (PUB-REF O-5, D-12): each book.
- Package pieces other than the three books (PK-10/12/13): Volume One G1.
- SET CONSOLE, SET INDEX, SET RELATION, SET TALK: accepted, not acted on
  (Volume One and QR).
- ON ESCAPE / ON KEY: accepted, not carried out (Volume One and QR).
- DO ... WITH a character constant: not passed intact (Volume One DO).
- REPLACE into a memo field: not stored (Volume One REPLACE, Appendix E).

## Acceptance

Run on this worktree (INITECH_QMP_STDIO=1, the corpus paths exported):

- make pubs: 13 documents; the two InitechBase volumes typeset a second
  time for the master index. Volume One 280 pp, Volume Two 116 pp, Quick
  Reference 32 pp, covers 2 pp (front and back), spines 1 p.
- make test-pubs-build: green, 0 overfull everywhere.
- test-pubs-commands: InitechDOS leg unchanged (129 shipped, 45 documented,
  84 omissions); InitechBase: Using InitechBase 109 shipped = 109
  documented (4 DRAFT), Quick Reference 109 = 109 (4 DRAFT); mentions 189
  shipped; messages 146 catalogue = 146 entries (112 DRAFT), 126 inline
  quotations verbatim; samir.allow 0 -> 0.
- test-pubs-lint, test-pubs-fonts, test-pubs-samir-disk (47 files): green.
- test-pubs-examples: green, 255 examples byte for byte (254 dot-prompt
  sessions, 1 program).
- test-pubs-screens: the two InitechBase screens equal their live capture;
  RED overall on the InitechDOS chkdsk captures (Frictions 13, base drift).
- test-pubs-samir-programs: green (both reference bodies, 11 and 10 output
  lines).
- test-pubs-mutant legs (harness/pubs/mutants.sh run directly, Friction 13):
  green, 20 named mutants each RED for its own reason.
- make test-pubs-repro: green (13 PDFs byte-identical across two clean
  builds); test-pubs-repro-mutant: green; test-pubs-families: green (7
  families); test-makefile-vars: green.
- Viewed (pdftoppm): Quick Start opener and its first spread, a command
  entry page, a message page, the index, both covers, the QR cover and its
  command pages.

## Pointers

- pubs/manuals/initechbase-volume-one/, -volume-two/, -quick-reference/
- pubs/examples/samir/disk/ (MANIFEST, *.dat, *.PRG)
- pubs/class/initech-xbase.sty; initech-manual.cls, initech-screen.sty,
  initech-syntax.sty, pubs/common/colophon.tex, pubs/build.sh, pubs/pubs.mk
- harness/pubs/samir_inventory.c, samir_replay.c, samir_sample_disk.c,
  samir_tables.c, samir_programs.sh, pubs_gate.c, replay.sh, mutants.sh,
  samir.allow; pubs/screens/capture.sh

## Where to resume

The books are complete for the shipped surface. To grow them toward the DS
extents honestly, the shipped surface must grow first (each item of
Frictions 1-11 that is fixed lets an entry lose its DRAFT marker or an
example be added). When SAMIR changes, run `make pubs-examples` and
`make pubs-screens`, review the diff, and the gates re-prove the books.
