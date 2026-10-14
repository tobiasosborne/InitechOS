# WL-0109 -- SAMIR screen LOOK pass 6

## Context

Database-screen lane at BASE_COMMIT 882ac7d98ba8f081486a510d32c8d85e8d3674f5:
initech-1y0h (M027), initech-r3pa (M028), initech-vz6z (B68/M021), and
initech-3cz2 (M022). The operator supplied the bead texts; bd is unavailable.
No push was requested or performed. Commits use the copied metadata in
build/lane.git on lane-dbscreen. Other project directories were read only.

Read CLAUDE.md, PRD 6.6, the pass-6 report/triage batch 5, pass-5 L064/L065,
ADR-0008/0009 and amendment, the SAMIR plan/research, and WL-0032/0033/0097/0101.
The requested spec/xbase_coercion.json does not exist at this base; the actual
locked contract is spec/samir/xbase_coercion.json. It is unchanged.

## What changed

query.c has one descriptor-aware field display path for default and explicit
field LIST/DISPLAY, DISPLAY ALL, and current-record DISPLAY. N(w,d) cells use
exactly d places and right alignment via the existing freestanding decimal
formatter's left-space padding. SET DECIMALS continues to govern expressions
rather than stored field precision. Whole cells stream across the 512-byte
staging boundary; a wide row no longer loses its later columns.

HEADING defaults ON. Column width is the greater of heading length and field
width; OFF removes titles and their width expansion. Computed headings are
resolved after scope positioning, so a prior LIST's EOF cannot cause a false
zero-division error while preparing the next LIST's heading. Descriptor
headings do not evaluate field values.

CONSOLE defaults ON. Screen output and program input prompts/echo can be
suppressed inside programs; dot commands remain visible. Nested DO restores
the caller's dot context. Error diagnostics remain visible. Hidden input
uses an optional PAL no-echo cooked-line callback; the Milton binding retains
the redirected byte framer and uses raw CON input without echo when needed.
File writes and the existing storage/clock contracts are unchanged.

STATUS defaults OFF because SAMIR does not load CONFIG.DB. A PAL callback
receives the selected table filename, current record and count. The text model
reserves rows 22/23, protects them during command-output scrolling, refreshes
after commands, and erases them on OFF/exit. The Milton binding uses the
installed ANSI.SYS transport, never writes terminal controls into redirected
files, and does not call or edit kernel console internals.

The manual cell projection is: Command Line at column 0, (drive:>) at 17,
database at 23, Rec: at 46, position/count at 51, and guidance at column 24 on
row 23. Separators are columns 16/22/45/65/72. Reverse and normal PAL attributes
are 0x70 and 0x07. The scan is a layout source, not a framebuffer RGB golden.
Activation begins a fresh 22-row command page. That page-reset transition is
an authored implementation choice, with no historical transition golden.

New host gates are in TEST_UNIT_GATES (+7); emulator gates are in
TEST_EMU_GATES (+8). Existing test-interp-list and test-interp-set were extended.
The authored query.out program fixture now describes headings and field widths;
no real-dBASE output file was rewritten.

## Why / Law-1 sources

Local manual root: initech-manuals-ref/dbase3plus/dBASE III Plus 1.1 (1986)
Manuals/. pdftotext -layout extractions and rendered relevant pages are in build/.

- Using dBase III Plus.pdf, printed U5-144, PDF 256: LIST OFF example, UNITCOST
  814.00 and aligned right edges; TOURS UNITCOST is N(10,2).
- Using dBase III Plus.pdf, printed U5-210, PDF 322: CONSOLE ON/OFF, default ON,
  program-only suppression, invisible input prompts/typing, visible errors,
  and the SAY/GET exception. Printer routing is outside this screen-output seam.
- Using dBase III Plus.pdf, printed U5-235, PDF 347: HEADING normally ON, LIST and
  DISPLAY titles, column width equal to heading or field width, whichever is larger.
- Using dBase III Plus.pdf, printed U5-259, PDF 371, Figure 5-17: status fields,
  guidance, updates, and default OFF without CONFIG.DB (installed single-user
  CONFIG.DB normally turns it ON).
- Using dBase III Plus.pdf, printed U5-16, PDF 128: row 22 reservation; rows 0..23
  and columns 0..79 are zero-based.
- dBase III Quick Reference.pdf: printed leaves are UNNUMBERED; PDF 13 CONSOLE,
  PDF 15 HEADING, PDF 16 STATUS. The commit's pp 13/15 references mean PDF pages,
  not invented printed folios.
- dbase3-decomp INDEX.md, re/harness-setup.md, re/mint-formats.md and
  re/mint-results-001/002/003.md; specs/commands/navigation-query-display.md,
  set-commands.md and runtime/numeric-and-string-formatting.md. Mint-001 supplies
  the formatter's ties-toward-positive-infinity rule; there is no libc formatting
  in the artifact.

Searched the corpus goldens, mint/work PRG/TXT files, INDEX and mint instructions
for LIST/DISPLAY/HEADING/STATUS/CONSOLE. LIST entries in mint recipes are proposed
commands, not captured output goldens. No captured record LIST/DISPLAY or status
screen output was found. ALL new layout/cell expectations are therefore
**manual-derived, no golden**. The Python differential writes raw DBFs with
independent struct/Decimal code; it never uses SAMIR's writer or formatter for
expected output. Numeric mutation proof is on the implementation.

Real corpus files ARE graded by the existing independent-reader and round-trip
gates. The new status integration also opens mint/work/TOURS.DBF and CLIENTS.DBF
read-only and checks the independently documented 30/49 record counts and live
work-area selection. This verifies input/metadata, not historical output layout.
The program differential remains its existing authored Tier-0 stdout and
independent Tier-1 DBF check; real DBASE.EXE Tier 2 is not run.

## Frictions

- The added screen code initially failed the existing COM heap-reservation link
  assertion. A clean original-base build links at 87,488 bytes. The old link
  profile with the new code exceeds the limit. Function/data sections plus
  --gc-sections remove unreachable code; the assertions and 36 KiB PAL request
  stay intact. Final COM is 87,776 bytes (+288 over base), BSS 50,624 bytes.
  Load end 0x557e0 plus reserved BSS 0x10000 and PAL/MCB 0x9020 is 0x6e800,
  below the 0x6f000 ceiling by 2,048 bytes. It is a flat COM, not the kernel.
- The numeric oracle originally shared a fixture directory between normal and
  mutant runs. It now isolates them by executable name, safe under -j2.
- Status pixels use the existing ANSI reverse attribute. ppm_text_check gained
  an additive reverse-paper band option; its original banner/cell/background
  checks and all previous invocations remain intact. First failed runs exposed
  an incorrect key-count assertion and an overbroad band-color edit; corrected
  and rerun alone. Final production PPM/serial captures are preserved separately
  from mutant scratch captures.
- test-makefile-vars failed at the BASE_COMMIT too: the i123_menu_stale invocation
  omitted flair-data-rules' fourth WORD.EXE argument. Supplied the existing
  IWORD_EXE argument, matching adjacent calls; the gate is green.
- SET STATUS ON starts a fresh command page, rather than attempting to recover
  the kernel's existing cursor/history through an unavailable PAL query.
  Historical transition fidelity is not asserted.

## Acceptance

All make invocations supplied the five absolute corpus roots from the operator.
At most -j2; emulator gates ran one at a time with INITECH_QMP_STDIO=1. Deliberate
RED emulator mutants were rerun alone. No SKIP_* options, whole test/test-unit/
test-emu vector, or Bochs target was run. Bochs is **not run**, per constraint.

New gates and their final evidence lines:

| Make target | Evidence |
| --- | --- |
| test-samir-numeric-format | test-samir-numeric-format: 8957 cells PASS (manual-derived, no LIST golden) |
| test-samir-numeric-format-fraction-mutant | test-samir-numeric-format-fraction-mutant: PASS (implementation mutant RED: descriptor differential) |
| test-samir-numeric-format-left-mutant | test-samir-numeric-format-left-mutant: PASS (implementation mutant RED: descriptor differential) |
| test-interp-list-heading-mutant | test-interp-list-heading-mutant: PASS (implementation mutant RED: screen controls) |
| test-interp-set-console-mutant | test-interp-set-console-mutant: PASS (implementation mutant RED: screen controls) |
| test-samir-status | test-samir-status: 1888 checks, 0 failures |
| test-samir-status-mutant | test-samir-status-mutant: PASS (status-band implementation mutant RED) |
| test-samir-status-emu | test-samir-status-emu: 18 checks, 0 failures |
| test-samir-status-off-emu | test-samir-status-off-emu: 17 checks, 0 failures |
| test-samir-status-emu-mutant | test-samir-status-emu-mutant: PASS (same status-band mutant RED on serial and video) |
| test-samir-numeric-format-emu | test-samir-numeric-format-emu: 15 checks, 0 failures |
| test-samir-numeric-format-emu-fraction-mutant | test-samir-numeric-format-emu-fraction-mutant: PASS (same numeric implementation mutant RED in guest) |
| test-samir-numeric-format-emu-left-mutant | test-samir-numeric-format-emu-left-mutant: PASS (same numeric implementation mutant RED in guest) |
| test-samir-console-emu | test-samir-console-emu: 18 checks, 0 failures |
| test-samir-console-emu-mutant | test-samir-console-emu-mutant: PASS (same CONSOLE suppression mutant RED in guest) |

Status video: band[352,368) has 9,272 reverse-paper pixels (>=7,000),
band[368,384) has 821 guidance pixels (>=300). Both ppm_text_check legs PASS;
serial contains Command Line, CLIENTS, Rec: 2/3 and the guidance text. OFF clears
both rows. Console guest accepts hidden input, prints it once after ON, and
reports #9 Data type mismatch while OFF. Mutant runs remain live, with all keys
injected and no guest panic/triple fault; crashes cannot qualify as mutant RED.

Targeted regressions, all green:

- test-interp-list: 108 checks, 0 failures; test_interp_set: 65 checks, 0 failures.
- test-interp-list-mutant: CONTINUE re-scope correctly RED;
  test-interp-set-mutant: EXACT default-ON correctly RED.
- test-samir: SAMIR foundation green, including dbf_ref 147 and ndx_ref 116 checks.
- test-dbase-diff: xbase-prog-diff: 8 checks, 0 failures (authored Tier 0 + DBF Tier 1).
- test-dbase-roundtrip: 69 checks, 0 failures (meaningful bytes).
- test-xbase-coercion: 8037 checks, 0 failures.
- test-samir-boot: [5/5] screendump shows the dot prompt and LIST table; PASS.
- test-samir-write: [5/5] PERSISTED, 4 records, BAL=9999.99 and 5555.55 on disk.
- test-samir-canon-y2k: [4/4] screendump shows the aging report; enforced wrong ages PASS.
- test-flair-samir-suspend: [4/4] pre.ppm == post.ppm, desktop BIT-IDENTICAL; PASS.
- test-kernel-repro: green, sha256 acaca40fdaed0cada0b80212de5fe1d1f9804617ff9b956362d619b04c31daea.
- test-spec: PASS; test-makefile-vars: green; ASCII source check: PASS.

COM sha256: 2ec51f684490b452e836e305eccc79a9d205e6b0814bbd75ca2697ce7d859f8f.
All changes are committed in the lane. The deliverable is build/lane.bundle,
created with BASE_COMMIT..lane-dbscreen and verified using git bundle verify.
There are no unfinished code items in these four supplied beads. Captured real
LIST/STATUS authenticity and Bochs are not certified by this lane.

## Pointers

- os/samir/cmd/query.c, set.c, proc.c, workarea.c; samir_main.c.
- os/samir/core/screen.c, include/samir/screen.h, pal/pal_milton.c.
- harness/diff/dbf_diff/numeric_format_ref.py, numeric_format_driver.c,
  test_samir_status.c, test_samir_screen_emu.c, test_interp_list.c, test_interp_set.c.
- tools/ppm_text_check.c -- additive reverse-band option.
- build/numeric-certificate.log, build/certificate-host.log, build/kernel-repro.log,
  build/final-test-*.log and build/lane.bundle.verify.
- build/samir-screen/accepted-{status,off,numeric,console}.{ppm,serial};
  accepted-status.png is the viewed production screenshot.
- No RECORD_SCRIPTS entry or clip applies: SAMIR is a full-screen DOS text app,
  not a FLAIR window; its console gates use qemu_harness keyboard/PPM machinery.
  test-flair-samir-suspend verifies the separate desktop suspend/resume contract.
