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

## YOUR SUBSYSTEM: desk accessories, small apps, Control Panels, the TPS report generator, the film scene

Boot flair_tenants_interactive.img with flair_data.img. In APPS: CALC.EXE,
CALCULAT.EXE, TEXT.EXE (InitechText, a TeachText-like editor), PAINT.EXE
(InitechPaint), NOTEPAD.EXE, SCRAPBK.EXE (Scrapbook), CPANELS.EXE (Control
Panels incl. the Font Manager, General, Mouse, Date & Time, Sound as they
exist), TPSREP.COM with TPS.DBF / TPSPARM.DAT / LEDGER.TXT / MICHAEL.CFG
(the TPS Report Generator; its manual is pubs/manuals/tps-report-generator),
plus the Apple menu (About This Computer and any desk accessories it
lists). Then boot flair_live_interactive.img alone: the film-scene desktop
(Office Space frame: pie chart 116%, FILE COPY "Saving tables to disk...",
the 570- figure) and drive everything on it. The Macintosh System Software
User's Guide is in /home/tobias/Projects/initech-manuals-ref/macos; Mac OS 8
captures and the DA / control panel specs are in
/home/tobias/Projects/system7-decomp; the frame itself is fixtures/ and
spec/assets/preview.webp in the repo. The 2026-10-05 drive barely touched
these; everything here is new ground.

Hunt in particular, per app:
- CALC / CALCULAT: every key by mouse and keyboard, chained operations
  (2+3*4=, 5==, 1/0, 0/0, negative zero, 9999999999*9999999999, sqrt of a
  negative, percent, decimal overflow, C vs CE, Backspace, very fast typing),
  paste-board, the paper-tape or display width overflow, the window's
  chrome; what the two calculators differ in and whether both are meant to
  exist.
- InitechText: type a long document, wrap at the window edge, resize while
  text is wrapped, select with mouse and shift-arrows, Cut/Copy/Paste through
  the Edit menu and command keys, paste into another window (Scrap across
  tenants), Undo, Open/Save/Save As with every prompt branch, a file with
  CR/LF and LF-only line ends, 8-bit characters, a 32 KB file and a bigger
  one, Geneva 12 on 16-px lines, scroll bar correctness with exactly one
  line more than fits, the I-beam cursor, blinking caret.
- InitechPaint: every tool, every pattern, draw to the canvas edges, off
  the edges, with the window partly off screen, Undo, select/move/clear,
  fill with a leaky outline, save/open, resize.
- Notepad and Scrapbook: page turning, the first and last page, paste in
  and out, persistence across close/reopen and across Shut Down, long text.
- Control Panels: every panel, every control, cancel vs OK, values at the
  limits (double-click speed, mouse tracking, date 29 Feb, 23:59:59, year
  1999->2000->2099, the clock actually advancing), the Font Manager (install
  / remove / list, what the Finder and apps do after a change), persistence
  of settings across Restart (where is it stored? extract the data image).
- TPSREP: run it with the shipped parameters and ledger, then with edited
  MICHAEL.CFG/TPSPARM.DAT/LEDGER.TXT (missing file, empty file, 1000 lines,
  bad numbers, a rate of 0 and of 100): the rounding-error canon must be
  played straight, but the program must not crash, hang, or overwrite data
  without saying so; compare its output with the manual's printed example.
- The film scene image: every window, menu and button on it (the FILE COPY
  dialog's progress and Cancel, the pie chart window, the lower Photoshop
  bar's menus), keyboard, drag, resize, the "Saving tables to disk..."
  behaviour to completion, Shut Down, and the pixel comparison of the rest
  state with fixtures/ (the frame) and spec/assets/preview.webp: list every
  visible difference as a nit with coordinates.
- Across apps: launch each from the Finder twice, launch two at once, quit
  via every path, Shut Down with each open, memory/slot limits reported
  honestly, each app's About box and version, each app's menu bar against
  the Macintosh conventions (Apple, File, Edit order; command keys shown).
