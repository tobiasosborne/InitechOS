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
| `kernel/` | builds and boots, InitechDOS only. Host tests parked in `kernel/tests/`, not yet running. Mutation hooks listed in `kernel/tests/MUTANTS-TODO.md`. |
| `dos/` | COMMAND.COM (still linked into the kernel image) and fifteen external utilities, built and on the disk. Known debts in `dos/README.md`: the shell calls kernel internals; the disk utilities use INT 13h/25h/26h directly. |
| `flair/region/` | host-green, mutants converted (8 patches, all killed), compiles for the target. |
| `flair/` | host-green: 52 test runs, 451,944 checks, 25 seconds; builds `libflair.a` for the target. Everything it needs from the kernel is one explicit interface (`flair_platform_*`, 15 functions; `NOTES-FOR-KERNEL.md`). Gate record layouts are pinned against the SDK in target builds. 243 mutation hooks still to convert. |
| `flair/desktop/` | parked, not built: the legacy `kmain.c` (as `kmain_legacy.c`) and the Finder/desktop pieces that were in the kernel. |
| `sdk/` | host-green: self-contained app headers in `sdk/include`, the runtime, one shared fake Toolbox (`sdk/host`), the EXE build rule (`mk/tenant.mk`), ABI tests that pin trap numbers and i386 record layouts. Limits imposed by the OS side are in `sdk/README.md`. |
| `apps/initech123/` | host-green: 18 test programs, about a million checks, 59 real Lotus `.WK1` files, 15 seconds. Builds the real `123.EXE` (73,552 bytes, validated by an independent reader) on the shared fake. 41 mutation hooks still to convert (`tests/MUTANTS-TODO.md`). |
| `apps/initechword/` | host-green: 13 test programs, 22,394 checks, 25 seconds. Target objects compile against `sdk/include`; not yet on `mk/tenant.mk` or the shared fake. 75 mutation hooks still to convert. |
| `apps/initechbase/` | host-green: 75 suites, 44,504 counted checks. Builds 23 target objects and packaging tools without attic dependencies; uses factory/mz. Packaging is blocked by the app's 65,536-byte reserve declaration: 65,616 bytes required. The extended kernel loader already accepts larger reserves; no kernel change is needed. Pair/boot validation and mutation conversion remain with InitechBase (component README and tests inventories). |
| `apps/initechbase/` | host-green: 75 suites (43 against real dBASE III PLUS captures), 44,504 checks, 37 seconds. Codename removed. Target objects build; packaging `INITBASE.EXE` fails on purpose: the app's own loader reserve is 65,536 bytes and it now needs 65,616 (`README.md`, Known blockers; no kernel change needed). 303 mutation hooks still to convert. |
| other `apps/*` | source moved; not started. **Frozen** until the vertical slice below works. |
| `turbo/` | **Self-hosting.** Turbo Initech emits machine code directly (flat programs and extended-memory executables with relocation tables), runs in extended memory, compiles the 17-program corpus inside the OS on QEMU and Bochs with results equal to the Free Pascal goldens, and compiles itself inside the OS to a fixed point: `K2 == K3`, 163,843 bytes, SHA-256 `b37f96ad...3088e3` (`make -C turbo selfhost`, about 2.5 minutes). The independent bootstrap through Free Pascal reaches the same bytes (`make -C turbo ddc`). Tier 0: 45 programs in 20 seconds, 12 encoder mutants killed. Not done: the IDE; the startup runtime is a checked-in byte table derived from NASM (regenerated and compared in tier 0) rather than emitted by the compiler's own encoders; the fixed point is proven on QEMU, not yet on Bochs. |
| `factory/` | QEMU and Bochs drivers, the image builder, the smoke session, the reproducibility and parity checks. |
| `goldens/` | empty; corpora are still read from sibling checkouts, unpinned. |

## In flight

- Import on a branch: the FLAIR Toolbox.
- Conventions pass (owner of `mk/`, the top Makefile, `factory/testlib`,
  `goldens/`): target build fragment, per-test timeouts and component
  time budgets, rebuild on flag changes, pinned corpora, the import
  checks.
- Kernel and DOS host tests (owner of `kernel/`, `dos/`).

## The vertical slice comes first

Host-green components prove nothing about the product until one app
runs on the new tree end to end. Before any further app is imported:
the spine boots, the Toolbox and the desktop come up on it, and Initech
123 is launched from the Finder, saves a worksheet, and the file is read
back on the host, on QEMU and on Bochs. That run is the template for
every other app.

Known blocker for the slice: the OS-side loader refuses app files over
64 KiB (`loader_load_tenant`), and `123.EXE` built with plain flags is
73,552 bytes. That cap is policy from the old size regime and is lifted
as part of the slice.

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
