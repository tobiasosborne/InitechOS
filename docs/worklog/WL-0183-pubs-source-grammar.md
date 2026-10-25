# WL-0183: Fail closed in the InitechBase source lockstep gate

## Context

Review of 31b286e4 found source-gate false greens: skipped or quoted entries
counted as executed, executed entries ignored, unchecked DRAFT kinds, and
missing Transcript files. Ref: PUB-REF-0001 GC-01/03; PRD Sec 6.6/8;
CLAUDE.md Laws 1/2, Rules 2/3/6/11/12. Bead: initech-j29.

## What changed and why

`harness/pubs/srclockstep.h` defines a restricted grammar with named RED
diagnostics. Passive macros consume and check their entire arguments;
entries and inputs must occur in executable document context. Unknown
control sequences, conditionals, unaudited definitions/Lua, spaced
environment syntax, and non-comment text after the document are rejected.
The existing preamble definitions and ASCII-table Lua are accepted exactly.
Version guards pin the audited class/style/common files: a support delta
requires grammar review and the real session oracle before updating a guard.
These guards are syntax-version checks, not expected command inventories.

Inputs resolve to regular, non-symlink files with literal basenames in the
book directory, or four explicitly named generated tables. The latter are
parsed, never skipped by prefix; local shadows, missing files, traversal,
and common-directory fallbacks are RED. The Make target supplies the
generated directory and depends on those tables without invoking LaTeX.

QREntry validates kinds through the index reader's unchanged vocabulary
before any DRAFT exemption. Transcript requires its .txt/.prg pair.
DotExample preserves the class's ordered typed-line/suffix/boundary and
printable-byte checks without regenerating or replaying expected output.

The session targets, real commands reader, typesetting route, build
parallelism, and TEST_UNIT_GATES remain intact. The separate merge vector
belongs to another lane.

## Acceptance

The expanded mutants first failed against 31b286e4 because conditional ABS
survived (make exit 2). The fixed source baseline checks 142 entries in each
reference, 286 DotExamples and one Transcript. All 43 injected faults fail
with their named diagnostics; a commented entry and the restored copy pass.
The six original mutants are retained alongside every concrete review
reproducer and grammar-boundary cases.

The real `make test-pubs-commands` remains green, including typesetting all
38 documents. Both real InitechBase main.cmds files pass `pubs_gate commands`
against the shipped inventory. A 517-case old/new commands differential
has identical output and exit status. ASCII, shell syntax and diff checks
pass. ASan/UBSan checks cover the baseline and all mutants; LeakSanitizer
is unavailable under the execution environment's ptrace and was disabled
for these optional sanitizer runs only.

## Pointers

- `harness/pubs/srclockstep.h`: grammar, resolved inputs, typed-line check.
- `harness/pubs/srclockstep_mutants.sh`: 43 named faults and positive controls.
- `pubs/pubs.mk`: explicit generator/header dependencies; session gates retained.
- `build/pubs/review-fix/`: local validation logs (untracked).
