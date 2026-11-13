# Status

Rewritten, never appended to. Last rewritten 2026-11-13.

## Where we are

The reboot started on 2026-11-13. The legacy tree is frozen at the tag
`legacy-final` and runs from `../InitechOS-legacy`.

`main` boots. `make image` builds the product disk set from clean in
about 7 seconds; `make smoke` boots it on QEMU and Bochs at 8 MiB, runs
a scripted DOS session (VER, DIR, directories, COPY, REN, DEL, a batch
file, SORT, MORE, FIND, CHKDSK), reads the written files back on the
host with mtools and checks for CPU faults, in about 30 seconds.
`make test` runs every component's host tests. There is no desktop on
`main` yet; for the GUI, use the legacy tree.

The kernel is 105,848 bytes with one 14-line entry point and no test
scenarios in it. The loader takes a kernel of any size up to its
512 KiB window (`make -C boot bigkernel-check` proves it with a
237,112-byte kernel on both emulators).

## Import board

*Host-green*: its host tests pass and it is listed in `COMPONENTS`.
*Imported* (CLAUDE.md): also its real artifact builds for the target, no
deliberately wrong code is left in product source, and its build reads
nothing from `attic/`. Only `flair/region` is close; none is fully
imported yet.

| Component | State |
|---|---|
| `boot/` | builds and boots; chunked loader; oversized-kernel check. No host tests (none in legacy). |
| `kernel/` | host tests imported: 86 gate invocations, FAT/mtools/reference/property suites plus DOS services. Product builds and boots. 268 mutation patches converted: 229 host-killed, 39 explicitly pending tier-1/diagnostic coverage. |
| `dos/` | 31 host gate invocations; shell/batch/env, CHKDSK, disk utilities, EDLIN, XCOPY/TREE, directory and catalog checks. 160 converted patches: 114 host-killed, 46 pending. COMMAND.COM remains resident; interface debts remain in README. |
| `flair/region/` | host-green, mutants converted (8 patches, all killed). Not yet compiled for the target. |
| `flair/` (the rest) | source moved |
| `flair/desktop/` | parked, not built: the legacy `kmain.c` (as `kmain_legacy.c`) and the Finder/desktop pieces that were in the kernel. |
| `sdk/` | source moved (`tbx.h`, runtime, linker script) |
| `apps/initech123/` | host-green: 18 test programs, about a million checks, 59 real Lotus `.WK1` files, 15 seconds. Target objects compile; the EXE waits for `sdk/`. 41 mutation hooks still to convert (`tests/MUTANTS-TODO.md`). |
| `apps/initechword/` | host-green: 13 test programs, 22,394 checks, 25 seconds. Target objects compile; the EXE waits for `sdk/`. 75 mutation hooks still to convert. Borrows shared headers from `attic/spec`. |
| other `apps/*` | source moved; not started. **Frozen** until the vertical slice below works. |
| `turbo/` | source moved (`compiler/`, `seed/`) |
| `factory/` | QEMU and Bochs drivers, the image builder, the smoke session, the reproducibility and parity checks. |
| `goldens/` | empty; corpora are still read from sibling checkouts, unpinned. |

## In flight

- Imports on branches: InitechBase, Turbo Initech, the SDK, the FLAIR
  Toolbox.
- Conventions pass (owner of `mk/`, the top Makefile, `factory/testlib`,
  `goldens/`): target build fragment, per-test timeouts and component
  time budgets, rebuild on flag changes, pinned corpora, the import
  checks.
- Kernel/DOS tests on import/kernel-tests: no product mutation guards remain.
  Default patch sweeps cover proved host faults; all 85 pending patches are
  listed in kernel/tests/MUTANTS-TODO.md and dos/tests/MUTANTS-TODO.md.
  factory smoke-programs drives normal programs on a separate test data disk
  under QEMU and Bochs. Full legacy emulator migration is in both SMOKES.md files.

## The vertical slice comes first

Host-green components prove nothing about the product until one app
runs on the new tree end to end. Before any further app is imported:
the spine boots, the Toolbox and the desktop come up on it, and Initech
123 is launched from the Finder, saves a worksheet, and the file is read
back on the host, on QEMU and on Bochs. That run is the template for
every other app.

## Next

1. The vertical slice: Toolbox + desktop on the booting kernel, Initech
   123 launched, saved and read back.
2. The remaining host cores (InitechBase, InitechWord, the seed
   compiler, the Toolbox managers), then the desktop and the apps.
3. Product work against `GOLDEN-MASTER.md`, starting with the frame's
   ledger and the FILE COPY data-loss bug.

## Notes for this week

- Disk layout today (inherited): IDE master is the boot volume (C:),
  IDE slave is the working volume (A:) with CONFIG.SYS, the utilities
  and a COMMAND.COM stub. A single installable disk is still to design.
- Bochs here has no 486 model and runs its Pentium model; QEMU runs
  `-cpu 486`.
- Open question for the operator: Python is used by some inherited
  tests as an independent reader of foreign file formats, which the
  constitution's "C, make and POSIX shell" does not allow as written.
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

## Remaining import ownership

- Kernel/DOS test owner: pending mutations, ASM-only filter/TPS Report Generator
  regression sessions, additional mapped normal-image probes, and the overlay
  relocation-address deviation from captured DOS 3.30 behavior (loader.c).
- Factory/diagnostic owner: the retired harness/meta/layout material still in
  attic, panic/pixel graders and eventual diagnostic-object image support.
- FLAIR desktop owner: DESKTOP.DB/Finder and old desktop shell/layout host tests
  left in attic. The exact-address COMMAND checker is retired.
- Operator: whether independent Python readers are permitted. Every imported
  runtime dependency is enumerated in kernel/tests/PYTHON.md; no port was made.
