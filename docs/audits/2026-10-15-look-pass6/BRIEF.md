# Audit pass 6: the LOOK of InitechOS against the Mac OS 8 / System 7 references, pixel by pixel

InitechOS is an operating system for emulated 486 PCs: a DOS 3.3 personality
underneath, a Mac OS 8 "Platinum" style desktop (FLAIR) on top, and its own
applications modelled on real ones (a dBASE III PLUS work-alike, a Lotus
1-2-3 R2.2 work-alike). The owner's bar: it must look really awesome for the
early 1990s; a person who used that era's Mac software should say "yes,
that's it". The previous five audits covered FUNCTION widely and LOOK thinly.
This pass is the appearance pass the owner is owed: compare what the screen
shows with the real thing, and write down every nit.

## What is in ./images (built from the commit named in images/COMMIT)

- flair_tenants_interactive.img + flair_data.img (second IDE disk): the live
  desktop. Finder, disk windows, icons, drag and drop, scroll bars, two menu
  bars, HELLO and NOTES, and in APPS the disk applications TENANTFX.EXE,
  CTENANT.EXE and 123.EXE (Initech 123). New tonight: Special > Restart and
  Shut Down work (Shut Down shows the "safe to switch off" screen with a
  Restart button); Initech 123 asks Save / Cancel / Don't Save when you quit
  a changed worksheet (File > Quit, Ctrl-Q, the close box, /Quit); at the
  DOS prompt Ctrl-C aborts and Ctrl-Z is end-of-file.
- samir_list.img as the second disk: the database (SAMIR), reached from the
  desktop by the hotkey used in the Makefile target test-flair-samir-suspend.
- tracer_boot.img (+ flair_data.img): the DOS prompt.
- flair_live_interactive.img: the film-scene desktop (pie chart, "Saving
  tables to disk..." dialog).
Copy an image before booting it if you want a pristine one later.

## How to work

- Drive the guest through QEMU with -display none and QMP (keys, mouse,
  screendump as PPM/PNG), as the earlier passes did; reuse their drivers
  from /home/tobias/Projects/initech-os/docs/audits/*/ (read-only). If your
  sandbox refuses a socket, `-qmp stdio` works. The owner uses the real
  desktop on X display :0: never send input to it or capture it.
- Shared machine: at most two emulators at a time; kill only processes you
  started, by pid. Other QEMU runs belong to other people.
- REFERENCES on this machine, all read-only: /home/tobias/Projects/system7-decomp
  (goldens/captures/*.png are real System 7.5.3 and Mac OS 8 screendumps;
  specs/ has measured chrome, menu, window, dialog, font and icon metrics;
  mint/ shows how they were captured), /home/tobias/Projects/win31-decomp,
  /home/tobias/Projects/lotus123-decomp (real 1-2-3 R2.2 screens),
  /home/tobias/Projects/dbase3-decomp, and the original manuals in
  /home/tobias/Projects/initech-manuals-ref. For every expectation say which
  reference file (and pixel coordinates or page) it comes from, or that it
  is from your memory.
- Method: take a screendump of each InitechOS state, open the matching
  reference capture, and compare at the pixel level with a script (crop,
  diff, count). Measure: title-bar height and stripe pattern, window frame
  weights and shadows, close box and zoom box art, scroll bar parts (arrows,
  thumb, track pattern) at rest / pressed / inactive, menu bar height, menu
  title spacing, pull-down geometry, item heights, separators, disabled
  gray, check marks, command-key glyphs, the Apple/system menu, desktop
  pattern, icon art and label font/position/selection highlight, dialog
  frame and button rings, alert layout, text: Chicago 12 and Geneva 9
  strikes, baselines, inter-character spacing, bold/italic, truncation with
  ellipsis; cursor shapes (arrow, I-beam, hourglass busy); colour values
  (the Platinum grays, the highlight colour); the DOS screen (font, colours,
  prompt layout) against the DOS 3.3 manual; Initech 123 against the real
  1-2-3 R2.2 captures (control panel rows, mode indicator, column borders,
  cell pointer, status line); the database screen against the dBASE
  material. Also judge motion where you can: menu drop timing, window drag
  outline, flicker on repaint, scroll redraw.
- Repo /home/tobias/Projects/initech-os is read-only (no edits, builds,
  make, commits). Read its sources, spec/*.json and Makefile freely.
- Check the five earlier reports (REPORT.md + TRIAGE.md in each audit dir)
  so you do not re-describe a known finding; list its id under "still
  present" instead.

## Deliberate, not bugs

Two stacked menu bars (the lower one Photoshop-like at rest), the teal
desktop, an hourglass busy cursor, a pie chart summing to 116%, a `570-`
figure, the "Saving tables to disk..." dialog, "PC LOAD LETTER" as the panic
text, an accounts program that treats year 00 as 1900, the shut-down screen
saying "computer" rather than "Macintosh". Report them only if drawn badly.
A panic or a hang is always a finding.

## Hand back, in your working directory

- REPORT.md, ASCII only: (1) a per-area verdict (matches / close / wrong)
  with the measured numbers; (2) findings M001, M002, ... one per nit, each
  with area, severity (P2 clearly wrong vs the reference; P3 polish), exact
  state and steps to reach it, what InitechOS draws (numbers), what the
  reference draws (file, coordinates, numbers), the diff image; (3) "still
  present" ids; (4) coverage: what you compared and what you could not
  reach; (5) the twenty look fixes you would make first, ranked.
- NITS.tsv: id, area, severity, one-line summary.
- shots/ (InitechOS screendumps), refs/ (the reference crops you used),
  diffs/, evidence/, and your driver scripts.
Report only what you measured in this session; verify each cited image
shows what you say. Breadth and precision matter more than speed. Shut down
what you started and end with a short summary.
