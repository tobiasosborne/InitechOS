# WL-0115 - XCOPY and TREE (initech-zuvo)

## Context

One-shot XCOPY/TREE coding lane in the sibling xcopy clone. The operator
requested an uncommitted working tree, no beads writes, QEMU only, no aggregate
`make test`, and the shipping-volume regressions. `bd show initech-zuvo` could
not open this clone's database; the main clone's read-only lookup could not
open its write lock. The local `.beads/issues.jsonl` export supplied the issue
description and the closed initech-m0dc precedent. No tracker edits were made.

## Ground truth

All cited PDF pages were rendered with pdftoppm at 110 dpi and viewed.
The PDF directory is `/home/tobias/Projects/initech-manuals-ref/dos33/`.

| Source | Printed page / PDF page | Contract |
| --- | --- | --- |
| InitechOS-PRD.md | 6.1 | DOS API and FAT differential oracle |
| ADR-0003 | Appendix A, DEC-08 | INT21 services and flat native applications |
| MS-DOS_3.3_Users_Guide_198707.pdf | 50-51 / 135-136 | COPY paths, self refusal, binary data |
| same | 54 / 139 | US date separators; 80-79 / 1980-2079 year window |
| same | 122 / 207 | TREE traversal and /F |
| same | 127-129 / 212-214 | XCOPY operands, switches, return codes |
| same | 314,324,341 / 399,409,426 | Cyclic refusal, F/D prompt, diagnostics |
| ZR23-6845-01_PC_DOS_Operation_Training_Version_3.3_and_4.0_PC_and_PS2_198903.pdf | D-6 / 62 | Pre-DOS-4 TREE Path/Sub-directories/Files columns |
| 6138519_DOS_3.10_Reference_Feb85.pdf | 7-184 / 289 | TREE volume heading and None |
| 80X0945_DOS_3.30_Technical_Reference_Apr87.pdf | 5-11 / 118 | Eleven-byte volume label and attributes |
| same | 6-59 / 182 | Raw AH=07h input; XCOPY handles Ctrl-C itself |
| mcopy -s/-m, mdir -/, mattrib, fsck.fat -n | identical cloned FAT12 fixtures | Namespace, binary bytes, attributes, dates, disk consistency |

The comma in the F/D question follows the operator's explicitly supplied
literal. No new syscall numbers or FAT traversal implementation were added.

## Changes and reasons

`dosutil.c` is the shared freestanding C implementation. The small NASM entry
clears BSS, saves the PSP command tail, and supplies the native register bridge.
The linker places its entry at PROGRAM_IMAGE. All file and directory work goes
through the existing COPY/DIR INT21 backend. Parent searches restart by ordinal
after a recursive call because this kernel has one active find context; no
private DTA resume bytes or FAT entries are interpreted by either utility.

XCOPY implements /S, /E, /P, /V, /W, /A, /M and US /D:date. /S prunes empty
trees through lazy directory creation; /S /E retains them. File data is binary,
modification dates survive the copy, and /M clears the source archive bit only
after successful copying and verification. The self/cyclic guards compare
absolute, case-folded paths with dot components resolved. Omitted targets use
the working directory. F/D answers control the target's actual type.

TREE's expected output is independently rendered in factory C from mdir's
recursive inventory and the manual sample. The INT21 DTA emitter now treats
a volume label as eleven label bytes instead of inserting an 8.3 extension
dot or losing words after an internal space. Ordinary file formatting remains
the existing path. This branch has its own LABEL mutant.

The two programs append to APPS after 123.MNU. Their positions are 5 and 6;
the damaged-volume fixture's stranded APPS/TRASH moves to cell 7. The locked
Finder service trace was deliberately re-keyed from (+20,+72) to (+156,+72)
after opening APPS, with the subsequent park route adjusted to preserve its
absolute waypoint. The four-column grid (pitch 68 by 52) gives center
(279,174), row 1 column 3, using zero-based indices. This re-key is part of
initech-zuvo's explicitly requested grown-volume regression scope.

The mixed locked purge now removes exactly seven writable leaves: README,
CTENANT, 123, WORD, 123.MNU, XCOPY, TREE. TENANTFX remains locked and APPS
remains refused, so the exact expectation is `purged=7 refused=2`. Both new
leaves are also checked absent. Neither pixel grades nor protection assertions
were relaxed. Fixture contracts include both shipped files.

## Red, green, and mutation evidence

The host gate was authored before implementation and first failed with
`XCOPY/TREE RED: /S /E /V execution` against a placeholder returning 4. The
QEMU gate was also authored and run against the placeholder before the real
implementation. Additional red cases caught descendant-only sources, a
file/directory-kind hole in the oracle, labels with spaces, the default
AH=08h Ctrl-C abort, and the DOS 3.3 date window. The QEMU cancellation gate
went red with `QEMU /W Ctrl-C exit must be 2` before switching to AH=07h.

All final gates are green:

- `make test-xcopy-tree`: mcopy/mdir/mattrib differential plus dosfstools.
- `make test-xcopy-tree-mutant`: all 24 named host mutants caught.
- `make test-xcopy-tree-emu`: external flat EXEC, real keyboard /W and /P,
  FAT result differential, byte-exact TREE output, SHELL-EXIT.
- `make test-xcopy-tree-emu-cancel`: injected Ctrl-C, return code exactly 2,
  unchanged destination, SHELL-EXIT.
- `make test-xcopy-tree-emu-mutant`: S, TREE_LAYOUT and CTRL_C each booted,
  reached ZUVO-DONE and failed the intended disk/text/return-code assertion.
- `make image`.
- `make test-flair-locked-purge`.
- `make test-flair-finder-service`.
- `make test-flair-app-launch`: all ten legs.
- `make test-fileio`: 225 checks, zero failures.
- `make test-80k`: 41 checks, zero failures.

Both host targets are in TEST_UNIT_GATES; all three emulator targets are in
TEST_EMU_GATES. Mutants are compile-time branches used only by those gates:
S, E, A, M, D, P, V, W, TARGET, COUNT, TREE_F, TREE_LAYOUT, DEFAULT, WILD,
DESCEND, TIME, HIDDEN, E_REQUIRES_S, CYCLIC, SELF, LABEL, OMIT, CTRL_C,
DATE_RANGE. The Makefile prints the actual RED assertion for every mutant.
Each QEMU invocation used INITECH_QMP_STDIO=1. No Bochs or aggregate vector ran.

New source is ASCII. Forced independent rebuilds produced byte-identical
6052-byte applications:

```
c90e433f60b56d51efab01e55101d8bc5d2b32a1f58e3e5b4031b4fa1a57f6fb  xcopy_program.bin
7eec6a699a4f46610214c13b49ac4cfe3ea8f1b4a5e58ad1c4ffe521bfd28cb9  tree_program.bin
```

## Limits and pointers

/D accepts the manual's US month/day/year syntax with hyphen or slash, including
two-digit years and leap days. Country-specific input localization was not
added. Execution retains the kernel's existing one-mounted-volume and writable
FAT12 limits; writable FAT16 and multi-drive routing were not added. The
factory fixtures and QEMU acceptance use FAT12. No commits, pushes, .git edits,
CLAUDE.md changes, HANDOFF.md changes, or beads writes were performed.

The gate implementation and reproducible commands live in
`harness/diff/fat_diff/xcopy_tree.mk`, `xcopy_tree.sh`, and
`test_xcopy_tree.c`. Runtime evidence is under `build/zuvo_*`; the programs
are `build/{xcopy,tree}_program.bin` and shipped as `APPS/{XCOPY,TREE}.COM`.
