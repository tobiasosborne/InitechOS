# 2026-11-13: the reboot review, and the end of the legacy tree

This directory is the last thing committed to the legacy tree. The commit
that contains it is tagged `legacy-final`. After that tag, `main` starts
over: same repository, same source code, new structure. Everything that
existed before (the 35,716-line Makefile, the gate vector, the merge
process, the handoff, the worklogs, the ADRs, the rulings) stays reachable
at the tag and is no longer in force.

## What was found

Five independent investigations ran on 2026-11-13 at `b571f1e3`:

| Report | Subject |
|---|---|
| `report-A-build-gates.md` | The build and test system, measured |
| `report-B-architecture.md` | Module boundaries, memory map, what to keep |
| `report-C-product-state.md` | Distance from here to a golden master |
| `report-D-process.md` | Where the effort went, June to November |

A fifth, unanchored opinion (Muse Wonder Full) reached the same diagnosis
without being told it. Links inside the reports to a session scratch
directory are dead; the reports themselves are complete.

The product is real. On the day of the review the tree booted in QEMU,
the Finder opened the disk, Initech 123 launched from `APPS` and summed
two cells (`screen-initech123-579.png`). The film frame exists as a scene
with `TODO_FRAME` in its ledger (`screen-film-frame-todo.png`).

The factory around the product had grown larger than the product:

- 1,740 active gates; 46% mutant twins; 46% boot an emulator.
- 352 kernel link variants, 269 of them mutants.
- A change to any file in Initech 123's core selected 1,909 gates.
- Every build target depended on one global Makefile stamp.
- The kernel sat 120 bytes under a size cap that came from the boot
  loader reading it in one real-mode pass into a 192 KiB buffer.
- Product code fell from 47% of the work in June to 26% in November.
- 82 numbered decisions, 112 operator rulings and 134 stored memories,
  with no way to delete one, only to supersede it.

No step was a mistake when it was taken. Together the rulings left one
legal move, shaving bytes, and the swarm made it eight times.

## What was decided (operator, 2026-11-13)

1. Keep the code. Replace the factory. A rewrite is not justified.
2. `main` starts over in this repository; the old tree stays at
   `legacy-final` as a runnable reference.
3. All rulings lapse. The new tree starts from one short page.
4. The boot loader reads the kernel in chunks; the size cap, the kernel
   overlays, the size rounds and the cold-code splits are retired. This
   reverses the 2026-10-24 ruling for overlays.
5. One product image. Tests drive it from outside or run as programs on
   its disk. Test scenarios are no longer compiled into the kernel.
6. Each component has its own directory, build and tests, behind two
   interfaces: INT 21h for DOS programs, INT 81h for desktop apps.
7. Tests run in three tiers: per-component host tests on every change,
   an emulator smoke for interface changes, the full certificate nightly
   and on release candidates.
8. Changes land on `main` directly when their tier is green. The lane
   certificates, proof packs, serial merge role and sentinel are retired.
9. The golden master is a closed list: boot and DOS; the film frame live
   and complete; InitechBase at the dot prompt with a frozen omission
   list; Turbo Initech self-hosting inside the OS with `K2 == K3` and a
   minimal IDE; Initech 123, InitechWord and the accessories at their
   present depth with every visible command working or removed; a working
   install path; manuals regenerated once at the end. Everything else is
   Release 2.

## Where things are

- The legacy tree: `git checkout legacy-final`.
- Unmerged legacy work worth knowing about: `origin/exp/dbase-oneshot`
  (eight full-screen InitechBase apps at a relaxed bar),
  `origin/lane/flaircmd` (FLAIR started as a command from DOS),
  `origin/lane/pascaltbx`, `origin/lane/iwordp4`, `origin/lane/l123man`.
- The issue tracker export at the tag: `.beads/issues.jsonl` (417 open).
- The manuals: `pubs/` at the tag. They return at the end.
