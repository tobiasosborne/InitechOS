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
- Inactive mutation guards were converted to patches; see tests/MUTANTS-TODO.md.

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
factory sessions. `make mutants` runs proved host patches; pending patches
remain explicitly unproved under each core's mutants/pending/ directory.
See tests/IMPORT.md and tests/MUTANTS-TODO.md for counts and remaining work.
