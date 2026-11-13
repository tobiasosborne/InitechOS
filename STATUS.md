# Status

Rewritten, never appended to. Last rewritten 2026-11-13.

## Where we are

The reboot started today. The legacy tree is frozen at the tag
`legacy-final` and runs from `../InitechOS-legacy`. `main` now holds the
same source in the new layout, with no working build yet: components
come back one at a time, each with its own Makefile and tests.

Nothing on `main` boots yet. To see the product, use the legacy tree.

## Import board

A component is *imported* when `make test` passes inside it and it is
listed in `COMPONENTS` in the top Makefile.

| Component | State |
|---|---|
| `boot/` | source moved; chunked loader being proven on the legacy tree |
| `kernel/` | source moved (still contains the shell, the DOS utilities and the desktop glue in `kmain.c`) |
| `dos/` | empty; to be split out of `kernel/` |
| `flair/region/` | **imported**: 2 oracle programs in about a second; 8 mutants killed |
| `flair/` (the rest) | source moved |
| `flair/desktop/` | empty; Finder files and the desktop half of `kmain.c` go here |
| `sdk/` | source moved (`tbx.h`, runtime, linker script) |
| `apps/initech123/` | **imported**: 18 test programs, about a million checks, 59 real Lotus `.WK1` files, 15 seconds. Target objects compile; the EXE waits for `sdk/`. 41 mutation hooks still to convert (`tests/MUTANTS-TODO.md`). |
| other `apps/*` | source moved; none imported |
| `turbo/` | source moved (`compiler/`, `seed/`) |
| `goldens/`, `factory/` | empty; filled from `attic/` as components claim things |

## In flight

- Spine: `boot/` + an InitechDOS-only `kernel/` + `dos/` + `factory/`
  to an `A:\>` prompt on QEMU and Bochs, branch `spine/boot-kernel`.
  The loader spike it builds on is proven (branch `reboot/loader-spike`):
  a 300,108-byte kernel boots on both emulators.
- Imports on branches: InitechBase, InitechWord, Turbo Initech, the
  SDK, the FLAIR Toolbox.

## Next

1. Merge the spine.
2. The remaining host cores (InitechBase, InitechWord, the seed
   compiler, the Toolbox managers), then the desktop and the apps.
3. Product work against `GOLDEN-MASTER.md`, starting with the frame's
   ledger and the FILE COPY data-loss bug.

## Notes for this week

- Orchestration: implementation and review go to Sol 6.1 at xhigh by
  default; Astra for hands-on emulator driving; Opus for hard design;
  one Muse Wonder Full at a time for an independent opinion.
- 8 MiB is the minimum machine. In QEMU that is `-m 8 -machine
  acpi=off`; plain `-m 8` loses memory to ACPI tables and is refused.
- Reference corpora are read from `~/Projects/*-decomp`
  (`mk/config.mk`). They get pinned under `goldens/` later.
- The tracker was reset from the triage (`attic/triage/`): 119 issues
  are golden-master work (label `gm`, `comp:<component>`,
  `sev:<severity>`; data loss is P0), 17 need checking against the
  running product (`verify`), 164 are Release 2 (deferred), 117 were
  closed as obsolete. `bd ready` now shows only golden-master work.
- The per-component import manifest is in `attic/manifest/`.
- Codenames are retired (CLAUDE.md, Names). Directories are renamed
  to product names. Identifiers and file names (`samir_*`, `pal_milton.c`,
  `SAMIR.EXE`, `tps.pas`) are renamed as each component is imported.
