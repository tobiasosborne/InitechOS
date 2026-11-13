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
| `flair/` | source moved |
| `flair/desktop/` | empty; Finder files and the desktop half of `kmain.c` go here |
| `sdk/` | source moved (`tbx.h`, runtime, linker script) |
| `apps/*` | source moved; none imported |
| `tps/` | source moved (`compiler/`, `seed/`) |
| `goldens/`, `factory/` | empty; filled from `attic/` as components claim things |

## In flight

- Loader spike: chunked kernel load on the legacy tree, branch
  `reboot/loader-spike`. Everything else about `boot/` waits on it.
- Import manifest: per-component list of sources, tests, goldens and
  couplings, written from the legacy Makefile.
- Triage of the 417 legacy issues against `GOLDEN-MASTER.md`.

## Next

1. Pilot imports of two host-only cores to settle the conventions in
   `mk/`: the region engine and Initech 123.
2. The spine: `boot/` + `kernel/` + `factory/` to a booting
   `A:\>` prompt on QEMU and Bochs, as the first product image.
3. The remaining host cores (InitechBase, InitechWord, the seed
   compiler, the Toolbox managers), then the desktop and the apps.
4. Product work against `GOLDEN-MASTER.md`, starting with the frame's
   ledger and the FILE COPY data-loss bug.

## Notes for this week

- Orchestration: implementation and review go to Sol 6.1 at xhigh by
  default; Astra for hands-on emulator driving; Opus for hard design;
  one Muse Wonder Full at a time for an independent opinion.
- Reference corpora are read from `~/Projects/*-decomp`
  (`mk/config.mk`). They get pinned under `goldens/` later.
- The tracker still holds the legacy issues. It is reset once the triage
  lands.
