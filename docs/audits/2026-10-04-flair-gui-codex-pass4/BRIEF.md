# Fourth pass: check the newest fixes on the InitechOS desktop, then keep hunting

InitechOS is an operating system for emulated 486 PCs. Its desktop (FLAIR) is
meant to look and feel like Mac OS 8 "Platinum" on top of a DOS 3.3
personality. The owner's bar: it should look really awesome for the 1990s and
include all the features a user of the time expected. He wants every edge
case and problem found.

Three earlier passes are in the repo under
/home/tobias/Projects/initech-os/docs/audits/ (2026-10-03-flair-gui-codex,
-pass2 and -pass3: REPORT.md, TRIAGE.md, drivers). Read them.

Since then the developers claim to have changed these things. build/ now
holds images built from commit 40581c5:
- a Finder window's icons follow the window through drag, zoom, restore,
  collapse and grow, and can be clicked, selected and dragged where they are
  drawn (pass-1 F01);
- scroll bars work: a click on one never moves the window; arrows step and
  repeat while held, the track pages, the thumb drags; the Finder view
  scrolls and icons can be selected at their scrolled position; a window
  with nothing to scroll shows disabled bars that ignore clicks (F02);
- no menu command is enabled and dead: Select All and Arrange (by Name)
  work; Get Info, Duplicate, Empty Trash, Restart, Shut Down, Open on a
  document and the upper bar's About are drawn disabled and cannot be
  chosen (F03, F05, F07, G10). Known exception: the upper bar's Quit;
- closing the last Finder window leaves the Finder in front with its bar
  and NO window drawn active; clicking an app's window switches to that app;
  clicking the desktop brings the Finder forward (F06);
- the earlier fixes still hold after these changes: released modifiers do
  not latch, gestures track until release, close/zoom/collapse boxes act on
  release inside.

First, try to break each claim on the live image, as a sceptic would: does
it really hold, on a long session, under odd timing, at the edges? Say
plainly for each one: holds, partly holds, or fails, with evidence. Judge the
new text and the drag feedback against the real Mac captures in
/home/tobias/Projects/system7-decomp as a matter of look, not only function.

Then keep hunting for new problems in whatever direction you judge most
likely to pay off. Known and already filed (do not re-report): everything in
the three earlier reports and their TRIAGE.md files (for example the long
cross-menu gesture H01 and the movable TRASH directory H03).

You are not fixing anything; the deliverable is a findings report.

## Starting point

- Repo: /home/tobias/Projects/initech-os (docs, Makefile, sources).
- Live desktop boot disk: build/flair_tenants_interactive.img with
  build/flair_data.img as the second IDE disk. Other prebuilt images are in
  build/; the Makefile shows how each is booted.

## Limits

- The repo is read-only for you: no edits, no builds, no `make`, no commits.
  Copy any disk image into your working directory (where this brief is) and
  boot the copy; write only inside that directory.
- The owner is using the real desktop on X display :0. Never send input to
  it or capture it. Drive the guest through QEMU itself, as before.
- Shared machine: one emulator at a time. Kill only processes you started,
  by pid (never `pkill -f`).

## Deliberate, not bugs

Two stacked menu bars (the lower one Photoshop-like at rest), the teal
desktop, an hourglass busy cursor, a pie chart summing to 116%, a `570-`
figure, the "Saving tables to disk..." dialog, "PC LOAD LETTER" as the panic
text. Report them only if drawn or behaving badly. A panic or hang is always
a finding.

## Report

`REPORT.md` in your working directory, screenshots under `shots/`, raw logs
under `evidence/`. First a table of the five claims with verdict and evidence.
Then new findings numbered J01, J02, ... each with what you did (replayable),
what you saw, what you expected and why, and the evidence. Only report what
you observed in this session; say when an expectation comes from your own
memory rather than a reference on disk; check each cited screenshot shows
what you say. ASCII only. Shut down what you started and end with a short
summary.
