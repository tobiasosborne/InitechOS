# DOS messages, extended BREAK checks, and QEMU output paths

One-shot lane initech-vlhp / initech-nstg. No commits, pushes, tracker
updates, or changes to CLAUDE.md/HANDOFF.md. `bd show` cannot open a database
in this clone; neither requested id occurs in .beads/issues.jsonl. The lane
brief supplied the implementation scope.

## Ground truth and scope

Read CLAUDE.md top to bottom, PRD 5/6.1/8, ADR-0003 Appendix A/C and
DEC-16 3.3/C-6. Rendered the following scanned manual pages with pdftoppm
at 110 dpi and inspected their images. All page numbers below are printed
Reference numbers inside MS-DOS_3.3_Users_Guide_198707.pdf.

| Item | Source and disposition |
| --- | --- |
| MSG-DOS-0006 | Appendix F p.350 / PDF435: unavailable device cannot transmit data. The existing ATA_ERR_NO_DRIVE now reaches the critical-error handler with its original error code; a read prints the approved catalogue wording with drive A before A/R/F. The locked catalogue omits the manual's additional word "error"; no spec change was made. |
| MSG-DOS-0008 | Appendix F p.365 / PDF450: writing a write-protected medium. Unchanged: the shipped ATA fixed-disk backend exposes no write-protect status, and its write callback is always present. A read-only file is Access denied, not this condition. The factory read-only blockdev has no shipped mount path; crit_blockdev_init preserves its NULL write callback for the existing absolute-disk contract. No invented classification of ATA_ERR_DEVICE as write protection. |
| MSG-DOS-0013 | Appendix F p.338 / PDF423 explains the overwritten transient interpreter reload; the exact insert-disk example is p.60 / PDF145. Unchanged: COMMAND is resident kernel code, never overwritten or reloaded. ADR-0003 5.3 expressly makes transient reload discretionary. |
| MSG-DOS-0014 | Appendix F p.310 / PDF395: COMMAND.COM missing, moved, or invalid. Left to the TPS exit-to-DOS lane as instructed; that lane's new path is absent from this clone. |
| MSG-DOS-0015 | Appendix F p.350 / PDF435: non-system boot disk. Already implemented by stage2.asm fail; no duplicate path. Existing negative boot gate and all three real loader mutants re-run GREEN. |
| BREAK ON | p.38 / PDF123: adds checks during disk reads/writes. Non-console INT 21h entry now polls without blocking and calls INT 23h for pending 03h only when ON; normal typeahead is preserved. Raw 06h/07h and existing console checks are unchanged. IBM DOS 3.30 Technical Reference p.6-107 / PDF230 independently confirms AH=33h state ABI (rendered and inspected). |
| README requirements | spec/hardware.json cpu and memory.flair_heap; ADR-0001 DEC-02. 486-class, 386 desirable/non-blocking, at least 4 MB extended above 1 MB. |
| QEMU path bounds | PRD 8, Rule 2, local sys/un.h sun_path[108]. Reject an oversized Unix QMP path with lengths and a remedy BEFORE unlink/fork; long paths fitting the normal output buffers work over stdio. Existing 1024-byte output bound remains loud. |

## Acceptance

Tests were authored before implementation. Initial host REDs are in
build/dosmsg-host-red.log; the disk-loop QEMU gate also failed before the
BREAK check existed. Message fixture injects removal at the actual blockdev
seam; it does not call a message-printing test stub. Fixture code is never
linked into a shipped image. The only .COM is an unshipped NASM disk-read
test, following m0dc's flat program/EXEC pattern.

All ten new targets are in TEST_UNIT_GATES / TEST_EMU_GATES through
harness/diff/fat_diff/dosmsg.mk:

- test-qemu-paths, test-qemu-paths-mutant
- test-dos-break, test-dos-break-mutant
- test-dos-break-emu, test-dos-break-emu-mutant
- test-dos-not-ready, test-dos-not-ready-mutant
- test-dos-not-ready-emu, test-dos-not-ready-emu-mutant

All GREEN. Final evidence: build/dosmsg-final-acceptance.log and the path-host
output at the beginning of build/dosmsg-catalogue-final.log. A boundary
assertion also went RED for legacy AH=00h being excluded from the non-console
set (build/dosmsg-legacy-red.log); the final entry check includes it.
Path testing traps fork,
so even its mutant never launches QEMU. Every emulator run used
INITECH_QMP_STDIO=1. The emulator gates assert decoded VGA-font console text,
successful completion, no triple fault, unchanged identical FAT images,
mcopy byte equality, and fsck.fat cleanliness.

Mutation witnesses:

- INT21_MUTATE_DISK_BREAK: `RED dos-break: ON must abort non-console call before service` and `RED dos-break-emu: on did not return after disk Ctrl-C`.
- INT21_MUTATE_NOT_READY: `RED dos-not-ready: missing exact MSG-DOS-0006 before A/R/F` and `RED dos-not-ready-emu: MSG-DOS-0006 missing`.
- QEMU_MUTATE_PATH_PREFLIGHT: `RED qemu-paths: QMP socket path too long must fail loudly BEFORE launch`.
- Existing BOOT_MUT_ORDER / BOOT_MUT_ATTR_R / BOOT_MUT_CHAIN: `RED boot-system loader-<name> missing Non-System message`.

Neighbor checks GREEN: test-4tw (83), test-er3h/test-int21 (175),
test-int24 (44), test-int24-wired (19) and mutants, test-fileio (225),
test-int21-irqstorm, test-boot-system-negative and
test-boot-system-emu-mutant. See build/dosmsg-regression-host.log,
build/dosmsg-boot-regressions.log and build/dosmsg-irqstorm.log.
make image and test-flair-app-launch / test-flair-locked-purge /
test-flair-finder-service passed again on the final kernel; see
build/dosmsg-final-acceptance.log. No utilities were added to the shipped
data volume. No full make test or Bochs run. Modified/new code is ASCII.

## Outstanding certification failure outside the lane

test-dosmsg passes catalogue/header consistency and image presence, including
MSG-DOS-0006, but its existing source-purity tooth fails on untouched files:

- os/milton/chkdsk.c: ` bytes free`
- os/milton/xcopy_tree.c: `Access denied`, `Unable to create directory`,
  `File cannot be copied onto itself`

No oracle weakening or unrelated edits. The combined make command stopped
there, so the existing test-dosmsg-mutant was not run. All new mutant gates
did run. PM owns tracker changes and integration.
