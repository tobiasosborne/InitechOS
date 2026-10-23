# InitechDOS source triage - 2026-10-23

## Wrong or overstated findings

- L029 is WRONG as a defect: recovering a shell after arbitrary native #UD is
  not the contract. CLAUDE.md Rule 2 requires fail-fast contextual panic, and
  its deliberate-canon list excludes protected process isolation. The inspected
  shots/prog-garbage-result.png visibly says PC LOAD LETTER and ERROR 06 -- SEE
  OPERATOR; evidence/prog-garbage-serial.log:67-68 records vec=06 and HALTED.
  os/milton/panic.c:173 calls panic_draw; panic_screen.h:50-54 paints the reason.
  Verdict BY-DESIGN; no recovery or isolation bead.
- L030 is partly WRONG: the demand to return to a usable shell contradicts
  CLAUDE.md's deliberate-canon foreign-MZ panic and ADR-0003 DEC-08a.5. The D
  retained here is only the absent visible explanation. The inspected screenshot
  shots/prog-mz16-result.png has just the command; evidence/prog-mz16-serial.log
  has the unsupported-16-bit reason and HALTED. loader.c:1138-1152 never paints.
- L040 needs a narrower remedy than making both applications run in DOS.
  ADR-0013 AC-3 DEC-AC3-1 distinguishes relocatable FLAIR tenants from DOS EXEC;
  Makefile:10371-10396 packages 123/WORD as Toolbox-gate applications. The
  audit's preserved headers in evidence/recovery-extra-verification.json show
  123 e_minalloc=0x48d0 paragraphs (298240 bytes), WORD=0x77fd (491472 bytes).
  Using their header-declared load modules and spec/memory_map.h's DOS arena,
  loader.c:176-182 leaves only 65168/79792 bytes after the existing 64 KiB reserve.
  Refusal is correct; D retains the inaccurate Bad command diagnosis and
  loader memory/format-status conflation. This satisfies the report's explicit
  alternative of an accurate refusal; it does not refute that diagnostic finding.
  Do not force these apps into DOS EXEC.
- L017's assertion that malformed FIND must return 2 is not proven by its
  cited DOS pp.72-73; the report explicitly relies on memory. The source does
  return 1 on parameter error and always 0 on a completed search. Keep the
  result-status defect, but capture a real DOS 3.3 golden before locking the
  exact error code. No modern Windows command documentation is ground truth.
- L046 is not a violation of the shipped manual. ch2-entries-a.tex:67-68
  explicitly permits unknown operands to leave BREAK unchanged, matching
  command.c:2995. Treat requested strict rejection as a small compatibility
  feature/investigation, with reference capture first, not a source-confirmed
  contradiction of that manual. A reference confirming leniency closes B19.

All shot/evidence paths in this section are relative to
docs/audits/2026-10-23-dos/. Source paths are repository-relative.

## Basis and filing boundary

Audited report: docs/audits/2026-10-23-dos/REPORT.md and NITS.tsv, L001-L047.
Audited build: e67b8311092873fe16e6c6d62ad82f894bfb8d7d, as recorded by the report.
Source citations pinned to 3e44338f53906c23e54fc747a3d689a5d7e0ee2a.
During triage HEAD advanced to c995e23f7d366e5999abbc155cad41dcce6835bf by
adding the separate i123 audit. A read-only git diff confirmed no changes to
the DOS audit, os/, spec/, pubs/, Makefile, CLAUDE.md, PRD, ADRs or harness
between those commits, so all implementation citations remain at the pin.
Read CLAUDE.md, the 2026-10-05 full-drive TRIAGE format, this report, PRD 6.1,
the relevant ADRs/specs, local manual extracts, evidence and implementation.
This is read-only source triage, not a new emulator audit. No source edits,
builds, tracker mutations, commits or pushes were made. Acceptance oracles and
mutants below are required future work, not claims that they have run green.
The only outputs are this file and BEADS.tsv in the scratch workspace.

BRIEF.md's explicit read-only/output-only scope takes precedence over the
repository's generic bd/create/push session-close instructions. The supplied
OPEN-BEADS.txt and LANES-IN-FLIGHT.txt are authoritative snapshots; no live
tracker status was inferred. A broad open epic is not a duplicate of every
new narrow bug. VERIFY is a direct DUP of initech-f9z4. The fourteen dispatched
DOS IDs are all LANE and excluded from BEADS.tsv regardless of underlying D/F.
The pre-existing initech-ioke and initech-7z2z reds own no new DOS L-id here.
Old audit IDs under Still present / Now fixed retain their original namespaces;
they are not refiled as additional findings.

D = source-confirmed defect in implemented behavior. F = absent behavior at
the reported entry, even when helpers or narrower forms exist. DOC = incorrect
shipped documentation. BY-DESIGN follows deliberate canon/panic policy.
DUP and LANE are tracking verdicts; their notes retain the underlying finding.
Memory-only period claims are marked explicitly and require an independent
DOS 3.3 capture before implementation. Existing comments or tests that merely
assert DOS behavior are not independent references.

The new batches are ordered by harm: invalid output/data exposure first,
blocked or incorrectly controlled workflows, missing compatibility features,
then pager polish. P1 is not inherited from the auditor when refusal itself is
correct. Lane-owned data-loss defects L042/L032/L001 stay with their dispatched
owners; their absence from the new queue is not a reduction in urgency.

## All 47 findings

| id | verdict | owner | note |
| --- | --- | --- | --- |
| L001 | LANE | initech-66oc | in lane initech-66oc; D: command.c:4098-4099 CREATs a predictable PIPE1.$$$ and :4175 unlinks it, without exclusive allocation. |
| L002 | LANE | initech-66oc | in lane initech-66oc; D: command.c:1792 ignores AH=40h EAX/CF; redirection teardown at :3912 reports only dispatch control flow. |
| L003 | LANE | initech-66oc | in lane initech-66oc; D: command.c:4146 proceeds after producer write failure; :4164 tests only EXIT/break, not producer I/O completion. |
| L004 | LANE | initech-66oc | in lane initech-66oc; D: COPY failed-write cleanup at command.c:3113 substitutes MSG-DOS-0002 for a disk-full cause. |
| L005 | D | B03 | command.c:4375 truncates at BATCH_FILE_MAX=4096 without a limit/error result. |
| L006 | F | B12 | No argument reservoir beyond the nine supplied arguments: command.c:4650/:4670 and view[10] at :4444. The manual states the current limit. |
| L007 | DOC | B12 | SHIFT correctly shifts %0 at command.c:4575; shipped ch4-batch.tex:247 is wrong against original DOS p.151. |
| L008 | D | B04 | command.c:1319/:1336 produces only a .COM candidate for a qualified extensionless name. |
| L009 | F | B11 | Interactive secondary processor absent: command.c:4705 requires /C even though the shipped COMMAND entry promises typing in a second copy. |
| L010 | D | B05 | command.c:4727 restores the parent g_errorlevel over the just-latched child status. |
| L011 | LANE | initech-q45q | in lane initech-q45q; D: command.c:2873 sends C: to CHDIR and :2875 stays silent; no drive-only GETCWD query branch. |
| L012 | LANE | initech-q45q | in lane initech-q45q; D: dosutil.c:36 rejects a qualified drive unequal to du_drive despite readable mounted volumes. |
| L013 | LANE | initech-q45q | in lane initech-q45q; D: chkdsk_program.c:12 uses INT25, but int21.c:5952 still rejects every explicit drive other than A. |
| L014 | LANE | initech-q45q | in lane initech-q45q; D: xcopy_tree.c:95 strips the drive prefix; :414/:415 walk and label the current volume. |
| L015 | LANE | initech-q45q | in lane initech-q45q; F compatibility beyond the current-drive-only manual; dosutil.c:290 rejects another drive. |
| L016 | F | B06 | find_program.asm:62 skips filenames and :134 unconditionally reads stdin; named-file mode never built. |
| L017 | D | B06 | find_program.asm:254 always exits 0 after searching. Exact parse-error status 2 is unverified auditor memory; current :263 returns 1. |
| L018 | D | B06 | find_program.asm:109/:123 splits at every quote and overwrites nlen, including an empty last segment; not merely a first-quote truncation. |
| L019 | D | B06 | find_program.asm:93 consumes unknown option letters without error. |
| L020 | F | B13 | sort_program.asm:64 recognizes only /R; :272 always compares from byte zero. /+n was never implemented. |
| L021 | F | B16 | No fractional parser/transport: command.c:972 rejects .cc and :2210 passes DL=0; int21.c:4924 documents one-second RTC resolution. |
| L022 | DUP | initech-f9z4 | F underneath tracking verdict: missing VERIFY is explicitly named in this open bead; command.c:61-138 has no VERIFY classifier/handler. No new item. |
| L023 | F | B17 | ECHO. has no syntax entry; command.c:184 tokenizes it as a separate external command. |
| L024 | F | B17 | No prompt-level echo state: command.c:4541 updates only a batch frame; :2968/:2971 report on or print the operand. |
| L025 | F | B10 | command.c:4887 dispatches the prompt line without batch_expand; defined prompt environment substitution never built. |
| L026 | D | B07 | int21.c:3850 masks only VOLLABEL/DIRECTORY, so hidden and system entries leak through ordinary DIR searches. |
| L027 | LANE | initech-66oc | in lane initech-66oc; D: command.c:3863 reports MSG-DOS-0002 after denied destination open/create. Protected bytes survive. |
| L028 | LANE | initech-q45q | in lane initech-q45q; D: fdisk.c:125 hardcodes C: for every DOS partition instead of using DEC-18 identity. |
| L029 | BY-DESIGN | CLAUDE.md Rule 2 | WRONG recovery demand: an invalid-opcode program reaches contextual ERROR 06; panic.c:173 and panic_screen.h:50-54 implement the authorized fail-loud halt. |
| L030 | D | B09 | Missing visible reason only: loader.c:1141-1152 emits serial context then halts without painting. Terminal refusal itself is BY-DESIGN. |
| L031 | D | B01 | int21.c:5392 returns from an interrupted read with request EAX intact; command.c:2797/:2809 neither check the break latch nor bound the read result. |
| L032 | LANE | initech-q45q | in lane initech-q45q; D/data loss: xcopy_tree.c:95 discards both source and target drive prefixes before copy_dir. |
| L033 | D | B02 | command.c:3595/:4695 nests plain BAT invocation just like CALL; :4636 restores the caller rather than transferring control. |
| L034 | F | B14 | The Y/N decision was never built: command.c:4618 exits immediately on the latch; shipped manual explicitly describes that narrower behavior. |
| L035 | F | B15 | Directory-as-files expansion absent at command.c:3345; direct UNLINK correctly refuses a directory. DOS p.56 specifies the shell feature. |
| L036 | F | B16 | builtin_date at command.c:3469 has no no-argument entry loop; only startup_datetime at :3654 does. |
| L037 | F | B16 | builtin_time at command.c:3499 has no no-argument entry loop; only startup_datetime at :3664 does. |
| L038 | D | B08 | int21.c:1536 aliases persistent template and live input; :1544 preserves the old count but not the overwritten text on Ctrl-C. |
| L039 | D | B20 | more_program.asm:125-127 pauses without any indicator write; redirected passthrough silence is correct. |
| L040 | D | B04 | Narrow to incorrect error translation: audited e_minalloc exceeds the DOS arena; these FLAIR tenants are not promised DOS executables. command.c:3616 hides the cause. |
| L041 | LANE | initech-f42q | in lane initech-f42q; D: diskutil.c:165 requires three C: source files and uses a memory message for any source failure; source() at :61 requires COMMAND.COM absent from the fixture. |
| L042 | LANE | initech-f42q | in lane initech-f42q; D/data corruption: diskutil_program.c:14 writes raw INT13 sectors without the INT25/26 cache-update hook (kmain.c:590); fileio_fat.c:1045 retains the pre-write FAT. |
| L043 | D | B01 | command.c:2741 ignores the interruption after AH=08h, then :2766 continues enumeration; shared returning recovery mechanism with L031. |
| L044 | D | B13 | sort_program.asm:76 ignores every switch other than /R and returns normal sorted output. |
| L045 | D | B10 | batch.c:265 emits unset closed variables literally; test_batch.c:119 pins it. Replace that expectation only after independent DOS 3.3 capture. |
| L046 | F | B19 | Strict argument rejection absent, but current leniency matches shipped ch2-entries-a.tex:67-68. The audit has no period invalid-argument golden. |
| L047 | F | B18 | cmd_render_prompt at command.c:995 has no V/v case; default at :1147 prints V literally. Original DOS p.101 requires version expansion. |

## Developer pickup batches

Each B-number is one row in BEADS.tsv, in the same order. The TSV has a
header and five columns: title, type, priority, description, L-ids. Each
finding belongs to exactly one batch or existing owner. Companion manual
changes go with behavioral changes; they do not create duplicate findings.

### B01 - Stop interrupted TYPE and DIR before consuming invalid syscall results (P1; bug)

Findings: L031, L043.

Root cause: os/milton/int21.c:5392 invokes INT 23h and returns before servicing the interrupted call; int21.c:5337 only sets the resident recovery latch. os/milton/command.c:2797 treats the unchanged AH=3Fh request register as a read count, and :2809 writes that count from a 128-byte stack buffer without testing the latch or bounding got. Thus an interrupted read can turn EAX=0x3F00 into 16128 output bytes. command.c:2741 ignores the same latch after the DIR /P key wait and :2766 continues enumeration.

Ground truth: MS-DOS 3.3 User's Guide, Reference printed p.159 / PDF244 (evidence/manual-page-244.txt); ADR-0003 DEC-16 Fork A; PRD 6.1. Cancellation must not manufacture file bytes.

Acceptance oracle and mutant: Extend harness/diff/fat_diff/test_dos_break.c and the INT21 host seam with Ctrl-C at read, write and DIR pause boundaries. Verify CLOSE and all stdin/stdout restoration paths. Replay controlshort and gaps2 on copied disks under QEMU, require an immediate usable prompt, no later HUGE/F299 lines and zero non-file bytes; also redirect TYPE and compare the committed prefix independently. Add a mutant that omits the interrupted check or permits got > sizeof(chunk); it must fail the byte-boundary and cancellation checks.

Scope / dependencies: Keep child-program INT 23h termination intact. This is a returning resident-call completion bug, not another request to add BREAK ON checkpoints (initech-bsy.4).

Audit evidence:

- docs/audits/2026-10-23-dos/shots/resume-controlshort-ctrlc.png
- docs/audits/2026-10-23-dos/shots/resume-gaps2-dir-break.png
- docs/audits/2026-10-23-dos/evidence/resume-control-verification.json
- docs/audits/2026-10-23-dos/evidence/resume-controlshort-serial.log
- docs/audits/2026-10-23-dos/evidence/resume-gaps2-serial.log
- docs/audits/2026-10-23-dos/evidence/resume-fixtures/HUGE.TXT
- docs/audits/2026-10-23-dos/evidence/resume_driver.py

### B02 - Transfer batch control on a plain invocation and reserve return for CALL (P2; bug)

Findings: L033.

Root cause: os/milton/command.c:3595 sends every resolved BAT to run_batch_invoke; :4695 always nests run_batch and :4636 restores the previous frame. The plain invocation has no transfer-control result distinguishing it from BL_CALL at :4581. The shared g_batch_buf also needs explicit ownership when frames transfer or return.

Ground truth: MS-DOS 3.3 User's Guide, Reference printed p.142 / PDF227 (evidence/manual-page-227.txt); shipped Reference ch4-batch.tex CALL entry.

Acceptance oracle and mutant: Add an execution-level C host oracle for parent -> child -> grandchild with both plain invocation and CALL, including IF/FOR bodies. Replay resume_driver.py batch on QEMU: CHILD=arg appears, SHOULD-NOT-RETURN does not, while CALL runs its post-call sentinel. Mutant: route a plain invocation through CALL-style frame restoration; require the forbidden sentinel to make the oracle red.

Scope / dependencies: Keep this pickup on frame transfer and buffer ownership; the batch termination question is B14.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/resume-batch-flow.png
- docs/audits/2026-10-23-dos/evidence/resume-batch-serial.log
- docs/audits/2026-10-23-dos/evidence/resume-fixtures/CHAIN.BAT
- docs/audits/2026-10-23-dos/evidence/cases/CHILD.BAT
- docs/audits/2026-10-23-dos/evidence/manual-page-227.txt

### B03 - Execute complete batch files or reject an oversized script before execution (P2; bug)

Findings: L005.

Root cause: os/milton/command.c:4212 caps the shared batch buffer at 4096 bytes; batch_load_file at :4375 silently stops at the cap and returns a normal length at :4396. run_batch at :4614 interprets the prefix as a complete file, so the final commands disappear without an error.

Ground truth: pubs/manuals/initechdos-reference/ch4-batch.tex:19-25 promises sequential execution; CLAUDE.md Rule 2 forbids silently incomplete success.

Acceptance oracle and mutant: Extend a C execution/PAL oracle at lengths 4095/4096/4097 and the BIG.BAT length, with split lines, labels and CALL reloads crossing the boundary. QEMU B063 must execute BIG-END then BIG-RETURNED, or reject the whole oversized file visibly before BIG-BEGIN. Mutant: restore silent 4096-byte truncation; the final sentinel or reject-before-side-effects assertion must fail.

Scope / dependencies: A streaming/re-reading interpreter is preferable for full compatibility; an explicit all-or-nothing size refusal is the bounded safety fix. Merely raising the constant leaves the defect at the next boundary.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/suite4-B063-batch-long.png
- docs/audits/2026-10-23-dos/evidence/suite4-B063.txt
- docs/audits/2026-10-23-dos/evidence/cases/BIG.BAT
- docs/audits/2026-10-23-dos/evidence/cases/B063.BAT
- docs/audits/2026-10-23-dos/evidence/suite.py

### B04 - Resolve qualified executable suffixes and preserve meaningful EXEC failures (P2; bug)

Findings: L008, L040.

Root cause: os/milton/command.c:1319 makes an explicit path a single candidate and :1336 appends only .COM, omitting .EXE/.BAT. run_external at :3612 discards every nonzero EXEC error and :3616 reports Bad command. For the audited GUI files, loader.c:367 rejects e_minalloc beyond the DOS arena but returns LOADER_ERR_BAD_FORMAT; kmain.c:813 maps that to DOS 0Bh, not a memory-specific result. Header arithmetic independently confirms 123 needs 298240 bytes versus 65168 available, WORD 491472 versus 79792.

Ground truth: pubs/manuals/initechdos-reference/ch4-batch.tex:32; ADR-0003 DEC-08a.2/.3 and DEC-11; spec/memory_map.h:276/:382/:389; ADR-0013 AC-3 DEC-AC3-1 defines FLAIR GUI launch separately from DOS EXEC. evidence/recovery-extra-verification.json preserves the original MZ headers.

Acceptance oracle and mutant: Extend os/milton/test_command.c candidate planning with qualified COM/EXE/BAT suffix search and explicit-extension controls. Extend test_mzload.c with the audited header geometry to distinguish insufficient memory from invalid format. QEMU followup must run qualified BATCH2 without .BAT; native 123/WORD may still be refused but must receive the actual memory/launch-context diagnosis. Keep test-flair-app-launch green. Mutants: qualified paths try only COM, or map every existing-file EXEC refusal to Bad command; each must be detected.

Scope / dependencies: Do not expand DOS memory limits, shrink BSS reserves or force FLAIR tenants through the DOS ABI to satisfy L040. It is a diagnostic defect; direct DOS execution of these GUI tenants is not an established contract. No TSR is used, so initech-h4bk is not this failure.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/followup-commands-env.png
- docs/audits/2026-10-23-dos/evidence/followup-serial.log
- docs/audits/2026-10-23-dos/evidence/special-fixtures/BATCH2.BAT
- docs/audits/2026-10-23-dos/shots/resume-app123-qualified.png
- docs/audits/2026-10-23-dos/shots/resume-appword-qualified.png
- docs/audits/2026-10-23-dos/evidence/resume-app123-serial.log
- docs/audits/2026-10-23-dos/evidence/resume-appword-serial.log
- docs/audits/2026-10-23-dos/evidence/recovery-extra-verification.json

### B05 - Expose the child exit status after COMMAND /C (P2; bug)

Findings: L010.

Root cause: os/milton/command.c:4713 saves g_errorlevel before the secondary command; run_external at :3609 correctly latches the child result, then builtin_command at :4727 unconditionally restores the old errorlevel and loses it.

Ground truth: pubs/manuals/initechdos-reference/ch4-batch.tex:185-186 and MS-DOS 3.3 IF, Reference printed p.147, define ERRORLEVEL from the last program. Capture real DOS 3.3 COMMAND /C exit propagation before locking exact secondary-shell edge cases.

Acceptance oracle and mutant: Extend the secondary-command execution oracle with ERR0.COM/ERR7.COM directly, through /C, through nested /C and with prior nonzero parent status; retain private environment and EXIT isolation. Replay advanced-misc under QEMU and require CHILD-EXIT7 and DIRECT-EXIT7, not CHILD-STATUS-LOST. Mutant: restore the saved parent status after /C; the child-status assertion must fail.

Scope / dependencies: Keep status ownership explicit for a builtin or failed lookup that does not run a program; do not overwrite unrelated parent state.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/advanced-misc-errorlevel-shift.png
- docs/audits/2026-10-23-dos/evidence/advanced-misc-serial.log
- docs/audits/2026-10-23-dos/evidence/more-fixtures/ERR7.COM
- docs/audits/2026-10-23-dos/evidence/more-fixtures/ERR0.COM
- docs/audits/2026-10-23-dos/evidence/prepare_more.py

### B06 - Complete FIND file operands, quoted strings, validation and result status (P2; feature)

Findings: L016, L017, L018, L019.

Root cause: os/milton/find_program.asm:62 scans past ordinary operand characters and :134 always reads handle 0; file-operand opening was never implemented. At :109 every quote closes a string and :123 overwrites nlen for subsequent quote groups, so doubled quotes can produce an empty final needle. Unknown option letters fall through at :93. The normal exit at :254 always returns 0, even when count is zero; parameter errors return 1 at :263.

Ground truth: MS-DOS 3.3 User's Guide, Reference printed pp.72-73 / PDF157-158 (evidence/manual-page-157.txt and manual-page-158.txt), for named files, quoted literal strings and switches. L017's exact malformed-argument status of 2 is auditor memory, not established by those pages; mint actual DOS 3.3 match/no-match/parse-error statuses before changing that branch.

Acceptance oracle and mutant: Extend test-find-filter with one/multiple named files, stdin as a control, doubled quotes, /V /C /N, valid documented Initech /I, invalid switches and batch ERRORLEVEL tests. Compare bytes, headings and exit statuses to a real DOS 3.3 golden (document /I as the Initech extension). QEMU B032 must finish without keyboard EOF and QUOTES.TXT must print only the matching quoted line. Mutants: always read stdin, stop at the first doubled quote, accept /BAD, or always exit 0; each needs a failing assertion.

Scope / dependencies: One filter parser/result pickup. Do not claim modern FIND error codes are a DOS 3.3 golden; preserve the independently established filter behavior.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/suite2-B032-BLOCKED.png
- docs/audits/2026-10-23-dos/evidence/cases/B032.BAT
- docs/audits/2026-10-23-dos/evidence/suite2-serial.log
- docs/audits/2026-10-23-dos/shots/followup-find-quotes-exit.png
- docs/audits/2026-10-23-dos/evidence/followup-serial.log
- docs/audits/2026-10-23-dos/evidence/special-fixtures/QUOTES.TXT
- docs/audits/2026-10-23-dos/evidence/resume-external-serial.log
- docs/audits/2026-10-23-dos/evidence/manual-page-157.txt
- docs/audits/2026-10-23-dos/evidence/manual-page-158.txt

### B07 - Apply DOS hidden and system attribute filtering in handle searches (P2; bug)

Findings: L026.

Root cause: os/milton/int21.c:3850 limits the special-attribute mask to volume labels and directories, omitting DIR_ATTR_HIDDEN and DIR_ATTR_SYSTEM. command.c:2734 correctly requests directories (0x10), so the backend still returns hidden/system files into ordinary DIR, /W and wildcard listings.

Ground truth: MS-DOS 3.3 User's Guide DIR, Reference printed pp.57-58 / PDF142-143; DOS AH=4Eh attribute-mask contract; spec/dos_structs.h. The audit fixture attributes were independently set with mattrib.

Acceptance oracle and mutant: Extend os/milton/test_int21_wildcard.c and test_dir_disk.c with ordinary, H, S, HS, directory and combined attributes, exact and wildcard searches and multiple requested masks. Compare with a real DOS 3.3 DTA golden and mtools fixture inventory. Replay B001/B003 on QEMU; no H/S names may enter ordinary output. Mutant: omit H/S from the search filter; the enumeration and rendered-name assertions must fail.

Scope / dependencies: Fix the shared AH=4Eh/4Fh mechanism, while retaining intentional hidden-file searches by callers that request the attributes; a DIR-only string filter hides the backend bug.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/suite2-B003-dir-wide.png
- docs/audits/2026-10-23-dos/shots/suite2-B001-dir-paths.png
- docs/audits/2026-10-23-dos/evidence/suite2-B001.txt
- docs/audits/2026-10-23-dos/evidence/suite2-B003.txt
- docs/audits/2026-10-23-dos/evidence/preparation.json

### B08 - Preserve the last completed command template after Ctrl-C (P2; bug)

Findings: L038.

Root cause: os/milton/int21.c:1536 gives conin_cooked_line the same buf+2 for output and the saved template. conedit.h:102 overwrites those bytes while typing. The Ctrl-C path at int21.c:1544 skips publishing a new count but does not restore the old bytes; command.c:3632 retains the static AH=0Ah block, so F3 reuses cancelled text with the previous count.

Ground truth: pubs/manuals/initechdos-users-guide/ch8-keys.tex:25-33 says completed Enter commits the template and Ctrl-C discards the unfinished line. MS-DOS 3.3 keyboard-template pages are supporting reference; no new historical claim is needed to enforce this shipped promise.

Acceptance oracle and mutant: Extend harness/conkeys/test_conkeys.c plus the INT21 AH=0Ah wrapper tests: a completed template, a cancelled replacement that is shorter/equal/longer, then F1/F3; include empty input, Escape, Insert and F5 controls. QEMU resume_driver.py keyboard must recall the preceding completed command, never ECHO CANCELED. Mutant: retain the aliasing/no-rollback path; the recalled-byte assertion must fail.

Scope / dependencies: The pure conedit snapshot alone is insufficient: the loss happens in the persistent caller buffer across two AH=0Ah calls.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/resume-keyboard-cancel-template.png
- docs/audits/2026-10-23-dos/evidence/resume-keyboard-serial.log
- docs/audits/2026-10-23-dos/evidence/resume_driver.py

### B09 - Render the foreign-MZ refusal reason on the panic screen before halting (P2; bug)

Findings: L030.

Root cause: os/milton/loader.c:1138 loader_panic_foreign_mz writes its name and reason only to COM1 at :1141-1148, then executes cli/hlt at :1152. It never calls the framebuffer panic painter. panic_screen.h:37 only supports numeric exception vectors, so a typed loader-reason path is needed rather than inventing a CPU vector.

Ground truth: CLAUDE.md Rule 2 and the explicit deliberate-canon foreign-MZ rule; ADR-0003 DEC-08a.5 requires refusal/panic for untagged 16-bit MZ. The requirement is a visible contextual panic, not execution or return to a shell.

Acceptance oracle and mutant: Add a host rendering assertion for a typed foreign-MZ reason, plus a QEMU foreign-MZ leg using T_DOS16.EXE that captures the screen after the serial HALTED marker. Require PC LOAD LETTER and a readable unsupported 16-bit MZ reason; test-panic must still show ERROR 06 for #UD. Mutant: bypass the framebuffer reason call while retaining serial PANIC/HALTED; the screen assertion must fail.

Scope / dependencies: Do not turn the authorized terminal refusal into recovery or add 16-bit execution. This batch is only the missing visible context.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/prog-mz16-result.png
- docs/audits/2026-10-23-dos/evidence/prog-mz16-serial.log
- docs/audits/2026-10-23-dos/evidence/special-fixtures/T_DOS16.EXE
- docs/audits/2026-10-23-dos/evidence/prepare_special.py

### B10 - Use one correct environment-expansion path for prompt and batch input (P2; feature)

Findings: L025, L045.

Root cause: os/milton/command.c:4887 sends raw interactive input directly to the pipeline dispatcher; batch_expand is called only in run_batch at :4627. os/milton/batch.c:265 deliberately emits an undefined closed variable literally, and test_batch.c:119 currently pins that behavior without an independent period golden.

Ground truth: pubs/manuals/initechdos-reference/ch4-batch.tex, Replaceable Parameters, promises %NAME% as the value; ADR-0003 DEC-11 grounds COMMAND compatibility. The exact interactive, unset, unmatched-percent and percent-escape rules need a real DOS 3.3 golden before replacing existing expectations; the report labels these expectations as memory.

Acceptance oracle and mutant: Mint a real DOS 3.3 corpus for defined/unset/mixed-case variables, %% escapes, unmatched percent, %0..%9 and interactive FOR %F versus batch %%F. Extend test_batch and a command-entry C host oracle; expand exactly once before pipe/redirection parsing while preserving FOR syntax. Replay advanced-keyboard and BENV.BAT on QEMU. Mutants: bypass interactive expansion or preserve an unset closed variable in batch; compare against the independent corpus, not the same implementation.

Scope / dependencies: Do not blindly reuse batch %% collapsing at the interactive prompt or expand IF/FOR bodies a second time. Reconcile the existing literal-unset host expectation only after independent reference capture.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/advanced-keyboard-interactive-expansion.png
- docs/audits/2026-10-23-dos/evidence/advanced-keyboard-serial.log
- docs/audits/2026-10-23-dos/shots/resume-keyboard-recall-kinds.png
- docs/audits/2026-10-23-dos/evidence/resume-keyboard-serial.log
- docs/audits/2026-10-23-dos/evidence/resume-batch-serial.log
- docs/audits/2026-10-23-dos/evidence/resume-fixtures/BENV.BAT

### B11 - Provide a secondary interactive COMMAND session with EXIT back to its caller (P2; feature)

Findings: L009.

Root cause: os/milton/command.c:4705 rejects every COMMAND invocation that does not begin /C. There is no interactive secondary-prompt entry even though /C already saves private environment and shell-exit context at :4711-4727.

Ground truth: pubs/manuals/initechdos-reference/ch2-entries-b.tex:102-104 promises typing commands in a second processor; MS-DOS 3.3 COMMAND, Reference printed pp.46-47. ADR-0003 DEC-11 defines the native interpreter.

Acceptance oracle and mutant: Extend the command-session host oracle and a QEMU followup trace: COMMAND -> private SET/PROMPT -> COMMAND /C ECHO NESTED -> EXIT -> parent prompt. Verify the parent environment, CWD/handle rules from the real DOS 3.3 golden, nested depth refusal, and permanent-shell EXIT. Mutant: retain the mandatory /C guard or let EXIT unwind the permanent processor; each must fail the session marker assertions.

Scope / dependencies: Factor an interactive loop that does not rerun SYSINIT/AUTOEXEC or reset the parent state. Coordinate the shared builtin_command epilogue with B05; each remains a separate acceptance item.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/followup-commands-env.png
- docs/audits/2026-10-23-dos/evidence/followup-serial.log

### B12 - Keep batch arguments beyond percent-9 and correct the SHIFT manual (P2; feature)

Findings: L006, L007.

Root cause: os/milton/command.c:4650 allocates only ten argv slots and :4670 stops parsing after nine supplied parameters; batch_frame.view[10] at :4444 and the count clamp at :4611 provide no later-argument reservoir for SHIFT. SHIFT at :4575 correctly starts at zero, so %0 changes. pubs/manuals/initechdos-reference/ch4-batch.tex:247 incorrectly promises %0 is unchanged.

Ground truth: MS-DOS 3.3 User's Guide SHIFT, Reference printed p.151 / PDF236 (evidence/manual-page-236.txt), explicitly describes more than ten parameters and losing the original %0. The shipped nine-argument limit describes the current narrower feature; only the %0 statement is a present documentation defect.

Acceptance oracle and mutant: Extend an execution-level C batch oracle with 11+ short parameters within the command-tail limit, repeated SHIFT through exhaustion, empty arguments and independent nested CALL frames. QEMU B060/TEN must expose ten then eleven in %9 while shifting %0 as DOS does. Correct the Reference comment/text and recorded SHIFT example; run test-pubs-examples and the DOS manual gate. Mutants: stop storing after nine or start shifting at slot one; both must fail independent expected output.

Scope / dependencies: Keep the visible substitution window %0..%9; the missing feature is retaining later command-tail arguments, not adding modern %10 syntax. Correct the separate manual limit when the reservoir ships.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/suite4-B060-batch-args.png
- docs/audits/2026-10-23-dos/shots/advanced-misc-errorlevel-shift.png
- docs/audits/2026-10-23-dos/evidence/suite4-B060.txt
- docs/audits/2026-10-23-dos/evidence/cases/ARGS.BAT
- docs/audits/2026-10-23-dos/evidence/more-fixtures/TEN.BAT
- docs/audits/2026-10-23-dos/evidence/manual-page-236.txt

### B13 - Honor SORT starting columns and reject unsupported options (P2; feature)

Findings: L020, L044.

Root cause: os/milton/sort_program.asm:64 scans for /R only and silently advances past every other switch at :76. compare_lines at :272 always starts both pointers at the first byte; no /+n offset exists.

Ground truth: MS-DOS 3.3 User's Guide SORT, Reference printed p.116 / PDF201 (evidence/manual-page-201.txt), defines /R and /+n. Unsupported-option refusal should use an independently captured DOS 3.3 diagnostic/status rather than a new invented success.

Acceptance oracle and mutant: Extend test-sort-filter with SORTCOL.TXT, /+1, /+4, reverse plus column, short/empty lines, invalid columns, /BAD and status checks, graded against literal independent results and a real DOS 3.3 argument golden. QEMU followup must put 02 apple before 01 zebra with /+4, and external must refuse /BAD. Mutants: ignore the column offset or accept unknown switches; require both to go red.

Scope / dependencies: Keep the existing explicit capacity refusal and ordinary case-insensitive/reverse comparisons intact; over-capacity truncation was not found by this audit.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/followup-sort-column-cap.png
- docs/audits/2026-10-23-dos/evidence/followup-serial.log
- docs/audits/2026-10-23-dos/evidence/special-fixtures/SORTCOL.TXT
- docs/audits/2026-10-23-dos/shots/resume-external-filters.png
- docs/audits/2026-10-23-dos/evidence/resume-external-serial.log
- docs/audits/2026-10-23-dos/evidence/manual-page-201.txt

### B14 - Offer the DOS batch-termination question and support continuing with N (P2; feature)

Findings: L034.

Root cause: os/milton/int21.c:5337 sets the shell recovery latch; os/milton/command.c:4618 immediately exits the batch loop whenever it is set. There is no batch-specific Y/N decision or resume path for a PAUSE or ECHO/GOTO loop.

Ground truth: MS-DOS 3.3 User's Guide PAUSE, Reference printed p.149 / PDF234 (evidence/resume-manual-page-234.txt). pubs/manuals/initechdos-reference/ch4-batch.tex:38-39 currently documents immediate cancellation, so this is a missing compatibility feature, not a broken narrow promise.

Acceptance oracle and mutant: Add a C execution-state oracle and QEMU resume_driver.py batch legs: Ctrl-C at PAUSE and in LOOP.BAT, Y ends and N continues exactly once; test invalid answer, nested CALL and redirected input/output cleanup. Capture real DOS 3.3 behavior for which nested frame is cancelled. Mutant: always terminate or always resume regardless of answer; each must fail a post-question sentinel. Update the manual and its examples with the new behavior.

Scope / dependencies: Depends on reliable resident-call interruption handling from B01; do not use a nonlocal jump that skips open-handle restoration.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/resume-batch-pause-break.png
- docs/audits/2026-10-23-dos/shots/resume-batch-loop-break.png
- docs/audits/2026-10-23-dos/evidence/resume-batch-serial.log
- docs/audits/2026-10-23-dos/evidence/cases/BREAK.BAT
- docs/audits/2026-10-23-dos/evidence/resume-fixtures/LOOP.BAT
- docs/audits/2026-10-23-dos/evidence/resume-manual-page-234.txt

### B15 - Implement DEL directory as confirmed deletion of its contained files (P2; feature)

Findings: L035.

Root cause: os/milton/command.c:3321 recognizes delete-all confirmation only for a literal *.* leaf. A non-wildcard directory reaches dos_unlink at :3347, so the backend correctly rejects deleting the directory as a file. No directory-operand expansion to contained files exists at the shell entry.

Ground truth: MS-DOS 3.3 User's Guide DEL, Reference printed p.56 / PDF141 (evidence/manual-page-141.txt), explicitly accepts a directory name to delete its files. Backend rejection of unlinking directories must remain.

Acceptance oracle and mutant: Extend the host DOS safety/FAT oracle with a named directory, nested subdirectory, read-only child and 300 ordinary files; N must preserve every byte and Y must delete only eligible immediate files while retaining directories and protected children. Replay B023 under QEMU and verify the stopped image with mtools/fsck.fat. Mutants: route the directory straight to unlink, delete without confirmation, or recurse into children; the state assertions must fail.

Scope / dependencies: Resolve the operand as a directory before transforming it to directory\*.*; retain drive identity and the existing protected-file/no-cap enumeration behavior.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/suite2-B023-del-directory.png
- docs/audits/2026-10-23-dos/evidence/suite2-B023.txt
- docs/audits/2026-10-23-dos/evidence/extracted/suite2/T/DELDIR/ONE.TXT
- docs/audits/2026-10-23-dos/evidence/manual-page-141.txt

### B16 - Add DATE and TIME entry prompts and TIME fractional-field parsing (P2; feature)

Findings: L021, L036, L037.

Root cause: os/milton/command.c:3469 and :3499 only report the clock when no argument is supplied. The entry loops exist solely in startup_datetime at :3654/:3664. cmd_parse_time at :972 rejects remaining .hundredths text, and dos_settime at :2206 always passes DL=0. int21.c:4924 explicitly ignores fractional seconds because the current RTC seam has one-second resolution.

Ground truth: MS-DOS 3.3 User's Guide DATE printed p.55 / PDF140 and TIME pp.120-121 / PDF205-206 (evidence/manual-page-140.txt, manual-page-205.txt, manual-page-206.txt); spec/int21h_register.json AH=2Ch/2Dh. The shipped manual currently promises display-only/no-fraction forms.

Acceptance oracle and mutant: Extend test_command date/time parsing plus a fake-clock C execution oracle: no-arg entry, blank retains, invalid reprompts, leap-day and range limits, .00/.99 and malformed suffixes. Replay B038/B039 under QEMU; TIME 13:14:15.99 must be accepted and both no-arg commands must request input. Mutation: omit the no-arg prompt or reject the fractional field; require each leg to fail. Document the RTC rounding/resolution contract explicitly rather than claiming retained hundredths without implementing and testing it.

Scope / dependencies: Reuse entry/validation logic without rerunning startup as a side effect; synchronize the Reference syntax and examples when the broader forms ship.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/suite3-B038-date-formats.png
- docs/audits/2026-10-23-dos/shots/suite3-B039-time-values.png
- docs/audits/2026-10-23-dos/evidence/suite3-B038.txt
- docs/audits/2026-10-23-dos/evidence/suite3-B039.txt
- docs/audits/2026-10-23-dos/evidence/manual-page-140.txt
- docs/audits/2026-10-23-dos/evidence/manual-page-205.txt
- docs/audits/2026-10-23-dos/evidence/manual-page-206.txt

### B17 - Implement prompt-level ECHO state and the blank-line shorthand (P2; feature)

Findings: L023, L024.

Root cause: os/milton/command.c:184 tokenizes ECHO. as a distinct external word and the classify table at :86 recognizes only ECHO. dispatch_control at :4541 updates echo_on only when a batch frame exists; otherwise builtin_echo at :2968 always reports on and :2971 prints OFF/ON as text. No processor-level echo state or ECHO. syntax exists.

Ground truth: MS-DOS 3.3 User's Guide ECHO, Reference printed p.143 / PDF228 (evidence/manual-page-228.txt), defines ON/OFF; ADR-0003 DEC-11. Mint actual DOS 3.3 interactive ON/OFF and ECHO. behavior because the report explicitly labels the blank-line shorthand as memory. Shipped Reference currently documents the narrower prompt behavior.

Acceptance oracle and mutant: Extend command parser/execution host tests with ECHO., ECHO text, ON/OFF/query, @ suppression and CALL/secondary processor state. Replay B036 and advanced-keyboard on QEMU: blank line without Bad command; OFF followed by a query reports off. Grade ECHO. bytes and state lifetime against the real DOS 3.3 golden. Mutants: classify ECHO. externally or keep frame-only state; each must fail. Update both ECHO manual entries and examples.

Scope / dependencies: Define state inheritance/restoration with B11 before changing nested-session semantics; the feature must not consume literal message text as an option unexpectedly.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/suite3-B036-echo.png
- docs/audits/2026-10-23-dos/evidence/suite3-B036.txt
- docs/audits/2026-10-23-dos/shots/advanced-keyboard-interactive-expansion.png
- docs/audits/2026-10-23-dos/evidence/advanced-keyboard-serial.log
- docs/audits/2026-10-23-dos/evidence/manual-page-228.txt

### B18 - Expand PROMPT $V using the native DOS version contract (P3; feature)

Findings: L047.

Root cause: os/milton/command.c:995 cmd_render_prompt has no V/v switch arm; its default at :1147 emits the character literally, producing V>. dos_getver at :2343 already exposes AH=30h, but prompt_ctx_t has no version supplied to the renderer.

Ground truth: MS-DOS 3.3 User's Guide PROMPT special prompts, Reference printed p.101 / PDF186 (evidence/manual-page-186.txt), explicitly defines $v as the version number; ADR-0003 DEC-12 defines reported version 3.30.

Acceptance oracle and mutant: Extend test_command.c with upper/lowercase $V, $V$G, mixed codes and renderer capacity boundaries using a fixed independent version expectation. QEMU promptversion must show the correct version rather than V>; compare formatting to real DOS 3.3 and keep VER/AH=30h consistent. Mutant: remove the V arm or force a stale version; the prompt assertion must fail. Add $V to the shipped prompt table.

Scope / dependencies: A missing documented period prompt code; do not reinterpret every other unknown code as part of this pickup.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/resume-promptversion-version.png
- docs/audits/2026-10-23-dos/evidence/resume-promptversion-serial.log
- docs/audits/2026-10-23-dos/evidence/manual-page-186.txt

### B19 - Validate BREAK operands against an independently captured DOS grammar (P3; task)

Findings: L046.

Root cause: os/milton/command.c:2989 recognizes ON/OFF but :2995 silently falls through for every other operand and :3000 reports the unchanged state. This is explicit existing policy, also documented in pubs/manuals/initechdos-reference/ch2-entries-a.tex:67-68; strict invalid-operand reporting was never built.

Ground truth: Shipped Reference BREAK syntax at ch2-entries-a.tex:59 and the explicit lenient text at :67-68; ADR-0003 DEC-16 3.3 governs the flag, not invalid command-line wording. The audit supplies only a tester expectation for rejection, so capture a real DOS 3.3 BREAK INVALID/trailing-operand golden before changing the narrow policy.

Acceptance oracle and mutant: Add C command-argument tests for empty, mixed-case ON/OFF, INVALID and extra words; assert rejected input never changes the flag. Replay gaps2 on QEMU and grade the chosen diagnostic/status against the DOS 3.3 capture. Mutant: treat an invalid operand as the bare query; the diagnostic assertion must fail if the reference requires rejection. If the reference confirms the present leniency, close this candidate as reference-correct instead of changing code. Update manual text only if the policy changes.

Scope / dependencies: Low-priority compatibility investigation and bounded implementation, not a proven violation of the shipped manual or an authorization to replace its explicit policy from memory.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/resume-gaps2-builtins.png
- docs/audits/2026-10-23-dos/evidence/resume-gaps2-serial.log
- docs/audits/2026-10-23-dos/evidence/suite3-B042.txt

### B20 - Show and clear the MORE indicator at interactive page waits (P3; bug)

Findings: L039.

Root cause: os/milton/more_program.asm:125 reaches the page boundary then calls AH=08h directly at :126-127. No marker string is emitted or cleared anywhere in the pagination path. Its existing passthrough path intentionally has no prompt.

Ground truth: MS-DOS 3.3 User's Guide printed p.20 / PDF105 (evidence/resume-manual-page-105.txt) explicitly describes the --More-- indicator.

Acceptance oracle and mutant: Extend test-more-filter with a QEMU CON-output trace using PAGES.TXT: assert the visible indicator at both waits, no progress while paused, proper erase/continue and Ctrl-C return. Preserve the existing redirected byte-exact passthrough test and require no indicator bytes there. Mutant: omit the marker write or emit it to redirected stdout; screen and byte checks must catch both.

Scope / dependencies: Only pager feedback; the report proves the waits themselves already work.

Audit evidence:

- docs/audits/2026-10-23-dos/shots/resume-external-more-page1.png
- docs/audits/2026-10-23-dos/shots/resume-external-more-page2.png
- docs/audits/2026-10-23-dos/evidence/resume-external-serial.log
- docs/audits/2026-10-23-dos/evidence/resume-fixtures/PAGES.TXT
- docs/audits/2026-10-23-dos/evidence/resume-manual-page-105.txt

## Completeness check

47 unique report IDs: D=16, F=14, DOC=1, BY-DESIGN=1, DUP=1, LANE=14.
31 unowned D/F/DOC findings form 20 developer batches.
No LANE, DUP or BY-DESIGN ID appears in BEADS.tsv. All cited evidence
paths and root file/line references were checked for existence; this is
structural validation of the triage, not execution of the proposed fixes.
