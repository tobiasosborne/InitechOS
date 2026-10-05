# Drive InitechOS hard: every part, every application, and write down every nit

InitechOS is an operating system for emulated 486 PCs: a DOS 3.3 personality
underneath, a Mac OS 8 "Platinum" style desktop (FLAIR) on top, and its own
applications modelled on real ones (a dBASE III PLUS work-alike, a Lotus
1-2-3 R2.2 work-alike, more to come). The owner's bar: it should look really
awesome for the early 1990s, include every feature a user of the time
expected, and become a fully fledged operating system. A person who used
that era's Mac and DOS software should say "yes, that's it".

Your job is the widest and hardest hands-on drive it has had: use everything,
in every way a real user and a hostile tester would, and record every defect
down to the smallest nit: wrong behaviour, missing features, a pixel out of
place, a label that reads oddly, a pause, a flicker, anything a careful 1990s
reviewer would mark. Small things count; the owner asked for all of them.

## What is on the system (images built from commit 7030e2f, in ./images)

- flair_tenants_interactive.img + flair_data.img (second IDE disk): the live
  desktop. Finder with disk windows, icons, drag and drop, scroll bars, two
  menu bars, built-in apps HELLO and NOTES, and in the APPS folder three disk
  applications: TENANTFX.EXE (a test app with its own menus), CTENANT.EXE,
  and 123.EXE (Initech 123: the 1-2-3 worksheet screen, cell pointer, typing
  labels and numbers). New since the last audit: the Trash opens as a window
  and Special > Empty Trash works with a confirm alert; disk apps receive
  their own menu choices; 123.EXE and CTENANT.EXE exist.
- samir_list.img as the second disk instead: the database (SAMIR), reached
  from the desktop by a system hotkey (see the Makefile target
  test-flair-samir-suspend in the repo for how).
- tracer_boot.img (+ fat_data.img or flair_data.img as second disk): the DOS
  prompt, COMMAND.COM. New since the last audit: COPY and DEL were fixed
  (COPY onto itself, COPY to a directory, DEL with a path, DEL of more than
  sixteen files, DEL *.* confirmation).
- flair_live_interactive.img: the film-scene desktop (the frame from the film
  Office Space that the project reproduces: pie chart, "Saving tables to
  disk..." dialog and so on).
Copy an image before booting it if you want a pristine one later; everything
you write stays in your working directory (where this brief is).

## How to work

- Cover everything: every menu and every item, every window control, every
  icon, every keyboard shortcut you can find or guess, every application,
  every DOS command, every database command, long sessions, odd orders, fast
  input, slow input, full disks, long names, many files, many windows, bad
  input. Follow your nose where something looks fragile.
- Check the five reports in /home/tobias/Projects/initech-os/docs/audits/
  (REPORT.md and TRIAGE.md in each; drivers you can reuse) so you know what
  is already known. Do not re-describe a known finding; if it is still
  present just list its id under "still present". Verify that the fixes named
  above really hold, and say so per fix.
- Judge the look as well as the function, against references on this machine:
  Mac OS 8 / System 7 captures and specs in /home/tobias/Projects/system7-decomp,
  real 1-2-3 R2.2 captures in /home/tobias/Projects/lotus123-decomp, real
  dBASE III PLUS material in /home/tobias/Projects/dbase3-decomp, and the
  original manuals (DOS 3.3, dBASE III PLUS, 1-2-3, Macintosh) in
  /home/tobias/Projects/initech-manuals-ref. Say for each expectation whether
  it comes from a named reference or from your own memory.
- The repo /home/tobias/Projects/initech-os is read-only for you (no edits,
  builds, make, or commits); read its sources and Makefile freely to learn
  how things are driven.
- Drive the guest through QEMU itself (-display none, QMP for keys, mouse and
  screenshots), as the earlier passes did; their drivers are in the audit
  directories. If your sandbox refuses to create a socket, QEMU's `-qmp stdio`
  works. The owner is using the real desktop on X display :0: never send
  input to it or capture it.
- Shared machine: at most two emulators at a time, and kill only processes
  you started, by pid. Other people's QEMU and Bochs runs are in progress;
  leave them alone.

## Deliberate, not bugs

Two stacked menu bars (the lower one Photoshop-like at rest), the teal
desktop, an hourglass busy cursor, a pie chart summing to 116%, a `570-`
figure, the "Saving tables to disk..." dialog, "PC LOAD LETTER" as the panic
text, an accounts program that treats year 00 as 1900. Report them only if
drawn or behaving badly. A panic or a hang is always a finding.

## What to hand back, in your working directory

- REPORT.md, ASCII only:
  1. Verdict per recent fix (holds / partly / fails) with evidence.
  2. Findings numbered L001, L002, ... one per defect, each with: area
     (desktop, Finder, menus, windows, dialogs, Trash, apps, Initech 123,
     database, DOS, look, performance, other), severity (P1 data loss / crash
     / blocked core task; P2 broken or missing feature; P3 look or polish
     nit), exact replayable steps, what you saw, what you expected and why
     (reference or memory), and the evidence file. Do not merge different
     nits into one finding; do not drop a nit because it is small.
  3. "Still present": ids of known findings you saw again.
  4. What you covered and what you could not reach, by area.
  5. The twenty things you would fix first to make a 1990s user believe in
     this system, ranked.
- NITS.tsv: one line per finding (id, area, severity, one-line summary).
- shots/ for screenshots, evidence/ for raw logs, and your driver scripts.
Report only what you observed in this session, and check that each cited
screenshot shows what you say it shows. Take the time this needs; breadth and
completeness matter more than speed. Shut down what you started and end with
a short summary.
