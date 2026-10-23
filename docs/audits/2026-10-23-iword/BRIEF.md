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

## YOUR SUBSYSTEM: InitechWord (the WordPerfect 5.1 work-alike, APPS\WORD.EXE)

Boot flair_tenants_interactive.img with flair_data.img; open the data disk
window, APPS, double-click WORD.EXE. Real WordPerfect 5.1 captures, minted
goldens and format specs are in /home/tobias/Projects/wordperfect51-decomp;
the WP 5.1 manuals are in /home/tobias/Projects/initech-manuals-ref/wordperfect51.
The engine is os/iword/ with its key map in spec/iword/fkeys.h, the code
set in spec/iword/reveal_codes.h and the file format in
spec/iword/wp5_format.h (all read-only). The project's own InitechWord
Reference and Workbook are in pubs/manuals/initechword-*; what they promise
is a contract. No earlier audit has driven InitechWord: everything is new.

Hunt in particular:
- Typing and wrap: type past the right margin with long words, words longer
  than a line, many spaces, hyphens, Tab and Indent (F4) at the end of a
  long word (a known unminted disagreement), hard returns, Backspace at the
  start of a line, Delete at the end, join lines, blank lines, a document
  of several hundred lines, the status line Doc/Pg/Ln/Pos arithmetic against
  WP 5.1's.
- Codes: every formatting key the fkeys map lists (Bold F6, Underline F8,
  Center Shift-F6, Flush Right Alt-F6, Indent F4 and Shift-F4, Tab, Hard
  page Ctrl-Return, Line Format Shift-F8 settings, Font Ctrl-F8, Date
  Shift-F5...), typed on, typed off, nested, crossing, empty pairs, deleted
  from inside Reveal Codes (Alt-F3 / F11) and from the text, Backspace onto
  a code (WP asks "Delete [Bold]? No (Yes)"), cursor movement across codes.
- Block (Alt-F4 / F12): define forwards and backwards, by word/line/para,
  then every operation WP 5.1 allows on a block (Delete with the Y/N
  prompt, Move/Copy via Ctrl-F4, Bold/Underline/Center/Flush Right applied
  to a block, Save a block, Append, Case), Undelete (F1) and its three
  levels, block then Escape, block across a code, block across a page.
- Search (F2 / Shift-F2) and Replace (Alt-F2): forwards, backwards, with
  codes, case, not found, replace with empty, replace all, w/Confirm.
- Files: Save (F10) and Exit (F7) with every prompt branch (Y/N, replace,
  new name, cancel), Retrieve (Shift-F10) of a saved file and of a real
  WP 5.1 file from the corpus copied into the data image with mtools before
  boot, List Files (F5) with every key it offers (Retrieve, Look, Delete,
  Rename, Copy, Name Search, Esc, Cancel), long and bad names, a full disk,
  an empty file, a file with only codes. Extract the saved .WP5 bytes from
  the stopped image and check the header/prefix and code bytes against the
  format spec and real files.
- Screen: Help (F3), Cancel (F1) at every prompt, the Setup screen, Reveal
  Codes split and its separator, scrolling in both panes, Screen (Ctrl-F3)
  rewrite and window split, Ctrl-Home GoTo, Home-Home-Up/Down, the
  keyboard template strip the manual prints vs what the keys do.
- Quit paths: Exit F7 every branch, File > Quit, Ctrl-Q, close box, Shut
  Down from the Finder with an unsaved document.
- Look: the editing screen (black with the status line at bottom right,
  Doc 1 Pg 1 Ln 1" Pos 1"), the Reveal Codes pane, List Files layout,
  every prompt's wording and position, against the real WP 5.1 captures.
