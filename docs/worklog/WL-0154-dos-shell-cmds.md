# WL-0146 - dos-shell-cmds lane (first launch; superseded by WL-0147)

Second launch completed the authorized FAT truncation prerequisite and COPY
concatenation. The fresh, final 57-target certificate and current mutation
evidence are in WL-0155-doscmds-truncate.md. The partial results below describe
the first launch only.

## Context

One-shot, working alone in the doscmds sibling clone. Read CLAUDE.md from top
through the Laws and numbered Rules, PRD 6.1/8, ADR-0003 Appendix A, the supplied
bead export, WL-0071, WL-0116, WL-0130/0131, WL-0142 and WL-0144. bd show
could not find a live database; .beads/issues.jsonl supplied the requested rows.
No git commands, commits, pushes, tracker writes, CLAUDE.md or HANDOFF.md edits.
No agents, Bochs, or aggregate make test. Every QEMU gate exported
INITECH_QMP_STDIO=1. RAM/disk checks showed about 20-21 GiB available and
119-120 GiB free before the heavy runs.

## Stop condition and result

This lane is NOT complete. The final expanded host oracle fails:

    RED shell-files: APP.TXT bytes

Reproduction: the fixture APP.TXT starts with alpha, Ctrl-Z, tail, CRLF.
COPY APP.TXT + B.TXT should replace the first source EOF with B.TXT, then
append one EOF (MS-DOS 3.3 Reference pp.51-52). The implementation seeks to the
first EOF and invokes AH=40h with count zero. IBM Technical Reference p.6-140
explicitly says this truncates at the current file position. The backend's
fat12_write_partial, os/milton/fat12.c around line 2998, explicitly returns
success without changing size for len==0. int21.c sft_write forwards the call
through fileio_fat.c fat_write_at to that primitive. Actual APP.TXT retains
its original EOF/tail and appends B.TXT after the original directory EOF.

The existing FAT partial-write API needs a resize/truncation primitive and
allocation/rollback oracle. This is a prerequisite outside the listed shell
and directory-spec changes. Stopped at the operator's no-scope-growth rule;
no filesystem resize implementation or oracle weakening was attempted.
The new regression remains RED for the PM to reproduce. bwzs is incomplete.
The expanded fixture also adds three-file and fixed/pattern wildcard outputs;
its later assertions and the expanded QEMU run have not completed. The earlier
narrower fixture was GREEN on host and QEMU and was mutation-proven. Those
results are historical evidence, not a final GREEN for the expanded oracle.

## Changes

- command.c/command.h: COPY grammar separates /A, /B, /V and + from filenames;
  inherited source/destination modes; source lists, wildcard enumeration and
  output masks; identity probes precede destructive explicit concatenation;
  /V reads recorded bytes back; original single-copy timestamps and protection
  hooks remain exercised. The omitted-target ASCII path is blocked as above.
- REN enumerates matches, preserves parent paths, substitutes padded 8/3
  destination fields, rejects collisions without overwriting, and uses the
  manual's Duplicate file name diagnostic. Collision reference operations use
  mren -D s and require the whole reference image unchanged.
- COMMAND and COMMAND.COM /C share native dispatch, copied environment,
  bounded nesting, restored EXIT/permanence/error state and parent batch frame.
  Existing redirection scopes restore handles. Nested ECHO, SET isolation,
  EXIT, missing-command and primary VER controls pass QEMU.
- FAT creation rejects illegal leaf bytes independently of INT21's search
  masks. INT21 rejects device directory/new-file/rename targets; ordinary
  CREAT routes device targets through OPEN. Numbered aliases cover COM1..4
  and LPT1..3 through the existing AUX/PRN backend. Independent physical ports
  were not implemented or claimed. The raw 39h/3Ch/5Bh invalid-name matrix
  requires byte-identical images after refusal; qualified devices open as devices.
- AH=39h/3Ah/3Bh/47h rows added to locked calling-convention JSON with EDX
  versus DL/ESI pinned. New row-removal mutants exercise that documentation.
- XCOPY's relative source already resolves correctly in this clone. Added
  host/QEMU decoy regression and DU_MUT_REL_ROOT; production behavior unchanged.
- DEL confirmation already existed. Its existing k15 gates and mutants pass.
- MSG-DOS-0061 is the capital-F COPY footer; 0062 is Duplicate file name.
  JSON, Appendix C, codegen count, audit expectations and footer assertions
  updated deliberately. Two obsolete footer allow rows removed; frozen allow
  baseline was not expanded or regenerated.

## Files changed

Makefile
os/milton/command.c
os/milton/command.h
os/milton/test_command.c
os/milton/int21.c
os/milton/fat12.c
os/milton/xcopy_tree.c
spec/dos_messages.json
spec/int21h_calling_convention.json
docs/adr/ADR-0003-InitechDOS-Base-OS-Personality.md
harness/diff/fat_diff/shell_files.c
harness/diff/fat_diff/shell_files_host.h
harness/diff/fat_diff/shell_files.mk
harness/diff/fat_diff/shell_files.sh
harness/diff/fat_diff/command_c.sh
harness/diff/fat_diff/xcopy_cwd.sh
harness/diff/fat_diff/dir_abi.c
harness/diff/fat_diff/dosmsg.allow
harness/diff/fat_diff/dos_safety.sh
harness/diff/fat_diff/copy_stamp.sh
docs/worklog/WL-0154-dos-shell-cmds.md

## Oracle design and evidence

Host seam compiles the actual command.c builtins with the actual INT21/FAT
backend; host stack pointers are bounced into its flat aperture. Emulator
fixture drives AUTOEXEC plus injected EXIT, checks SHELL-DONE, key counts,
completion markers and no triple fault. mcopy/mren/mdir/mtype and fsck.fat
judge identical cloned images. mren has no DOS output-wildcard mapping, so
reference leaf renames are explicit independent p.104 substitutions. ASCII
and concatenated expected bytes are independent literal fixtures from pp.50-52.
No artifact helper constructs expected bytes. All twelve new targets are in
TEST_UNIT_GATES/TEST_EMU_GATES through shell_files.mk.

Before behavior changes, host and QEMU failed ASCII.TXT missing. COMMAND /C
failed child output/handle restoration. ABI validator failed 39h row missing.
The relative-XCOPY implementation was already GREEN; its root mutant is RED.

New targets and their last GREEN tails follow. The shell-files entries below
refer to the earlier narrower matrix; final host status is RED, as above.

```text
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
test-dir-abi | GREEN test-dir-abi: 39h/3Ah/3Bh EDX and 47h DL/ESI, CF/AX; IBM pp.6-119..121,6-188
test-dir-abi-mutant | GREEN test-dir-abi-mutant: ABI_39 ABI_3A ABI_3B ABI_47 caught RED
```

Named mutation RED evidence, captured against the earlier matrix:

```text
CMD_MUTATE_REN_ENUM | RED shell-files: REN membership != mtools
CMD_MUTATE_REN_MAP | RED shell-files: REN membership != mtools
CMD_MUTATE_COPY_PLUS | RED shell-files: JOINED.BIN bytes
CMD_MUTATE_COPY_ASCII | RED shell-files: ASCII.TXT bytes
CMD_MUTATE_COPY_EOF | RED shell-files: TEXT.TXT bytes
CMD_MUTATE_COPY_SWITCH | RED shell-files: ASCII.TXT bytes
CMD_MUTATE_COPY_VERIFY | RED shell-files: COPY /V accepted corrupt read-back
CMD_MUTATE_COPY_WILD | RED shell-files: DST membership != mtools
FAT12_MUTATE_CREATE_CHARS | RED shell-files: created-name matrix
INT21_MUTATE_MKDIR_DEVICE | RED shell-files: created-name matrix
CMD_MUTATE_COMMAND_C | RED command-c: child output/handle restoration
CMD_MUTATE_COMMAND_ENV | RED command-c: child environment leaked
DU_MUT_REL_ROOT (host and QEMU) | RED xcopy-cwd: relative source selected root decoy
ABI_39 | RED dir-abi: 39h row missing
ABI_3A | RED dir-abi: 3Ah row missing
ABI_3B | RED dir-abi: 3Bh row missing
ABI_47 | RED dir-abi: 47h row missing
CMD_MUTATE_REN_MAP (QEMU) | RED shell-files: REN membership != mtools
CMD_MUTATE_COPY_WILD (QEMU) | RED shell-files: DST membership != mtools
```

Required and directly relevant regressions passed before the final fixture
expansion. Full transcripts are build/lane-doscmds/<target>.log. Exact final
non-separator evidence follows (test-command rerun after its grammar assertions
were added: 203 checks, 0 failures).

```text
test-command | >>> test-command: green
test-command-mutant | >>> test-command-mutant: green (no-samefile mutant correctly RED -- the COPY onto-itself guard bites)
test-fileio | >>> test-fileio: green
test-fat12-mkdir | >>> test-fat12-mkdir: green
test-spec | VERDICT   : PASS -- ADR-0003 spec-data parses, sizes hold, banner verbatim, INT 25h/26h locked
test-shell |             (QEMU only; tri-emulator agreement pending beads initech-x0i)
test-dosmsg | >>> test-dosmsg: green
test-dosmsg-mutant | >>> test-dosmsg-mutant: green
test-conkeys | GREEN test-conkeys: injected template keys + pause transcript; LINES.TXT == mtools on identical FAT12 images; dosfstools clean
test-dir-match-host | GREEN dir-match-host: *.TXT / A*.* / FOO?.* == mdir + test-80k matcher; identical image, fsck.fat clean
test-dir-match-host-mutant | GREEN test-dir-match-host-mutant: INT21_MUTATE_WILD_QMARK_LITERAL caught RED
test-dir-match-emu | GREEN dir-match-emu: *.TXT / A*.* / FOO?.* == mdir + test-80k matcher; identical image, fsck.fat clean
test-dir-match-emu-mutant | GREEN test-dir-match-emu-mutant: CMD_MUTATE_DIR_ARGS caught RED
test-xcopy-tree-emu | XCOPY/TREE EMU GREEN: external flat EXEC; injected /W /P keys; FAT differential; TREE byte-exact; SHELL-EXIT
test-dos-folder-host | GREEN test-dos-folder-host: exact DOS/APPS order, AUTOEXEC bytes, artifact FAT reader == mtools, fsck.fat
test-dos-folder | ppm_flair_disk_windows_check: leg dosfolder PASS (9 icon strikes graded off the finder_icons.h ASCII maps, 9 label-band relations, 0 empty cells, 11 chrome assertions, 2 bare-teal controls)
test-edlin-emu | GREEN test-edlin-emu (all command scripts; independent manual model)
image | >>> image data: build/flair_data.img (FAT12 primary-slave volume; README.TXT + APPS + DOS + AUTOEXEC.BAT)
test-flair-app-launch | >>> test-flair-app-launch: green
test-flair-locked-purge | VERDICT: PASS -- test-flair-locked-purge (locked bytes/attributes intact, visible alert pixels, mtools + fsck.fat)
test-flair-finder-service | >>> test-flair-finder-service: green
test-flair-exit-to-dos | >>> test-flair-exit-to-dos: green (QEMU; Bochs leg owed)
test-zs24 | >>> test-zs24: green
test-zs24-mutant | >>> test-zs24-mutant: green (ALL FIVE mutants ran + RED for the right reason -- write-back, CREATE gate, UNLINK, grow relink/EOC, grow-refuse all bite)
test-zs24-exec |             (QEMU only; tri-emulator agreement pending beads initech-x0i)
test-zs24-exec-mutant | >>> test-zs24-exec-mutant: green (BOTH legs RED for the right reason; the oracle bites on do_exec resolve AND loader subdir-find)
test-dos-safety-k15 | VERDICT: PASS -- test-dos-safety-k15y (booted shell + mtools bytes + fsck.fat)
test-dos-safety-k15-mutant | VERDICT: PASS -- test-dos-safety-k15-mutant (no prompt / non-Y accepted / Y refused each RED)
test-dos-stamp-copy | PASS copy-stamp-emu: COPY + XCOPY bytes and exact source date/time == mcopy -m; mdir equal; fsck.fat clean
test-dos-stamp-copy-emu-mutant | PASS copy-stamp emulator mutant: XCOPY_TIME booted, completed and caught RED
test-dosmsg-source-mutant | GREEN dosmsg mutant NASM_DIAGNOSTIC: named assertion RED
test-makefile-vars | >>> test-makefile-vars: green
test-fixture-volumes | >>> test-fixture-volumes: green (334 scratch names vs 144 fixture names; 70 inline image writers)
```

Initial flagship rebuild lacked build/flair_dac.h. Generated it with the
existing tools/flair_dac_gen.c and locked clut.json/color_canon.json; did not
change palette source or its build rules. Subsequent image/desktop gates passed.
No claim of a fresh clean-build certificate. Source additions are ASCII and
new shell scripts pass sh -n. Final failing run is
build/lane-doscmds/final-copy-matrix.log; fixture bytes and transcript are in
build/shell-files-host-green (the historical directory name does not mean its
latest contents pass). No further implementation/gates were run after the stop.

## Law-1 sources

All cited pages were rendered at 110 dpi with pdftoppm and viewed. Local manual
root: /home/tobias/Projects/initech-manuals-ref/dos33/.

- MS-DOS_3.3_Users_Guide_198707.pdf, Guide p.17 / PDF30: valid leaf characters,
  reserved device names. Reference pp.7,10-11,13 / PDF92,95-96,98: paths,
  wildcards, directory operations and DEL confirmation; p.56 / PDF141: DEL.
- Same, Reference pp.46-47 / PDF131-132: secondary COMMAND environment and /C;
  pp.50-52 / PDF135-137: COPY operands, /A /B /V placement, text EOF, joining,
  default target, timestamps and self refusal; p.104 / PDF189: REN masks;
  p.325 / PDF410: Duplicate file name; p.127 / PDF212: relative XCOPY operands;
  p.91 / PDF176: LPT1..3 names.
- 80X0945_DOS_3.30_Technical_Reference_Apr87.pdf, pp.6-119/120/121 / PDF242-244:
  directory path/CF+AX ABI; p.6-188 / PDF311: DL and DS:SI, 64 bytes, no leading
  backslash or drive; p.6-54 / PDF177: AUX/COM1..4; pp.6-139/140 / PDF262-263:
  AH40 short-write handling and zero-count truncation (the unmet prerequisite).
- ADR-0003 Appendix A, DEC-04a, DEC-08, DEC-11 and Appendix C; PRD 6.1/8.
- Independent mtools 4.0.43 and dosfstools operations on identical FAT12 images.
  No live DOS 3.3/86Box golden was minted. COM5+ and LPT4+ edges were not added
  from modern DOS assumptions; covered names are explicitly listed above.
