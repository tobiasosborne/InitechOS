# Hunt every edge case and weird bug in InitechOS: hands-on drive of one subsystem

InitechOS is an operating system for emulated 486 PCs: a DOS 3.3 personality
underneath (InitechDOS), a Mac OS 8 "Platinum" style desktop (FLAIR) on top,
and its own applications modelled on real ones: a dBASE III PLUS work-alike
(InitechBase, program SAMIR), a Lotus 1-2-3 R2.2 work-alike (Initech 123), a
WordPerfect 5.1 work-alike (InitechWord), desk accessories, control panels.
The owner's bar: it should look really awesome for the early 1990s, include
every feature a user of the time expected, and be a dependable period
workstation. A person who used that era's Mac and DOS software should say
"yes, that's it".

The owner's request today, verbatim: "it would be awesome to catch all the
edge cases, weird bugs, etc etc". You are one of six auditors tonight, each
driving ONE subsystem as hard as a hostile tester and a meticulous 1990s
reviewer would: odd orders of operations, boundary values, fast and slow
input, full disks, long names, many files, many windows, cancel in the middle
of everything, repeat everything twice, leave things open and come back,
every key the real product accepted, every menu item, every error path.
Record every defect down to the smallest nit: wrong behaviour, missing
features, a pixel out of place, a label that reads oddly, a pause, a
flicker, a hang, a panic. Small things count; the owner asked for all of them.

## Images (in ./images, built from the commit in ./images/COMMIT)

- flair_tenants_interactive.img + flair_data.img as the second IDE disk: the
  live desktop. Finder with disk windows, icons, drag and drop, scroll bars,
  two menu bars, built-in apps HELLO and NOTES, Trash as a window, Special >
  Restart / Shut Down, Empty Trash with confirmation. In the APPS folder of
  the data disk: TENANTFX.EXE, CTENANT.EXE (test apps with their own menus),
  123.EXE (Initech 123), WORD.EXE (InitechWord), CALC.EXE, CALCULAT.EXE,
  TEXT.EXE (InitechText), PAINT.EXE, NOTEPAD.EXE, SCRAPBK.EXE, CPANELS.EXE
  (Control Panels), TPSREP.COM (the TPS report generator, with TPS.DBF,
  TPSPARM.DAT, LEDGER.TXT, MICHAEL.CFG). The DOS folder holds the external
  DOS commands (CHKDSK ATTRIB LABEL XCOPY TREE EDLIN MORE FIND SORT FDISK
  FORMAT SYS DISKCOPY DISKCOMP).
- flair_tenants_interactive.img + samir_list.img as the second disk: the
  database (SAMIR), reached from the desktop by the system hotkey
  Ctrl+Alt+backslash (the Makefile target test-flair-samir-suspend shows the
  key token and the trace shape).
- tracer_boot.img + flair_data.img (or fat_data.img) as the second disk: the
  DOS prompt, COMMAND.COM.
- flair_live_interactive.img: the film-scene desktop (the frame from the
  film Office Space that the project reproduces: pie chart, "Saving tables
  to disk..." dialog, FILE COPY).
Boot COPIES of the images (the driver does this) so you can always return to
a pristine one. Everything you write stays in your working directory (where
this brief is).

## What changed since the big 2026-10-05 drive (so you know what is new ground)

Restart and Shut Down; Ctrl-C and Ctrl-Z at the DOS prompt; Initech 123
formulas, @functions, the complete slash-menu tree, File Retrieve/Save of
.WK1, a 240-character entry line, EDIT/GOTO/PgDn/Tab, cell hit-testing,
resized geometry, Save/Cancel/Don't Save on quit; InitechWord is new (WP 5.1
keys, formatting codes and Reveal Codes, block operations, List Files on F5,
.WP5 save/retrieve); the Finder gained limits and alerts (disk full, refused
moves, replace-on-exists, Put Away), drag-move of icons, Trash views that
purge, a real double-click interval, the arrow cursor at boot, Geneva 12;
SAMIR gained RUN and !, DO <file>, ON ERROR, MESSAGE(), error numbers, REPORT
FORM, memo fields, indexes, SET options; InitechDOS gained the external
commands listed above, COMMAND /C, nested EXEC, drive letters A: C: D: as
real volumes with per-drive current directories and LASTDRIVE, F3 template
recall, two Shift keys plus CapsLock, FAT clusters of 2..64 sectors, the DOS
3.3 COPY with + lists and wildcards and /A /B /V; an easter egg (the Jump to
Conclusions Mat); Control Panels and a Font Manager; the desk accessories
above. Expect fresh bugs where features are new and where they meet.

## How to work

- Drive the guest through QEMU itself: -display none, -cpu 486, -m 16, QMP
  on stdio for keys, mouse and screendumps, exactly as audit_driver.py in
  this directory does (it is the 2026-10-05 driver, copied here for you;
  extend it as you like). If your sandbox refuses to create a socket,
  `-qmp stdio` is the way. The owner uses the real desktop on X display :0:
  never send input to it and never capture it.
- Shared machine, six auditors plus the owner's own emulator runs: run AT
  MOST ONE emulator at a time, kill only processes you started, by pid, and
  leave every other qemu/bochs/codex process alone.
- The repository /home/tobias/Projects/initech-os is READ-ONLY for you: no
  edits, builds, make, commits, or tracker commands. Read its sources,
  Makefile, spec/*.json, spec/*.h and docs freely to learn how things are
  driven and what the designers intended. The project's own manuals under
  pubs/manuals/ (LaTeX) describe what the system promises: anything a
  shipped manual promises that the system does not do, or does differently,
  is a finding (quote the manual file and line).
- Seven earlier audits live in /home/tobias/Projects/initech-os/docs/audits/
  (REPORT.md + TRIAGE.md each; the drivers are reusable). Read the ones
  touching your subsystem so you do not re-describe a known finding; if it
  is still present list its id under "Still present". Everything else that
  you see is new and gets its own number.
- Judge the look as well as the function, against references on this
  machine (all read-only): Mac OS 8 / System 7 captures and metric specs in
  /home/tobias/Projects/system7-decomp, Windows 3.1 in
  /home/tobias/Projects/win31-decomp, real 1-2-3 R2.2 in
  /home/tobias/Projects/lotus123-decomp, real dBASE III PLUS in
  /home/tobias/Projects/dbase3-decomp, real WordPerfect 5.1 in
  /home/tobias/Projects/wordperfect51-decomp, and the original manuals (DOS
  3.3, dBASE III PLUS, 1-2-3 R2.2, WordPerfect 5.1, Macintosh) in
  /home/tobias/Projects/initech-manuals-ref. Say for each expectation whether
  it comes from a named reference (file, page, pixel region) or from your own
  memory.
- Verify by evidence, not by inference: a screenshot for what the screen
  showed, the serial log for what the kernel said, and where bytes matter
  (files written, .dbf/.wk1/.wp5 contents) extract them from the stopped
  image copy with mtools and show the bytes.

## Deliberate, not bugs

Two stacked menu bars (the lower one Photoshop-like at rest), the teal
desktop, an hourglass busy cursor, a pie chart summing to 116%, a `570-`
figure, the "Saving tables to disk..." dialog, "PC LOAD LETTER" as the panic
text, an accounts program that treats year 00 as 1900, the shut-down screen
saying "computer" rather than "Macintosh", a hidden Jump to Conclusions Mat.
Report them only if drawn or behaving badly. A panic or a hang is always a
finding, and so is anything that loses or corrupts a user's data.

## What to hand back, in your working directory

- REPORT.md, ASCII only:
  1. Findings numbered L001, L002, ... one per defect, each with: area,
     severity (P1 data loss / corruption / crash / hang / blocked core task;
     P2 broken or missing feature or wrong behaviour; P3 look or polish nit),
     exact replayable steps (coordinates, keys, files), what you saw, what
     you expected and why (reference or memory), and the evidence files.
     Do not merge different nits into one finding; do not drop a nit because
     it is small. Where you suspect a root cause in the source, cite the
     file and line, marked as a guess.
  2. "Still present": ids of known findings from the earlier audits you saw
     again, and "Now fixed": known ids you tried that no longer reproduce.
  3. What you covered and what you could not reach, with the reason.
  4. The fifteen things you would fix first in this subsystem to make a
     1990s user believe in it, ranked.
- NITS.tsv: one line per finding: id, area, severity, one-line summary.
- shots/ for screenshots (PNG), evidence/ for serial logs, extracted files
  and JSON, and your driver scripts, all replayable.
Report only what you observed in this session, and check that each cited
screenshot shows what you say it shows. Take the time this needs; breadth
and completeness matter more than speed. Shut down what you started and end
with a short summary.

## YOUR SUBSYSTEM: InitechDOS (the DOS 3.3 personality: COMMAND.COM, the external commands, the file system, drives)

Boot tracer_boot.img with flair_data.img (A: is the boot volume; the data
disk carries DOS\ with the external commands and AUTOEXEC.BAT sets
PATH=A:\DOS; also try fat_data.img, and build your own data disks with
mtools: many files, deep trees, a nearly full disk, odd names, read-only
and hidden attributes, 8.3 corner cases). Drive letters A:, C:, D: are
real volumes per decision DEC-18 (the Makefile and docs/worklog/WL-0156*
describe how the volumes are attached and which is FAT16); find the
arrangement and drive them. The original MS-DOS 3.3 User's Guide is in
/home/tobias/Projects/initech-manuals-ref/dos33; the project's own InitechDOS
Reference and User's Guide (pubs/manuals/initechdos-*) are a contract (some
entries carry DRAFT markers; everything else should hold). The kernel is
os/milton/ (read-only). The 2026-10-05 audit verified COPY/DEL fixes and
filed DOS findings; go past them.

Hunt in particular:
- Every builtin: DIR (/P /W, wildcards, drive and path forms, a path that
  does not exist, a file not a directory, 300 files, DIR of a full disk's
  free-space line), TYPE (binary file, empty file, huge file, Ctrl-C and
  Ctrl-S during output, MORE pipe), CD/CHDIR (.., \, A:\DOS, trailing
  backslash, a nonexistent dir, CD on another drive without switching),
  MD/RD (existing, non-empty, root, the 64-entry root limit, a dir named
  like a file), COPY (every form the 3.3 manual documents: + concatenation,
  wildcards both sides, output mask, /A /B /V, CON as source and target,
  copy onto itself through every spelling, copy to a full disk, copy of a
  zero-length file, timestamps), DEL/ERASE (*.* confirmation, read-only
  files, a directory name, wildcards across subdirs), REN (wildcards,
  existing target, cross-directory, to a bad name), VER, VOL, DATE, TIME
  (bad values, every format), CLS, ECHO (ECHO., ECHO OFF, long strings),
  PATH, PROMPT (every $ code), SET, BREAK, VERIFY, EXIT, CTTY if present,
  TYPE > redirection, >> append, < input from a file, | pipes with MORE
  SORT FIND, redirection to a full disk, redirection of a nonexistent
  command, a 127-character command line and 128.
- Batch files: AUTOEXEC.BAT and your own: ECHO/REM/PAUSE/GOTO/IF (ERRORLEVEL,
  ==, EXIST, NOT)/FOR %%F IN/SHIFT/CALL if present/%1..%9/%0/labels with
  spaces, nested batch, a batch that calls itself, Ctrl-C in a batch
  ("Terminate batch job (Y/N)?"), a batch on another drive.
- External commands: CHKDSK (/F, a dirty disk, cross-linked clusters you
  craft with a hex editor, lost chains), ATTRIB (+R +H +A, /S, then try to
  DEL/COPY), LABEL (empty, 11 chars, bad chars), XCOPY (/S /E /P /V /W,
  empty subdirs, onto itself), TREE (/F, deep trees), EDLIN (every command:
  L I D E Q and line ranges, a file with no CR/LF, a 64 KB file), MORE,
  FIND (/V /C /N, quotes, no match), SORT (/R /+n, binary input), FDISK,
  FORMAT (the boot disk!, /S, /V, Y/N prompts, a disk that is in use),
  SYS, DISKCOPY, DISKCOMP (two unequal disks, the same disk): each against
  the DOS 3.3 manual's page, including exact message text and exit codes
  where ERRORLEVEL can see them.
- The keyboard: F3 template recall after every kind of line, F1 one-char
  recall, Insert/Delete in the template if present, Backspace past the start,
  CapsLock with Shift, both Shift keys, Ctrl-C at the prompt mid-line, Ctrl-Z
  as EOF in COPY CON, typing while a program runs (type-ahead and the
  16-key buffer: type 20 keys fast), the keyboard beep.
- Programs: run SAMIR.COM, 123.EXE and WORD.EXE from the prompt (an .EXE in
  the InitechMZ container; a 16-bit MZ you craft should fail loud, not hang),
  a .COM that is just garbage, a 0-byte .COM, a 64 KB .COM, a program that
  does not exist on the PATH vs in the current dir, COMMAND /C DIR, nested
  COMMAND, EXIT from the nested one, "Bad command or file name" wording.
- The file system: files that exactly fill a cluster, a directory with
  exactly 16/32/64 entries, the last cluster of the disk, a file open during
  DEL, a disk pulled? (not possible) but a second disk with no FAT, a bad
  BPB, a file dated 1980 and 2099; extract the image after each session and
  check with mtools (mdir, minfo, mcheck) that the FAT chains and directory
  entries are what DOS 3.3 would write.
- Look: the DOS screen font and colours, the prompt, DIR layout and the
  "n File(s) nnnn bytes free" line, message wording, cursor shape, against
  the manual's printed examples.
