# Host test import

Baseline: legacy-final at /home/tobiasosborne/Projects/InitechOS-legacy.
The corresponding legacy host targets were actually run; make -n commands
were checked for source/seam parity. No retired make fragment is included.

Every assigned legacy HOST gate, including mutant aliases and exclusions, is
listed in legacy-host-gates.tsv (gate, class, destination/reason). Its draft
attribution is corrected where shell or utility code belonged to DOS. The
three apparent host targets dir-args/path-command/drive-operands actually boot
an emulator; they belong in SMOKES.md. Outside-active-vector host helpers
for DOS break/not-ready are also retained.

| Test main source | Class | Legacy / new counted checks |
|---|---|---|
| kernel/tests/disk/dos_safety_fixture.c | SUPPORT (not a suite) | uncounted / uncounted |
| kernel/tests/disk/fat_dump.c | SUPPORT (not a suite) | uncounted / uncounted |
| kernel/tests/dosmsg_scan.c | SUPPORT (not a suite) | uncounted / uncounted |
| kernel/tests/test_dos_safety.c | UNIT | 8 x2, 12, 16, 17 / 8 x2, 12, 16, 17 |
| kernel/tests/test_redir.c | UNIT | 25 / 25 |
| dos/chkdsk/tests/chkdsk_fixture.c | SUPPORT (not a suite) | uncounted / uncounted |
| dos/chkdsk/tests/chkdsk_host.c | UNIT | uncounted / uncounted |
| dos/command/tests/batch2_host.c | UNIT | uncounted / uncounted |
| dos/command/tests/batch_break.c | UNIT | uncounted / uncounted |
| dos/command/tests/dir_match.c | ORACLE-STRONG | uncounted / uncounted |
| dos/command/tests/dos_parity_host.c | UNIT | uncounted / uncounted |
| dos/command/tests/dosutil_vol.c | UNIT | uncounted / uncounted |
| dos/command/tests/shell_files.c | UNIT | uncounted / uncounted |
| dos/command/tests/test_batch.c | UNIT | 101 / 101 |
| dos/command/tests/test_batch_exec.c | UNIT | 63 / 63 |
| dos/command/tests/test_cmdline_fuzz.c | ORACLE-STRONG | uncounted / uncounted |
| dos/command/tests/test_command.c | UNIT | 203 / 203 |
| dos/command/tests/test_dir_disk.c | UNIT | uncounted / uncounted |
| dos/command/tests/test_dir_look.c | UNIT | uncounted / uncounted |
| dos/command/tests/test_env.c | UNIT | 63 / 63 |
| dos/command/tests/test_redir_parse.c | UNIT | 160 / 160 |
| dos/common/tests/dosutil_host.c | UNIT | uncounted / uncounted |
| dos/diskutil/tests/formatsys_check.c | SUPPORT (not a suite) | uncounted / uncounted |
| dos/diskutil/tests/formatsys_host.c | UNIT | uncounted / uncounted |
| dos/edlin/tests/host.c | UNIT | uncounted / uncounted |
| dos/edlin/tests/model.c | SUPPORT (not a suite) | uncounted / uncounted |
| dos/fdisk/tests/test_fdisk.c | UNIT | uncounted / uncounted |
| dos/xcopy/tests/test_xcopy_tree.c | UNIT | uncounted / uncounted |

Unique registered mains: 28; ORACLE-STRONG 2, SUPPORT (not a suite) 6, UNIT 20.
Counted assertion totals across the retained gate invocations: 676 legacy /
676 new. Repeated counts use xN in the table; all counts/multiplicities match.
The table includes
all uncounted exit-status/differential mains instead of inventing counts.

Fuzz counts are deliberately bounded, not numerically compared with their full
legacy run: default CONFIG/command line 64 seeds, FAT model 12 and external
reader 2; THOROUGH=1 restores 4000, 200 and 40. Corruption uses the FAT model
seed count (12 default / 200 thorough), 40 operations per seed. CONFIG uses
24 lines; command line uses 320 operations. All invariants and shrinking remain.
Other retained differentials cover mtools bytes, fsck.fat, FAT-copy agreement,
rollback atomicity, nested directory operations, FAT16/windowed reads, physical
partitions and DOS captures. EDLIN retains its independent C model; CHKDSK uses
fsck.fat and genuine DOS hidden/label captures. FORMAT/SYS uses a data-only
system-file fixture, not a special kernel or the legacy transient interpreter.

The shell adapters are C executables that exec a POSIX shell script, preserving
exit status. Each case gets fresh writable images under OUT/host/cases; a short
temporary symlink avoids the mtools configuration line-length limit in mutation
scratch paths. The generic runner copies component sources, checks its exact
baseline, applies a zero-fuzz patch and rejects build errors as kills. Only
proved host patches are in the default sweep. Pending patches remain explicit.

No product behavior was fixed. Mutation-hook/comment removal leaves INITDOS.SYS
byte-identical: 19f2607679594684bbc2af47dcb3e890fc2d532fa66dcbb1e9de726ecb2800cc.
Host-seam edits remove stale includes and configure the reference interpreter; the
EDLIN model keeps a local format-truncation warning exemption. Target-build
rules and mk/ are unchanged; top-level changes only add kernel/dos to COMPONENTS.

Left in attic: retired SIZE/LAYOUT and META policy/generator checks belong to
the factory owner for eventual deletion. Pixel/panic checkers and special-image
recipes belong to the future factory diagnostic owner. DESKTOP.DB, Finder and
FLAIR shell/layout tests belong to the desktop owner. The old exact-address
COMMAND fixture checker and its baked-image gates are retired; functional file
loading remains covered by FAT, loader and normal-program tests. Real-DOS
capture minting/emulator adapters remain unbuilt, for the factory owner. Boot
mutation hooks are outside kernel/DOS ownership. ASM-only filters and TPS
Report Generator still need their mapped normal-image regression sessions.

Acceptance on this worktree:

- From a clean top-level build directory: kernel 86 gates, 0 failures, 24.45 s;
  DOS 31 gates, 0 failures, 11.90 s (-j64). Top-level make test: ALL GREEN,
  59.28 s, with kernel and DOS in COMPONENTS.
- make image and make smoke passed. Smoke includes both original sessions and
  the new normal-program session on QEMU and Bochs; combined wall 52.92 s.
- make repro passed across two clean builds. Boot, data and INITDOS.SYS hashes
  match the pre-import product bytes (see factory/ACCEPTANCE.md baseline).
- Red/restored controls: FAT12 read-file chain bound, INT21 ignored handle
  offset, batch GOTO missing label and CHKDSK hidden-file accounting all failed
  retained tier 0 in isolated patch copies. The live loader exit-code fault
  leaves all host tests and the original smoke green, but the new program
  session fails its nested-EXEC marker. The live source was restored.
- Kernel default host sweep: 229/229 killed. DOS sweep: 111 killed and three
  mis-selected survivors; corrected DIR/VOL selectors kill all three (114
  distinct host kills). Keyboard mutants also rechecked without retired Oz
  tuning: 13/13 killed. 85 converted patches remain explicitly unproved in
  mutants/pending; they are NOT part of this host certificate.
- Python absence is red: PYTHON_FAT_REFERENCE=/no/such/python makes test-fat
  fail loudly. Shell syntax checks and ASCII source scan passed.

Known inherited discrepancy, not fixed: loader_validation.c:479 applies overlay
fixups within the loaded destination, while the genuine DOS 3.30 unequal-factor
capture patches at the factor address. fixtures/dos33_overlay/README.txt records
this old deviation as awaiting operator ratification. FIND also still ignores
filename operands (dos/README.md); its redirected-input smoke is unchanged.
