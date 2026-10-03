# Second pass: drive the InitechOS desktop harder and find everything else

InitechOS is an operating system for emulated 486 PCs. Its desktop (FLAIR) is
meant to look and feel like Mac OS 8 "Platinum" on top of a DOS 3.3
personality. The owner is unhappy with the window system. His bar: it should
look really awesome for the 1990s and include all the features a user of the
time expected. He wants every edge case and problem found.

A first pass already ran today. Its report, screenshots, evidence and a
working QMP driver are in
/home/tobias/Projects/initech-os/docs/audits/2026-10-03-flair-gui-codex/
(REPORT.md, TRIAGE.md, drive.py). Read them first. Do not repeat what it
established; go where it did not go and push harder where it only scratched.
What to try is up to you. Things it did not reach, as a starting point only:
the other prebuilt desktop images in build/ (the film-prop scene with the
"Saving tables to disk..." dialog and the pie chart, modal dialogs, controls),
a disk-launched app's own menu bar (new since the first pass), window
behaviour at the screen edges and under heavy overlap, fast and odd input,
keyboard-only use, long sessions, full or awkward disks, many files, and
whatever a hostile tester would think of next.

You are not fixing anything; the deliverable is a findings report.

## Starting point

- Repo: /home/tobias/Projects/initech-os (docs, Makefile, sources). build/
  now holds images built from commit 1cf74a6, newer than the first pass.
- Live desktop boot disk: build/flair_tenants_interactive.img with
  build/flair_data.img as the second IDE disk. The Makefile shows how each
  other image is meant to be booted (search for the image name).
- Reference renderings of the real systems: /home/tobias/Projects/system7-decomp
  and /home/tobias/Projects/win31-decomp.

## Limits

- The repo is read-only for you: no edits, no builds, no `make`, no commits.
  Other agents are working in it. Copy any disk image into your working
  directory (where this brief is) and boot the copy; write only inside that
  directory.
- The owner is using the real desktop on X display :0. Never send input to it
  or capture it. Drive the guest through QEMU itself, as the first pass did.
- Shared machine: one emulator at a time. Kill only processes you started, by
  pid (never `pkill -f`).

## Deliberate, not bugs

The product reproduces a film prop, so these exist on purpose: two stacked
menu bars (the lower one Photoshop-like at rest), the teal desktop, an
hourglass busy cursor, a pie chart summing to 116%, a `570-` figure, the
"Saving tables to disk..." dialog, and "PC LOAD LETTER" as the panic text.
Report them only if they are drawn or behave badly. A panic or hang itself is
always a finding.

## Report

Write `REPORT.md` in your working directory, screenshots under `shots/`,
raw logs under `evidence/`: the worst problems ranked, then every finding
with what you did (replayable), what you saw, what you expected and why, and
the evidence. Number findings G01, G02, ... so they do not collide with the
first pass. Say which image each finding was observed on. Only report what
you observed in this session, and say when an expectation comes from your own
memory of Mac OS rather than from a reference on disk. Check each screenshot
you cite actually shows what you say it shows. ASCII only.

Shut down what you started when done, and end with a short summary.
