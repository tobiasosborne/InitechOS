# InitechDOS audit - 2026-10-23

Image build: `e67b8311092873fe16e6c6d62ad82f894bfb8d7d` (`images/COMMIT`). This report continues the interrupted audit in this directory. Findings L001-L030 form the initial reconstruction; later numbers add resumed checks and additional recovered cases. No original finding numbers were assigned in its narration.

All tests use QEMU `-display none -cpu 486 -m 16 -qmp stdio`, with disposable disk copies. A: is the second IDE data disk; C: is the boot disk partition at byte 32256; D: is the added primary partition at byte 2097152. Commands below are typed with Enter unless another key is specified. Files, screenshots and logs are relative to this audit directory. Source and manual paths below are relative to `/home/tobias/Projects/initech-os/`.

Original DOS reference: `/home/tobias/Projects/initech-manuals-ref/dos33/MS-DOS_3.3_Users_Guide_198707.pdf`; its reference-section printed pages are PDF page minus 85. Saved text extracts are `evidence/manual-page-N.txt`. Memory-based expectations are explicitly labeled.

## Findings

### L001 - A pipeline destroys an existing PIPE1.$$$ file

Area: Pipes. Severity: P1.

Steps: Boot followup with multi-boot-fixture.img and followup-fixture.img. CD \T; TYPE PIPE1.$$$; TYPE ONE.TXT | SORT; DIR PIPE1.$$$; TYPE PIPE1.$$$.

Observed: The sentinel USER-OWNED-PIPE-FILE is readable before the pipe. The pipe prints ALPHA, then the sentinel file is gone. The stopped-image extraction also has no PIPE1.$$$.

Expected and basis: Choose an unused temporary name and preserve existing files (tester expectation for temporary-file handling; original DOS Reference pp.132-135 describes pipes).

Evidence: [shots/followup-pipe-clobber.png](shots/followup-pipe-clobber.png), [evidence/followup-serial.log](evidence/followup-serial.log), [evidence/followup-filesystem.json](evidence/followup-filesystem.json), [evidence/followup.py](evidence/followup.py).

### L002 - Output to a full disk silently becomes an empty file

Area: Redirection. Severity: P1.

Steps: Boot re-full0 with multi-boot-fixture.img and full0-fixture.img. CD \T; DIR /W; ECHO TEST > FULL.OUT; DIR FULL.OUT; TYPE FULL.OUT.

Observed: DIR reports 0 bytes free. ECHO returns without an error; FULL.OUT exists with zero bytes instead of TEST plus CR/LF. The source files remain intact.

Expected and basis: Report a disk-full write error; do not silently discard requested output (tester expectation; DOS Reference redirection pp.132-135).

Evidence: [shots/re-full0-redirection.png](shots/re-full0-redirection.png), [evidence/re-full0-serial.log](evidence/re-full0-serial.log), [evidence/storage-verification.json](evidence/storage-verification.json), [evidence/extracted/re-full0/T_FULL.OUT](evidence/extracted/re-full0/T_FULL.OUT), [evidence/recheck.py](evidence/recheck.py).

### L003 - A pipeline on a full disk silently loses its input

Area: Pipes. Severity: P1.

Steps: On the same full0 fixture, CD \T; TYPE ONE.TXT | SORT; ECHO PIPE-RETURNED.

Observed: The pipe prints nothing and no error. PIPE-RETURNED appears normally. ONE.TXT contains ALPHA plus CR/LF. Repeated after filling the last 512 bytes in re-full512.

Expected and basis: Print ALPHA or report failure to create/write the pipe intermediate (tester expectation; DOS Reference pp.132-135).

Evidence: [shots/re-full0-pipe-empty.png](shots/re-full0-pipe-empty.png), [evidence/re-full0-serial.log](evidence/re-full0-serial.log), [evidence/re-full512-serial.log](evidence/re-full512-serial.log), [evidence/recheck.py](evidence/recheck.py).

### L004 - COPY reports Bad command for a disk-full write

Area: COPY diagnostics. Severity: P3.

Steps: On full0-fixture.img, CD \T; COPY LONG.TXT FULL.BIN; DIR FULL.BIN.

Observed: COPY prints Bad command or file name and leaves a zero-byte FULL.BIN. LONG.TXT exists and is 76800 bytes.

Expected and basis: A disk-space/write failure diagnostic identifying why COPY failed; command syntax and source are valid (tester expectation).

Evidence: [shots/re-full0-copy-error.png](shots/re-full0-copy-error.png), [evidence/re-full0-serial.log](evidence/re-full0-serial.log), [evidence/storage-verification.json](evidence/storage-verification.json).

### L005 - A long batch file silently stops before its final command

Area: Batch files. Severity: P2.

Steps: Boot suite4 with multi-boot-fixture.img and suite-fixture.img; run \CASES\B063.BAT. It CALLs BIG.BAT, which contains BIG-BEGIN, 65 long REM lines, and BIG-END.

Observed: BIG-BEGIN and the caller's BIG-RETURNED print, but BIG-END never executes; no limit/error is reported.

Expected and basis: Run the complete batch or explicitly reject it. pubs/manuals/initechdos-reference/ch4-batch.tex:19-25 describes a batch as a sequence of commands, with no file-size restriction. Suspected cause (guess): os/milton/command.c:4212 BATCH_FILE_MAX=4096. Exact contract, pubs/manuals/initechdos-reference/ch4-batch.tex:20-21: "InitechDOS carries out the commands in it one after another".

Evidence: [shots/suite4-B063-batch-long.png](shots/suite4-B063-batch-long.png), [evidence/suite4-B063.txt](evidence/suite4-B063.txt), [evidence/cases/BIG.BAT](evidence/cases/BIG.BAT), [evidence/cases/B063.BAT](evidence/cases/B063.BAT), [evidence/suite.py](evidence/suite.py).

### L006 - SHIFT loses arguments after the ninth

Area: Batch SHIFT. Severity: P2.

Steps: Run \CASES\B060.BAT, or TEN one two three four five six seven eight nine ten eleven on more-fixture.img.

Observed: The original ninth argument is nine. After SHIFT, %9 is empty; ten and eleven never become available.

Expected and basis: Shift later arguments into %9 (DOS Reference printed p.151, PDF236). The Initech manual explicitly limits initial arguments to nine, but this is a DOS compatibility gap.

Evidence: [shots/suite4-B060-batch-args.png](shots/suite4-B060-batch-args.png), [shots/advanced-misc-errorlevel-shift.png](shots/advanced-misc-errorlevel-shift.png), [evidence/suite4-B060.txt](evidence/suite4-B060.txt), [evidence/advanced-misc-serial.log](evidence/advanced-misc-serial.log), [evidence/cases/ARGS.BAT](evidence/cases/ARGS.BAT), [evidence/more-fixtures/TEN.BAT](evidence/more-fixtures/TEN.BAT), [evidence/manual-page-236.txt](evidence/manual-page-236.txt).

### L007 - SHIFT changes %0 contrary to the shipped manual

Area: Batch documentation. Severity: P3.

Steps: Run \CASES\B060.BAT with its eleven arguments.

Observed: ZERO initially identifies the batch path; after SHIFT, ZERO=one.

Expected and basis: The shipped Reference ch4-batch.tex:247 says "%0, the name of the batch file, is not changed." Actual shifting of %0 agrees with original DOS p.151; fix the manual contract rather than changing DOS-compatible behavior.

Evidence: [shots/suite4-B060-batch-args.png](shots/suite4-B060-batch-args.png), [evidence/suite4-B060.txt](evidence/suite4-B060.txt), [evidence/cases/ARGS.BAT](evidence/cases/ARGS.BAT), [evidence/manual-page-236.txt](evidence/manual-page-236.txt).

### L008 - A path-qualified batch name requires the .BAT extension

Area: Command lookup. Severity: P2.

Steps: Boot followup; CD \T; A:\T\BATCH2; A:\T\BATCH2.BAT; BATCH2.

Observed: The first form says Bad command or file name. Both controls print BATCH2-RAN.

Expected and basis: Resolve .BAT for a qualified name as for a bare name. pubs/manuals/initechdos-reference/ch4-batch.tex:32 says to type the name without its extension; original DOS batch invocation uses drive/path forms. Exact wording, pubs/manuals/initechdos-reference/ch4-batch.tex:32: "type its name without the extension".

Evidence: [shots/followup-commands-env.png](shots/followup-commands-env.png), [evidence/followup-serial.log](evidence/followup-serial.log), [evidence/special-fixtures/BATCH2.BAT](evidence/special-fixtures/BATCH2.BAT).

### L009 - COMMAND without /C cannot open a nested prompt

Area: COMMAND. Severity: P2.

Steps: Boot followup; type COMMAND. Then try COMMAND /C COMMAND /C ECHO NESTED.

Observed: Bare COMMAND says Invalid number of parameters. Nested /C works and prints NESTED.

Expected and basis: Start a secondary command processor and allow EXIT back to the caller. pubs/manuals/initechdos-reference/ch2-entries-b.tex:102-104 promises typing commands in a second copy; DOS Reference COMMAND pp.46-47. Exact wording, pubs/manuals/initechdos-reference/ch2-entries-b.tex:102-104: "so that you can type commands without leaving the program".

Evidence: [shots/followup-commands-env.png](shots/followup-commands-env.png), [evidence/followup-serial.log](evidence/followup-serial.log).

### L010 - COMMAND /C loses its child program exit status

Area: COMMAND error status. Severity: P2.

Steps: Boot advanced-misc (geometry-boot-fixture.img, more-fixture.img); CD \T; ERR0.COM; COMMAND /C ERR7.COM; IF ERRORLEVEL 7 ECHO CHILD-EXIT7; IF NOT ERRORLEVEL 7 ECHO CHILD-STATUS-LOST. Run ERR7.COM directly as a control.

Observed: The /C form prints CHILD-STATUS-LOST; the direct form prints DIRECT-EXIT7. ERR7.COM exits through INT 21h/AH=4Ch with AL=7.

Expected and basis: Propagate the child status so ERRORLEVEL can test it (tester DOS command-processor expectation; Reference IF p.147 and Initech ch4-batch.tex:185-186).

Evidence: [shots/advanced-misc-errorlevel-shift.png](shots/advanced-misc-errorlevel-shift.png), [evidence/advanced-misc-serial.log](evidence/advanced-misc-serial.log), [evidence/more-fixtures/ERR7.COM](evidence/more-fixtures/ERR7.COM), [evidence/prepare_more.py](evidence/prepare_more.py).

### L011 - CD drive: does not display that drive's current directory

Area: CD. Severity: P2.

Steps: Boot followup; CD \T; CD C:\CHOME; CD D:\DHOME; CD C:; CD D:; CD.

Observed: Both drive-only queries are silent; CD alone still prints A:\T. Drive-relative file access and switching drive show that per-drive directories were set.

Expected and basis: Display C:\CHOME and D:\DHOME without switching drives (DOS Reference printed p.42/PDF127).

Evidence: [shots/followup-cd-tree-drive.png](shots/followup-cd-tree-drive.png), [evidence/followup-serial.log](evidence/followup-serial.log), [evidence/suite4-B054.txt](evidence/suite4-B054.txt), [evidence/manual-page-127.txt](evidence/manual-page-127.txt).

### L012 - ATTRIB cannot find a readable file on D:

Area: ATTRIB. Severity: P2.

Steps: Boot advanced-driveutils; CD \T; TYPE D:\DHOME\SAME.TXT; ATTRIB D:\DHOME\SAME.TXT; ATTRIB +R D:\DHOME\SAME.TXT.

Observed: TYPE prints D-DRIVE. Both ATTRIB forms print Invalid path (or file not found).

Expected and basis: Read and set attributes on the specified mounted drive. pubs/manuals/initechdos-reference/ch2-entries-a.tex:9-12 accepts a drive-qualified file. The filename syntax at pubs/manuals/initechdos-reference/ch2-entries-a.tex:11 explicitly begins with optional d: and path; line 26 says "names the files to change or display".

Evidence: [shots/advanced-driveutils-failures.png](shots/advanced-driveutils-failures.png), [evidence/advanced-driveutils-serial.log](evidence/advanced-driveutils-serial.log), [evidence/advanced.py](evidence/advanced.py).

### L013 - CHKDSK reports a FAT error on valid C: and D: volumes

Area: CHKDSK. Severity: P2.

Steps: Boot advanced-driveutils using the geometry-corrected boot fixture; CHKDSK C:; CHKDSK D:. Compare ordinary DIR/TYPE/COPY operations on those volumes.

Observed: Both checks say Disk error reading (or writing) FAT. The same volumes support file reads and writes. CHKDSK on A: works.

Expected and basis: Check the requested mounted FAT12 volume; do not report a disk error simply for its drive letter (DOS CHKDSK and pubs/manuals/initechdos-reference/ch2-entries-a.tex, CHKDSK).

Evidence: [shots/advanced-driveutils-failures.png](shots/advanced-driveutils-failures.png), [evidence/advanced-driveutils-serial.log](evidence/advanced-driveutils-serial.log), [evidence/suite4-B055.txt](evidence/suite4-B055.txt), [evidence/prepare.py](evidence/prepare.py), [evidence/prepare_more.py](evidence/prepare_more.py).

### L014 - TREE ignores an explicit other-drive operand

Area: TREE. Severity: P2.

Steps: Boot suite4; CD \T; CD D:\DHOME; TREE D: /F. Also TREE D:\DHOME /F in followup.

Observed: TREE D: lists volume INITECH and A:\T contents. The absolute D: form says Invalid path despite the directory being readable with TYPE.

Expected and basis: Walk the specified drive/path and label that volume (pubs/manuals/initechdos-reference/ch2-entries-c.tex:125-140). The TREE syntax at pubs/manuals/initechdos-reference/ch2-entries-c.tex:127-128 includes a drive-qualified path; line 131 says "names the directory to start from".

Evidence: [shots/suite4-B057-cross-drive-tools.png](shots/suite4-B057-cross-drive-tools.png), [shots/followup-cd-tree-drive.png](shots/followup-cd-tree-drive.png), [evidence/suite4-B057.txt](evidence/suite4-B057.txt), [evidence/followup-serial.log](evidence/followup-serial.log).

### L015 - LABEL rejects a valid mounted D: volume

Area: LABEL. Severity: P2.

Steps: Boot suite4; VOL D:; LABEL D: NEWLABEL.

Observed: VOL displays EXTRA, but LABEL returns Invalid drive specification.

Expected and basis: Honor a specified valid drive (original DOS LABEL drive syntax; tester DOS 3.3 expectation). pubs/manuals/initechdos-reference/ch2-entries-b.tex:240-242 explicitly restricts LABEL to the current drive, so this is a compatibility gap, not a broken promise in that manual.

Evidence: [shots/suite4-B057-cross-drive-tools.png](shots/suite4-B057-cross-drive-tools.png), [evidence/suite4-B056.txt](evidence/suite4-B056.txt), [evidence/suite4-B057.txt](evidence/suite4-B057.txt).

### L016 - FIND ignores filename operands and waits for keyboard input

Area: FIND. Severity: P2.

Steps: Boot suite2; run \CASES\B032.BAT, whose first command is FIND "Alpha" LINES.TXT. Wait 12 seconds; or run that command directly from A:\T.

Observed: No search results or file header appear; FIND waits for standard input. Ctrl-Z plus Enter returns. FIND "Alpha" < LINES.TXT works.

Expected and basis: Open the named input file (DOS Reference pp.72-73/PDF157-158). Initech documentation only describes redirection; this is a compatibility gap.

Evidence: [shots/suite2-B032-BLOCKED.png](shots/suite2-B032-BLOCKED.png), [evidence/suite2-serial.log](evidence/suite2-serial.log), [evidence/interactive-serial.log](evidence/interactive-serial.log), [evidence/cases/B032.BAT](evidence/cases/B032.BAT), [evidence/manual-page-157.txt](evidence/manual-page-157.txt).

### L017 - FIND returns incorrect status for no match and malformed arguments

Area: FIND status. Severity: P2.

Steps: Boot followup; CD \T; FIND "absent" < ONE.TXT; IF ERRORLEVEL 1 ECHO RC1; IF NOT ERRORLEVEL 1 ECHO RC0. Also run FIND bad < ONE.TXT; IF ERRORLEVEL 2 ECHO BAD-ARGS-RC2; IF NOT ERRORLEVEL 2 ECHO BAD-ARGS-LOW.

Observed: No line matches, but RC0 is printed. The malformed form prints Invalid number of parameters but BAD-ARGS-LOW, not BAD-ARGS-RC2.

Expected and basis: Return 1 when no matching line exists, 0 when a match exists (tester DOS FIND exit-status memory; distinguish from malformed-argument status).

Evidence: [shots/followup-find-quotes-exit.png](shots/followup-find-quotes-exit.png), [evidence/followup-serial.log](evidence/followup-serial.log), [evidence/suite4-B064.txt](evidence/suite4-B064.txt), [shots/resume-external-filters.png](shots/resume-external-filters.png), [evidence/resume-external-serial.log](evidence/resume-external-serial.log).

### L018 - FIND stops its search string at the first embedded quote

Area: FIND quoting. Severity: P2.

Steps: Boot followup; CD \T; FIND "He said ""yes""" < QUOTES.TXT.

Observed: Both He said "yes" and He said no are printed, as if searching only for He said .

Expected and basis: Match the embedded quoted word, returning only the first line (DOS Reference pp.72-73/PDF157-158, quotation-mark handling).

Evidence: [shots/followup-find-quotes-exit.png](shots/followup-find-quotes-exit.png), [evidence/followup-serial.log](evidence/followup-serial.log), [evidence/special-fixtures/QUOTES.TXT](evidence/special-fixtures/QUOTES.TXT), [evidence/manual-page-157.txt](evidence/manual-page-157.txt), [evidence/manual-page-158.txt](evidence/manual-page-158.txt).

### L019 - FIND silently accepts an unknown switch

Area: FIND arguments. Severity: P3.

Steps: Boot followup; CD \T; FIND /BAD "ALPHA" < ONE.TXT.

Observed: It prints ALPHA with no invalid-switch diagnostic.

Expected and basis: Reject an unsupported switch with a useful error (tester expectation; valid switches listed on DOS p.72).

Evidence: [shots/followup-find-quotes-exit.png](shots/followup-find-quotes-exit.png), [evidence/followup-serial.log](evidence/followup-serial.log), [evidence/manual-page-157.txt](evidence/manual-page-157.txt).

### L020 - SORT /+n ignores the requested starting column

Area: SORT. Severity: P2.

Steps: Boot followup; CD \T; SORT /+4 < SORTCOL.TXT. The input is 01 zebra then 02 apple.

Observed: The output remains 01 zebra then 02 apple.

Expected and basis: Sort apple before zebra starting at column 4 (DOS Reference printed p.116/PDF201).

Evidence: [shots/followup-sort-column-cap.png](shots/followup-sort-column-cap.png), [evidence/followup-serial.log](evidence/followup-serial.log), [evidence/special-fixtures/SORTCOL.TXT](evidence/special-fixtures/SORTCOL.TXT), [evidence/manual-page-201.txt](evidence/manual-page-201.txt).

### L021 - TIME rejects hundredths of a second

Area: TIME. Severity: P2.

Steps: Boot suite3; run \CASES\B039.BAT, or TIME 13:14:15.99.

Observed: Invalid time; previous time remains unchanged.

Expected and basis: Accept the .hundredths field (DOS Reference p.120/PDF205). The narrower shipped Initech syntax does not promise it; this is DOS compatibility.

Evidence: [shots/suite3-B039-time-values.png](shots/suite3-B039-time-values.png), [evidence/suite3-B039.txt](evidence/suite3-B039.txt), [evidence/manual-page-205.txt](evidence/manual-page-205.txt).

### L022 - VERIFY is unavailable

Area: VERIFY. Severity: P2.

Steps: Boot stock or suite3; VERIFY; VERIFY ON; VERIFY OFF.

Observed: All forms say Bad command or file name.

Expected and basis: Provide the DOS write-verification switch and state query (DOS Reference p.125/PDF210). COPY /V is separately implemented.

Evidence: [shots/suite3-B042-builtin-gaps.png](shots/suite3-B042-builtin-gaps.png), [evidence/suite3-B042.txt](evidence/suite3-B042.txt), [evidence/manual-page-210.txt](evidence/manual-page-210.txt).

### L023 - ECHO. cannot emit a blank line

Area: ECHO. Severity: P2.

Steps: Boot suite3; run \CASES\B036.BAT, or type ECHO. at the prompt.

Observed: Bad command or file name.

Expected and basis: Output a blank line (tester DOS 3.x memory; no claim this shorthand is specified in the shipped manual).

Evidence: [shots/suite3-B036-echo.png](shots/suite3-B036-echo.png), [evidence/suite3-B036.txt](evidence/suite3-B036.txt).

### L024 - ECHO OFF at the prompt prints OFF and leaves echo on

Area: ECHO. Severity: P2.

Steps: Boot advanced-keyboard; ECHO OFF; ECHO; ECHO ON.

Observed: OFF is printed as ordinary text; the query still says ECHO is on. The batch form does change state.

Expected and basis: Change echo state at the prompt too (tester DOS memory; DOS Reference p.143 documents ON/OFF). The Initech manual documents a separate, narrower prompt behavior.

Evidence: [shots/advanced-keyboard-interactive-expansion.png](shots/advanced-keyboard-interactive-expansion.png), [evidence/advanced-keyboard-serial.log](evidence/advanced-keyboard-serial.log), [evidence/manual-page-228.txt](evidence/manual-page-228.txt).

### L025 - Percent-delimited variables are not expanded at the prompt

Area: Environment expansion. Severity: P2.

Steps: Boot advanced-keyboard; SET X=expanded; ECHO %X%.

Observed: The output is literally %X%.

Expected and basis: Expand the environment reference to expanded (tester DOS command-line memory). Initech documentation only promises expansion in batch files, so this is compatibility, not a false manual-contract claim.

Evidence: [shots/advanced-keyboard-interactive-expansion.png](shots/advanced-keyboard-interactive-expansion.png), [evidence/advanced-keyboard-serial.log](evidence/advanced-keyboard-serial.log).

### L026 - DIR includes hidden and system files in normal listings

Area: DIR. Severity: P2.

Steps: Boot suite2; run \CASES\B003.BAT. HIDDEN.TXT has +H and SYSTEM.TXT has +S, set by mattrib during fixture preparation.

Observed: DIR /W includes HIDDEN.TXT and SYSTEM.TXT; normal and .TXT listings do too.

Expected and basis: Omit hidden/system files from ordinary DOS 3.3 DIR output (original DOS Reference DIR pp.57-58; tester attribute expectation).

Evidence: [shots/suite2-B003-dir-wide.png](shots/suite2-B003-dir-wide.png), [shots/suite2-B001-dir-paths.png](shots/suite2-B001-dir-paths.png), [evidence/suite2-B001.txt](evidence/suite2-B001.txt), [evidence/suite2-B003.txt](evidence/suite2-B003.txt), [evidence/preparation.json](evidence/preparation.json).

### L027 - Protected-file redirection reports Bad command

Area: Redirection diagnostics. Severity: P3.

Steps: Boot suite2; CD \T; ECHO LOST > READONLY.TXT; ECHO LOST >> READONLY.TXT; TYPE READONLY.TXT.

Observed: Both attempts say Bad command or file name. The protected contents remain PROTECTED.

Expected and basis: Report Access denied for a protected destination (tester diagnostic expectation; pubs/manuals/initechdos-reference/ch2-entries-a.tex:227-228 uses that wording for COPY protection).

Evidence: [shots/suite2-B030-redir-readonly.png](shots/suite2-B030-redir-readonly.png), [evidence/suite2-B030.txt](evidence/suite2-B030.txt), [evidence/extracted/suite2/T/READONLY.TXT](evidence/extracted/suite2/T/READONLY.TXT).

### L028 - FDISK labels both mounted primary partitions C:

Area: FDISK display. Severity: P3.

Steps: Boot advanced-fdisk with geometry-boot-fixture.img; FDISK; 4; Enter.

Observed: Both partition rows start C:, though the second volume is mounted and accessible as D:. The corrected CHS fixture rules out the earlier bad-cylinder display.

Expected and basis: Show C: for the first and D: for the second mapped volume (tester consistency expectation; FDISK partition display).

Evidence: [shots/advanced-fdisk-partitions.png](shots/advanced-fdisk-partitions.png), [evidence/advanced-fdisk-serial.log](evidence/advanced-fdisk-serial.log), [evidence/prepare_more.py](evidence/prepare_more.py), [evidence/suite4-B056.txt](evidence/suite4-B056.txt).

### L029 - A malformed COM file halts the entire machine

Area: Program error handling. Severity: P1.

Steps: Boot prog-garbage using program-fixture.img; CD \T; GARBAGE.COM. Fixture bytes are 0F 0B (invalid opcode). Try ECHO ALIVE afterwards.

Observed: PC LOAD LETTER / ERROR 06 appears; serial reports PANIC vec=06 and HALTED. No prompt returns.

Expected and basis: Report the program failure and recover a usable shell (tester robustness expectation for this native hobby OS; not a promise that historical real-mode DOS isolated arbitrary programs). The panic wording itself is deliberate.

Evidence: [shots/prog-garbage-result.png](shots/prog-garbage-result.png), [evidence/prog-garbage-serial.log](evidence/prog-garbage-serial.log), [evidence/special-fixtures/T_GARBAGE.COM](evidence/special-fixtures/T_GARBAGE.COM), [evidence/prepare_special.py](evidence/prepare_special.py).

### L030 - An unsupported 16-bit MZ file halts without a screen diagnostic

Area: Program loader. Severity: P1.

Steps: Boot prog-mz16 using program-fixture.img; CD \T; DOS16.EXE. The crafted 32-byte MZ header and tiny 16-bit exit body are in prepare_special.py.

Observed: The screen stays on DOS16.EXE with no new prompt/message. Serial says foreign-mz: untagged 16-bit DOS .EXE and HALTED.

Expected and basis: Refuse the unsupported format visibly and return to the shell; the brief explicitly requires a loud failure rather than a hang. Executing 16-bit programs is not demanded.

Evidence: [shots/prog-mz16-result.png](shots/prog-mz16-result.png), [evidence/prog-mz16-serial.log](evidence/prog-mz16-serial.log), [evidence/special-fixtures/T_DOS16.EXE](evidence/special-fixtures/T_DOS16.EXE), [evidence/prepare_special.py](evidence/prepare_special.py).

### L031 - Ctrl-C during TYPE prints non-file bytes and does not stop output

Area: TYPE cancellation. Severity: P2.

Steps: Run python3 evidence/resume_driver.py controlshort. It boots geometry-boot-fixture.img plus resume-fixture.img, CD \T, TYPE HUGE.TXT, then sends Ctrl-C after HUGE LINE 00005. The fixture has 2000 ASCII-only lines (88000 bytes).

Observed: The screen fills with non-text characters. The serial capture contains 14863 NUL bytes although the input contains none. Output then reaches HUGE LINE 01999 and the shell accepts ECHO AFTER-CANCEL. The original 10000-line repeat was stopped after 125 seconds, still printing at line 09233. Ctrl-S separately holds the serial byte count steady and resumes correctly.

Expected and basis: Ctrl-C should stop TYPE and return a clean prompt; no bytes absent from the file should be printed (tester cancellation expectation and the brief; Initech Reference BREAK ch2-entries-a.tex:76 documents output-time break checking).

Evidence: [shots/resume-controlshort-ctrlc.png](shots/resume-controlshort-ctrlc.png), [shots/resume-controlshort-returned.png](shots/resume-controlshort-returned.png), [evidence/resume-controlshort-serial.log](evidence/resume-controlshort-serial.log), [evidence/resume-control-verification.json](evidence/resume-control-verification.json), [evidence/resume-fixtures/HUGE.TXT](evidence/resume-fixtures/HUGE.TXT), [evidence/resume_driver.py](evidence/resume_driver.py), [evidence/advanced-control-serial.log](evidence/advanced-control-serial.log).

### L032 - XCOPY targeting D: overwrites files on A:

Area: XCOPY drives. Severity: P1.

Steps: Run resume_driver.py xcopy. From A:\T: XCOPY A:\T\ONE.TXT D:\DHOME\ /V; DIR D:\DHOME; DIR A:\DHOME; TYPE A:\DHOME\ONE.TXT; TYPE D:\DHOME\ONE.TXT. Stop QEMU and extract both volumes. For the overwrite repeat, run resume_driver.py xcopyoverwrite: MD A:\DHOME; ECHO KEEP-A > A:\DHOME\ONE.TXT; repeat XCOPY A:\T\ONE.TXT D:\DHOME\ /V.

Observed: XCOPY reports 1 File(s) copied. D:\DHOME still contains only SAME.TXT. It instead creates A:\DHOME\ONE.TXT containing 41 4c 50 48 41 0d 0a (ALPHA plus CR/LF). A source qualified with D: also yields zero copies. The stopped-image mcopy return codes are 0 for the A: file and 1 (not found) for the D: file. In the repeat, the unrelated A: sentinel changes from KEEP-A to ALPHA while no requested D: copy appears.

Expected and basis: Resolve source and destination drive letters independently and write only to the requested destination (DOS XCOPY; pubs/manuals/initechdos-reference/ch2-entries-c.tex:216-229 accepts drive-qualified source and target). Exact target contract, pubs/manuals/initechdos-reference/ch2-entries-c.tex:225: "The second names the target."

Evidence: [shots/resume-xcopy-location.png](shots/resume-xcopy-location.png), [shots/resume-xcopy-wrong-volume.png](shots/resume-xcopy-wrong-volume.png), [evidence/resume-xcopy-serial.log](evidence/resume-xcopy-serial.log), [evidence/resume-xcopy-verification.json](evidence/resume-xcopy-verification.json), [evidence/resume-xcopy-A-ONE.TXT](evidence/resume-xcopy-A-ONE.TXT), [evidence/resume-xcopy-D-listing.txt](evidence/resume-xcopy-D-listing.txt), [evidence/resume_driver.py](evidence/resume_driver.py), [shots/resume-xcopyoverwrite-overwritten.png](shots/resume-xcopyoverwrite-overwritten.png), [evidence/resume-xcopyoverwrite-serial.log](evidence/resume-xcopyoverwrite-serial.log), [evidence/resume-final-verification.json](evidence/resume-final-verification.json).

### L033 - Invoking a batch without CALL returns to the caller anyway

Area: Batch chaining. Severity: P2.

Steps: Run resume_driver.py batch. From A:\T run CHAIN.BAT; it prints BEFORE, invokes \CASES\CHILD.BAT arg without CALL, then has ECHO SHOULD-NOT-RETURN.

Observed: CHILD=arg is followed by SHOULD-NOT-RETURN. The older followup run likewise prints CHAIN-AFTER.

Expected and basis: A plain batch invocation transfers control; only CALL resumes the caller (DOS Reference CALL p.142/PDF227).

Evidence: [shots/resume-batch-flow.png](shots/resume-batch-flow.png), [evidence/resume-batch-serial.log](evidence/resume-batch-serial.log), [evidence/resume-fixtures/CHAIN.BAT](evidence/resume-fixtures/CHAIN.BAT), [evidence/cases/CHILD.BAT](evidence/cases/CHILD.BAT), [evidence/manual-page-227.txt](evidence/manual-page-227.txt).

### L034 - Ctrl-C ends a batch without the DOS termination question

Area: Batch cancellation. Severity: P2.

Steps: Run resume_driver.py batch; \CASES\BREAK.BAT waits at PAUSE. Press Ctrl-C. Repeat with LOOP.BAT, which repeatedly uses ECHO and GOTO.

Observed: Both immediately return to the DOS prompt after ^C. There is no Terminate batch job (Y/N)? and no option to continue. The PAUSE remainder is not executed.

Expected and basis: Ask Terminate batch job (Y/N)? and allow N to continue (DOS Reference p.149/PDF234). pubs/manuals/initechdos-reference/ch4-batch.tex:38-39 documents immediate cancellation, so this is an explicit compatibility gap.

Evidence: [shots/resume-batch-pause-break.png](shots/resume-batch-pause-break.png), [shots/resume-batch-loop-break.png](shots/resume-batch-loop-break.png), [evidence/resume-batch-serial.log](evidence/resume-batch-serial.log), [evidence/cases/BREAK.BAT](evidence/cases/BREAK.BAT), [evidence/resume-fixtures/LOOP.BAT](evidence/resume-fixtures/LOOP.BAT), [evidence/resume-manual-page-234.txt](evidence/resume-manual-page-234.txt).

### L035 - DEL directory refuses the documented delete-all-files form

Area: DEL directory operand. Severity: P2.

Steps: Boot suite2; run \CASES\B023.BAT: MD DELDIR; COPY ONE.TXT DELDIR\ONE.TXT; DEL DELDIR; DIR DELDIR.

Observed: DEL reports Access denied; ONE.TXT remains in the directory.

Expected and basis: Treat the directory name as its contained files and apply the delete-all confirmation, leaving the directory itself (DOS Reference p.56/PDF141 explicitly says to give a directory name to delete its files).

Evidence: [shots/suite2-B023-del-directory.png](shots/suite2-B023-del-directory.png), [evidence/suite2-B023.txt](evidence/suite2-B023.txt), [evidence/extracted/suite2/T/DELDIR/ONE.TXT](evidence/extracted/suite2/T/DELDIR/ONE.TXT), [evidence/manual-page-141.txt](evidence/manual-page-141.txt).

### L036 - DATE without an argument does not offer date entry

Area: DATE. Severity: P2.

Steps: Boot suite3; run \CASES\B038.BAT or type DATE.

Observed: Only Current date is ... is displayed; execution proceeds to the next command. Startup without AUTOEXEC does have the date question.

Expected and basis: Offer Enter new date (mm-dd-yy): and let Enter retain it (DOS Reference p.55/PDF140). The Initech manual describes its display-only behavior, so this is compatibility.

Evidence: [shots/suite3-B038-date-formats.png](shots/suite3-B038-date-formats.png), [evidence/suite3-B038.txt](evidence/suite3-B038.txt), [evidence/manual-page-140.txt](evidence/manual-page-140.txt).

### L037 - TIME without an argument does not offer time entry

Area: TIME. Severity: P2.

Steps: Boot suite3; run \CASES\B039.BAT or type TIME.

Observed: Only Current time is ... is displayed; the shell continues without an entry question. Startup without AUTOEXEC does have a time prompt.

Expected and basis: Prompt for a new time when no argument is supplied (DOS Reference p.121/PDF206). The Initech manual describes display-only TIME; this is compatibility.

Evidence: [shots/suite3-B039-time-values.png](shots/suite3-B039-time-values.png), [evidence/suite3-B039.txt](evidence/suite3-B039.txt), [evidence/manual-page-206.txt](evidence/manual-page-206.txt).

### L038 - F3 recalls the line that Ctrl-C cancelled

Area: Keyboard template. Severity: P2.

Steps: Run resume_driver.py keyboard. ECHO TEMPLATE; then type ECHO CANCELED without Enter. Press Ctrl-C, then F3, Enter.

Observed: F3 restores ECHO CANCELED and executes it, replacing the preceding completed template.

Expected and basis: Discard the cancelled line and keep the last completed command as the template. pubs/manuals/initechdos-users-guide/ch8-keys.tex:25 says "The completed line becomes the template"; lines 32-33 say InitechDOS "discards" the cancelled line. The earlier completed template is ECHO XEMPLATE in the full replay.

Evidence: [shots/resume-keyboard-cancel-template.png](shots/resume-keyboard-cancel-template.png), [evidence/resume-keyboard-serial.log](evidence/resume-keyboard-serial.log), [evidence/resume_driver.py](evidence/resume_driver.py).

### L039 - MORE pauses without displaying --More--

Area: MORE display. Severity: P3.

Steps: Run resume_driver.py external. MORE < PAGES.TXT; wait; press Enter; wait; press Ctrl-C. PAGES.TXT has 65 numbered CR/LF lines.

Observed: The first pause is after PAGE LINE 023, the second after 047. Both are silent, with no --More-- indicator. The serial byte count stays fixed while paused. Ctrl-C returns to the shell.

Expected and basis: Show --More-- at a page break (DOS Reference p.20/PDF105 explicitly describes the bottom-of-screen indicator).

Evidence: [shots/resume-external-more-page1.png](shots/resume-external-more-page1.png), [shots/resume-external-more-page2.png](shots/resume-external-more-page2.png), [evidence/resume-external-serial.log](evidence/resume-external-serial.log), [evidence/resume-fixtures/PAGES.TXT](evidence/resume-fixtures/PAGES.TXT), [evidence/resume-manual-page-105.txt](evidence/resume-manual-page-105.txt).

### L040 - The shipped 123.EXE and WORD.EXE cannot run from DOS

Area: Native program launch. Severity: P1.

Steps: Run resume_driver.py app123 and appword in separate sessions. CD \APPS; DIR 123.EXE; 123.EXE; A:\APPS\123.EXE. Repeat with WORD.EXE.

Observed: Both present native InitechMZ files are rejected with Bad command or file name. DIR and stopped-image extraction prove the files exist (64892 and 52472 bytes). The prompt survives. SAMIR.COM, the 65536-byte native COM and the small exit programs run.

Expected and basis: Run the shipped applications from their explicit DOS paths, or give an accurate unsupported/memory diagnostic (the brief's Programs test and tester workstation expectation). This concerns the supplied native applications, separately from the foreign 16-bit MZ refusal in L030.

Evidence: [shots/resume-app123-qualified.png](shots/resume-app123-qualified.png), [shots/resume-appword-qualified.png](shots/resume-appword-qualified.png), [evidence/resume-app123-serial.log](evidence/resume-app123-serial.log), [evidence/resume-appword-serial.log](evidence/resume-appword-serial.log), [evidence/recovery-extra-verification.json](evidence/recovery-extra-verification.json), [evidence/appsamir-serial.log](evidence/appsamir-serial.log).

### L041 - FORMAT /S cannot transfer the system from the supplied boot volume

Area: FORMAT system install. Severity: P2.

Steps: Run resume_driver.py format: FORMAT D: /S; DIR D:\. The boot volume C: is the supplied tracer image partition, with IO.SYS and INITDOS.SYS.

Observed: FORMAT immediately reports Insufficient memory for system transfer. The D: contents remain intact. The source C: root has no COMMAND.COM file. Ordinary FORMAT D: followed by SYS D: succeeds when the target root entries are free.

Expected and basis: Provide a usable shipped /S installation path or accurately identify the missing source file. pubs/manuals/initechdos-reference/ch2-entries-b.tex:207-209 promises that /S "copies the system files and COMMAND.COM from drive C" and the result can start the system. Missing COMMAND.COM is a suspected cause, not a proven exclusive cause.

Evidence: [shots/resume-format-system-before.png](shots/resume-format-system-before.png), [evidence/resume-format-serial.log](evidence/resume-format-serial.log), [evidence/advanced-format-C-listing.txt](evidence/advanced-format-C-listing.txt), [evidence/resume-format-verification.json](evidence/resume-format-verification.json).

### L042 - A write after FORMAT and SYS corrupts the transferred system files

Area: Filesystem after disk utilities. Severity: P1.

Steps: Run resume_driver.py syswrite. FORMAT D:; answer EXTRA, Y. SYS D:; DIR D:\; ECHO NEW-DATA > D:\NEW.TXT; TYPE D:\NEW.TXT; DIR D:\. Stop QEMU and extract D: from byte 2097152 of resume-syswrite-boot.img.

Observed: SYS reports System transferred. DIR lists IO.SYS=1647 and INITDOS.SYS=196608 but claims 1456640 bytes free, already wrong. After the ten-byte NEW.TXT write, mcopy can recover only 512 bytes of IO.SYS; INITDOS.SYS cannot be read. fsck.fat -n reports a free cluster in INITDOS.SYS and shortened chains. A separate control stopped after SYS has both files byte-identical to the original C: files and 1259008 bytes free. No error is shown by the guest.

Expected and basis: Keep filesystem allocation state coherent after raw disk utilities; later ordinary writes must preserve the installed system files (tester filesystem correctness expectation; InitechDOS SYS contract ch2-entries-c.tex:74-77). Stale mounted FAT state is a root-cause guess; corruption is established by the extracted bytes and independent checker.

Evidence: [shots/resume-syswrite-after-write.png](shots/resume-syswrite-after-write.png), [evidence/resume-syswrite-serial.log](evidence/resume-syswrite-serial.log), [evidence/resume-syswrite-verification.json](evidence/resume-syswrite-verification.json), [evidence/resume-syswrite-fsck.txt](evidence/resume-syswrite-fsck.txt), [evidence/resume-syswrite-IO.SYS](evidence/resume-syswrite-IO.SYS), [evidence/resume-syswrite-INITDOS.SYS](evidence/resume-syswrite-INITDOS.SYS), [evidence/resume-format-verification.json](evidence/resume-format-verification.json), [evidence/resume-format-D-IO.SYS](evidence/resume-format-D-IO.SYS), [evidence/resume-format-D-INITDOS.SYS](evidence/resume-format-D-INITDOS.SYS), [evidence/resume_driver.py](evidence/resume_driver.py).

### L043 - Ctrl-C advances DIR /P instead of cancelling the listing

Area: DIR cancellation. Severity: P2.

Steps: Run resume_driver.py gaps2. DIR \MANY /P; at the first Press any key to continue prompt, press Ctrl-C. Continue with Space until all 300 files are listed.

Observed: Ctrl-C advances to the next page and another pause. The listing runs through F299.TXT. A final Ctrl-C at the shell only clears surplus queued spaces.

Expected and basis: Abort the active command on Ctrl-C (DOS editing-keys Reference p.159/PDF244; tester DIR expectation).

Evidence: [shots/resume-gaps2-dir-break.png](shots/resume-gaps2-dir-break.png), [shots/resume-gaps2-dir-returned.png](shots/resume-gaps2-dir-returned.png), [evidence/resume-gaps2-serial.log](evidence/resume-gaps2-serial.log), [evidence/manual-page-244.txt](evidence/manual-page-244.txt).

### L044 - SORT silently accepts an unknown switch

Area: SORT arguments. Severity: P3.

Steps: Run resume_driver.py external: SORT /BAD < LINES.TXT; IF NOT ERRORLEVEL 1 ECHO SORT-RC0.

Observed: It sorts normally and prints SORT-RC0; no invalid-switch diagnostic appears.

Expected and basis: Reject unknown switches so mistyped options are visible (tester diagnostic expectation; DOS Reference p.116 lists /R and /+n).

Evidence: [shots/resume-external-filters.png](shots/resume-external-filters.png), [evidence/resume-external-serial.log](evidence/resume-external-serial.log), [evidence/manual-page-201.txt](evidence/manual-page-201.txt).

### L045 - An undefined batch environment variable is left as literal text

Area: Batch expansion. Severity: P2.

Steps: Run resume_driver.py keyboard on a fresh fixture where TEST is unset; run BENV.BAT, which contains ECHO VALUE=%TEST%. As a control, resume_driver.py batch sets TEST=VALUE first.

Observed: The unset case prints VALUE=%TEST%; the defined case prints VALUE=VALUE.

Expected and basis: Expand an undefined environment variable to the empty string (tester DOS batch-expansion memory). Preserve the ordinary literal-percent escape %% separately, which passes here.

Evidence: [shots/resume-keyboard-recall-kinds.png](shots/resume-keyboard-recall-kinds.png), [shots/resume-batch-flow.png](shots/resume-batch-flow.png), [evidence/resume-keyboard-serial.log](evidence/resume-keyboard-serial.log), [evidence/resume-batch-serial.log](evidence/resume-batch-serial.log), [evidence/resume-fixtures/BENV.BAT](evidence/resume-fixtures/BENV.BAT).

### L046 - BREAK INVALID is treated as a state query

Area: BREAK arguments. Severity: P3.

Steps: Run resume_driver.py gaps2: BREAK OFF; BREAK; BREAK ON; BREAK; BREAK INVALID.

Observed: Valid ON/OFF forms work. INVALID produces BREAK is on rather than an argument error.

Expected and basis: Reject unsupported settings (tester command-diagnostic expectation; DOS BREAK syntax allows ON/OFF or no operand).

Evidence: [shots/resume-gaps2-builtins.png](shots/resume-gaps2-builtins.png), [evidence/resume-gaps2-serial.log](evidence/resume-gaps2-serial.log), [evidence/suite3-B042.txt](evidence/suite3-B042.txt).

### L047 - PROMPT $V prints V instead of the DOS version

Area: PROMPT compatibility. Severity: P2.

Steps: Run resume_driver.py promptversion; type PROMPT $V$G.

Observed: The prompt becomes V>, rather than showing the version number.

Expected and basis: Expand $V to the version number (original DOS Reference printed p.101/PDF186, Special prompts table). The shipped Initech prompt table omits this DOS code, so this is compatibility.

Evidence: [shots/resume-promptversion-version.png](shots/resume-promptversion-version.png), [evidence/resume-promptversion-serial.log](evidence/resume-promptversion-serial.log), [evidence/manual-page-186.txt](evidence/manual-page-186.txt).

## Still present

IDs in this section retain their original audit namespace. They are not new L-numbers in this report.

- **2026-10-15-look-pass6 M014**: the DOS console still uses pale gray text on green. See [stock DIR](shots/stock-dir.png) and [new keyboard capture](shots/resume-keyboard-cancel-template.png). This repeats the prior console finding; the deliberate teal desktop is outside this claim.
- **2026-10-05-astra-full-drive L030, partial**: ordinary new files now have valid timestamps, but CHKDSK's recovered FILE0000.CHK and FILE0001.CHK still show `0-00-80 12:00a`. See [rescan](shots/re-dirty-rescan.png), [serial](evidence/re-dirty-serial.log), and [mtools listing](evidence/re-dirty-mdir.txt). Do not close the entire timestamp finding on the strength of ordinary COPY/ECHO tests.
- **2026-10-05-astra-full-drive L081-L086, L088-L093, L095-L103, L106**: APPEND, ASSIGN, BACKUP, CHCP, COMP, CTTY, EXE2BIN, FASTOPEN, FC, GRAPHICS, JOIN, KEYB, MODE, NLSFUNC, PRINT, RECOVER, REPLACE, RESTORE, SELECT, SHARE, SUBST and DEBUG remain unavailable. Each command was actually invoked: [B059 transcript](evidence/suite4-B059.txt), with CTTY in [B042](evidence/suite3-B042.txt). Their absence is not refiled as 22 new findings.

## Now fixed in the tested forms

- **2026-10-04-flair-gui-codex-pass5 K01-K04**: self-COPY through relative/absolute spellings preserves SELF.BIN; COPY to a directory preserves the directory and its sentinel; path-qualified wildcard DEL removes only its intended files; deleting 300 matches completes. [B009](evidence/suite2-B009.txt), [B011](evidence/suite2-B011.txt), [B022](evidence/suite2-B022.txt), [B067](evidence/suite4-B067.txt), [extracted metadata and hashes](evidence/suite2-filesystem.json).
- **Same audit K10, K11**: DIR paths/wildcards/location headers work; COPY output masks, REN masks and batch FOR wildcard expansion work. [B001](evidence/suite2-B001.txt), [B016](evidence/suite2-B016.txt), [B024](evidence/suite2-B024.txt), [B061](evidence/suite4-B061.txt).
- **Same audit K12, K15**: Ctrl-C cancels prompt input, Ctrl-Z ends COPY CON, and DEL *.* asks for confirmation and honors both answers. [new keyboard log](evidence/resume-keyboard-serial.log), [COPY CON extracted bytes](evidence/resume-final-verification.json), [interactive log](evidence/interactive-serial.log). TYPE output cancellation remains a distinct new problem, L031.
- **2026-10-05-astra-full-drive L005, L006**: COPY and DEL preserve read-only files. [B010](evidence/suite2-B010.txt); the deeper ATTRIB /S repeat also protects against both operations: [recursive transcript](evidence/resume-attribrecursive-serial.log).
- **Same audit L025-L029**: CD backslash returns to root; MD refuses wildcard and reserved device names; COPY /A and /B parse as switches; explicit plus concatenation works. [B006](evidence/suite2-B006.txt), [B007](evidence/suite2-B007.txt), [B013](evidence/suite2-B013.txt), [B014](evidence/suite2-B014.txt), [prefix-switch repeat](evidence/resume-copyedge-serial.log). The timestamp finding L030 is only partly fixed, as described above.
- **Same audit L031-L035**: interactive FOR and IF work; normal F3 recall works; COMMAND /C executes commands, including nested /C; permanent-shell EXIT leaves a usable prompt. [batch/interactive controls](evidence/resume-batch-serial.log), [keyboard](evidence/resume-keyboard-serial.log), [nested /C](evidence/followup-serial.log), [EXIT/VER](evidence/resume-gaps2-serial.log). Plain interactive COMMAND and cancelled-line recall have narrower new defects, L009 and L038.
- **Same audit L036-L043, L087, L094, L104, L105, L107**: CHKDSK, FORMAT, DISKCOPY, ATTRIB, TREE, FIND, SORT, MORE, DISKCOMP, LABEL, SYS, XCOPY and EDLIN are now installed and callable from PATH. This closes only the old availability claims, not the functional defects above. [stock PATH checks](evidence/stock-serial.log), [same-disk utilities](evidence/advanced-disktools-serial.log), [format/SYS](evidence/resume-format-serial.log), [editor](evidence/advanced-editor-serial.log).
- **2026-10-15-look-pass6 M015-M020, M023**: the prompt cursor blinks; CLS homes to the top; DIR shows volume, split name/extension columns, timestamps, free space and capitalized File(s). [stock DIR](shots/stock-dir.png), [stock CLS](shots/stock-cls.png), [cursor frame 0](shots/resume-gaps2-cursor-0.png), [cursor frame 1](shots/resume-gaps2-cursor-1.png), [six-frame comparison](evidence/resume-final-verification.json). Only the underline region x=40..47, y=14..15 changes in the new A:\T> sample.

The earlier G11 stuck-modifier finding was for the FLAIR input path. Both Shift keys, CapsLock and Ctrl release behaved correctly in this DOS run, but the FLAIR reproduction was not repeated, so G11 is not declared fixed here.

## Coverage, replay and limits

The original audit's complete per-keystroke journal is [actions.jsonl](evidence/actions.jsonl). Its [recovered narration](evidence/recovered-narration.txt) includes a correction to an early mistaken COMMAND /C claim. The fixtures and scripts are the executable replay specification; every session JSON records QEMU arguments, copied-image hashes and shutdown status. Run scripts from this directory, one at a time. Do not regenerate or overwrite evidence images while inspecting them.

| Area | Actual coverage and evidence |
| --- | --- |
| Baseline and look | Stock tracer + flair_data, stock fat_data, VER/VOL/CLS, DOS prompt, font/columns/colors and blinking cursor. The original manual's printed examples establish wording/layout, not exact RGB. Only the already-known console-color issue is claimed. |
| DIR and paths | /W, /P, wildcards, .TXT, file and nonexistent paths, trailing slash, 300 files, 16/32/64-entry directories, drive-relative paths, root and .., CD on another drive without changing selection. [B001-B008](evidence/cases.json), [pagination cancellation](evidence/resume-gaps2-serial.log). |
| File commands | Same-file COPY spellings, default and explicit targets, plus lists including a target also present later in the list, missing-source failure, wildcards/output masks, /A /B /V including prefix switches, CON input/output, empty files, binary EOF byte, 1980/2099 timestamp preservation, read-only source/destination, full disks; DEL confirmation, directory operands and 300 matches; REN masks, collision, cross-directory rejection and read-only file rename. [suite2](evidence/suite2-results.json), [byte/metadata extraction](evidence/suite2-filesystem.json), [full disks](evidence/storage-verification.json). |
| Shell state | SET creation/query/removal/case and 512-byte exhaustion; PATH clearing and a successful third-directory search; PROMPT all DOS codes, ANSI escape, unknown code and reset; BREAK ON/OFF/invalid; VERIFY and CTTY; EXIT; DATE leap years, range failures, hyphens/slashes/periods, trailing operands and no operand; TIME bounds, seconds, hundredths, suffix and no operand. [suite3](evidence/suite3-results.json), [followup](evidence/followup-serial.log), [PROMPT sweep](evidence/resume-promptcodes-serial.log). |
| Redirection and filters | Byte-exact TYPE > for binary/empty/no-newline/76800-byte inputs; >> append; nonexistent-command redirection; input files; two/three-stage pipes; MORE pagination/cancellation/passthrough; FIND /V /C /N and combined switches, quoted text, filename operand, no match, invalid input, exit status; SORT ascending/reverse/column switch, binary and over-capacity input. [B028-B035](evidence/cases.json), [external repeat](evidence/resume-external-serial.log). |
| Batch files | AUTOEXEC PATH; ECHO, @, REM, PAUSE, GOTO/missing label/labels with spaces; IF ERRORLEVEL, ==, EXIST, NOT and FOR with literals/wildcards/nested IF; %% and defined/undefined environment expansion; %0..%9 and SHIFT beyond nine; CALL, nesting, self-recursion, chaining without CALL, long file truncation, Ctrl-C at PAUSE and in an ECHO/GOTO loop, batch on D:. [B060-B065](evidence/suite4-results.json), [resumed batch cases](evidence/resume-batch-serial.log). Recursion fails with Insufficient memory and unwinds rather than hanging. |
| Attributes, labels and XCOPY/TREE | ATTRIB +R/+A, removal, listing and /S through a nested child; +H/+S rejection tested; existing hidden/system fixtures; LABEL empty/delete N/Y, 11/12 characters, invalid character and redirected input; XCOPY /S /E /P /V /W, empty/nonempty children, target F/D question, read-only preservation, same-file/cyclic rejection, current-directory resolution and cross-drive writes; TREE /F, 12-level tree, absent/file path and invalid switch. [suite3/4](evidence/cases.json), [recursive attributes](evidence/resume-attribrecursive-serial.log), [XCOPY repeat](evidence/resume-xcopy-serial.log). |
| CHKDSK and disk utilities | Clean A:, file-contiguity query, bad option, crafted cross-link and two lost chains; /F recovery Y and no-fix N; extracted .CHK data. FORMAT A: refusal, D: invalid switch, wrong label, N preservation, /V, /S, fixed-disk /B rejection, C: boot-volume cancellation and actual formatting of a disposable copy. SYS occupied-root refusal and success on an empty root, followed by a write. DISKCOMP A:A and /1 /8, DISKCOPY A:A, and nonremovable C:D rejection; same-disk operations leave image hashes unchanged. [dirty verification](evidence/storage-verification.json), [disktools](evidence/advanced-disktools-serial.log), [format](evidence/resume-format-serial.log), [SYS corruption](evidence/resume-syswrite-verification.json). |
| FDISK | Display, next-drive/running-volume refusal, active menu/cancel, actual active-partition change, create-existing refusal, delete N, actual delete, maximum-size create and restart question. MBR cancel remains byte-identical; active flags move to partition 2; recreation yields an active partition at LBA63 with 8001 sectors. [menu replay](evidence/resume-fdiskmenus-serial.log), [create](evidence/resume-fdiskcreate-serial.log), [active](evidence/resume-fdiskactive-serial.log), [raw table verification](evidence/resume-final-verification.json). |
| EDLIN | Create/open, no CR/LF, 64 KB windowed editing, line edit, L/I/D/E/Q, ranges and bad range, .BAK refusal, C/M/S/R/T/P/W/A, backup preservation, Q=N/Y. Ctrl-C correctly ends Insert; Ctrl-Z is literal there, as the original DOS manual p.180 says. The saved 64000-byte large file is preserved plus the normal final Ctrl-Z. [interactive](evidence/interactive-serial.log), [advanced](evidence/advanced-editor-serial.log), [saved files](evidence/advanced-editor-filesystem.json), [EOF check](evidence/resume-final-verification.json). |
| Keyboard | F1/F2/F3/F4/F5, Insert/Delete template behavior, Esc, Backspace at start, both Shift keys, CapsLock with Shift, Ctrl-C mid-line, Ctrl-Z COPY CON, recall after builtin/external/batch/error/redirection/blank/cancelled lines, 127/128 characters, Ctrl-S, 20-key type-ahead. Twenty individually delivered keys during TYPE all survive. [keyboard](evidence/resume-keyboard-serial.log), [paced type-ahead](evidence/resume-typeaheadpaced-serial.log). |
| Programs | SAMIR.COM reaches its dot prompt and cancels; supplied 123/WORD native EXEs, 16-bit MZ, invalid-opcode COM, empty COM, 65536-byte native COM, missing names in current/path-qualified forms, PATH search, COMMAND /C nesting, environment isolation, exit statuses. Empty COM is rejected; 64 KB COM exits cleanly. [program fixtures](evidence/prepare_special.py), [SAMIR](evidence/appsamir-serial.log), [native launch evidence](evidence/recovery-extra-verification.json). |
| Filesystem | Exactly one cluster and one byte beyond it for FAT12 cluster sizes 2/4/8/16/32/64 sectors; COPY/rename/delete/subdir/TYPE bytes verified independently; actual 64-entry BPB root full plus standard 224-entry root full; last free cluster 2848 used by a six-byte write, then full-disk refusal; FAT16 long cross-FAT-window reads; zero-sector-per-cluster BPB and no-FAT disk rejected while C: remains usable; deletion while a handle is open. [storage verification](evidence/storage-verification.json), [true 64-entry root](evidence/resume-root64real-serial.log), [last-cluster and root metadata](evidence/resume-final-verification.json). |

### Outcomes deliberately not filed as new defects

- EDLIN Insert mode ending on Ctrl-C agrees with the original DOS manual (printed p.180). The first resumed external script waited for an asterisk after Ctrl-Z and timed out; the separate edlineof run corrected that test expectation and completed the save.
- SORT's 76800-byte input produces `Insufficient memory` (21 bytes in SORTLONG.TXT), not silently truncated sorted data. The 600-line/6000-byte file sorts in full. Binary text-filter newline normalization is not called byte corruption.
- FAT16 reading works; writes are explicitly unsupported in docs/worklog/WL-0156-dos-volumes.md:36, "FAT16 remains read-only." This is recorded as a product limitation, not a newly discovered implementation defect.
- ATTRIB +H/+S rejection and LABEL truncation after 11 characters match the shipped narrow contracts; original DOS 3.3 ATTRIB's advertised attribute changes are R/A. LABEL's other-drive limitation is separately listed as compatibility finding L015.
- CHKDSK identifies cross-linked files and recovers lost chains; it does not automatically separate the cross-linked files. The existing cross-link after /F is not attributed to the checker. `fsck.fat -n` was read-only throughout.
- SYS on a root with an occupied first slot correctly refuses. The clean-root rerun proves system transfer itself can succeed; L042 covers the subsequent corruption.
- FORMAT C: erases the disposable boot partition only after its explicit label and Y confirmation; the current A: session continues. The loss of those intentionally formatted files is not a bug.
- The original `root64-fixture.img` had 64 files in a 224-entry root, not a 64-entry BPB. The resumed root64real fixture corrects that coverage gap.
- A single QMP packet containing 40 key events delivered only eight letters. Because QEMU/PS2 delivery can be the limit, that result is excluded as a guest defect. Twenty explicit down/up pairs at 50 ms per key all arrive. No claim is made that an exactly 16-key BIOS queue was measured.
- Early interactive screenshots sometimes lagged the serial stream and had shifted punctuation from the old input method. They are not used to assert the later keyboard, cancellation, MORE or ANSI outcomes. New captures wait after explicit down/up events. The first long Ctrl-C run was stopped at the test's 125-second bound; the shorter isolated run reaches the actual prompt and proves the result.

### What could not be reached or was not measured

- Unequal removable-media DISKCOMP/DISKCOPY and actual disk swapping: B: is absent; C:/D: are reported non-removable; the supplied A: is a fixed IDE attachment. Same-A and rejection paths were tested. No image was edited underneath a running emulator to simulate a swap.
- Successful FORMAT /S installation is blocked by L041. SYS's clean-target transfer was tested independently. Rebooting the fresh FDISK partition was not used as a filesystem test: it has just been repartitioned and has not been formatted or populated. The intended MBR mutations and restart prompts were checked on disk.
- An interactive nested COMMAND prompt and EXIT from it are blocked by L009. Nested COMMAND /C and parent-environment isolation were tested.
- Native 123/WORD runtime behavior is blocked at DOS launch by L040; their GUI workflows belong to the other audit lanes. Foreign 16-bit execution is outside the intended native ABI; only refusal quality was tested.
- No physical disk removal, real printer, alternate national keyboard/code-page package, or audible PC-speaker recording. Beep behavior was not assessed through silent QEMU. There is no pixel-perfect VGA font reference claim from a scanned manual.
- Full historical DOS implementations of the still-missing utilities above, FAT16 writes, and non-US DATE formats cannot be exercised in this supplied build. These are explicit coverage limits, not passing tests.

## Fifteen fixes to make first

1. **L042** - Keep mounted allocation state coherent after FORMAT/SYS so the next write cannot damage the installed system.
2. **L032** - Honor XCOPY's drive letters and preserve unrelated A: files.
3. **L001** - Allocate pipe temporary files without clobbering existing user files.
4. **L002** - Report full-disk redirection failure before output is silently lost.
5. **L003** - Propagate pipe-intermediate write failure instead of returning empty success.
6. **L030** - Refuse a foreign MZ visibly and return to a usable shell.
7. **L029** - Recover a usable prompt after a native program faults.
8. **L040** - Make the supplied native applications launchable from DOS, with accurate failure diagnostics.
9. **L031** - Make Ctrl-C stop TYPE cleanly without printing non-file bytes.
10. **L005** - Execute complete batch files or reject oversized scripts explicitly.
11. **L013** - Make CHKDSK work on mounted C:/D: volumes.
12. **L012** - Make ATTRIB work on other mounted drives.
13. **L014** - Make TREE use the requested drive and directory.
14. **L006** - Preserve arguments beyond %9 through SHIFT.
15. **L017** - Make FIND's result status usable in batch decisions.

## Evidence and cleanup validation

All cited finding screenshots were visually reviewed, individually or at native size in the saved review contact sheets. The report contains only ASCII. Numbering, required fields, NITS rows and local evidence links were checked by [validate_report.py](evidence/validate_report.py); results are in [report-validation.json](evidence/report-validation.json).

All resumed QEMU instances were ended through their own QMP connection and have a recorded stopped_utc and zero host process exit status. The corrupt guest disk and the deliberately halting guest programs are preserved as evidence; a zero QEMU exit status does not imply a passing guest test. [cleanup journal](evidence/cleanup.jsonl), [final stopped-session and image checks](evidence/resume-final-verification.json), [SYS D: corruption check](evidence/resume-syswrite-fsck.txt).

No project source, build artifact in the repository, tracker or reference file was edited. No display :0 was opened or captured. Only the auditor's copied images and local evidence were written; at most one auditor-owned emulator ran at a time.
