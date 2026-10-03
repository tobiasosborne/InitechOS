# Drive the InitechOS desktop and tell us what is wrong with it

InitechOS is an operating system for emulated 486 PCs. Its desktop (FLAIR) is
meant to look and feel like Mac OS 8 "Platinum" on top of a DOS 3.3
personality. The owner is unhappy with the window system. His bar: it should
look really awesome for the 1990s and include all the features a user of the
time expected.

Boot it, use it hard, the way a demanding 1990s reviewer would, and find the
bugs, the rough edges, the things that look cheap or wrong, and the features
that are missing or fake. How you drive it and what you try is up to you.
You are not fixing anything; the deliverable is a findings report.

## Starting point

- Repo: /home/tobias/Projects/initech-os (docs, Makefile, sources; there is a
  `bd` issue tracker CLI you can query from there).
- Live desktop boot disk: build/flair_tenants_interactive.img, with
  build/flair_data.img as the second IDE disk. The Makefile target
  `run-flair-tenants` shows the QEMU line. Other prebuilt images are in build/.
- Reference renderings of the real systems: /home/tobias/Projects/system7-decomp
  and /home/tobias/Projects/win31-decomp.

## Limits

- The repo is read-only for you: no edits, no builds, no `make`, no commits,
  no `bd` writes. Other agents are working in it. Copy any disk image into
  your working directory (where this brief is) and boot the copy; write only
  inside that directory.
- The owner is using the real desktop on X display :0. Never send input to it
  or capture it. Your DISPLAY is :99; start a private Xvfb there if you want a
  screen. If a tool would act on the real desktop, do not use it.
- Shared machine: one emulator at a time. Kill only processes you started, by
  pid (never `pkill -f`).

## Deliberate, not bugs

The product reproduces a film prop, so these exist on purpose: two stacked
menu bars (the lower one Photoshop-like), the teal desktop, an hourglass busy
cursor, a pie chart summing to 116%, a `570-` figure, the "Saving tables to
disk..." dialog, and "PC LOAD LETTER" as the panic text. Report them only if
they are drawn or behave badly. A panic or hang itself is always a finding.

## Report

Write `REPORT.md` in your working directory, screenshots under `shots/`:
an honest verdict, the worst problems ranked, then every finding with what you
did (replayable), what you saw, what you expected and why, and the screenshot
or serial-log evidence. Include a list of expected-for-the-era features with
their status (works / broken / fake / absent / could not test). Only report
what you observed in this session, and say when an expectation comes from
your own memory of Mac OS rather than from a reference on disk. ASCII only.

Shut down what you started when done, and end with a short summary.
