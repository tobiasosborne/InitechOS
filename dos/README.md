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
message catalogue).
The utility entry and register bridges use cdecl; their transfer/return runs
in the product smoke. FIND consumes redirected stdin; filename operands,
unknown switches and empty or unterminated search strings are refused with
its period parameter diagnostic and status 1 (initech-iuuni; exact DOS 3.30
FIND text awaits genuine media, initech-0e5gu). Smoke uses `FIND "apple" < WORDS.TXT`;
`make smoke-find-args` checks the refusals in the booted product.

Published application bindings (CLAUDE.md, "Two interfaces"):

- CHKDSK and LABEL use INT25/26 for mounted-volume sectors and INT21 for
  console/process services. ATTRIB uses INT21 attribute services; the shared
  LABEL adapter's raw-sector helpers are not an executed ATTRIB path.
- FORMAT/SYS/DISKCOPY/DISKCOMP and FDISK use INT13 physical-disk and drive-map
  services plus INT21 console/process services.
- The filters, EDLIN, XCOPY/TREE and TPSREP use INT21; entry fallbacks may use
  the standard INT20 exit. Disk programs link their own utility cores only.
- CHKDSK takes its memory figures from the kernel's OEM INT 21h AH=F9h
  arena summary and FDISK restarts through OEM AH=FAh (initech-q9p80;
  contracts in kernel/README.md and kernel/include/dos_oem.h). Neither
  utility links kernel code or touches hardware ports for these services.
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
XCOPY retains the whole source in a 128 KiB conventional-memory buffer before
creating or truncating its destination, so a source read failure at any offset leaves
a pre-existing destination's bytes intact (genuine XCOPY buffers sources
in memory; its exact read-failure behavior is still uncaptured,
initech-0e5gu, so retention on read failure is the stated safe rule,
initech-wz16r/initech-q2w4b/initech-gze1f). /V compares the destination
with those retained bytes without re-reading the source. Larger files refuse
before writing with Insufficient memory and exit 4 (MS-DOS 3.3 Guide p.129);
128 KiB is a local capacity, not a captured genuine XCOPY limit. Once a destination has been opened for writing, a failed copy
or a failed /V verification removes the failed output rather than leaving a
partial or source-stamped bad copy (initech-wz16r, initech-w33id).
Batch physical lines retain 127 characters and discard the remainder, exactly
as the synchronized OEM DOS capture does. Separate 122- and 123-space DEL
cases retain KEEP.DAT and JUNK.TMP; tests compare the captured input volume,
diagnostics and surviving bytes directly.

Mutation coverage now includes an external product-session runner under
`tests/runtime/`: target code is patched in disposable copies, built normally,
and linked into the same product composition. Tests never add scenarios to
kernel startup. `make mutants-host` runs host patches; `make mutants-sessions`
builds the product image and proves the runtime patches, each twice after a
green baseline. `make mutants` runs both, with factory-compatible counts.
The DOS smoke leg remains `sh factory/smoke.sh` from the repository root.

The current DOS inventory is 154 host patches and 48 product-session patches,
with zero pending entries. This includes the 46 formerly pending faults and
four new controls for physical-line truncation, COPY output cleanup, missing
REM sentinel dispatch and execution of a REM tail.
`make smoke-errors` checks the captured COPY ERRORLEVEL state in the booted
product as well as destination removal and source preservation.

Runtime verdicts distinguish semantic assertions (exit 42 plus a stable assertion
record and a healthy completed emulator report) from transport/setup errors
(exit 2). Only the same explicit assertion failing twice kills a patch. The
no-op, injected timeout and startup-failure controls run before the default
session sweep; unexpected exits and incomplete transport are errors. TPS invalid
input tests supply enough input for faulty programs to finish, then grade the
required diagnostic. Missing completion is never a mutation kill.

`tests/mutation-verdicts.tsv` records the corrected phase-2 re-sweep of the
46 formerly pending patches: eight host and 38 runtime faults, each failing
twice. Runtime entries name the actual completed-run semantic assertion.
The prior timeout-based totals are superseded by that explicit re-sweep.
The post-compatibility repeat used product base `db0cc4bd92a3e8f2ca8c60bc9e537eb1a8e4ca52`:
all 46 faults failed twice, with 76 healthy completed runtime assertions and
16 host semantic failures. The AUTOEXEC fault initially failed to build after
its removed call became the helper's only use; the refreshed patch retains an
unused helper reference while still skipping execution. That build error was
recorded as an error, then replaced by two explicit missing-AUTOEXEC assertions.

The DOS 3.30 compatibility study is retained in command/tests/compat33/.
The default host gate verifies its evidence hashes, not the intentionally
pending comparisons of old product observations. Each repaired behavior is
promoted separately to a product regression against the genuine capture.

Captured DOS 3.30 regressions also cover invalid DEL/REN switches, FOR
separators and wildcard continuation, IF EXIST wildcards, batch invocation
names, CD.., and COPY destination 8.3 spelling. `make smoke-batch-return`
checks nested COMMAND continuation; `make smoke-unknown-redirect` checks
unknown-command output and file preservation. `make smoke-dir-wide` compares
complete single-member and multirow DIR /W transcripts to the synchronized
OEM capture, including tabs and CRLF. The default wide-row host gate enumerates
the captured input volume independently with mtools.
`make smoke-pipe-names` compares DOS 3.30's two clock-named temporary buffers,
rotation, reuse and cleanup against fresh pinned captures. Product sessions
also seed the clock namespace with foreign files and verify every byte survives.
The fraction-time capture is retained separately for initech-ubpjy; the pipe
regression uses a separately captured seconds-only TIME input. TIME now
accepts the captured hundredths suffix and '.' field separators
(reference/datetime{,2,3}); the parsed hundredths are passed to AH=2Dh in DL,
which the current kernel RTC seam discards at one-second resolution.
`make smoke-control-syntax` checks the captured IF/FOR diagnostic decisions and
keeps error output on CON. The pure IF result also carries its diagnostic kind;
false conditions, silent nonnumeric ERRORLEVEL and missing command bodies have
separately captured outcomes. The numeric FOR variable is a valid control.
`make smoke-utility-errors` checks the recovered CHKDSK, SYS, DISKCOPY,
DISKCOMP and ATTRIB invalid-switch cases. Raw DISKCOPY/DISKCOMP refusals retain
their blank line and BEL. CHKDSK reports the invalid switch and continues its
report; empty hidden categories are omitted as in the genuine label captures.
ATTRIB sends its captured blank line and Syntax error to CON. Utility exit
codes beyond the existing adapter contracts still need separate AH=4Dh captures.

GM2 (operator, 2026-11-14): FAT16 volumes are import media. FORMAT refuses
both an existing FAT16 filesystem (including larger cluster sizes) and a
proposed FAT16 layout. SYS, LABEL changes, CHKDSK /F, and DISKCOPY/DISKCOMP
FAT16 targets are refused with catalogue MSG-DOS-0008 before writes. Plain
CHKDSK remains a reader. FDISK preserves every mapped FAT16 partition entry,
including its active flag, before writing the MBR. Blank media and FAT12
maintenance retain their existing behavior. File-based utilities use INT 21h;
the kernel refuses their FAT16 mutations with Access denied.
`test-fat16-utilities` uses mtools-created images, writable adapters, write-call
counters and whole-image identity; existing fsck/mtools FAT12 controls remain.
`make smoke-fat16` runs the DOS import/refusal session on QEMU and Bochs,
including mtools readback of Bochs's actual private writable boot image.
For the identity-oracle negative control, set DOS_POLICY_WRITE_CONTROL=1;
a legitimate FAT12 write to C: must produce the explicit whole-image failure.

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: dos
CHKDSK's memory figures come from the OEM INT 21h AH=F9h summary and FDISK
restarts through OEM AH=FAh (initech-q9p80).
The resident command processor's existing kernel calls are the separate
CLAUDE.md exception, ending at separately loaded COMMAND.COM (Release 2).
Interface temporary: resident-command initech-1frx2
Interface use-binding: kernel/include/blockdev.h
Interface use-binding: kernel/include/conedit.h
Interface use-binding: kernel/include/devices.h
Interface use-binding: kernel/include/diskutil.h
Interface use-binding: kernel/include/dos_structs.h
Interface use-binding: kernel/include/environment.h
Interface use-binding: kernel/include/fat12.h
Interface use-binding: kernel/include/fileio_fat.h
Interface use-binding: kernel/include/find_data.h
Interface use-binding: kernel/include/idt.h
Interface use-binding: kernel/include/int21.h
Interface use-binding: kernel/include/io.h
Interface use-binding: kernel/include/memory_map.h
Interface use-records: kernel/include/drive_letters.h
Interface use-records: build/kernel/dos_messages.h
Interface shell-services: fileio_fat_volume_label int21_batch_checkpoint int21_current_drive int21_directory_exists int21_directory_path int21_get_psp int21_io_drive int21_same_file int21_select_path int21_set_break_recovery int21_write_fault kernel_graphical_environment
<!-- end-interface-declarations -->

SORT resource safety is checked by `make smoke-sort-limits`: 64 KiB input
and 7680 line records fit below ENV_BLOCK; additional bytes/records refuse
before output. These are local safety bounds, not captured DOS limits.
The MS-DOS 3.3 User's Guide pp.116-117 and p.357 describes SORT and its
memory diagnostic but gives no numeric limit. Authentic SORT/FIND binaries
are absent from the pinned media. initech-0e5gu tracks the exact-boundary,
read-error and diagnostic/ERRORLEVEL captures. Until then, limit refusals
use the period catalogue's "Insufficient memory" and status 1; read failures
use "Error reading file" and status 1. These are stated safety assumptions.

`make smoke-find-limits` checks FIND's provisional 64 KiB input bound with
its only match beyond 70 KiB. FIND probes EOF when full and refuses before
emitting any line or count if more input remains. It does not claim the
recovered DOS utility's streaming capacity; initech-0e5gu tracks that capture.

`make smoke-filter-full` checks the write side (initech-8ao8f): on a full
disk, SORT, FIND and FIND /C redirected to a file end with nonzero
ERRORLEVEL (a diagnostic reaches CON through the shell) and mtools proves
the output really is a truncated prefix. A reported write failure never
exits 0. The in-filter diagnostic "Insufficient disk space" (MSG-DOS-0005)
and status 1 are stated safety assumptions pending genuine media
(initech-0e5gu).

SORT /+n compares from the documented one-based column (MS-DOS 3.3 User's
Guide p.116), including short/empty suffixes and /R. `make smoke-sort-columns`
checks literal manual-derived column order. Unknown/malformed switches refuse
with the period catalogue's "Invalid parameter", status 1, under GM2's
explicit-refusal requirement. That exact SORT diagnostic/status is a stated
assumption pending authentic media in initech-0e5gu.
