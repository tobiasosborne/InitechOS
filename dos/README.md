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
- Complex mutation guards remain disabled; see kernel/tests/MUTANTS-TODO.md.

Tier 0 is intentionally pending: command/batch/env host tests were moved
untouched to kernel/tests/ with the other kernel tests. Move these to dos/tests
when importing their legacy target dependencies. Claim utility host tests and
real-DOS fixtures from attic/harness/diff with git mv, adjust include paths,
use mk/host-test.mk, and retain existing host seams (COMMAND_FILE_HOST,
COMMAND_BATCH_HOST etc.). `make smoke` uses the outside factory session.

FLAIR is a built-in command that calls the optional public kernel environment
hook. It returns to the existing interpreter and current batch context when
the user leaves the desktop; it does not start another COMMAND.COM. Product
AUTOEXEC.BAT ends with FLAIR. The system link supplies the environment, while
the standalone DOS kernel reports that it is not installed.
