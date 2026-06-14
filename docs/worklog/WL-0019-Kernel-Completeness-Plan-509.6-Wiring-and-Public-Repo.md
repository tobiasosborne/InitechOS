# WL-0019 -- Kernel-completeness plan, 509.6 MCB wiring + Bochs geometry fix, public repo, design soul

Branch: `command-com-default`. This shard covers a planning-heavy session that
also landed one real kernel deliverable (`initech-509.6` part 2) and stood up the
project's public face and its captured design intent.

## Context

Operator directives this session, in order:
1. Establish InitechDOS's current kernel feature-completeness vs the DOS 3.3-5.0
   era (kernel/INT 21h capabilities vs user-facing software/command coverage).
2. Capture all findings as **beads**, chained, so nothing is forgotten; then get
   the **core** landed -- kernel 100% feature-complete first, then required
   utils, then GUI + dBASE; the rest phased toward eventual full DOS 5.0.
3. Orchestrate the feature work (subagents) following the Laws/Rules.
4. Sidequest: a README + GitHub repo.
5. The **design soul** (the load-bearing reframing): InitechOS is a recursive
   joke that must, at first glance, be indistinguishable from a real early-90s
   corporate OS and NEVER admit it; only the FINAL build is completely straight,
   the dev journey is transparent on purpose, and it builds steadily.

## What landed

**Kernel feature-completeness audit (grounded).** A multi-agent audit + my own
`int21.c` grep established the truth: the dispatch (`int21_dispatch_body`)
implements **39** INT 21h functions; `ah_is_listed()` is only a recognition
table. The real Appendix-A gap = `39/3A/3B` (MKDIR/RMDIR/CHDIR), `43` (CHMOD),
`44` (IOCTL), `48/49/4A` (ALLOC/FREE/SETBLOCK), `56` (RENAME), `57` (FILETIME),
`5B` (CREATNEW), `31` (KEEP), FCB `0F-24h`; plus FS enablers (subdirs, FAT16,
absolute disk, partitions) and the shell built-ins. Software coverage: 7 internal
COMMAND.COM commands, 0 external utilities.

**The plan, as 40 chained beads.** Phase 1 (kernel feature-complete) = children
of `initech-bsy` capstoned by **`initech-40oq`**, a coverage-oracle certificate
asserting zero Appendix-A functions remain in the not-yet-impl arm. Phase 2 =
shell built-ins + a new `util-epic` (FORMAT/CHKDSK/FDISK/SYS/filters/...). Phase 4
= the parked `dos5-epic` (post-Appendix-A surface, ADR-amendment-gated). FCB
(`509.9`) is **backburnered (P4) but REQUIRED** -- the canonical "vestigial dead
code that actually works," whose flagship consumer is the **TPS Report Generator**
(`8479.1`: must exit FLAIR to the prompt, must work, must use FCB). Thematic canon
beads filed: the Y2K accounting system (`586.1`), Michael Bolton's salami-slicer
with the rounding-error bug (`586.2`), and the `packaging-epic` (the final straight
build: floppy images, period manuals, box, `A:SETUP`).

**`initech-509.6` part 2 (the real kernel deliverable).** AH=48h/49h/4Ah wired in
`int21.c` to an `mcb_arena_t`. Arena = the **authentic single-big-block** model
over the already-locked `[PROGRAM_BASE 0x30000, PROGRAM_ALLOC_END 0x70000)` window
-- **NO `memory_map.h` edit** (Rule 8). Owner = current PSP (== AH=62h value);
freed on terminate, symmetric with the JFT close. New gates `test-mcb-int21`
(host) + `test-mcb-emu` (in-emulator), both mutation-proven. Built by a background
workflow in an isolated git worktree, adversarially re-verified, then merged.

**Bochs geometry regression -- caught by the tri-emulator leg (Rule 5).** The
509.6 `KERNEL_SECTORS` 96->112 bump grew the boot image to `IMG_SECTORS=160`, a
multiple of 32 but NOT 64 (= 2.5 cylinders), which the Bochs harness
(`harness/emu/bochs.c`) rejects ("not a whole 2x32 geometry"). QEMU was green;
`make test-boot-bochs` was red. Fixed to `192` (3 whole cylinders) + a parse-time
fail-loud build guard, **mutation-proven** (160 and 64 both error). Bochs now
boots green.

**Directory-chain build brief (research; ready to implement).** 3 independent
research agents + a synthesizer produced a build brief for `ti8 -> u6wa -> ut6d`
that corrected one agent's conflation of `ti8` with MKDIR (`ti8` = read/traverse
only; creation is `u6wa`). Two-layer additive design (a `fat12_dir_t` descriptor
branch in `fat12.c`; a `resolve_path` seam + `g_cwd` in `int21.c`), a 3-way
differential oracle on a real mtools nested image, named mutation points, and a
RED-first step. Persisted to the `initech-ti8` (approach) and `initech-u6wa`
(DOS-3.3-PRM open questions) bead notes.

**Public face + design soul.** `github.com/tobiasosborne/InitechOS` (public,
AGPL-3.0). The README is straight in-character Initech Systems Corporation product
documentation (no out-of-universe framing). The design intent is captured as
durable `bd` memories: the recursive joke, the build-transparency principle (only
the final build is straight; the journey is transparent), the found-footage /
clean-room aesthetic, and ADR-by-committee (subagent roles) **for big features
only**.

## Why this shape

Per the WL-0018 discipline and the Laws: read-only analysis/research FANNED OUT
(the audit, the directory-chain brief); the shared-file kernel implementation
(509.6) ran in an ISOLATED worktree and was adversarially verified before merge;
nothing merged without an independent green `make test` re-run on the main thread
(Rule 4). The directory chain is a hard dependency chain over shared files, so it
is deliberately reserved for serial, oracle-gated implementation -- NOT
parallelized.

## Acceptance

`make test` = **59 host + 22 emu gates GREEN** (independently re-run twice: after
the 509.6 merge and after the geometry fix). `make test-boot-bochs` GREEN (the
boot-affecting `KERNEL_SECTORS` bump confirmed on Bochs). New gates
mutation-proven; the `IMG_SECTORS` build guard mutation-proven. `initech-509.6`
closed.

## Frictions (learned the hard way)

- **`bd create --graph <file> --dry-run` IGNORES `--dry-run` and actually creates
  the issues**, AND the graph path silently drops per-node `deps`/`parent`. Caught
  by a `bd stats` delta (169 -> 255); cleaned up 46 stray beads. Deps/parents were
  re-wired with `bd dep add` / `bd update --parent`. (Also: epics can only block
  epics, tasks only tasks.) Saved to memory.
- **The Rule-5 catch is the lesson.** An adversarial subagent verifier reported
  509.6 "solid, merge" -- and it was, on QEMU. The Bochs leg (which the verifier
  explicitly deferred) was red. Running the full tri-emulator check myself before
  pushing is what caught the geometry regression. QEMU-green is not done.
- **Presentation vs dev process.** An initial README admitted the homage/AI; over-
  corrected by making the repo private; reconciled to: only the presentation layer
  (README/manuals/box) stays in-character now, the dev journey is transparent.

## Pointers / next

- **Resume at `initech-ti8`** -- the keystone, READ-side subdirectory traversal.
  Full build brief + RED-first step in its bead notes; DOS-3.3-PRM open questions
  in `initech-u6wa`. Drive serially, oracle-gated. Then `u6wa` (MKDIR/RMDIR/CHDIR)
  -> `ut6d` (MD/RD/CD), and the rest of the Phase-1 kernel children toward the
  `initech-40oq` certificate.
- Hygiene: `initech-l6o0` (`make test` aborts at `test-assets` on a clean checkout
  because `spec/assets/preview.webp` is gitignored/untracked).
- Tri-emulator (`initech-x0i`) remains the home for the full Bochs/86Box
  differential beyond the boot leg.
