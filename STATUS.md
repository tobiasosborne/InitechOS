# Status

Rewritten, never appended to. Last rewritten 2026-11-13, end of the
first day of the reboot. A new machine: start with `docs/SETUP.md`. A fresh clone from GitHub
at `~/Projects/` was checked at this commit: `make -j16 test`, `make
image` and `make smoke` are green.

## Where we are

The legacy tree is frozen at the tag `legacy-final`. `main` is the
product again, in a new structure: every component has its own
directory, Makefile and tests, and the factory around them is about 600
lines of make and shell.

What `main` does, checked mechanically on QEMU and Bochs at 8 MiB:

- Boots to InitechDOS; `AUTOEXEC.BAT` runs `FLAIR`; the Finder comes up
  with one menu bar. Special > Exit to DOS returns to the prompt.
- Initech 123 launches from the `APPS` folder, takes input, saves; an
  independent `.WK1` reader on the host confirms the file.
- InitechBase runs from the prompt: USE, LIST, REPLACE, APPEND, INDEX
  and SEEK, programs, the Y2K accounting program with its canon bug;
  the `.dbf` and `.ndx` are verified on the host.
- Turbo Initech compiles the corpus inside the OS, and compiles itself
  to a fixed point: `K2 == K3`, 163,843 bytes, SHA-256 `b37f96ad...`
  (`make -C turbo selfhost`); the Free Pascal bootstrap reaches the same
  bytes (`make -C turbo ddc`).

The commands:

| Command | What | Time |
|---|---|---|
| `make corpus` | reference files present and matching `goldens/CORPORA.lock` | under 1 s warm |
| `make -j16 test` | every component's host tests | about 75 s |
| `make image` | the product disk set | about 50 s clean, seconds warm |
| `make smoke` | five sessions on both emulators: DOS, test programs, the desktop journey, Turbo Initech, InitechBase | about 3 min |
| `make status` | which components are imported, and which condition each fails | seconds |
| `make repro` | two clean builds, identical hashes | about 1 min |
| `make cert` | tier 2. **Not on main** (fails on purpose); built on branch `wave2/factory` | - |

`make status` today: imported: `flair/region`, `apps/initech123`,
`apps/initechbase`. Host-green: the rest. Three of the host-green lines
are the audit's own false positives (kernel, dos and flair have no
hooks left); the fix is on `wave2/factory`.

## Three branches are finished or nearly so and NOT merged

They are pushed to GitHub. Land them in this order; each needs the
adjustments listed, because `main` moved under them.

### 1. `wave2/initechword` (2 commits; its agent was stopped mid-run)

Restructures the desktop smoke so each app owns its journey
(`apps/<app>/tests/smoke/journey.sh`, discovered from a list, several
journeys per boot), moves the Initech 123 journey to
`apps/initech123/tests/smoke/`, builds `WORD.EXE` through
`mk/tenant.mk`, puts it on the disk with its own journey and an
independent `.WP5` check, converts InitechWord's 75 mutation hooks to
patches. Not verified by the orchestrator.

To land: merge; the one-menu-bar change on `main` moved everything
below the menu bar up 20 pixels, so apply to the moved 123 journey
what `main` applied to `factory/desktop-trace.sh` (the `open_disk`
trace: both `m100:-88` become `m100:-98`) and check the Word journey's
coordinates the same way (targets moved: volume 600,64 to 600,44; APPS
56,102 to 56,82; Special menu press 170,30 to 170,10); run
`make smoke`; check its final steps (import check, README inventory)
were finished.

### 2. `wave2/smallapps` (10 commits; finished and self-verified)

Calculator, Note Pad, Scrapbook, InitechText, Control Panels,
InitechCalc and InitechPaint: built with `mk/tenant.mk`, host tests on
the shared fake Toolbox, 42 mutants as patches (all killed), each
launched on QEMU (InitechCalc and InitechText also on Bochs) with a
host-side check of what it saved. `apps/ref` is deleted; the font
specimen and the C example are SDK examples, not on the disk.

To land: (a) the top Makefile's `desktop-apps` must build the six
product apps before the image is assembled (today it builds 123 and
InitechBase; `make repro` fails without this); (b) its journeys were
written against the two-band desktop: move click targets below the
menu bar up 20 pixels, and plug them into the journey list from branch
1; (c) InitechCalc compiles Initech 123's sources through a marked
temporary block: switch it to `build/apps/initech123/libi123.a`, which
is now on `main`; (d) add its components to `COMPONENTS`.

### 3. `wave2/factory` (6 commits; its agent was stopped mid-run)

Separates build variants that wrote the same object file (the
`dos/command.o` thrash), fixes the status audit's false positives,
declares target artifacts, converts the SDK's 3 and the seed's 17
mutation hooks, and composes `make cert`. Not verified by the
orchestrator.

To land: merge last (it rewrites the top Makefile; keep the
`desktop-apps` lines from branches 1 and 2 and `SMOKE_COMPONENTS`).
**It removes two deliberately wrong entry points from the compiler's
runtime, so the self-host fixed point changes.** Regenerate the
compiler's runtime table if the branch did not, then re-run
`make -C turbo test`, `selfhost` and `ddc` and record the new hash
here. Then run `make cert` for the first time.

## Known defects found on day one

- Initech 123's window still opens at y=40, leaving a 20-pixel strip of
  desktop under the single menu bar. Apps request their own initial
  frame; each needs moving up.
- The menu bar's mark is still the Apple glyph. Ruled out by the
  operator (CLAUDE.md, constitution point 2); to be replaced with
  Initech's mark derived from the boot logo.
- Finder icon labels overlap when names are long (`ATTRIB.COM`).
- `test_iword_listfiles_ops` fails when the checkout path is very long
  (seen in a deep scratch directory; green at `~/Projects/InitechOS`).
- FIND ignores file name operands; the kernel's overlay loading differs
  from captured DOS 3.30 (both noted by the kernel test import).
- The film frame's ledger still has `TODO_FRAME` cells. Under the lore
  these are gaps in what survived and are shown as gaps, not invented.
- Mutants converted but not yet proven killed: kernel 39, dos 46,
  flair 16, initechbase 28. Each needs the emulator or a better test.
- Initech 123 shows 224 slash-menu leaves it refuses; InitechPaint 24.
  GOLDEN-MASTER item 6: each either works or is removed.

## Next, in order

1. Land the three branches above; run `make cert`; fix what it finds.
2. `make status` says "imported" for every component.
3. Initech's mark in the menu bar; app windows up 20 pixels.
4. The frame on the new desktop (GOLDEN-MASTER item 3): the film scene,
   FILE COPY made safe, the ledger with its gaps shown honestly.
5. The seven data-loss bugs at the top of `bd ready`.
6. Dead commands: a proposed split (implement or remove) for Initech
   123, InitechWord and the small apps, for the operator.
7. The Turbo Initech IDE; the runtime emitted by the compiler's own
   encoders; the fixed point on Bochs.
8. The install path and a single product disk (GOLDEN-MASTER item 8).

## Notes for this week

- Data-loss reference research is committed locally on `goldens/dataloss`
  (not pushed or landed): `goldens/dataloss/` captures real DOS 3.30 COPY,
  Lotus R2.2 Name Table/Labels protection, and WordPerfect 5.1 backup
  collision/full-disk saves. In particular, Lotus retains unprotected prefix
  writes before a protection error, and WP may leave the old document only
  as `.BK!`. The four data-loss issues remain open; product fixes are still
  outstanding. Corpus HEADs and the existing input lock are unchanged.
- One owner per directory when several agents work at once; the shared
  places (`mk/`, the top Makefile, `factory/`) had one owner, the
  `wave2/factory` branch. After it lands, name the next owner here.
- Day one's lesson on parallel work: branches that start before a
  shared interface settles need an integration pass. Land interface
  changes first, then fan out.
- Briefs that fence an agent too tightly make it stop; give each agent
  the directories its task really touches.
- 8 MiB is the minimum machine: `-m 8 -machine acpi=off` in QEMU.
- Bochs here runs its Pentium model (no 486 model in this build); QEMU
  runs `-cpu 486`.
- Disk layout is inherited: IDE master is the boot volume, IDE slave
  the working volume (A:). A single installable disk is still to design.
- Orchestration: implementation and review on Sol 6.1 at xhigh; Astra
  for hands-on emulator driving; Opus for hard design; one Muse Wonder
  Full at a time for an independent opinion. Verify what an agent
  reports before merging it.
