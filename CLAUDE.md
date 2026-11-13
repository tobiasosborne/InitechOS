# CLAUDE.md -- InitechOS

Read this top to bottom at the start of every session and again after a
context compression. It is short on purpose. It is the only standing law
in this repository.

## What this is

InitechOS is the fake computer screen from *Office Space* ("Saving tables
to disk...") built as a real early-1990s product: a DOS 3.3-style kernel
under a Mac OS 8 Platinum-style desktop, booting on an emulated 486, with
a dBASE III+ clone, a Lotus 1-2-3 clone, a WordPerfect clone and a
self-hosting Pascal compiler on top. It really boots and the software
really works. That is the joke, and it is told with a straight face.

- `InitechOS-PRD.md` is the product vision.
- `GOLDEN-MASTER.md` is the closed list of what ships. It governs scope.
- `STATUS.md` is where the project stands today. It is rewritten, never
  appended to.

## The constitution

These nine points are the project's identity. Only the operator changes
them.

1. **It is real.** Deterministic code all the way down. No mock-ups, no
   baked screenshots, no hallucinated behaviour.
2. **Straight face.** Everything a user can see (the screen, the README,
   the manuals, the box) reads as a real corporate product of 1991-93 and
   never admits the joke. The marks are Initech's own: no other company's
   logo appears anywhere. The product is framed as abandonware: the
   found software of a company that no longer exists, preserved and
   reconstructed. Its original "license agreement" is reproduced as a
   period document and is not a licence; the real one is the AGPL in
   `LICENSE`, mentioned once and discreetly. The canon bugs are enforced, not fixed: the pie
   chart sums to 116%, the busy cursor is an hourglass, negatives print
   as `570-`. Working documents (this file, STATUS, component READMEs,
   commit messages) are plain English.
3. **Period-plausible.** A 486 with 8 MiB, VGA/VBE, ATA, a PS/2 mouse.
   32-bit flat, cooperative scheduling, no memory protection, no real
   16-bit binaries. Nothing from 2026 leaks into the artifact: no
   timestamps, no host paths, no nondeterminism.
4. **The oracle is the truth.** Work is done when a mechanical check is
   green, and the check compares against something independent of the
   code under test: real period software where a reference exists (real
   dBASE III+, real Lotus 1-2-3, real WordPerfect, Free Pascal, mtools),
   a property suite where it does not. A check that computes its expected
   value from the code it grades is not a check. A test that has never
   failed has proven nothing: show it red once.
5. **Ground truth before code.** Cite the source for every behavioural
   decision: a capture from real software, a hardware reference, the
   frame. If you do not have the source, get it before writing the code.
6. **Three languages, never blurred.** The OS and its bundled apps are C
   and assembly. Turbo Initech and the programs it compiles are Pascal.
   The factory (everything that builds and judges the product) is C,
   make and POSIX shell. One exception: a test may use a Python script
   as an independent reader of a foreign file format, because a second
   reader in another language is a stronger check. Python builds
   nothing and ships nowhere.
7. **Reproducible builds.** The same source produces the same bytes. The
   self-host proof (`K2 == K3`) depends on it.
8. **Closed scope.** If it is not in `GOLDEN-MASTER.md` it is not being
   built. New ideas go on that file's Release 2 list. Only the operator
   moves an item between the lists.
9. **Local only.** No GitHub CI and no remote automation. Checks run on
   this machine.

## Names

No codenames. A program is called by its product name everywhere: in
directories, file names, identifiers, messages and documents. The one
exception is FLAIR, the name of the graphical environment. The legacy
codenames MILTON, SAMIR, TPS, ATKINSON and STAPLER are retired; when you
import a component, rename what still carries one.

The product names are inconsistent on purpose, the way a real corporate
catalogue is. Write each exactly as shown and never normalise them:

| Product | Directory | On disk |
|---|---|---|
| InitechOS | the whole | |
| InitechDOS | `kernel/`, `dos/` | `IO.SYS`, `INITDOS.SYS`, `COMMAND.COM` |
| FLAIR | `flair/` | |
| InitechBase | `apps/initechbase/` | `INITBASE.EXE` |
| Initech 123 | `apps/initech123/` | `123.EXE` |
| InitechWord | `apps/initechword/` | `WORD.EXE` |
| InitechCalc | `apps/initechcalc/` | `CALC.EXE` |
| InitechPaint | `apps/initechpaint/` | `PAINT.EXE` |
| InitechText | `apps/initechtext/` | `TEXT.EXE` |
| Turbo Initech | `turbo/` | `TURBO.COM` |
| FILE COPY | part of FLAIR | |
| Calculator, Note Pad, Scrapbook, Control Panels | `apps/calculator/`, `apps/notepad/`, `apps/controlpanels/` | `CALCULAT.EXE`, `NOTEPAD.EXE`, `SCRAPBK.EXE`, `CPANELS.EXE` |
| TPS Report Generator | `dos/` | `TPSREP.COM` |

"TPS" survives only in the TPS Report Generator, which is a product
about TPS reports, not a codename.

## How the tree is organised

```
boot/          MBR, stage2, IO.SYS: gets the kernel into memory
kernel/        InitechDOS. No GUI code.
dos/           COMMAND.COM and the external utilities
flair/         the Toolbox (regions, windows, menus, controls, events ...)
flair/desktop/ the Finder and the desktop shell
sdk/           the app interface (tbx.h) with a host fake
apps/<name>/   one directory per app
turbo/         Turbo Initech and its C seed compiler
goldens/       captures from real period software, hash-pinned
factory/       emulator drivers, image builder, diff tools
mk/            shared make fragments
attic/         legacy tests, specs and tools not yet claimed (see below)
```

**Two interfaces.** DOS programs reach the OS through INT 21h. Desktop
apps reach it through the Toolbox gate, INT 81h (`sdk/`). Nothing else
crosses a component boundary. A component may include its own headers,
`sdk/` headers, and the public headers (`<component>/include/`) of the
components its README lists as dependencies. Include paths are set per
component, and `#include "../..."` out of a component is forbidden; the
component's `make test` checks both.

**Every component is self-contained.** Its directory holds its source,
its tests, a `Makefile` and a short `README.md`. From inside it:

| Command | Meaning | Must be |
|---|---|---|
| `make` | build its artifacts into `build/<component>/` | deterministic |
| `make test` | tier 0: host tests against goldens | seconds, no emulator |
| `make smoke` | tier 1: drive it in the booted product image | minutes |
| `make mutants` | apply each patch in `mutants/`, expect red | tier 2 only |

**A component is imported when all four hold**, and `make test` in it
checks them: its host tests are green; its real artifact builds for the
target; no deliberately wrong code is left in its product source; its
build reads nothing from `attic/`. Until then it is only *host-green*.

**A target that is not implemented fails.** No stub ever reports
success.

**One product image.** `make image` at the top builds the image a user
would boot. Tests drive that image from outside (injected keys, serial
markers, files read back off the disk) or run as programs on its disk.
Test scenarios are never compiled into the kernel, and deliberately
wrong code never lives in product source: a mutant is a patch file.

## Landing a change

1. Run `make test` at the top. It runs every component's tier 0 and is
   meant to stay fast enough that nobody skips it; each component has a
   time budget and a slow suite is a defect. Run `make smoke` as well if
   you changed the kernel, the Toolbox or either interface.
2. Commit to `main` and push. No lanes, certificates or proof packs.
3. Tier 2 (`make cert` at the top: both emulators, pixel goldens, the
   mutation sweep, the reproducible hash) is run by the orchestrator at
   the end of each working day and on release candidates. If it goes
   red, revert or fix forward before new work starts.

When several agents work at once, each owns one component directory.
The shared places (`mk/`, `sdk/`, `factory/`, the top Makefile) have one
owner at a time, named in `STATUS.md`; everyone else reports what they
need instead of editing them.

## Rules have a scope and an expiry

- This file: standing law, operator-owned.
- A component's `README.md`: decisions that apply inside that component.
- `STATUS.md`: notes for the current week. They expire when it is next
  rewritten.

There is nowhere else for a rule to live. If a rule blocks the sensible
move, stop and bring the conflict to the operator with a proposed
repeal. Do not route around it, and do not add a rule to compensate for
another rule.

Anything called temporary names the event that removes it, in the place
where it is used. Comments and tests cite this file by section name;
citations of the legacy "Laws" and "Rules" by number are removed on
import, because those no longer exist.

## The legacy tree

Everything before 2026-11-13 is at the tag `legacy-final`, checked out
read-only at `../InitechOS-legacy` (build it there with
`. tools/lane-env.sh && make -j32 factory image`). Its Makefile, gates,
merge process, ADRs, handoff and rulings are history, not law. Use it:

- as the reference for behaviour: when the new tree and the legacy tree
  disagree on a screen or an output, find out why before changing either;
- to look up how something was compiled (`make -n <target>` there);
- as the source of tests and goldens that have not been imported yet.

`attic/` holds the legacy harness, specs, fixtures, tools and manuals.
Nothing in it is built. Claim what your component needs with `git mv`,
never edit in place, never build from there, never add to it. Every
import ends by naming, in `STATUS.md`, the future owner of whatever it
left behind. The attic is deleted before the master ships. Import notes
(`tests/IMPORT.md`, `SMOKES.md`, `MUTANTS-TODO.md`) are migration
records, not a format to keep producing: each is deleted once acted on.

## Hazards that are still true

- Real mode to protected mode is a minefield (A20, GDT, PE, the far
  jump). Verify in Bochs, not only QEMU. A triple fault in QEMU silently
  reboots: run with `-d int,guest_errors,cpu_reset`.
- The frame is a Photoshop mock-up, not a screenshot. Its
  inconsistencies (a Photoshop menu bar on a Mac window over DOS) are
  the spec. Do not correct them. One exception, by the operator: there
  is one menu bar, not the frame's two. It shows the front program's
  menus, so the Photoshop menus appear when InitechPaint is in front.
- Glyphs are hand-authored and back-checked against the frame. The still
  is too low-resolution to extract them.
- Regions are normalised on construction and asserted normal on entry to
  every merge. A non-normal region produces plausible garbage.
- `.dbf` round-trips compare meaningful bytes: normalise the header date
  and reserved bytes before diffing.
- xBase typing is strange. The coercion rules are captured from real
  dBASE, never inferred.
- Two compilers: the seed is C and runs on the host; Turbo Initech is
  Pascal and runs in the OS. The self-host certificate is `K2 == K3`,
  never `K1 == K2`.
- Scheduling is cooperative. An app that does not yield hangs the
  desktop. That is authentic.

## Tracking and session close

Work is tracked in `bd` (beads): `bd ready`, `bd show <id>`,
`bd update <id> --claim`, `bd close <id>`. Use `bd remember` for durable
technical facts, not for rules.

Before ending a session: tests for what you touched are green, the work
is committed and pushed, `STATUS.md` is true, and anything left over is
a `bd` issue.
