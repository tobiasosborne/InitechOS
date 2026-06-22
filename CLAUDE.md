# CLAUDE.md — InitechOS (codename STAPLER)

If you are an agent (Claude Code, an SDK harness, a downstream tool)
landing in this repo, read this **top to bottom every session**. After a
context compression, re-read. The rules drift out of working memory
faster than you think; that's why they're numbered.

---

## What this is

**InitechOS is a bootable, period-plausible operating system for emulated
386+ PCs** — a DOS-3.3 personality (`MILTON`) fused with a System-7-style
Toolbox (`FLAIR`), the exact chimera in the *Office Space* "Saving tables
to disk…" frame. It really boots, the windows really work, it ships a
dBASE-compatible database that really runs, and the north star is a
self-hosting Borland-style Pascal compiler (`Turbo Initech`, `TPS`) that
can recompile itself — and the Pascal programs it compiles — from inside
the OS.

The full product spec is **`InitechOS-PRD.md`** — read it. This file is
*how we build it*; the PRD is *what we build*. The ratified architecture
is in `docs/adr/` — ADR-0001 (386+, 32-bit flat), ADR-0002 (toolchain,
implementation language, executable format), ADR-0003 (InitechDOS base
OS), and CDR-0001 (interim toolchain deviation).

**Two systems coexist in this repo:**

1. **The artifact** — the OS itself. The bootloader, the InitechDOS
   kernel (`IO.SYS` / `INITDOS.SYS` / `COMMAND.COM`), the Toolbox
   (`FLAIR`), and the bundled apps for the current release are written
   in **C** (ADR-0002), targeting freestanding x86. **Pascal** is the
   language of **Turbo Initech** — the resident, self-hosting compiler
   and in-universe North Star (ADR-0007, pending) — and of the user
   programs Turbo Initech compiles. The whole OS stays period-authentic;
   nothing 2026 leaks into it.
2. **The factory** — everything that builds and *judges* the artifact,
   written in **C only**: the seed cross-compiler (now re-cast as the
   genesis of Turbo Initech, not the OS bootstrap), the emulator/oracle
   harness (QEMU/Bochs/86Box drivers, SSIM, differential suites), the
   asset-extraction tools, the specs-as-data. The factory is what makes
   the artifact trustworthy, and it is the factory cross-toolchain that
   rebuilds the C kernel/Toolbox.

The governing idea (PRD §8–§9): **every subsystem has a mechanical oracle,
and nothing ships on "looks right."** The emulator is the fitness signal.

## Read order

For any task, in this order:

1. This file (`CLAUDE.md`).
2. `InitechOS-PRD.md` — the spec. For a subsystem task, the relevant
   §6.x and its named oracle.
3. `bd ready` / `bd show <id>` — the issue you are working and its deps.
4. The locked spec-data for the subsystem you touch (`spec/` — region
   algebra, xBase coercion table, chrome metrics, hardware contract).
5. The reference fixture(s): the annotated frame (PRD Appendix A) and any
   golden file the oracle diffs against.

If you have not read this file and the PRD section for your subsystem, you
must refuse to add code. (Gate stated by named files, not ordinals, to
prevent count-drift.)

---

## The Laws

Four laws. Unconditional. If a "fast path" conflicts with any, choose the
Law.

**Law 1 — Ground truth before code.** Every decision cites a local
source: the PRD section, the locked spec-data, the annotated frame, a real
period-software golden (real DOS 3.3 / dBASE under 86Box), or an Intel/
hardware reference for the platform layer. If the source isn't local,
acquire it before writing the code that depends on it. Cite in comments
and commit messages, e.g.:

```pascal
{ Ref: PRD §6.2 — region merge is a scanline merge of inversion lists.
  Normal form = minimal inversion-point set; adjacent toggles collapse.
  Oracle: rasterize(A ∪ B) == rasterize(A) ∪ rasterize(B). }
```

**Law 2 — The oracle is the truth, not the agent.** A subsystem is "done"
when its mechanical oracle is green: the FAT differential matches the
`mtools`/Python reference, the region property suite passes, InitechBase
round-trips through real dBASE, Turbo Initech diffs clean against Free
Pascal, the self-host fixpoint converges (`K₂ == K₃` bit-for-bit). An
agent that *claims* it works but never ran the oracle — or ran it against
a stub golden — has produced nothing. Reviewer prose is not a signal.
**An oracle that computes its expected values from the same source the
artifact renders from is not an oracle** — it agrees *by construction* and
cannot catch a wrong value (the FLAIR palette-grading heresy; ADR-0010 /
Revocation Record HER-02). Grade against an *independent* golden (the
decomp corpora, the `test-clut` pattern).

**Law 3 — Know which language each piece speaks; never blur artifact and
factory.** The shipped OS — kernel/InitechDOS/Toolbox and the bundled
apps for the current release — is **C** (ADR-0002). **Pascal** is the
language of **Turbo Initech** (the resident self-hosting compiler,
ADR-0007) and the user programs it compiles. No Pascal in the harness,
and no factory-only C leaking into shipped sources as a 2026-ism. No
2026-isms (no timestamps, no host paths, no nondeterminism) baked into
the artifact — reproducible builds are a hard requirement because the
Turbo Initech self-host fixpoint depends on them (PRD §7).

**Law 4 — It must look and feel like the frame.** Fidelity is the product
(PRD §1, §3). The judge is a person who used early-90s Mac+DOS software
saying "yes, that's it" — the live, draggable arrangement with working
menus. SSIM is *intended* as a per-window guide, **never a hard numeric
gate** — but `harness/ssim.c` is **NOT YET BUILT** (`make ssim` is a stub;
`spec/ssim_params.h` crops are TODO_GOLDEN), so today fidelity rests on the
structural oracles + the operator's Law-4 eyeball; when built it grades
against the `../system7-decomp` / `../win31-decomp` rendered goldens, never
`preview.webp` (ADR-0010 / Revocation Record HER-11). The canonical bugs are canon: the
hourglass cursor (not a wristwatch), the `570-` trailing-minus, the pie
chart summing to **116%** — these are enforced, not fixed.

---

## The Rules

Numbered, non-negotiable. Re-read after compaction.

0. **Laws 1–4 apply.** Always.

1. **Red → Green → Refactor is the default loop.** Write the failing
   test/oracle first, watch it fail for the right reason, make it pass
   with the smallest honest change, then refactor green. See "TDD shapes"
   for the two valid forms.

2. **Fail fast, fail loud.** Panic on invariant violations (bad GDT
   selector, non-normalized region reaching a merge, `.dbf` header field
   count mismatch, malformed MZ header). A panic with context beats a
   silently-wrong framebuffer or a corrupt index. The in-universe panic
   screen renders `PC LOAD LETTER`; the real register dump goes to serial
   behind the debug flag (PRD Appendix B).

3. **All bugs are deep.** No bandaids, no "looks right for this frame." A
   region merge that happens to pass one test because the inputs don't
   overlap is exactly what the property suite exists to catch. Fix root
   causes; re-run the *full* oracle, not just the failing case.

4. **Skepticism.** Verify subagent output, prior-session claims, and your
   own memory against the current repo and the oracle. The green oracle
   is authoritative; conversation context and self-reports are not.
   Re-run the check yourself before trusting a "passes" report.

5. **Differential / tri-emulator from day one.** Boot milestones and the
   FAT/dBASE/compiler diffs run against references, and across QEMU (dev
   loop), Bochs (real→protected accuracy), and 86Box (period authenticity)
   — because "could have run on real PCs of the era" is a hard
   requirement, not a vibe (PRD §8). Do not let a fix pass on QEMU alone
   when the milestone calls for tri-emulator agreement.

6. **Golden files are mutation-proven.** A golden that never caught a
   regression is decoration. After capturing one (FAT image diff, dBASE
   round-trip, compiler corpus output), perturb the implementation by one
   branch/constant and confirm the oracle goes red; restore. The
   discipline is "the golden has caught a real regression," not "the
   golden was committed first."

7. **Tiered workflow — scale effort to change size.**
   - **Trivial** (<5 LOC; typo/comment/constant): direct edit, re-run the
     local oracle.
   - **Small** (one routine, <30 LOC): direct edit; one `Explore`
     subagent if the hardware/spec surface is unfamiliar; re-run the
     subsystem oracle.
   - **Core** (new subsystem, new oracle harness, new spec-data schema,
     cross-cut refactor): file/refine the beads issue; for contested
     design choices spawn 2–3 research subagents independently before
     implementing.

8. **Specs are locked data, not prose in someone's head.** Region
   algebra, xBase coercion table, chrome metrics, asset sheet, hardware
   contract live as versioned **plain JSON / C headers** under `spec/`.
   The locked spec is the contract; mush is what you get without it.
   Changing locked spec-data is a deliberate act with an issue + worklog
   note, never a silent edit to make one test pass.

9. **beads is the only cross-session tracker.** `bd` issues hold all
   multi-session work, dependencies, and status. Do NOT use TodoWrite or
   markdown TODO lists for cross-session work. `TaskCreate` is permitted
   for *in-session* sub-steps only. Use `bd remember` for persistent
   project knowledge.

10. **No GitHub CI, no automated remote runs.** Quality gates run locally:
    `make test`, the per-subsystem oracle, the emulator harness. The user
    has rejected automated CI across all their projects. Do NOT create
    `.github/workflows/`.

11. **Reproducible everything.** Deterministic codegen, deterministic
    symbol ordering, no timestamps in artifacts. The two-stage self-host
    certificate (`K₂ == K₃`) and DDC are meaningless otherwise (PRD §7).

12. **ASCII-clean agent output.** Grep agent-emitted source for non-ASCII
    before committing — a Cyrillic homoglyph in a constant or a smart-quote
    in a Pascal string is a silent, expensive bug. (Region/PRD math in
    *docs* may use Unicode; *code* stays ASCII.)

13. **Repeat rules.** Re-read this file at session start, after `/clear`,
    after any context compression. The agent that re-reads catches drift;
    the agent that doesn't ships it.

---

## TDD shapes — both valid

1. **Spec-from-scratch (RED → GREEN → refactor).** The classic loop. Use
   for new factory code (harness components, the seed compiler's passes,
   asset tools) and for artifact logic with no external reference (the
   event loop, window z-order). Write the test, see it fail for the right
   reason, make it pass, refactor.

2. **Differential / port-and-verify.** Where a period reference exists —
   FAT (vs `mtools`), `.dbf`/`.mdx` and xBase programs (vs real dBASE),
   Pascal codegen (vs Free Pascal on the shared subset) — capture the
   reference output as the golden, implement to match, then
   **mutation-prove** the golden catches regressions. Literal "RED first"
   is replaced by "the golden is generated from the reference and proven
   to bite." The region engine is the purest property-test case:
   `rasterize(A ⊕ B) == rasterize(A) ⊕_set rasterize(B)` over thousands of
   random regions with shrinking.

The discipline in both: a test that has never failed has proven nothing.

---

## Hallucination-risk callouts

Sharp pre-emptive warnings about mistakes that look right but aren't. When
you catch yourself about to do one, stop and re-check the cited reference.

- **The frame is a Photoshop mock-up, not a real OS screenshot.** Some
  "tells" are deliberately inconsistent (the Photoshop menu bar
  `File Edit Image Layer Select View Window Help` on a Mac window over
  DOS). InitechOS *out-reals the prop* by genuinely having those apps with
  those menus. Do not "correct" the inconsistencies — they are the spec.

- **The pie chart sums to 116% on purpose.** `40+35+18+14+9 = 116`. There
  is a test asserting `sum(slices) == 116`. The wristwatch cursor is the
  bug; the hourglass is correct. Don't "fix" canon.

- **Glyphs are hand-authored, not pixel-extracted.** The still is too
  low-res/compressed to recover Chicago/Geneva strikes by clustering. Match
  on-screen glyphs by hand and back-check; don't waste cycles on a CV
  extractor that can't work, and don't claim "extracted from frame" when
  it was authored (Law 1 honesty).

- **Two compilers, never conflated (PRD §4).** The *seed* cross-compiler
  is C, runs on the host, targets InitechOS — it is the **genesis of
  Turbo Initech**, not the OS bootstrap. (The C kernel/Toolbox is built
  by the factory cross-toolchain, not by the seed.) The *resident*
  compiler (`Turbo Initech`) is Pascal, runs on InitechOS, self-hosts.
  "Compile the compiler" means the resident one. Bugs in the seed are not
  bugs in the artifact.

- **Self-host certificate is `K₂ == K₃`, not `K₁ == K₂` — and it scopes to
  Turbo Initech.** `K₁` is emitted by the *seed* (a different compiler)
  and may differ byte-wise. The fixed-point proof is that the resident
  Pascal compiler reproduces itself: `K₂ == K₃` bit-for-bit (PRD §7); it
  is *Turbo Initech and the Pascal programs it compiles* that recompile
  from inside the OS, not the C kernel/Toolbox. Requires reproducible
  builds (Rule 11).

- **`.dbf` round-trips are on *meaningful* bytes.** The dBASE header stores
  a last-update date and reserved/language-driver bytes. Byte-exact
  reproduction of those is neither possible nor the point — normalize them
  before diffing; the bar is "real dBASE reads back what we wrote, and the
  records/index are correct."

- **xBase is dynamically, weirdly typed.** `SET EXACT`, string/number/
  date/logical coercion — capture the coercion table as locked data and
  fuzz against the real interpreter; do not infer the rules from intuition.

- **Real mode → 32-bit protected/flat is a minefield.** A20, GDT, the PE
  bit, the far jump to flush the prefetch queue — use a known-good
  sequence and verify in the **Bochs** debugger (strict transition
  checking), not just QEMU. A triple-fault in QEMU silently reboots; turn
  on `-d int,guest_errors,cpu_reset` and watch for it.

- **Flat `.COM`-equivalent apps AND InitechMZ flat-32 `.EXE` both ship
  (ADR-0003 DEC-08 + ADR-0003-AMENDMENT-DEC-08a); flat binary for the
  kernel.** Application executables ship as flat (`.COM`-equivalent)
  images; the InitechMZ `.EXE` loader ALSO ships (ADR-0003-DEC-08a): a
  real MZ container with a flat-32 load module and uint32 flat-base
  relocations (`os/milton/mz.c`, `loader_prepare_mz`). An untagged
  genuine-16-bit MZ panics fail-loud. The OS still does NOT run real
  16-bit binaries (no v8086, ADR-0001). The kernel itself is a flat
  binary handed off by stage2.

- **Cooperative, not preemptive.** Scheduling is a `WaitNextEvent`-style
  cooperative loop on the PIT tick. No preemption, no protected
  inter-process isolation — that's a non-goal (PRD §2). An app that
  doesn't yield hangs the desktop; that's authentic, not a bug to
  "fix" with preemption.

- **Regions must be normalized before every op.** A merge fed a
  non-normal-form inversion list produces plausible-looking garbage that
  passes non-overlapping cases. Normalize on construction; assert normal
  form on entry to merges (Rule 2).

---

## Build & test

> Until the harness is initialized this section is partly aspirational;
> create targets as the milestones land (PRD §11). Keep it C + `make` +
> thin shell — no extra runtimes (Law 3).

> **Toolchain note (ADR-0002 + CDR-0001).** The *target* toolchain for OS
> (C) targets is `i686-elf` cross-compilation (ADR-0002). The *interim*
> toolchain — accepted, time-limited, per CDR-0001 — is the host compiler
> in freestanding mode: `CC = gcc -m32 -ffreestanding -nostdlib` (+ `nasm`
> + `ld`). Repoint `CC_KERNEL` to the `i686-elf` cross-toolchain once the
> dev environment moves to a more capable device; CDR-0001 closes then.

```bash
# Toolchain / emulators (one-time, Ubuntu):
sudo apt install qemu-system-i386 bochs make nasm mtools
# Target OS toolchain is i686-elf (ADR-0002); interim is host gcc -m32
# -ffreestanding -nostdlib per CDR-0001 until the dev device is upgraded.
# 86Box + a period VGA BIOS for authenticity/golden minting — see docs.

# Build the factory tools and the seed cross-compiler (C):
make factory          # seed = genesis of Turbo Initech (Pascal), not the OS bootstrap

# Build a bootable image — C OS via the factory cross-toolchain (on host):
make image            # -> build/initech.img  (CC = i686-elf target / interim host gcc, CDR-0001)

# Dev-loop boot in QEMU with serial + gdb stub + screendump wired:
make run              # QEMU -s -S -serial stdio -d int,guest_errors,cpu_reset

# Accuracy boot in Bochs (real->protected transition checking):
make run-bochs

# Run a subsystem oracle (examples):
make test-region      # C property suite: homomorphism + normal-form + identities
make test-fat         # FAT12/16 differential vs mtools/python on identical images
make test-dbase       # InitechBase differential + round-trip vs real dBASE
make test-compiler    # Turbo Initech vs Free Pascal on the shared corpus

# The whole gate vector (PRD §8):
make test

# Self-host fixpoint check (the M8 finale certificate):
make selfhost         # K1=X(src); K2=K1(src); K3=K2(src); assert K2 == K3
make ddc              # diverse double-compilation: independent seed, compare K*

# Fidelity guide: per-window SSIM of a screendump vs the frame fixture:
make ssim FRAME=desktop
```

---

## Stop conditions (escalate to user)

Don't push through any of these.

- A hand-written reference (e.g. a known-good region or a hand-assembled
  boot stub) fails its own oracle — the spec, the golden, or the harness
  is wrong, none of which an agent should fix autonomously.
- The self-host fixpoint won't converge (`K₂ != K₃`) after reproducible
  builds are confirmed — points at miscompilation or nondeterminism;
  surface it with the diff, don't paper over it.
- An oracle disagrees across emulators (passes QEMU, fails Bochs/86Box) —
  that's the emulator-ism detector doing its job; investigate, don't pin
  to QEMU.
- You are about to weaken an oracle (loosen a diff, drop a tag class,
  raise a tolerance) to make something pass. Locked specs and oracles get
  strengthened, not relaxed, to ship code.
- You are about to add a second factory language or a proof assistant.
  The decree is C-only (PRD §14); revisit only with the user.

When escalating, attach: the subsystem, the failing oracle output, the
cited PRD section / spec-data, and the `bd` issue id.

---

## File map (proposed; create as needed)

```
initech-os/
├── CLAUDE.md                this file
├── InitechOS-PRD.md         the spec (read it)
├── Makefile                 factory + image + oracle targets
├── spec/                    LOCKED specs-as-data (JSON / C headers)
│   ├── hardware.json        the §5 hardware contract
│   ├── region_algebra.h     ops, normal-form rules
│   ├── xbase_coercion.json  the §6.6 coercion table
│   ├── chrome_metrics.json  title-bar height, pinstripe period, ...
│   └── assets/              palette, glyph strikes, icon sprites
├── seed/                    C seed cross-compiler (Pascal -> x86); genesis of Turbo Initech, not the OS bootstrap
├── harness/                 C oracle + emulator drivers
│   ├── emu/                 qemu.c, bochs.c, box86.c
│   ├── ssim.c               per-window fidelity guide (PLANNED -- NOT YET BUILT; `make ssim` is a stub)
│   ├── diff/                fat_diff, dbf_diff, compiler_diff
│   └── proptest/            region property suite + shrinker
├── os/                      THE ARTIFACT — C (ADR-0002); kernel/DOS/Toolbox/apps
│   ├── boot/                MBR -> stage2 -> 32-bit protected/flat   (C + asm)
│   ├── milton/              InitechDOS kernel (FAT, INT-21h, loader, shell)  (C)
│   ├── flair/               Toolbox (Window/Menu/Control/Event/Dialog)  (C)
│   │   └── atkinson/        region engine — the load-bearing math (§6.2)  (C)
│   ├── apps/                InitechCalc, FileManager, InitechPaint, FILE COPY  (C, current release)
│   ├── samir/               InitechBase (dBASE-alike)  (C)
│   └── tps/                 Turbo Initech (resident compiler + blue IDE) — PASCAL lives here;
│                            it is Turbo Initech's source language + user programs it compiles
├── fixtures/                the frame still(s), golden files
├── build/                   images, intermediates (gitignored)
└── docs/
    ├── worklog/             sharded session log (NNN-*.md)
    └── adr/                 design choices (NNNN-title.md)
```

## Session close

When the session is winding down:

1. Update beads: close finished issues, mark in-progress, file follow-ups
   for anything surfaced (`bd close` / `bd update` / `bd create`).
2. If a meaningful chunk closed, add a `docs/worklog/NNN-*.md` shard
   (Context → What changed → Why → Frictions → Acceptance → Pointers).
3. If a non-obvious lesson surfaced, `bd remember` it.
4. Confirm the oracle for what you touched is green (or the issue records
   why it isn't).

---

## Tool of last resort

If the Laws conflict with a fast path: choose the Laws. "Just ship and fix
later" is not a working mode here — a compiler that miscompiles one
construct in a hundred breaks the self-host fixpoint, and a region merge
that's subtly wrong corrupts the entire Toolbox. The cost of stopping to
re-read the PRD section, check the spec-data, and re-run the oracle is
minutes; the cost of a quietly-wrong oracle is unwinding everything built
on top of it.

When in doubt: re-read this file, re-read the PRD section, check the
locked spec-data, run the oracle, then ask the user.


<!-- BEGIN BEADS INTEGRATION v:1 profile:minimal hash:ca08a54f -->
## Beads Issue Tracker

This project uses **bd (beads)** for issue tracking. Run `bd prime` to see full workflow context and commands.

### Quick Reference

```bash
bd ready              # Find available work
bd show <id>          # View issue details
bd update <id> --claim  # Claim work
bd close <id>         # Complete work
```

### Rules

- Use `bd` for ALL task tracking — do NOT use TodoWrite, TaskCreate, or markdown TODO lists
- Run `bd prime` for detailed command reference and session close protocol
- Use `bd remember` for persistent knowledge — do NOT use MEMORY.md files

## Session Completion

**When ending a work session**, you MUST complete ALL steps below. Work is NOT complete until `git push` succeeds.

**MANDATORY WORKFLOW:**

1. **File issues for remaining work** - Create issues for anything that needs follow-up
2. **Run quality gates** (if code changed) - Tests, linters, builds
3. **Update issue status** - Close finished work, update in-progress items
4. **PUSH TO REMOTE** - This is MANDATORY:
   ```bash
   git pull --rebase
   bd dolt push
   git push
   git status  # MUST show "up to date with origin"
   ```
5. **Clean up** - Clear stashes, prune remote branches
6. **Verify** - All changes committed AND pushed
7. **Hand off** - Provide context for next session

**CRITICAL RULES:**
- Work is NOT complete until `git push` succeeds
- NEVER stop before pushing - that leaves work stranded locally
- NEVER say "ready to push when you are" - YOU must push
- If push fails, resolve and retry until it succeeds
<!-- END BEADS INTEGRATION -->
