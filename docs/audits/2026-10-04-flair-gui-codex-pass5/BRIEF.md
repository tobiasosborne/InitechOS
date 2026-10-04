# Fifth pass: check the newest fixes on the InitechOS desktop, then go where no pass has been

InitechOS is an operating system for emulated 486 PCs. Its desktop (FLAIR) is
meant to look and feel like Mac OS 8 "Platinum" on top of a DOS 3.3
personality. The owner's bar: it should look really awesome for the 1990s and
include all the features a user of the time expected, and it should become a
fully fledged operating system. He wants every edge case and problem found.

Four earlier passes are in the repo under
/home/tobias/Projects/initech-os/docs/audits/ (2026-10-03-flair-gui-codex,
-pass2, -pass3 and -pass4: REPORT.md, TRIAGE.md, drivers). Read them; reuse
the drivers.

## Part 1: two new claims (keep this short)

Since pass 4 the developers claim (images in build/ are from commit 8b1bf6d):
- a long held menu gesture that crosses several menus dispatches the item
  highlighted in the menu that is visible at release, however long the
  gesture lasts (pass-3 H01);
- the TRASH directory and DESKTOP.DB are shown in no Finder window and cannot
  be opened, moved, or dropped onto by any route; the volume root window
  shows two icons (H03, H05).
Try to break each on the live image, as a sceptic would. Say plainly for
each: holds, partly holds, or fails, with evidence. Spot-check that the
pass-4 verdicts have not regressed, but do not repeat pass 4.

## Part 2: territory no pass has reached (most of your effort)

No pass has yet exercised these. Go as deep as you can in each:
1. Applications. The data disk holds APPS\TENANTFX.EXE, launched by opening
   it in the Finder; the desktop also hosts two built-in apps (HELLO and
   NOTES). Launch, switch between apps, use each app's windows and menus,
   quit, relaunch, launch twice, close windows in every order. Known and
   filed: a disk app's own menu items are logged but not delivered to it.
2. The database. A full-screen text program (SAMIR, a dBASE III work-alike)
   is reached from the desktop by a system hotkey when build/samir_list.img
   is the second disk (see the Makefile target test-flair-samir-suspend for
   how). Use it as a dBASE user would (USE, LIST, and whatever else it
   accepts), then return to the desktop. Does the desktop come back intact?
3. The DOS side. build/tracer_boot.img boots to the COMMAND.COM prompt (with
   a data disk as second IDE disk). Use it as a DOS 3.3 user would: DIR,
   TYPE, CD, MD, RD, COPY, DEL, REN, running programs, batch files,
   redirection, wildcards, bad input. What works, what is missing, what
   misbehaves?
4. Endurance. One desktop session of at least 15 minutes of mixed real use:
   many windows, many folders, drags, menus, app launches. Look for drift,
   leaks, stale pixels, slowdowns, a hang or a panic.
5. Dialogs and alerts, wherever the system shows one.

## Part 3: the gap list

From everything you saw, write a ranked list of the ten things a user of a
1990s Macintosh or DOS PC would miss most on this system, each with one line
of evidence from your session. This list steers what gets built next, so
rank by how much a real user would feel the gap.

Known and already filed (do not re-report): everything in the four earlier
reports and their TRIAGE.md files, including pass-4 J01-J05; the Trash icon
does not open; Get Info, Duplicate, Empty Trash, Restart, Shut Down are drawn
disabled; the upper bar's Quit is enabled and dead.

You are not fixing anything; the deliverable is a findings report.

## Starting point

- Repo: /home/tobias/Projects/initech-os (docs, Makefile, sources).
- Built images in build/: flair_tenants_interactive.img (the live desktop,
  boot disk), flair_data.img (its data disk, second IDE disk),
  samir_list.img (database data disk), tracer_boot.img (DOS prompt),
  flair_tenants.img (a bounded gate image, halts by itself). Nothing else is
  built, and you cannot build.

## Limits

- The repo is read-only for you: no edits, no builds, no `make`, no commits.
  Copy any disk image into your working directory (where this brief is) and
  boot the copy; write only inside that directory.
- The owner is using the real desktop on X display :0. Never send input to
  it or capture it. Drive the guest through QEMU itself (-display none, QMP),
  as the earlier passes did.
- Shared machine: one emulator at a time. Kill only processes you started,
  by pid (never `pkill -f`). Other people's QEMU and Bochs processes are
  running on this machine tonight; leave them alone.

## Deliberate, not bugs

Two stacked menu bars (the lower one Photoshop-like at rest), the teal
desktop, an hourglass busy cursor, a pie chart summing to 116%, a `570-`
figure, the "Saving tables to disk..." dialog, "PC LOAD LETTER" as the panic
text, an accounts program that treats year 00 as 1900. Report them only if
drawn or behaving badly. A panic or hang is always a finding.

## Report

`REPORT.md` in your working directory, screenshots under `shots/`, raw logs
under `evidence/`. First the two claims with verdict and evidence. Then new
findings numbered K01, K02, ... each with what you did (replayable), what you
saw, what you expected and why, and the evidence. Then the gap list. Only
report what you observed in this session; say when an expectation comes from
your own memory rather than a reference on disk; check each cited screenshot
shows what you say. ASCII only. Shut down what you started and end with a
short summary.
