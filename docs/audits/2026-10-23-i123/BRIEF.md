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

## YOUR SUBSYSTEM: Initech 123 (the Lotus 1-2-3 Release 2.2 work-alike, APPS\123.EXE)

Boot flair_tenants_interactive.img with flair_data.img; open the data disk
window, then APPS, then double-click 123.EXE. 123.MNU beside it is the menu
tree it loads. Real 1-2-3 R2.2 captures and specs are in
/home/tobias/Projects/lotus123-decomp (goldens, mints, oracles, GAPS); the
Lotus manuals are in /home/tobias/Projects/initech-manuals-ref/lotus123.
The engine is os/i123/ in the repository (read-only). The 2026-10-05 audit
reached only labels and numbers (findings L008-L021, L080); everything below
is since then.

Hunt in particular:
- Entry: labels with every prefix (' " ^ \), numbers in every format
  (leading +, -, ., exponents, commas, currency, percent, 1e308, 1e-308,
  0.1+0.2, very long digit strings), the 240-character entry line at exactly
  240 and 241 characters, Backspace/Escape/Home/End/arrows inside EDIT (F2),
  entering with arrow keys vs Return, Tab/Shift-Tab, PgUp/PgDn, Home, End
  then arrow, GOTO (F5) to A1, IV8192, IV8193, a bad address, a range name.
- Formulas and @functions: every @function the slash menu or the engine
  lists, with wrong argument counts, empty ranges, text where numbers go,
  circular references, forward references, ranges spanning the sheet,
  division by zero, ERR and NA propagation, recalculation order and
  /Worksheet Global Recalculation settings, +A1 vs A1 as formula vs label.
- The slash menu: walk the ENTIRE tree (/Worksheet, /Range, /Copy, /Move,
  /File, /Print, /Graph, /Data, /System, /Add-In, /Quit), every leaf, with
  Escape at every level, with the first-letter keys and with the arrow
  keys, pointing ranges with arrows and with typed ranges, anchoring with
  . and unanchoring, the third line of the control panel (the description
  line) for every item against the real R2.2 captures.
- Worksheet operations: /Copy and /Move with overlapping source and target,
  relative and absolute ($) references after copy/move, insert/delete rows
  and columns through formulas that reference them, column widths at 1 and
  240, /Range Format on every format with every decimal count, /Range Erase,
  /Range Name Create/Delete/Labels, /Worksheet Erase with Y/N, Titles,
  Windows (horizontal/vertical/sync/unsync), Column Hide, Global settings.
- Files: /File Save then /File Retrieve of the same worksheet (compare the
  .WK1 bytes you extract from the data image with lotus123-decomp's real
  files and parse them), save with an existing name (Cancel/Replace/Backup),
  save to a full disk, retrieve a file saved by the real Lotus from the
  corpus if the image can carry one (copy it in with mtools before boot),
  /File List, /File Erase, /File Directory, long and bad file names.
- Capacity and recovery: fill the sheet to the storage limit and one past
  it (the earlier L080), MEM indicator, what happens on exhaustion, Undo if
  present, Ctrl-Break during a long recalculation.
- Quit paths: /Quit, File > Quit, Ctrl-Q, the close box, Shut Down from the
  Finder while a changed worksheet is open: Save / Cancel / Don't Save every
  branch, and twice.
- Look: the control panel rows, mode indicators (READY, LABEL, VALUE, EDIT,
  POINT, MENU, ERROR, WAIT, HELP), the column letters and row numbers, the
  cell pointer, the status line indicators (CALC, CIRC, NUM, CAPS, END,
  UNDO), the window frame, against the real captures pixel by pixel.
