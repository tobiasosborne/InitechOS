# Third pass: check the fixes on the InitechOS desktop, then keep hunting

InitechOS is an operating system for emulated 486 PCs. Its desktop (FLAIR) is
meant to look and feel like Mac OS 8 "Platinum" on top of a DOS 3.3
personality. The owner's bar: it should look really awesome for the 1990s and
include all the features a user of the time expected. He wants every edge
case and problem found.

Two earlier passes are in the repo under
/home/tobias/Projects/initech-os/docs/audits/ (2026-10-03-flair-gui-codex and
2026-10-04-flair-gui-codex-pass2: REPORT.md, TRIAGE.md, drivers). Read them.

Since then the developers claim to have changed these things. build/ now
holds images built from commit 52ede68:
- released Ctrl / Shift no longer stay latched (pass-2 G11);
- menus, window drags, grows and icon drags track until the button is
  released, however long it is held (G01);
- the close, zoom and collapse boxes show a pressed state and act only on
  release inside the box (G02);
- icons can be dragged into folders, between windows and onto the Trash,
  with an outline while dragging and a highlighted drop target;
- menu, title and button text is proportional Chicago instead of a fixed
  8-pixel cell;
- a disk-launched app shows its own menu bar.

First, try to break each claim on the live image, as a sceptic would: does
it really hold, on a long session, under odd timing, at the edges? Say
plainly for each one: holds, partly holds, or fails, with evidence. Judge the
new text and the drag feedback against the real Mac captures in
/home/tobias/Projects/system7-decomp as a matter of look, not only function.

Then keep hunting for new problems in whatever direction you judge most
likely to pay off. Known and already filed (do not re-report): everything in
the two earlier reports and their TRIAGE.md files, in particular Finder
window contents not following a moved window and scroll bars that do nothing.

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
under `evidence/`. First a table of the six claims with verdict and evidence.
Then new findings numbered H01, H02, ... each with what you did (replayable),
what you saw, what you expected and why, and the evidence. Only report what
you observed in this session; say when an expectation comes from your own
memory rather than a reference on disk; check each cited screenshot shows
what you say. ASCII only. Shut down what you started and end with a short
summary.
