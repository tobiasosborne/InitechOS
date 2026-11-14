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
| dos/command/tests/dos_parity_host.c | UNIT | uncounted / uncounted |
| kernel/tests/disk/dos_safety_fixture.c | SUPPORT (not a suite) | uncounted / uncounted |
| kernel/tests/disk/fat_dump.c | SUPPORT (not a suite) | uncounted / uncounted |
| kernel/tests/disk/truncate.c | UNIT | uncounted / uncounted |
| kernel/tests/dos_volumes_host.c | ORACLE-STRONG | uncounted / uncounted |
| kernel/tests/dosmsg_scan.c | SUPPORT (not a suite) | uncounted / uncounted |
| kernel/tests/fat12_corrupt_fuzz.c | ORACLE-STRONG | uncounted / uncounted |
| kernel/tests/fat12_fuzz.c | ORACLE-STRONG | uncounted / uncounted |
| kernel/tests/fixtures/dos33_overlay/check.c | SUPPORT (not a suite) | uncounted / uncounted |
| kernel/tests/int21_dpb_host.c | UNIT | uncounted / uncounted |
| kernel/tests/test_40oq.c | UNIT | 250 / 250 |
| kernel/tests/test_absdisk.c | UNIT | 47 / 47 |
| kernel/tests/test_ansi.c | UNIT | 1167 / 1167 |
| kernel/tests/test_ansi_bridge.c | UNIT | uncounted / uncounted |
| kernel/tests/test_ansi_wire.c | UNIT | 143 / 143 |
| kernel/tests/test_ata.c | UNIT | 1052 / 1052 |
| kernel/tests/test_block_device.c | UNIT | uncounted / uncounted |
| kernel/tests/test_config_sys.c | UNIT | 33 / 33 |
| kernel/tests/test_config_sys_fuzz.c | ORACLE-STRONG | uncounted / uncounted |
| kernel/tests/test_conin.c | UNIT | 148 x3 / 148 x3 |
| kernel/tests/test_conkeys.c | UNIT | uncounted / uncounted |
| kernel/tests/test_console.c | UNIT | 105 / 105 |
| kernel/tests/test_console_look.c | UNIT | uncounted / uncounted |
| kernel/tests/test_devices.c | UNIT | 150 / 150 |
| kernel/tests/test_devwire.c | UNIT | 129 x2 / 129 x2 |
| kernel/tests/test_dos_break.c | UNIT | uncounted / uncounted |
| kernel/tests/test_dos_not_ready.c | UNIT | uncounted / uncounted |
| kernel/tests/test_dos_safety.c | UNIT | 8 x2, 12, 16, 17 / 8 x2, 12, 16, 17 |
| kernel/tests/test_drive_letters.c | UNIT | uncounted / uncounted |
| kernel/tests/test_exec.c | UNIT | 68 / 68 |
| kernel/tests/test_fat12_bpb.c | UNIT | 28 x2 / 28 x2 |
| kernel/tests/test_fat12_chain.c | UNIT | 23 x2 / 23 x2 |
| kernel/tests/test_fat12_dir.c | UNIT | 83 x2 / 83 x2 |
| kernel/tests/test_fat12_fault_rollback.c | UNIT | 35 / 35 |
| kernel/tests/test_fat12_mkdir.c | ORACLE-STRONG | 51 / 51 |
| kernel/tests/test_fat12_mkdir_nested.c | ORACLE-STRONG | 40 / 40 |
| kernel/tests/test_fat12_mkdir_rollback.c | UNIT | 2863 / 2863 |
| kernel/tests/test_fat12_move.c | ORACLE-STRONG | 149 / 149 |
| kernel/tests/test_fat12_partial.c | UNIT | 204 / 204 |
| kernel/tests/test_fat12_rename.c | ORACLE-STRONG | 43 / 43 |
| kernel/tests/test_fat12_subdir.c | UNIT | 64 x2 / 64 x2 |
| kernel/tests/test_fat12_write.c | ORACLE-STRONG | 97 x2 / 97 x2 |
| kernel/tests/test_fat12_write_partial.c | ORACLE-STRONG | 219 / 219 |
| kernel/tests/test_fat16.c | UNIT | 59 / 59 |
| kernel/tests/test_fat16_window.c | UNIT | 57 / 57 |
| kernel/tests/test_fat_spc.c | ORACLE-STRONG | uncounted / uncounted |
| kernel/tests/test_fileio.c | UNIT | 225 / 225 |
| kernel/tests/test_fileio_subdir.c | UNIT | 4 x5, 5, 7, 65 / 4 x5, 5, 7, 65 |
| kernel/tests/test_fpu.c | UNIT | 18 / 18 |
| kernel/tests/test_idt.c | UNIT | 28 / 28 |
| kernel/tests/test_int21.c | UNIT | 179 x2 / 179 x2 |
| kernel/tests/test_int21_edge.c | UNIT | 101 / 101 |
| kernel/tests/test_int21_wildcard.c | UNIT | 41 / 41 |
| kernel/tests/test_int24.c | UNIT | 44 / 44 |
| kernel/tests/test_int24_wired.c | UNIT | 19 / 19 |
| kernel/tests/test_kbd.c | ORACLE-STRONG | 1471 / 1471 |
| kernel/tests/test_keep.c | UNIT | 44 / 44 |
| kernel/tests/test_loader.c | UNIT | 61 / 61 |
| kernel/tests/test_loader_big.c | UNIT | 14 / 14 |
| kernel/tests/test_mcb.c | ORACLE-STRONG | 57 / 57 |
| kernel/tests/test_mcb_int21.c | UNIT | 34 / 34 |
| kernel/tests/test_mz.c | UNIT | 60 / 60 |
| kernel/tests/test_mzload.c | UNIT | 53 / 53 |
| kernel/tests/test_native.c | ORACLE-STRONG | uncounted / uncounted |
| kernel/tests/test_overlay.c | UNIT | 105 / 105 |
| kernel/tests/test_partition.c | UNIT | uncounted / uncounted |
| kernel/tests/test_psp.c | UNIT | 97 / 97 |
| kernel/tests/test_redir.c | UNIT | 25 / 25 |
| kernel/tests/test_rtc.c | UNIT | 36 / 36 |
| kernel/tests/test_sft.c | UNIT | 56 / 56 |
| kernel/tests/test_tenant_load.c | UNIT | 25 / 25 |
| kernel/tests/test_tsr.c | UNIT | 41 / 41 |
| kernel/tests/test_tsr_io.c | UNIT | 16 / 16 |

Unique registered mains: 73; ORACLE-STRONG 14, SUPPORT (not a suite) 4, UNIT 55.
Counted assertion totals across the retained gate invocations: 11214 legacy /
11214 new. Repeated counts use xN in the table; all counts/multiplicities match.
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
  tuning: 13/13 killed. 85 converted patches were unproved at import and were NOT part of that
  certificate. Kernel's original39 pending faults now have repeated semantic
  kills; current component README records that completion. DOS owns its queue.
- Python absence is red: PYTHON_FAT_REFERENCE=/no/such/python makes test-fat
  fail loudly. Shell syntax checks and ASCII source scan passed.

The inherited unequal-factor overlay discrepancy is repaired and graded against
genuine DOS3.30 product-state captures (initech-i1swl); see the component README.
