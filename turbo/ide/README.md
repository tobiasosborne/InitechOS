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
  compares all 47 screens, characters, attributes and cursor, with the
  MINT06 captures named in `tests/journey.expect`, changed only by the
  explicit edits listed there (absent titles, legend items and Indent
  words; pulldowns without their absent items). The saved `DEMO.PAS` and
  `DEMO.BAK` must equal the bytes TP 5.5 saved in the same journey.
- `paging`: a 5000-line, 213 KB file edited at the start, line 2500 and
  the end, then saved; awk computes the expected bytes.
- `seed` and `turbo`: the C seed and Turbo Initech itself accept the source.
- `one-program`: the compiler's declarations and the IDE compile as one
  program with both compilers (the capacity one TURBO.COM needs).

File errors (stage E, host part): the IDE runs under `{$I-}` and checks
IOResult. Saving copies the old file to `NAME.$$$` first, writes the new
bytes, and only then turns the copy into `NAME.BAK`; a refused or short
write restores the old bytes and leaves no backup, as TP left `RO.PAS`
(MINT06 54). The Error box shows DOS 2/3 on create as "Invalid drive or
directory." (51) and 5 as "File is read only." (54); other codes say
"File error n." (not captured). Write to an existing name asks
"Overwrite NAME? (Y/N)" (49, 50). A cancelled Alt-X leaves the bar on File
(57). F3 on a missing file opens an empty buffer of that name (TP 5.5,
not captured). A compiler error replaces the info row in 0x4E with the
cursor on the token, an off-screen line placed on view row 11 (07, 34);
the next key restores the row (08, 35). The journey drives it with an
`error LINE COL` step until the compiler is folded in. `tests/fpc.pas`
and `tests/dos.pas` adapt Reset's open mode (FPC's FileMode 2 versus the
Turbo runtime's read-only open).

`make smoke` in `turbo/` also runs `tests/smoke.sh` (tier 1): Turbo
Initech (the hosted Pascal compiler) compiles the same driver to an
extended executable, which replays both journeys inside the booted product
image on QEMU and Bochs from a TEST disk; the dumps and saved files are read
back with mtools and graded by the same `screens` tool and `cmp`. With the
tab-stop mutant applied, the QEMU run reports 26 differences on
`load_verify` and others (seen red 2026-11-14).

`make mutants`: twelve patches (text colour, PgDn step, tab stops, missing
0x1A, missing backup, caret column, dropped page suffix, Ctrl-PgDn lost,
start-up menu state, error-row colour, refused save leaving its copy,
missing overwrite prompt), all killed.

Not yet here (see the handoff and bd initech-wkp2p): Compile/Make behind
their menu items (needs the IDE folded into the compiler's source as one
TURBO.COM, and a located-error record from the compiler), Run and User
screen (kernel initech-fc4c3 and the screen-page call), the product main
program (console through `CON`, ReadKey), deleting the work files.
