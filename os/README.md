# os/ — THE ARTIFACT (Pascal)

The InitechOS itself: kernel, Toolbox, apps, and the resident compiler, all
written in **Pascal** targeting freestanding x86 (CLAUDE.md Law 3, PRD
§6.7, §14). It stays period-authentic — **no C here, no 2026-isms** (no
timestamps, no host paths, no nondeterminism); reproducible builds are a
hard requirement because the self-host fixpoint depends on them (PRD §7).

Layout:

- `boot/` — MBR -> stage2 -> 32-bit protected/flat bring-up (PRD §5, M1).
- `milton/` — InitechDOS kernel: FAT12/16, `INT 21h` API, program loader,
  `COMMAND.COM`-alike shell (PRD §6.1).
- `flair/` — the System-7-style Toolbox: Window/Menu/Control/Event/Dialog
  managers and bitmap fonts (PRD §6.3).
  - `flair/atkinson/` — the region engine, the load-bearing GUI math
    (PRD §6.2).
- `apps/` — InitechCalc, File Manager, InitechPaint, FILE COPY (PRD §6.5).
- `samir/` — InitechBase, the dBASE-alike (PRD §6.6).
- `tps/` — Turbo Initech: the resident self-hosting compiler + blue
  Turbo-Vision IDE (PRD §6.7).

NOTE: per beads `initech-tse` this directory currently holds **dirs and
READMEs only** — no Pascal has been written yet (C-only factory comes
first).

Governed by: **PRD §4, §6.1-§6.7.**
