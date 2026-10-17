# WL-0156 -- cqej second half: tonight's Bochs boot legs

Solo lane in the bochs2 sibling. Changes are uncommitted for PM review.
No Git metadata, beads, CLAUDE.md or HANDOFF.md edits. Read CLAUDE.md in
full, PRD Sec 5/6.1/6.3/6.5/6.6/8/14, ADR-0003 Appendix A, WL-0131's
Bochs row and the available WL-0132..0155 shards. bd show initech-cqej
reported no database; read the checked-in .beads/issues.jsonl record.

## Changes and coverage boundary

Makefile includes harness/bochs_legs.mk after all lane image variables are
defined. The existing macro now supplies 68 additional named boot legs in
TEST_EMU_GATES. New exec/mzexec profiles use those kernels' real completion
markers; the absent-slave profile supplies no disk2 and requires the exact
READ-failure mount marker. Existing legs keep their previous boundaries.

The forced-0101h image gets an explicit VGA13 profile: exact VIDEO
320x200/8bpp, pitch320, aperture000A0000, DAC6 plus the 640x480 guard.
Makefile documents that Bochs cannot grade the desktop DAC/pitch presenter:
it has no usable VBE LFB here and the guard runs before that presenter.
This is boot-only coverage, with no input replay, desktop pixels, DML,
COPY/REN/truncation parity, FORMAT-created-volume boot, or restart claims.
The negative COMMAND fixtures differ after the same guard, so their named
leg uses the shared stock boot kernel. MZ/baked self-test legs use their
actual kernel/data images but do not claim the QEMU negative stimulus.
test-diskcopy-emu has no leg: WL-0147 explicitly records it as QEMU-RED,
outside the requested QEMU-green set.

The first attempted shell leg exposed a pre-existing build error:
KERNEL_SECONDARY_SHELL_MAIN_OBJ had serial_trace_pack as its first
prerequisite but still compiled $<. GCC treated the packer executable as
an unused linker input and produced no kmain_shell_secondary.o. The
one-line correction uses the existing kernel-main-compile house macro
with the unchanged BOOT_SHELL/BOOT_SECONDARY_SHELL flags. The secondary
image then built and passed the FAT/ANSI QEMU checks below.

New factory files harness/emu/test_bochs_legs.{c,sh} independently enumerate
the required gate names/images, inspect GNU make's parsed recipes and
require vector membership, dependencies, runtime images, completion/guard
checks and the truthful VGA13 geometry. Both new unit targets are in
TEST_UNIT_GATES. Mutation copies live in build; checkout sources are never
temporarily overwritten.

## Red, green and mutation witnesses

Before adding legs, make test-bochs-legs failed:

    RED bochs-legs MISSING: test-command-file-emu-bochs

Final new gate tails:

    GREEN test-bochs-legs: 68 named boot legs; vector, images, boundaries, honest VGA13
    GREEN test-bochs-legs-mutant: all five factory wiring mutants caught

Each named mutant changes actual factory make input, parses it with make,
and must exit 1 with its own reason. Infrastructure errors cannot count:

| Mutant | Exact witness |
| --- | --- |
| BOCHS_LEGS_MISSING | RED bochs-legs MISSING: test-command-file-emu-bochs |
| BOCHS_LEGS_VECTOR | RED bochs-legs VECTOR: test-command-file-emu-bochs |
| BOCHS_LEGS_IMAGE | RED bochs-legs IMAGE: test-command-file-emu-bochs |
| BOCHS_LEGS_BOUNDARY | RED bochs-legs BOUNDARY: test-block-device-emu-bochs |
| BOCHS_LEGS_DAC | RED bochs-legs DAC: test-flair-desktop-8bpp-bochs |

## Live Bochs result and PM handoff

Attempted make test-ata-no-drive-emu-bochs after the build correction.
The existing bochs_run host-wide flock was used; no concurrent Bochs run
was started by this lane. Result:

    bochs_run: RFB socket: Operation not permitted
    [bochs] launched=1 rfb_unblocked=0 timed_out=0 exit=1 signal=0
    [bochs] serial_len=0 marker_found=0 (expect=SHELL-READY)
    [bochs] triple_fault=0
    [bochs] OK=0
    !!! test-ata-no-drive-emu-bochs FAIL: RFB unblock failed -- Bochs did not run the guest

No guest serial or screendump exists to diff. This is an infrastructure
block, not a QEMU/Bochs guest disagreement. Following the explicit lane
fallback, no further live Bochs runs were attempted. Every one of the 68
new legs passed make -n individually; the PM must run them serially on a
host permitting the RFB socket. Logs: build/bochs2/<target>.dry-run.log,
test-ata-no-drive-emu-bochs-final.log and build/ata-no-drive-emu_bochs.*.
The shutdown clip and wording decision were left to the PM/operator.

## Law-1 grounding

- PRD Sec 8 / CLAUDE.md Rule 5: named cross-emulator legs; Rule 6: mutants.
- harness/bochs_legs.mk and harness/emu/bochs.{c,h}: existing boot-only
  contract, RFB limitation, host-wide lock, serial and fault checks.
- os/milton/kmain.c: BOOT_SHELL, BOOT_EXEC, BOOT_MZEXEC completion lines,
  VIDEO record and the FLAIR resolution guard. IO.SYS owns BIOS mode choice.
- docs/research/vesa8-ground-truth.md: forced-0101h seam and VGA13 floor;
  spec/hardware.json: unchanged platform contract.
- ADR-0003 Appendix A: preserved console/file/process ABI; DEC-08/08a:
  unchanged executable formats. No DOS command/message/column change.
- IBM DOS 3.30 Technical Reference, printed p.1-7 / PDF24: system-file
  boot order and COMMAND.COM; p.6-140 / PDF263: AH40 CX=0 truncation.
- MS-DOS 3.3 User's Guide, printed p.119 / PDF204: ordered hidden system
  files, permitted fragmentation, SYS excludes COMMAND.COM. All three
  scanned pages were rendered with pdftoppm at 110 DPI and viewed locally.
- Existing host/guest FAT oracles compare identical mformat seeds against
  mtools and require fsck.fat clean; boot-system additionally compares
  ordered entries, HSR and file bytes independently.

No new DOS behavior or new FAT implementation was introduced. The factory
inventory proves wiring, not those existing historical semantics.

## Added Bochs targets (all make -n GREEN; live runs pending)

- `test-command-file-emu-bochs`
- `test-command-file-negative-bochs`
- `test-flair-desktop-8bpp-bochs`
- `test-flair-notepad-bochs`
- `test-flair-scrapbook-bochs`
- `test-flair-notepad-itext-bochs`
- `test-flair-notepad-restart-bochs`
- `test-flair-notepad-corrupt-bochs`
- `test-flair-cpanels-bochs`
- `test-flair-cpanels-date-bochs`
- `test-flair-cpanels-patterns-bochs`
- `test-flair-cpanels-mouse-bochs`
- `test-flair-cpanels-sound-bochs`
- `test-flair-cpanels-memory-bochs`
- `test-flair-cpanels-settings-bochs`
- `test-flair-cpanels-persist-bochs`
- `test-flair-cpanels-corrupt-bochs`
- `test-flair-grow-box-bochs`
- `test-block-device-emu-bochs`
- `test-fdisk-emu-bochs`
- `test-formatsys-emu-bochs`
- `test-format-system-boot-bochs`
- `test-dos-folder-bochs`
- `test-conkeys-bochs`
- `test-j1lu-control-emu-bochs`
- `test-ansi-emu-bochs`
- `test-flair-finder-icons-bochs`
- `test-flair-trash-full-bochs`
- `test-flair-toolbox-hardening-bochs`
- `test-fat-hygiene-emu-bochs`
- `test-trash-hidden-emu-bochs`
- `test-ata-no-drive-emu-bochs`
- `test-baked-exec-guard-bochs`
- `test-mzload-entry-emu-bochs`
- `test-samir-core-emu-bochs`
- `test-samir-dml-emu-bochs`
- `test-samir-dml-clear-emu-bochs`
- `test-samir-dml-macro-emu-bochs`
- `test-samir-dml-close-emu-bochs`
- `test-samir-dml-index-emu-bochs`
- `test-samir-dml-reindex-emu-bochs`
- `test-samir-dml-sort-emu-bochs`
- `test-samir-dml-total-emu-bochs`
- `test-samir-dml-append-emu-bochs`
- `test-samir-dml-save-emu-bochs`
- `test-samir-dml-restore-emu-bochs`
- `test-samir-dml-copyfile-emu-bochs`
- `test-samir-dml-type-emu-bochs`
- `test-samir-dml-rename-emu-bochs`
- `test-samir-dml-erase-emu-bochs`
- `test-samir-dml-label-emu-bochs`
- `test-samir-dml-run-emu-bochs`
- `test-samir-print-emu-bochs`
- `test-shell-files-emu-bochs`
- `test-command-c-emu-bochs`
- `test-xcopy-cwd-emu-bochs`
- `test-fat-truncate-emu-bochs`
- `test-dir-match-emu-bochs`
- `test-shell-dir-look-bochs`
- `test-flair-iword-codes-bochs`
- `test-flair-iword-block-bochs`
- `test-flair-iword-format-bochs`
- `test-flair-iword-fkeys-bochs`
- `test-flair-iword-search-bochs`
- `test-flair-iword-replace-bochs`
- `test-flair-iword-undo-bochs`
- `test-flair-iword-setup-bochs`
- `test-kernel-size-r4-emu-bochs`

## Final focused acceptance and stop

No full make test was run. Every QEMU invocation exported
INITECH_QMP_STDIO=1; builds used at most two jobs. free -g showed 18 GiB
available and df -h / showed 115-116 GiB available before heavy work.
New C/shell/make source is ASCII; the shell script passes sh -n.

The following targets completed in this lane (logs under
build/bochs2; final.log suffix, except factory-regression.log for the
inherited completion parser family):

| Target | Exact terminal GREEN/PASS line |
| --- | --- |
| test-bochs-legs | GREEN test-bochs-legs: 68 named boot legs; vector, images, boundaries, honest VGA13 |
| test-bochs-legs-mutant | GREEN test-bochs-legs-mutant: all five factory wiring mutants caught |
| test-makefile-vars | >>> test-makefile-vars: green |
| test-fixture-volumes | >>> test-fixture-volumes: green (422 scratch names vs 152 fixture names; 70 inline image writers) |
| image | >>> image data: build/flair_data.img (FAT12 primary-slave volume; README.TXT + APPS + DOS + AUTOEXEC.BAT) |
| test-boot-system | GREEN test-boot-system: mdir order + mattrib HSR + mcopy/cmp both files + fsck.fat clean |
| test-fat-truncate-host | GREEN truncate rollback: FAT1/FAT2/directory write faults restore identical image and live FAT; short/cycle/free/bad chains refused |
| test-fat-truncate-emu | GREEN test-fat-truncate-emu: AH40 position/size, tail release, zero, boundary, subdir, regrow == mtools; fsck.fat |
| test-ansi-emu | >>> test-ansi-emu: green (15 AH=09h sequences, exact DSR replies via INT 21h stdin, SHELL-EXIT) |
| test-flair-app-launch | >>> test-flair-app-launch: green |
| test-bochs-completion | GREEN test-bochs-completion: exact complete lines, CRLF, second-boot wait |
| test-bochs-completion-mutant | GREEN test-bochs-completion-mutant: BOCHS_MUTATE_NO_REBOOT_WAIT caught: RED bochs-completion: one boot must not satisfy a two-boot wait |
| test-bochs-completion-substring-mutant | GREEN test-bochs-completion-substring-mutant: BOCHS_MUTATE_MARKER_SUBSTRING caught: RED bochs-completion: marker substrings must not count as boot lines |
| test-bochs-completion-partial-mutant | GREEN test-bochs-completion-partial-mutant: BOCHS_MUTATE_PARTIAL_LINE caught: RED bochs-completion: partial serial lines must not count as boot lines |

The host truncation differential also printed:

    GREEN test-fat-truncate-host: AH40 position/size, tail release, zero, boundary, subdir, regrow == mtools; fsck.fat

Required regression test-flair-locked-purge is RED:

    ppm_flair_trash_check: FAIL leg locked-full -- FULL Trash (TRASH_FULL): 617/1024 cells differ from the mint-008 capture
    FULL Trash (TRASH_FULL) cell (0,6) at (590,404): got #272727 want #3F3F3F (opaque)
    FAIL locked-trash: locked notice pixels missing

Evidence: build/bochs2/test-flair-locked-purge.final.log,
build/locked_trash_purge.ppm and build/locked_trash_purge.serial.
The alert bands matched 6496/6496; the failing sprite is a separate pixel
assertion. No icon/CLUT implementation or golden was changed by this lane.
Stopped at the explicit no-scope-growth instruction; did not weaken the
oracle or attempt an unrelated icon repair. The next queued
test-flair-finder-service was NOT run. No claim of a green full vector or
live Bochs acceptance is made.

Files changed: Makefile, harness/bochs_legs.mk,
harness/emu/test_bochs_legs.c, harness/emu/test_bochs_legs.sh, this worklog.
