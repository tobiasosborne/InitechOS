# WL-0023 -- u6wa: MKDIR / RMDIR / CHDIR (the subdir chain becomes usable)

Branch: `command-com-default`. Continues the ti8 subdir chain
(`ti8 L1 -> mzxa L2 -> u6wa`). Grounded read-only first, then implemented as two
serial worktree landings, each RED-first + mutation-proven, merged + re-gated on
the main thread.

## Context

After mzxa (READ-side subdir resolution through the INT 21h backend), the next
link `u6wa` (AH=39h/3Ah/3Bh) looked like a clean dispatch but was entangled:
MKDIR/RMDIR need WRITE-side directory creation, which overlaps the `zs24`
follow-up, and the bead demanded DOS-3.3-PRM grounding (Law 1). A read-only
grounding pass (wf_426c388f) resolved the entanglement before any code.

## What the grounding settled (so implementation didn't guess)

- **Sequencing:** u6wa is self-contained, NO `zs24` dependency for root-level
  dirs. CHDIR has zero FAT-write dependency; MKDIR/RMDIR for a ROOT parent are
  feasible with existing root-capable primitives + a small dot-dir writer. Only
  a NON-ROOT parent (`MD \SUB\NEW`) needs zs24's subdir-write -> fail-loud 0x0003.
- **`..` convention pinned EMPIRICALLY** (mtools 4.0.43, triple-confirmed): `.`
  start = the dir's own cluster; `..` start = the parent's cluster with **root
  encoded as 0** (not self, not 1). Matches `fat12_resolve_path`'s existing
  `0=>root` normalization -- reader and writer consistent, no spec drift.
- **DOS codes pinned** (MS-DOS Encyclopedia / RBIL): CHDIR-missing 0x0003;
  MKDIR-exists 0x0005; RMDIR-non-empty 0x0005; RMDIR-of-CWD 0x0010 (new code).
- A real **latent bug** surfaced: `fat_resolve` ignored `cwd_start`
  (`(void)cwd_start;`), so relative multi-component resolution was wrongly
  root-anchored. CHDIR had to fix it (the riskiest shared piece) -- which is why
  CHDIR landed first.

## What changed (both merged; each RED-first + mutation-proven)

- **Landing 1 -- CHDIR (AH=3Bh)** (`67c143c`): new `resolve_dir` backend vtable
  member (full-path-to-directory: validates `DIR_ATTR_DIRECTORY`, returns the
  target's own start cluster, emits canonical root-relative text); fixed
  `fat_resolve`'s ignored `cwd_start` (shared base-seeding helper
  `fat_descend_seed`; `fat12_resolve_path` split into a `_from` variant +
  start=0 wrapper, root byte-identical); `do_chdir` + `case 0x3B`; `g_cwd`
  written + `INT21_CWD_MAX` enforced on write (overlength fail-loud, not
  truncate). Mutants: m1 (CD into a file wrongly succeeds), m5 (revert the
  cwd_start fix -> relative CD from a non-root CWD breaks).
- **Landing 2 -- MKDIR/RMDIR root-only** (`91062055`): public
  `fat12_mkdir`/`fat12_rmdir` (parent_start==0 only; non-root -> 0x0003) + a
  dot-dir cluster writer (FIXED mtime/mdate, Rule 11); `mkdir`/`rmdir` vtable
  members; `do_mkdir`/`do_rmdir` + `case 0x39`/`0x3A`; new
  `INT21_ERR_CURRENT_DIR 0x0010` + a `do_geterr` arm. A **differential oracle**
  `test_fat12_mkdir` diffs the artifact's on-disk `.`/`..`/attr/start/EOC
  byte-for-byte against an `mmd`-minted golden (clock bytes normalized). Mutants:
  m2 (`..`=self -> mmd diff bites -- the canonical `..`-rule mutant), m3 (skip
  EOC), m4 (RMDIR skips empty-check).
- `KERNEL_SECTORS` 112 -> 128 (legitimate code growth; coordinated Makefile +
  `stage2.asm`; `IMG_SECTORS` stays 192 = 3 whole 2x32 cylinders; the parse-time
  geometry guard passes).

## Acceptance

`make test` GREEN: **68 host + 22 emu gates** (re-run on the main thread).
`make test-boot-bochs` PASS -- the 128-sector kernel boots under Bochs via the
mode-0x13 fallback (Rule 5 / the WL-0019 geometry discipline: a boot-geometry
change MUST be Bochs-verified, QEMU-green is not enough). 5 mutants
mutation-proven; the mmd differential confirms the `..`=parent/0 rule. Adversarial
verifier CONFIRMED 7/7. `kernel.bin` reproducible. `initech-u6wa` closed; `ut6d`
(shell MD/RD/CD) unblocked.

## Frictions / lessons

- **Ground before implementing, again.** The bead's line numbers were stale and
  it under-specified the seam (CHDIR can't reuse `resolve_dir_path` -- that
  returns the leaf's PARENT, a silent bug). The grounding caught it and pinned
  the `..` convention empirically rather than guessing.
- **A boot-geometry change is a tri-emulator obligation.** `KERNEL_SECTORS`
  112->128 is exactly the WL-0019 failure class; ran the Bochs leg before push.

## Pointers / next

- **`initech-ut6d`** -- shell built-ins MD/RD/CD (now unblocked; wires the
  COMMAND.COM REPL to the new AH=39h/3Ah/3Bh handlers + subdir-aware CD/prompt).
- **`initech-zs24`** -- subdir EXEC + subdir WRITE (CREATE/UNLINK/WRITE + MKDIR/
  RMDIR with a NON-ROOT parent); lifts u6wa's root-only restriction.
- Minor (verifier-flagged, non-blocking): the overlength-canonical-path
  fail-loud has no dedicated oracle assertion (correct by construction).
