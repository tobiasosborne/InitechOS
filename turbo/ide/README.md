# Turbo Initech IDE

The editor half of GOLDEN-MASTER item 7, in Pascal, compiled by Turbo
Initech. The surface it implements is frozen in `../README.md` "IDE"; the
reference is real TP 5.5 (turbopascal-decomp MINT06, pinned in
`goldens/CORPORA.lock`).

- `screen.pas`: the 80x25 cells (character, VGA attribute) and cursor.
- `buffer.pas`: the paged document. Only a 16 KiB page of whole lines is in
  memory; leaving it writes prefix + page + suffix to the other of two work
  files (`TURBOWK0.$$$`, `TURBOWK1.$$$`). Loading turns CR LF into LF and
  stops at 0x1A; saving writes CR LF and one 0x1A, keeping `NAME.BAK`.
- `editor.pas`: menu bar, pulldowns, name and Verify boxes, editing keys.

The language has no units or includes, so programs are built by
concatenation: `tests/head.pas`, the three files above, then a main
program. File transfers go through ShortString bytes (255 at a time) and
parameterless calls are written `Name()`, as the compiler requires.
Reset, Rewrite and Assign close an open file first (seed/rt/fileio.asm).

`make test` (tier 0, about a second):
- `journey`: the IDE built by Free Pascal replays `tests/journey.steps`
  (key codes from the IBM AT BIOS table in `tests/keys.c`); `tests/screens`
  compares all 36 screens, characters, attributes and cursor, with the
  MINT06 captures named in `tests/journey.expect`, changed only by the
  explicit edits listed there (absent titles, legend items and Indent
  words; pulldowns without their absent items). The saved `DEMO.PAS` and
  `DEMO.BAK` must equal the bytes TP 5.5 saved in the same journey.
- `paging`: a 5000-line, 213 KB file edited at the start, line 2500 and
  the end, then saved; awk computes the expected bytes.
- `seed` and `turbo`: the C seed and Turbo Initech itself accept the source.

`make smoke` in `turbo/` also runs `tests/smoke.sh` (tier 1): Turbo
Initech (the hosted Pascal compiler) compiles the same driver to an
extended executable, which replays both journeys inside the booted product
image on QEMU and Bochs from a TEST disk; the dumps and saved files are read
back with mtools and graded by the same `screens` tool and `cmp`. With the
tab-stop mutant applied, the QEMU run reports 26 differences on
`load_verify` and others (seen red 2026-11-14).

`make mutants`: nine patches (text colour, PgDn step, tab stops, missing
0x1A, missing backup, caret column, dropped page suffix, Ctrl-PgDn lost,
start-up menu state), all killed.

Not yet here (later stages, see the handoff and bd): Compile/Make/Run/User
screen behind their menu items, error display, recoverable file errors
(Error box, read-only, overwrite prompt, loading a missing file), the
console and keyboard of the product program, deleting the work files.
Those need the runtime of stage B and, for running programs, a kernel
change (`../tests/ide/probe`).
