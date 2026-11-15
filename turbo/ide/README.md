# Turbo Initech IDE

The blue IDE for GOLDEN-MASTER item 7, in Pascal, compiled by Turbo
Initech. The surface it implements is described in `../README.md` "IDE"; the
reference is real TP 5.5 (turbopascal-decomp MINT06, pinned in
`goldens/CORPORA.lock`).

- `screen.pas`: the 80x25 cells (character, VGA attribute) and cursor.
- `buffer.pas`: the paged document. Only a 16 KiB page of whole lines is in
  memory; leaving it writes prefix + page + suffix to the other of two work
  files (a pair of `TURBOnnn.$00` and `TURBOnnn.$01` names proved absent
  before either is created). Loading turns CR LF into LF and
  stops at 0x1A; saving writes CR LF and one 0x1A, keeping `NAME.BAK`.
- `editor.pas`: menu bar, pulldowns, name and Verify boxes, editing keys.
- `product.pas`: Compile/Make, first-error record, fresh output publication,
  Run by DOS EXEC, saved user page and the interactive product loop.

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
IOResult. Saving writes the new text to a distinct sibling stage, checking every
transfer and close before renaming the old source to `NAME.BAK` and the
stage to the source. A failure before renames leaves the named original
untouched. A failed publication rename rolls the backup back; if that
rename also fails, both backup and stage remain for recovery (operator
save ruling). Read-only sources are refused before staging (MINT06 54).
`.BAK` sources use `.BK1`; `.$$$` sources use `.$$1`. An existing stage
is refused. Full paths are retained separately from the displayed leaf. The Error box shows DOS 2/3 on create as "Invalid drive or
directory." (51) and 5 as "File is read only." (54); other codes say
"File error n." (not captured). Write to an existing name asks
"Overwrite NAME? (Y/N)" (49, 50). A cancelled Alt-X leaves the bar on File
(57). F3 on a missing file opens an empty buffer of that name (TP 5.5,
not captured). A compiler error replaces the info row in 0x4E with the
cursor on the token, an off-screen line placed on view row 11 (07, 34);
the next key restores the row (08, 35). The editor-only journey drives it
with an `error LINE COL` step; `compile` checks the actual compiler's stored
first error and all three compile/error screens. `tests/fpc.pas`
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

The four fragments are embedded verbatim in `compiler/turbo.pas`; its main
opens the IDE without arguments and compiles a named source with an argument.
`run-command` checks the DOS boundary order and post-Run MINT07 screens for
exit codes 0 and 7. `work-names` conserves pre-existing work-name sentinels.
Tier 1 also checks actual kernel screen cells through an independent C reader
and drives the real product with injected keys, reading saved files off disk.

Save conservation regressions (review initech-76j6x/qcg30/ge49u/v4gu0/zf5xe):
`page-failure` injects every read/write position and both closes during
navigation and direct Save; the dirty page and work identity survive, and
retry matches separately authored text. `save-failure` injects transfers,
closes, backup removal, each rename and double failure; literal OLD stays
at its path except the documented double failure, which retains both
recovery files. `save-identity` covers .BAK/case/temporary/extensionless
aliases; `first-save` covers Y/N/Esc and a failed existence probe;
`save-path` covers Load/Write to/F2 with directory and drive prefixes and a
root sentinel. The host path probe uses literal DOS backslashes in Unix
filenames, testing retained identity rather than pretending to emulate a
DOS filesystem. Fault adapters exist only in tests. Every regression was
observed red before its fix; each defect has a semantic mutation kill.

The factory scripts distinguish setup/read failures from completed property
failures (initech-kkqbw). Nested producers must emit their own positive
assertion summary; an exit code alone supplies no evidence. A missing named
output is checked as an existence property before byte comparison. Assembler,
FPC and reader errors are never relabelled as failed byte assertions.
