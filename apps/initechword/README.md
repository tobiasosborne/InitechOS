# InitechWord

WordPerfect 5.1 document core, .WP5 codec and FLAIR editing face. Dependencies:
sdk/include for the INT81 Toolbox interface, sdk/host for host tests only, and
factory/testlib for assertions. Core/fs compile with only -Iinclude. The face
has no private OS or graphics dependency. Product behavior preserves the real
WordPerfect captures and the legacy-final implementation cited in the source.

make builds WORD.EXE through mk/tenant.mk using plain TARGET_CFLAGS. No resource
file is required. The independent MZ reader validates relocations and BSS:
57,760 bytes on disk, 51,344-byte module, 493,520-byte BSS (below the OS-side
512 KiB limit). Drawing data already uses TBX_IN_IMAGE; file buffers stay in
BSS as allowed by sdk/README.md. No OS change was needed to run this executable.

make test runs 13 programs and 22,394 counted checks against the real pinned
WordPerfect corpus, captured grids and independent models/readers. Execution
fits the 30-second budget. Hosted code uses -O2, keeping every property
session: 13s before versus 10s after on the integration machine (codes 8s
versus 6s); target flags and timeout/budget limits are unchanged. Set
WORDPERFECT51_DECOMP to wordperfect51-decomp; missing or unpinned inputs fail.
Required host tools: C compiler, POSIX shell, Python 3 for the existing corpus
reader, mtools, fsck.fat, md5sum and sha256sum. New factory code is C/make/shell.
Tests use the shared SDK fake. tests/face_driver.h retains app-state driving,
recording aliases and explicit scratch import/export boundaries so independent
saved-byte checks remain unchanged. Captured directory metadata is injected
locally; List Files faults use the shared fake's errors and transfer limits.

make smoke drives the actual product on QEMU and Bochs. The app-owned journey
in tests/smoke types a paragraph, toggles Bold with F6, saves WORD.WP5 using
F10 and exits with F7. Its independent wp5_ref.py verifier requires exact text,
Bold on/off around reports, and a complete trailer without errors. The factory
runs it and the Initech 123 journey in one boot per emulator, then reads files
with mtools. factory/red-desktop.sh proves that an empty Word save fails this
host verifier. Screens and serial traces are under build/smoke-desktop/.

make mutants applies component-relative zero-fuzz patches via mk/mutants.mk.
All 73 mutants are killed twice by host tests; none is pending.
75 conditional sites became 73 patch files (the repeated NO_D4 and NO_LEAD
sites are each one fault). Product source contains no mutation hooks. The EXE
before/after SHA-256 is identical:
9453efb0e90e0ff491cb9cf630c928a70294316f9652329a6432b0b891ea698e.

## Present depth (frozen feature inventory)

This is the GOLDEN-MASTER item 6 inventory, not a claim of full WP parity.
Editing uses a two-bank piece table, wrapping, insertion/overwrite, deletion,
three undo records, cursor/page/Home/Go To navigation, paired attributes,
Reveal Codes, stream blocks, move/copy/retrieve, sentence/paragraph/page
selection, forward/backward code-aware search, case-preserving replacement,
justification, line spacing and margins. .WP5 preserves captured prefixes and
formatting streams; save uses the SDK temporary/write/close/rename transaction.
List Files supports directory navigation, retrieve, Look, delete, move/rename
and copy, including collision/error handling. Setup persists preferences.
The native face is an authored 76x24 adaptation of WP's 80x25 screen.

The following 69 visible command labels are inert or unavailable. Count each
entry point once, including separately drawn Move and Retrieve choices (49
entries), plus the 20 advertised children of blocked Format groups.
Sources: include/iword/{fkeys.json,listfiles.h,reveal_codes.h}, app/face_a.c
(function_key, prompt_key, format_key, setup_key, lf_key, block_key), frozen
legacy docs/audits/2026-10-23-iword/REPORT.md L040-L084 and TRIAGE.md, and
attic/triage/triage.md's apps-iword table. No command is fixed in this import.

- 23 function-key entries only show a gray command name: F3 Help; F9 End Field;
  Shift-F3 Switch, Shift-F4 reverse Indent, Shift-F5 Date/Outline, Shift-F6
  Center, Shift-F7 Print, Shift-F9 Merge Codes; Alt-F1 Thesaurus, Alt-F5 Mark
  Text, Alt-F6 Flush Right, Alt-F7 Columns/Table, Alt-F8 Style, Alt-F9 Graphics,
  Alt-F10 Macro; Ctrl-F1 Shell, Ctrl-F2 Spell, Ctrl-F3 Screen, Ctrl-F5 Text
  In/Out, Ctrl-F6 Tab Align, Ctrl-F7 Footnote, Ctrl-F9 Merge/Sort, Ctrl-F10
  Macro Define. The six modified F11/F12 slots have no WP command and are
  deliberately unmapped; they are not counted as visible commands.
- 5 List Files entries ignore their keys: 4 Print, 5 Short/Long Display,
  7 Other Directory, 9 Find, N Name Search.
- 5 Setup entries are gray: Display, Initial Settings, Keyboard Layout,
  Location of Files, Timed Document Backup.
- 9 Format entries leave the menu unchanged: Page, Document, Other; Line's
  Hyphenation, Hyphenation Zone, Height, Numbering, Tab Set, Widow/Orphan.
- 20 advertised Format children cannot be reached through those groups:
  Page's Center Page (top to bottom), Force Odd/Even Page, Headers and Footers,
  Margins Top/Bottom, Page Numbering, Paper Size/Type/Labels, Suppress;
  Document's Display Pitch, Initial Codes/Font, Redline Method, Summary;
  Other's Advance, Conditional End of Page, Decimal Characters, Language,
  Overstrike, Printer Functions, Underline Spaces/Tabs, Border Options,
  End Centering/Alignment.
- 2 Font choices return to editing without an operation: Base Font, Print Color.
- 5 block entries do nothing: Move's Tabular Column and Rectangle; Retrieve's
  Tabular Column and Rectangle; Append in the block-operation menu.

Additional partially working affordances (not included in the 69-label count):
Original Document Backup records Yes but creates no backup; entry/status units
are recorded but measurements stay in inches; List Files ignores typed/default
path changes; Look does not scroll plain text or navigate adjacent documents;
active-block Save, paragraph extension and code-search lack their block routes;
Ctrl-arrow and Ctrl-Delete act on units; inline Center/Flush Right codes have
no layout effect; resize can hide prompts; document clicking/scrollbars have
no viewport route; Finder power requests do not recognize File > Exit;
zero-inch margins are silently ignored; real FLAIR Ctrl-M does not reach the
search soft-return route. These are existing triage items, not new scope.

The corpus minting tools/fixtures are manual capture support, excluded from
all/test. Remaining legacy pixel-heavy emulator gates are listed in
 tests/SMOKES.md; their future owner is the InitechWord/factory importer.
