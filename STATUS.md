# Status

Rewritten, never appended to. Last rewritten 2026-11-13.

## Where we are

The reboot started on 2026-11-13. The legacy tree is frozen at the tag
`legacy-final` and runs from `../InitechOS-legacy`.

`main` is the product again. `make image` builds the disk set. It boots
to InitechDOS, `AUTOEXEC.BAT` runs `FLAIR`, and the desktop comes up
with the Finder. Initech 123 launches from the `APPS` folder, saves a
worksheet, and Special > Exit to DOS returns to the prompt.

- `make -j16 test`: every component's host tests, about 75 seconds from
  clean, with per-test timeouts, a time budget per component and the
  reference corpora verified against `goldens/CORPORA.lock` first.
- `make status`: the import board, generated. Use it instead of trusting
  the table below.
- `make smoke`: three sessions on QEMU and Bochs at 8 MiB, about 100
  seconds: a scripted DOS session; test programs run from a separate
  test disk; and the desktop journey (open the disk, open APPS, launch
  123, enter two numbers and a sum, save, quit, exit to DOS, start
  FLAIR again) with the saved file checked on the host by an
  independent `.WK1` reader.
- `make -C turbo selfhost` and `ddc`: Turbo Initech compiles itself
  inside the OS to a fixed point (`K2 == K3`), and the Free Pascal
  bootstrap reaches the same bytes.

The product kernel image is 268,670 bytes (kernel, shell, Toolbox,
desktop); the kernel alone is 106,116 bytes and still links and boots.
The loader takes any size up to its 512 KiB window.

## Import board

*Host-green*: its host tests pass and it is listed in `COMPONENTS`.
*Imported* (CLAUDE.md): also its real artifact builds for the target, no
deliberately wrong code is left in product source, and its build reads
nothing from `attic/`. Only `flair/region` is close; none is fully
imported yet.

| Component | State |
|---|---|
| `boot/` | builds and boots; chunked loader; oversized-kernel check. No host tests (none in legacy). |
| `kernel/` | builds and boots, InitechDOS only. Host tests run: 86 gates, 11,214 checks (the legacy totals), 24 seconds, including the FAT differential against mtools and an independent reader. All mutation hooks are out of product source and are patch files: 229 of 268 killed, 39 not yet proven. The product kernel is byte-identical before and after that conversion. |
| `dos/` | COMMAND.COM (still linked into the kernel image) and fifteen external utilities, built and on the disk. Host tests run: 31 gates, 676 checks, 12 seconds. Mutants: 114 of 160 killed, 46 not yet proven. Known debts in `dos/README.md`: the shell calls kernel internals; the disk utilities use INT 13h/25h/26h directly; FIND ignores file name operands. |
| `flair/region/` | host-green, mutants converted (8 patches, all killed), compiles for the target. |
| `flair/` | host-green: 52 test runs, 451,944 checks, 25 seconds; builds `libflair.a` for the target. Everything it needs from the kernel is one explicit interface (`flair_platform_*`, 15 functions; `NOTES-FOR-KERNEL.md`). Gate record layouts are pinned against the SDK in target builds. 243 mutation hooks still to convert. |
| `flair/desktop/` | builds and runs: the Finder, the event pump, tracking, launching, volumes, dialogs, extracted from the legacy `kmain.c` into eight files; the platform binding to the kernel. 17 host suites. `kmain_legacy.c` is down from 8,207 lines to 335 (film scene and text-app hand-off fragments, not built). Parked, not built: film scene, easter eggs, about box. |
| `system/` | the final link of the product kernel image and the list of what goes on the working volume. |
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

Nothing. All branches from day one are merged.

## The vertical slice is done

One app runs end to end on the new tree, on both emulators, checked
mechanically. That run is the template for every other app: build the
executable with `mk/tenant.mk`, add it to `system/working-volume.list`,
add its journey to the desktop smoke with a host-side check of what it
saved. App imports are unfrozen.

## Next

1. Make `make status` say "imported" for every component: convert the
   remaining mutation hooks to patches (Initech 123, InitechWord,
   InitechBase, the Toolbox, the SDK's three, the seed), declare each
   component's target artifacts, and check why the audit still flags
   the kernel after its hooks were converted (likely the pattern).
2. Put InitechWord and InitechBase on the disk and into the smoke, each
   with a host-side check of a saved file. InitechBase first needs its
   loader reserve raised (`apps/initechbase/README.md`).
3. Import the remaining apps: InitechCalc, InitechPaint, InitechText,
   Calculator, Note Pad and Scrapbook, Control Panels.
4. The frame (GOLDEN-MASTER item 3): the film scene on the new desktop,
   the real ledger in place of `TODO_FRAME`, FILE COPY made safe.
5. The Turbo Initech IDE; the runtime emitted by the compiler's own
   encoders; the fixed point on Bochs.
6. Tier 2 (`make cert`) and the first end-of-day run.
7. The golden-master bugs in `bd ready`, data loss first.

## Notes for this week

- Disk layout today (inherited): IDE master is the boot volume (C:),
  IDE slave is the working volume (A:) with CONFIG.SYS, the utilities
  and a COMMAND.COM stub. A single installable disk is still to design.
- Bochs here has no 486 model and runs its Pentium model; QEMU runs
  `-cpu 486`.
- Open question for the operator: Python is used by some inherited
  tests as an independent reader of foreign file formats, which the
  constitution's "C, make and POSIX shell" does not allow as written.
  The kernel's uses are listed with port estimates in
  `kernel/tests/PYTHON.md`.
- 85 kernel and DOS mutants are converted to patches but not yet proven
  killed (`make -C kernel mutants`, `make -C dos mutants`). Each is
  either a test gap or a mutant that needs the emulator.
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
