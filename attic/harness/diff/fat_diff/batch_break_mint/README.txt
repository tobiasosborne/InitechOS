Independent real MS-DOS 3.30 batch interruption mint (read-only reference media).
Reference media:
/home/tobias/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img
Existing mint pattern: harness/diff/fat_diff/chkdsk_label_mint.sh.
Hashes: media.sha256, command.sha256. DOS VER output: version.raw.

Fixtures AUTOEXEC.BAT, PARENT.BAT, CHILD.BAT use CRLF.
AUTOEXEC CALLs PARENT, PARENT CALLs CHILD, CHILD PAUSEs.
Every returning batch writes a distinct output file; AUTOEXEC ends with PAUSE.
Copies yes2/boot.img, no2/boot.img, invalid/boot.img are disposable.
Reference media was never modified.

Built with:
cc -std=c99 -O2 build/cx_doscmd_reference/drive.c -o build/cx_doscmd_reference/drive
QEMU 8.2.2, -cpu 486 -m 16, deterministic 1992-03-02 RTC, boot floppy.
The C driver uses HMP sendkey and pmemsave of physical VGA text memory.
Use absolute output directory and boot image arguments, then 'sendkey y',
'sendkey n' or 'sendkey x'. Each run first waits for PAUSE, sends Ctrl-C,
then waits for the exact termination question before sending the answer.

Observed results:
yes2/screen-00.txt: CHILD's PAUSE.
yes2/screen-01.txt: Ctrl-C produces Terminate batch job (Y/N)?
yes2/screen-02.txt: lowercase y returns directly to A>.
  No CHILD.OUT, PARENT.OUT or AUTO.OUT; every active batch frame cancelled.
no2/screen-02.txt: lowercase n executes CHILD_AFTER then PARENT_AFTER,
  writes all three outputs, and reaches AUTOEXEC's later PAUSE.
  no2/CHILD.OUT, no2/PARENT.OUT and no2/AUTO.OUT are the extracted files.
invalid/screen-02.txt: lowercase x echoes, then prints the question again;
  no output sentinel is executed before a valid answer.

Manual scan:
pause-pdf234.png is the full PAUSE page, MS-DOS 3.3 User's Guide,
printed Reference p.149 / PDF p.234. Direct visual inspection confirms:
Terminate batch job (Y/N)?
The OCR artifact '(YIN)' is not the printed text.

Durable reproduction (from repository root):
  sh harness/diff/fat_diff/batch_break_mint/mint.sh
Optional DOS33_BOOT selects the original DOS 3.30 floppy. mint.c is a C-only
HMP/VGA driver, and the three CRLF .BAT files are extracted from the actual
successful yes2 reference run. The re-mint writes only build/batch-break-remint;
it does not replace these goldens. version.raw records the reference VER output.

EXEC-child extension (review blocking item 1): exec_child.asm is assembled in
16-bit mode for DOS 3.30 and in flat-32 mode for the InitechDOS emulator gate.
The batch runs BRKCHILD < INPUT.TXT > OUT.TXT. The child prints READY, opens CON,
rebinds its own stdin, and waits in AH=08h. An ordinary return would print
EXEC-CHILD-SURVIVED. Ctrl-C instead kills the child; the batch then asks on CON.
exec-y.txt and exec-n.txt are the complete 80-column VGA captures after the
answer. Y returns to A>; N executes EXEC_AFTER and reaches the later PAUSE.
exec-output.raw contains the exact redirected READY and ^C bytes, with no
SURVIVED or question. exec-after.raw is the N sentinel including DOS ECHO's
space before redirection. exec.sha256 records these independent goldens.
The host and emulator batch-break gates grade their next-line decision against
the VGA captures. InitechDOS displays ^C on CON under its existing contract;
this extension grades the batch decision and child termination, not that stream
difference. All raw DOS/CRLF/VGA fixtures are byte-exact; do not trim whitespace.

The EXEC mint deletes BACKUP.COM only on its disposable boot copy to make room.
Its fsck diagnostics must match the unchanged original reference media (whose
root label predates an extended-BPB label); the original media hash is retained.
