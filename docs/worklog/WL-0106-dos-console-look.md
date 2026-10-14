# WL-0106 - DOS console LOOK pass 6

## Context

Lane `lane-doslook`, based on `BASE_COMMIT.txt` (`6e804e6`). Implements
initech-rtdw (M015), initech-3sxp (M016), initech-vjtp (M017),
initech-d31e (M018), initech-b0qk (M019), initech-irp2 (M020), and
initech-veub (M023). The measured evidence is REPORT.md / TRIAGE.md batch 4
in docs/audits/2026-10-15-look-pass6. The operator explicitly disallowed bd,
push, Bochs, aggregate test targets, and changes to sibling projects.

## What changed

The blocking keyboard input wait draws a temporary two-scanline underline
at the console cursor. Its phase is a function of the 100-Hz PIT tick count:
16 virtual 60-Hz frames on, 16 off (1.875 full cycles/second). It restores
the exact underlying pixels before echo/output or returning to FLAIR.
No interrupt handler draws, and no new wall-clock or random state is used.
CON form feed now clears and homes to zero-based (0,0); CLS no longer
scrolls 25 newlines and leaves the next prompt on row 24.

DIR reads the root volume-label entry, with an extended-BPB fallback and
an explicit no-label case. It displays the current CWD path, split 8.3
columns, size or <DIR>, entry modification date/time in 12-hour a/p form,
and the existing AH=36h free-cluster count times cluster size. Its existing
operand/wildcard behavior is retained; K10/oqiu is outside this lane.

Rule 8 deliberate change: add MSG-DOS-0021..0026 to
spec/dos_messages.json, each with a citation, and amend ADR-0003 Appendix C.
They are respectively ` Volume in drive A is `,
` Volume in drive A has no label`, ` Directory of `, ` File(s) `,
` bytes free`, and `<DIR>`. The build generator and catalogue gates now
cover 26 entries. Existing message and banner bytes are unchanged.
command.c uses generated catalogue references for these texts.

Seven host gates and two emulator gates are added to TEST_UNIT_GATES /
TEST_EMU_GATES through harness/diff/fat_diff/doslook.mk. The formatter golden
is a committed manual transcription; the disk/pixel oracle uses independent
mdir output. ppm_text_check gains explicit ink/blank cell manifests and a
cursor-cycle grade: exactly 16 foreground pixels in bottom scanlines 14..15
when on, zero when off, and identical pixels outside that cell.

## Why

Local Law-1 sources, converted with pdftotext -layout into build/:

- MS-DOS_3.3_Users_Guide_198707.pdf, printed p.5 / PDF 18: prompt and
  flashing cursor; printed pp.28-29 / PDF 41-42: DIR illustration;
  printed p.45 / PDF 130: CLS leaves only prompt and cursor.
- The same manual, User Reference printed p.12 / PDF 97: subdirectory
  illustration uses <Dir>. Printed p.362 / PDF 447: DIR/Label/VOL message
  `Volume in drive x: has no label`. Uppercase <DIR> and the no-colon drive A
  specialization are explicitly specified by this lane's operator.
- 80X0945_DOS_3.30_Technical_Reference_Apr87.pdf, printed p.3-6 / PDF 75:
  CUP home defaults to row/column 1. Printed p.3-13 / PDF 82: erase-display
  clears and goes home. This supplies the homing source missing from the
  audit's FROM MEMORY claim; the User Guide alone gives no row number.
- Technical Reference printed p.5-5 / PDF 112: FAT free entries; p.5-11 /
  PDF 118: 11-byte root volume label; pp.5-12..13 / PDF 119..120: packed
  modification time/date. Its AH=2Ah/2Ch descriptions are printed pp.6-98 /
  6-100, PDF 221/223; DIR uses the stored entry words, not these clock calls.
- ZR23-6845-01_PC_DOS_Operation_Training_Version_3.3_and_4.0_PC_and_PS2_198903.pdf
  was converted to build/dos33-training.txt as the supplementary reference.

The illustration supplies the adopted zero-based row columns: name 0,
extension 9, size ending at 20, date field 23..30, time field 33..38.
Footer count ends at 8, File(s) begins at 10, free-byte number ends at 26.
The golden reproduces:

```
COMMAND  COM    23612   8-21-87  12:00p
ANSI     SYS     1651   8-21-87  12:00p
       37 File(s)     17408 bytes free
```

The ready-cursor shape is sourced by the User Guide; the numeric rate is
operator-specified and documented in the FreeVGA Cursor Blink Rate excerpt
transcribed into boot-to-text-ground-truth.md Sec 6. No numeric VGA cursor
rate was found in the local DOS manuals. Direct download was unavailable
(DNS); the excerpt was acquired through the indexed technical document.

## Frictions

Initial host evidence was RED: `COMMAND.COM        23612` differed from the
manual's split columns/date/time. CLS read back `(24,18)` after form feed.
Initial live volume gate failed twice in isolation; cursor samples were
`on=0 off=3`. The existing FAT free-count backend was already correct:
1383424 bytes matched mdir, so the shell reuses it.

Closer PDF-page inspection corrected the independently transcribed footer
field to end at column 26. The golden and formatter were corrected from
that measurement, then rechecked with all mutants. The illustration is a
layout source, not a pixel-exact font/photo/color oracle.

One long regression command ended with signal 143 during test-zs24. That
family was rerun separately and passed. Editing the gate script while one
run was reading it produced `e: not found` after successful pixel checks;
the isolated retry with the fixed script passed both disk cases. No oracle
was weakened. Live runs now finish on the fourth A:\> prompt after recording
all input/hold events, and require keys_sent=12, mouse_events_sent=16,
frames_taken=30, and quit_after_found=1. Every emulator invocation used
INITECH_QMP_STDIO=1. All requested corpus roots were passed as absolute paths.

## Acceptance

Final host gates and evidence:

- `make test-console-look`: `CONSOLE LOOK PASS: CLS home; PIT phases
  0/27/54/80; exact pixel restore at 8/24/32 bpp`.
- `make test-dir-look`: `DIR LOOK PASS: manual rows, columns, packed
  dates/times and File(s)`.
- `make test-dir-look-mutant`: date, joined, lower each produced
  `DIR LOOK MUTANT PASS: <tag> RED against independent manual golden`.
- `make test-dir-free-space`: `DIR SPACE PASS: FAT=1383424 == mdir=1383424
  bytes free (2702 clusters x 512 bytes)`.
- `make test-dir-free-space-mutant`: `DIR SPACE RED: FAT=0 mdir=1383424`,
  then `DIR SPACE MUTANT PASS: zero-free-count RED against mdir`.
- `make test-dir-volume`: `DIR LABEL PASS` for `[ROOT LABEL]`, `[BOOT LABEL]`,
  and `[]`; root/BPB selection and fail-loud read faults passed.
- `make test-dir-volume-mutant`: `DIR LABEL RED: wanted [ROOT LABEL] got
  [BOOT LABEL]`, then `DIR LABEL MUTANT PASS: BPB-only label RED on
  conflicting root label`.

Final emulator gates:

- `INITECH_QMP_STDIO=1 make test-shell-dir-look`: labelled and unlabelled
  disks both passed. Independent mdir manifests graded 322 / 324 cells;
  CLS graded 1999 ink/blank cells. Cursor samples had on=9/off=7 and
  on=10/off=6, respectively, exactly 16 bottom pixels, with other pixels
  identical. Final line: `VERDICT: PASS -- test-shell-dir-look (volume,
  manual columns, mdir footer, CLS home, cursor on/off)`.
- `INITECH_QMP_STDIO=1 make test-shell-dir-look-mutant`: volume, date,
  joined, lower, home, cursor each booted and failed the named LOOK assertion
  on two sequential isolated runs. Each final evidence line is
  `SHELL DIR LOOK MUTANT PASS: <tag> attempt 2 booted, named LOOK assertion RED`.

Requested targeted regressions all passed: test-spec, test-command and
test-command-mutant (the existing DIR formatter gate), test-shell,
test-dir, test-zs24 and test-zs24-mutant, test-zs24-exec and its mutant,
test-ut6d and its mutant, test-80k and its mutant, test-fs,
test-kernel-repro, test-kernel-headroom, test-makefile-vars,
test-flair-desktop, and test-flair-samir-suspend. The latter's centerpiece
was `pre.ppm == post.ppm (desktop rebuilt BIT-IDENTICAL)`.
Kernel repro SHA256: a18555c92288379ed2439cbaf6e6d317b202404ee6ae73efc63ab1268c7bc61c.

Headroom gate: 149 kernels, 196608-byte boot window (384 sectors), 4096-byte
floor. Baseline flagship 172200 bytes -> 172872 (+672), headroom 23736.
Binding largest variant 173288 -> 173992 (+704), headroom 22616 (baseline
23320). Shell loaded bytes 93772 -> 95788 (+2016), all growth below 3072.
Shell placement: start 0x500000, _kernel_end 0x51c480 < KERNEL_CEIL 0x600000.
Changed source is ASCII clean; git diff --check passes.

Bochs targets: not run, explicitly prohibited. No aggregate test targets
were run. bd statuses were not changed (tool unavailable); the seven fixes
are verified for PM disposition. No push was attempted; delivery is the
verified build/lane.bundle from copied build/lane.git metadata.

## Pointers

- Audit: docs/audits/2026-10-15-look-pass6/REPORT.md, TRIAGE.md batch 4.
- Contract: ADR-0003 Appendix C/DEC-13; spec/dos_messages.json;
  spec/dos_banner.txt (unchanged).
- Artifact: os/milton/console.c, console.h, kmain.c, command.c, command.h,
  fileio_fat.c, fileio_fat.h; existing fat12.c and PIT source.
- Oracles: harness/diff/fat_diff/dos33_dir.golden, test_dir_look.c,
  test_dir_disk.c, test_console_look.c, shell_dir_look.sh, doslook.mk;
  tools/ppm_text_check.c. Existing WL-0012 and WL-0028 provide provenance.
- Evidence: build/doslook-final-*.log, doslook-regression-*.log,
  doslook-regression-host-family.log, doslook-baseline-headroom.log;
  shell_dir_look / shell_dir_unlabelled PPM frames and cell manifests.
