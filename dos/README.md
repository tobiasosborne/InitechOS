# InitechDOS command processor and utilities

command/ owns command.c, batch.c and env.c. Its temporary resident objects
link into INITDOS.SYS using only command/entry.h from the kernel. The real
permanent interpreter runs AUTOEXEC.BAT, prompts for date/time when absent,
and enters A:\>. COMMAND.COM on the data disk is a small flat INT21 EXEC
forwarder to the resident COMSPEC handler; it contains no kernel addresses and
is not the retired exact-address transient. It is not yet an independent
command interpreter.

To load COMMAND.COM independently: replace direct calls to int21_current_drive,
int21_io_drive, int21_select_path, int21_directory_path/exists, int21_same_file,
int21_write_fault, int21_get_psp, int21_set_break_recovery and
int21_batch_checkpoint, plus fileio_fat_volume_label, with a documented syscall
interface; move its env and break recovery state into a process; arrange a
permanent parent PSP, shell restart/termination semantics and COMSPEC child
routing. It also writes the fixed ENV_BLOCK for EXEC inheritance. Its ordinary
file/console/EXEC operations already issue INT21. Narrow entry access does not
make these internal dependencies disappear.

`make` builds all fifteen existing external utilities as normal flat .COM
programs in build/dos: MORE, FIND, SORT, CHKDSK, EDLIN, FDISK, FORMAT, SYS,
DISKCOPY, DISKCOMP, XCOPY, TREE, ATTRIB, LABEL and TPSREP. The shared pure helper
code is in common/; FORMAT/SYS/DISKCOPY/DISKCOMP share diskutil/;
XCOPY/TREE share xcopy/. Default cdecl and uniform shared -Os flags apply.
Dependencies: kernel/include (public DOS/machine contracts and generated
message catalogue); CHKDSK also links the pure MCB validator from kernel/mcb.c.
The utility entry and register bridges use cdecl; their transfer/return runs
in the product smoke. FIND's existing implementation consumes redirected stdin;
its filename operands are still ignored. Smoke uses `FIND "apple" < WORDS.TXT`.

Remaining boundary debt, inherited unchanged from legacy utilities:

- CHKDSK/ATTRIB/LABEL use period INT25/26 for raw volume sectors; CHKDSK also
  reads the conventional MCB chain directly. They therefore exceed a strict
  INT21-only program boundary. Keep independent disk/MCB oracles when moving
  this behind a future documented gate.
- FORMAT/SYS/DISKCOPY/DISKCOMP and FDISK use the existing INT13 physical-disk
  and drive-map interface. FDISK uses power.h's 8042 reset after a write.
  The standard filters, EDLIN, XCOPY/TREE and TPSREP use INT21 only.
- Utilities use the fixed conventional PSP/program layout, not a relocatable
  process ABI. The product keeps the old format/loader limits.
- Mutation variants are patch files, graded by host tests or product sessions.

`make test` runs the shell, batch/environment/redirection parsers, disk utilities,
CHKDSK, EDLIN, FDISK, XCOPY/TREE, ATTRIB/LABEL and directory/message checks.
Tests live in each core's tests/ directory; common integration wiring is in
tests.mk and tests/tools.mk. mk/host-test.mk builds and records each entry.
The inherited ASM-only filters/TPS Report Generator have no host core; their
behavior remains a program/session obligation, explicitly mapped in
tests/SMOKES.md. The existing product smoke runs the filters.

`make smoke-programs` only assembles the factory probes; factory installs them
on its separate disposable data disk. Product image assembly uses only the
fifteen PROGRAMS and never copies these probes. `make smoke` invokes the
factory sessions. `make mutants` runs the host and product-session patch inventories.
The import history for other legacy tests remains in tests/IMPORT.md.

FLAIR is a built-in command that calls the optional public kernel environment
hook. It returns to the existing interpreter and current batch context when
the user leaves the desktop; it does not start another COMMAND.COM. Product
AUTOEXEC.BAT ends with FLAIR. The system link supplies the environment, while
the standalone DOS kernel reports that it is not installed.

COPY disk-full behavior follows the provisional DOS 3.30 ruling: remove the
failed output, print the captured insufficient-space message and copied-file
count, and retain ERRORLEVEL. This includes destructive overwrite and append
failures. Sources: goldens/dataloss/dos and command/tests/dos33/reference.
Batch physical lines retain 127 characters and discard the remainder, exactly
as the new OEM DOS capture does; the reported long-mask scenario retains both
KEEP.DAT and JUNK.TMP in the reference and the real command/FAT host test.

Mutation coverage now includes an external product-session runner under
`tests/runtime/`: target code is patched in disposable copies, built normally,
and linked into the same product composition. Tests never add scenarios to
kernel startup. `make mutants-host` runs host patches; `make mutants-sessions`
builds the product image and proves the runtime patches, each twice after a
green baseline. `make mutants` runs both, with factory-compatible counts.
The DOS smoke leg remains `sh factory/smoke.sh` from the repository root.

The current DOS inventory is 124 host patches and 38 product-session patches,
with zero pending entries. This includes the 46 formerly pending faults and
two new controls for physical-line truncation and COPY output cleanup.
`make smoke-errors` checks the captured COPY ERRORLEVEL state in the booted
product as well as destination removal and source preservation.
