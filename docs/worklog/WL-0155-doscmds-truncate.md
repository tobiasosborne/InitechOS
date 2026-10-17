# WL-0147 - doscmds second launch: FAT truncation and final certificate

## Context and result

The PM explicitly authorized the WL-0146 prerequisite in this same sibling
clone. Read CLAUDE.md top to bottom, PRD 6.1/8, ADR-0003 Appendix A/B.1,
the lane and predecessor worklogs, and the local issue export. bd prime/show
were read-only; show did not find bwzs in the live database, so used the export.
No agents, git commands, commits, pushes, tracker writes, CLAUDE.md changes,
HANDOFF.md changes, Bochs, or aggregate make test. All QEMU gates exported
INITECH_QMP_STDIO=1. Before heavy runs, free -g showed 21 GiB available and
df -h / showed 118-119 GiB free.

All 57 individually invoked targets in build/doscmds-final/run.sh passed in
the final run. Per-target logs and the tail certificate are under
build/doscmds-final/. This supersedes the first launch's incomplete certificate.

## Changes and grounds

fat12_set_size in os/milton/fat12.c validates the entire old chain against
the old size before mutation, refuses short/cyclic/free/bad/overlong chains,
releases clusters past the new size, preserves the kept tail's EOC, and sets
start_cluster=0 for an empty file. A full packed FAT12 preimage in cooperative
scratch permits rollback after a fault in FAT1, FAT2, or the directory commit.
Both live allocation state and on-disk bytes are restored. Rollback failure
returns a write error. This is I/O-fault recovery, not power-failure journaling.
Growth delegates to the existing positioned writer. FAT16 writes stay deferred.

Zero-length fat12_write_partial now invokes that size path, so the existing
INT21/SFT/backend chain implements AH=40h CX=0 at the current file position,
returns zero bytes, refreshes the handle's entry, and preserves the position.
fat12.h and the existing AH=40h calling-convention row were deliberately updated
for bwzs's newly authorized prerequisite (Rule 8); the ABI gate now also checks
40h and has row-removal and zero-count-contract mutants.

COPY's omitted-target A+B path can now remove the first source's text EOF and
tail before appending. Its zero-count syscall wrapper preserves CF separately
from the zero-byte success result; source read failures and truncation errors
abort before appending. The host gate asserts that an injected truncation
refusal leaves the whole image identical. No new user-facing text was added.
The first-launch catalogue and parser changes remain in the working tree.

## Red before implementation and oracle design

Authored host C/INT21 and test-only flat NASM/QEMU gates first. Before the FAT
implementation the host failed RED truncate: AH40 size at current position;
QEMU booted, returned to COMMAND, and failed RED truncate: AH40 matrix.
Evidence: build/truncate-first-host-red.log and truncate-first-emu-red.log.

Both legs operate on identical mformat seeds and compare all output bytes
and free space against independent mcopy replacements, then require fsck.fat
clean. Cases cover a mid-cluster shrink, an exact boundary, zero length,
subdirectory entries, unchanged size, extension, truncate/regrow, read-only
files, read handles, bad handles, and unchanged file position. Host write faults
1/2/3 must actually fire and restore an identical whole image and live FAT.
Four malformed-chain cases must fail before any allocation or disk mutation.

FAT12_MUTATE_TRUNCATE_NO_FREE leaves discarded clusters allocated. Host and
booted QEMU both emit RED truncate: released clusters != mtools; file bytes
and the emulator's TRUNCATE-PASS marker are required before accepting that RED.
All four new gates are in TEST_UNIT_GATES/TEST_EMU_GATES through shell_files.mk,
alongside the twelve first-launch gates. The test COM is not shipped.

## Final tails of every added gate

These are from the final run, after the complete COPY fixture expansion.

```text
test-fat-truncate-host | GREEN truncate rollback: FAT1/FAT2/directory write faults restore identical image and live FAT; short/cycle/free/bad chains refused
test-fat-truncate-host-mutant | GREEN test-fat-truncate-host-mutant: FAT12_MUTATE_TRUNCATE_NO_FREE caught RED
test-fat-truncate-emu | GREEN test-fat-truncate-emu: AH40 position/size, tail release, zero, boundary, subdir, regrow == mtools; fsck.fat
test-fat-truncate-emu-mutant | GREEN test-fat-truncate-emu-mutant: FAT12_MUTATE_TRUNCATE_NO_FREE booted and caught RED
test-shell-files-host | GREEN test-shell-files-host: COPY modes/lists/wildcards, REN patterns/collisions, MD namespace; mtools and fsck.fat
test-shell-files-host-mutant | GREEN test-shell-files-host-mutant: INT21_MUTATE_MKDIR_DEVICE caught RED
test-shell-files-emu | GREEN test-shell-files-emu: COPY modes/lists/wildcards, REN patterns/collisions, MD namespace; mtools and fsck.fat
test-shell-files-emu-mutant | GREEN test-shell-files-emu-mutant: wild booted and caught RED
test-command-c-emu | GREEN test-command-c-emu: nested /C ECHO, copied environment, EXIT/error, redirection and primary VER
test-command-c-emu-mutant | GREEN test-command-c-emu-mutant: COMMAND_env caught RED
test-xcopy-cwd-host | GREEN test-xcopy-cwd-host: subdirectory source == mcopy; root decoy excluded; fsck.fat
test-xcopy-cwd-host-mutant | GREEN test-xcopy-cwd-host-mutant: DU_MUT_REL_ROOT caught RED
test-xcopy-cwd-emu | GREEN test-xcopy-cwd-emu: subdirectory source == mcopy; root decoy excluded; fsck.fat
test-xcopy-cwd-emu-mutant | GREEN test-xcopy-cwd-emu-mutant: DU_MUT_REL_ROOT booted and caught RED
test-dir-abi | GREEN test-dir-abi: 39h/3Ah/3Bh EDX, 47h DL/ESI, 40h ECX=0 size; IBM pp.6-119..121,6-140,6-188
test-dir-abi-mutant | GREEN test-dir-abi-mutant: ABI_39 ABI_3A ABI_3B ABI_47 ABI_40 ABI_40_ZERO caught RED
```

The host truncation log also ends its differential phase with:

```text
GREEN test-fat-truncate-host: AH40 position/size, tail release, zero, boundary, subdir, regrow == mtools; fsck.fat
```

## Final mutation evidence

```text
FAT12_MUTATE_TRUNCATE_NO_FREE (host/QEMU) | RED truncate: released clusters != mtools
CMD_MUTATE_REN_ENUM | RED shell-files: REN membership != mtools
CMD_MUTATE_REN_MAP (host/QEMU) | RED shell-files: REN membership != mtools
CMD_MUTATE_COPY_PLUS | RED shell-files: JOINED.BIN bytes
CMD_MUTATE_COPY_ASCII | RED shell-files: ASCII.TXT bytes
CMD_MUTATE_COPY_EOF | RED shell-files: TEXT.TXT bytes
CMD_MUTATE_COPY_SWITCH | RED shell-files: ASCII.TXT bytes
CMD_MUTATE_COPY_VERIFY | RED shell-files: COPY /V accepted corrupt read-back
CMD_MUTATE_COPY_WILD (host/QEMU) | RED shell-files: ALL.TXT missing
FAT12_MUTATE_CREATE_CHARS | RED shell-files: invalid raw name allocated; RED shell-files: created-name matrix
INT21_MUTATE_MKDIR_DEVICE | RED shell-files: reserved raw name allocated; RED shell-files: created-name matrix
CMD_MUTATE_COMMAND_C | RED command-c: child output/handle restoration
CMD_MUTATE_COMMAND_ENV | RED command-c: child environment leaked
DU_MUT_REL_ROOT (host/QEMU) | RED xcopy-cwd: relative source selected root decoy
ABI_39 | RED dir-abi: 39h row missing
ABI_3A | RED dir-abi: 3Ah row missing
ABI_3B | RED dir-abi: 3Bh row missing
ABI_47 | RED dir-abi: 47h row missing
ABI_40 | RED dir-abi: 40h row missing
ABI_40_ZERO | RED dir-abi: 40h zero-count size contract
CMD_MUTATE_DEL_NO_CONFIRM | k15 RED -- non-Y response changed disk
CMD_MUTATE_DEL_YES_ALWAYS | yesall RED -- non-Y response changed disk
CMD_MUTATE_DEL_REFUSE_Y | refusey RED -- DELTEST/*.TXT still present
```

## Required regressions

All remaining 41 targets in the final 57-target certificate passed:
test-command(+mutant), test-fileio, test-fat12-mkdir, test-spec,
test-fat-write-partial(+mutant), test-fat-fault-rollback(+mutant), test-shell,
test-dosmsg(+mutant), test-dosmsg-source-mutant, test-conkeys,
test-dir-match-host(+mutant), test-dir-match-emu(+mutant), test-xcopy-tree-emu,
test-dos-folder-host(+mutant), test-dos-folder(+mutant), test-edlin-emu,
test-zs24(+mutant), test-zs24-exec(+mutant), test-dos-safety-k15(+mutant),
test-dos-stamp-copy-host(+mutant), test-dos-stamp-copy(+emu-mutant), image,
test-flair-app-launch, test-flair-locked-purge, test-flair-finder-service,
test-flair-exit-to-dos, test-makefile-vars, test-fixture-volumes.

Final flagship evidence:

```text
>>> image data: build/flair_data.img (FAT12 primary-slave volume; README.TXT + APPS + DOS + AUTOEXEC.BAT)
>>> test-flair-app-launch: green
VERDICT: PASS -- test-flair-locked-purge (locked bytes/attributes intact, visible alert pixels, mtools + fsck.fat)
>>> test-flair-finder-service: green
>>> test-flair-exit-to-dos: green (QEMU; Bochs leg owed)
>>> test-makefile-vars: green
>>> test-fixture-volumes: green (334 scratch names vs 144 fixture names; 70 inline image writers)
GREEN final doscmds certificate: all 57 targets passed
```

New/modified source checked ASCII; new/modified shell scripts pass sh -n.
No code changed after the final certificate began; only these worklog notes.

## Law-1 sources and limits

All cited pages were rendered at 110 dpi with pdftoppm and inspected in this
launch. Manual root: /home/tobias/Projects/initech-manuals-ref/dos33/.

- MS-DOS_3.3_Users_Guide_198707.pdf: Guide p.17 (PDF30), valid/reserved names;
  Reference pp.7,10-11,13 (PDF92,95-96,98), paths, wildcards, directories and
  DEL confirmation; pp.46-47 (PDF131-132), COMMAND /C and copied environment;
  pp.50-52 (PDF135-137), COPY modes, joins, omitted target, self refusal and
  exact footer; p.56 (PDF141), DEL; p.91 (PDF176), LPT1..3; p.104 (PDF189),
  REN masks; p.127 (PDF212), XCOPY relative operands; p.325 (PDF410),
  Duplicate file name.
- 80X0945_DOS_3.30_Technical_Reference_Apr87.pdf: p.6-54 (PDF177), AUX/COM1..4;
  pp.6-119/120/121 (PDF242-244), directory path ABI; pp.6-139/140 (PDF262-263),
  write registers, short writes, CX=0 truncation and read-only refusal;
  p.6-188 (PDF311), GETCWD register/buffer layout.
- ADR-0003 Appendix A/B.1, DEC-04a/07/08 and Appendix C; PRD 6.1/8;
  docs/research/fat12-ground-truth.md sections 2-4; spec/dos_structs.h.
- Independent mtools 4.0.43 and dosfstools on identical FAT12 images.

No requested lane work remains blocked. Full make test, Bochs/86Box, a new
live-DOS golden, clean-build reproducibility, independent serial/parallel port
drivers, COM5+/LPT4+ edge names, and FAT16 writes were not implemented or run.
The unchanged XCOPY relative-source behavior was regression-proven rather than
rewritten. No new shipped file or grown-volume re-key was needed in this launch.
