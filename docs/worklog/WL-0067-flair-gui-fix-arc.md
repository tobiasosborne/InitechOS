<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0067 -- The FLAIR GUI FIX arc: the live desktop is the frame again (7 bugs, orchestrated)

**Type:** the fix arc WL-0066 mandated. Orchestrated (committee + parallel worktree lanes).
**Date:** 2026-07-07.
**Beads closed:** initech-mswo, -ojxn (P0); -v6t2, -a9iq, -4w15, -pipa, -dc4v, -jmc5, -qi8v (P1).
**Commits (7):** `1ff449f` mswo, `57778e4` v6t2, `8b88250` ojxn, `c9a0b01` a9iq, `7a287dc` 4w15,
`6c2c672` pipa, `16b02a7` jmc5/qi8v (base `5bc5104`).

## Context

WL-0066 halted feature work: the operator drove the live FLAIR desktop and found it "badly
broken in seconds"; 138 bugs were filed. The mandate for this session was a FIX arc, not new
features -- start at the 2 P0s, then the highest-impact compositor + chrome P1 families, then
stand up composited-pixel gates so the class cannot ship green again. Everything here was
orchestrated: each coding step delegated to a subagent (opus for the load-bearing/subtle, sonnet
for the mechanical), the orchestrator owning grading (re-running every oracle on the main tree,
never trusting a subagent report), integration, and Law-4 eyeballing.

## What changed (7 fixes, each mutation-proven)

1. **initech-mswo (P0, region).** `region_op`'s per-band `xmerge` wrote `out[no++]` with no bound
   into `scratch[RGN_ROW_X_MAX=256]`; an XOR of two max-density rows yields up to 512 outputs ->
   silent stack overflow. Fix: `xmerge` is capacity-bounded and fail-louds at the per-row cap
   (Rule 2, consistent with `region_normalize`). Oracle: over-cap probe + an ASAN `NO_XMERGE_CAP`
   mutant (a plain build masks the overrun because normalize aborts anyway -- ASAN catches the
   FIRST OOB write). Locked `RGN_ROW_X_MAX` untouched; the 256-vs-640 cap decision is `44ab`.

2. **initech-ojxn (P0, COMMAND.COM).** `COPY FOO FOO` truncated FOO to zero then reported success
   (`dos_creat` truncates before any read). Fix: a `cmd_same_file` guard after the source-open,
   before the create, prints the DOS-3.3 canon "File cannot be copied onto itself" + "0 file(s)
   copied". New MSG-DOS-0020 (ADR-0003 DEC-13 amendment). Oracle: host `test_same_file` + an EMU
   no-data-loss gate `test-copy-selfcopy` (multi-cluster fixture, since a single-cluster file
   round-trips -- the filed mechanism was partly wrong). Both mutants bite.

3. **initech-v6t2 (P1, window).** `reaffirm_active` flipped `hilited` but never seeded a repaint on
   a 1->0 deactivation, so an ex-front window kept active chrome. Fix: invalidate the deactivated
   window's visible region via `WindowMgr_invalidate`. Oracle: an 800-case owner-grid property
   (independent of `visible_into`, the primitive under test) + `NO_DEACT_INVAL` mutant.

4. **initech-a9iq (P1, chrome).** `flair_draw_document_window` had no `hilited` param -- every
   window drew the active pinstripe. Fix: an `int hilited` param; inactive = flat white
   (`FLAIR_PART_CONTENT`) title interior. Law-1 corrected the bd's "solid-gray": the decomp
   `title-bar.md` measures inactive fill = `#FFFFFF` (the difference is the racing stripes, not
   the base color). Active render proven byte-identical (`git show HEAD` + `cmp`). Oracle:
   `test-chrome-fidelity` inactive leg vs the decomp golden + `NO_INACTIVE` mutant.

5. **initech-4w15 (P1, kmain).** The app-switch drew the tenant menu into row 0 (the static
   System-7 bar), collapsing both chimera bars. Fix: the swap targets band 2 via `make_offset_view`;
   row 0 is never touched by the live loop. HELLO (Photoshop) is now the boot foreground so
   `shell_render` composes the DISTINCT chimera at rest. Oracle: `DISTINCT-CHIMERA` + `BAR1-STATIC`
   + `MENU-BAND` tiers, 4 mutants RED. **First revision regressed the boot to two identical
   System-7 bars -- caught by driving the boot screendump and LOOKING (Law 4); sent back and fixed.**

6. **initech-pipa (P1, window) + dc4v.** Dragging a window erased the modal FILE COPY dialog + the
   two menu bars (they were painted once, outside WindowMgr). Fix (ratified + red-teamed design): a
   ~15-LOC fold of a new `wm->overlay_rgn` (bars + modal) into `fronts_union`. Because `fronts_union`
   feeds the WHOLE damage chain, occluding via the overlay closes BOTH the overpaint AND the
   seafoam-erasure at once -- no `desktop.c` change, no re-composite wrapper, no uniform-window
   model. Aliasing-safe `rgn_accumulate` (never the naive self-alias, Rule 3). Oracle: new EMU
   `dc4v` gate (modal survives a -260px drag, 100% intact) + `IGNORE_OVERLAY` mutant (100% teal).
   **Visually confirmed: the "Saving tables to disk..." modal stays intact -- it IS the frame.**

7. **initech-jmc5 / -qi8v (P1, desktop).** App-switch AND drag erased a background window to teal.
   Root cause (pinned by a live re-test, NOT the filed "no exposure" guess and NOT region
   corruption): a stale `wm->desktop_update` seeded by the init `HideWindow` of the canon frame
   windows, never cleared by `desktop_paint_all`, teal-stomped by the first `desktop_paint_damage`.
   Fix: `desktop_paint_all` resets `wm->desktop_update` at its tail (a full composite satisfies all
   pending damage). One root cause, both symptoms. Oracle: a new `TIER-C` closes the exact probe gap
   that let jmc5 ship green -- it probes the raised window's own structure frame + `no_paintall_clear`
   mutant.

## Why (the orchestration shape)

The compositor P1s looked like one big architecture problem, so a 3-seat committee (layered vs
uniform-window vs minimal-surgical) deliberated, the chair synthesized, and a re-scoping re-test
then collapsed the scope: qi8v/jmc5 turned out to be a stale-damage bug (a 1-liner), not the
architecture; pipa alone needed the compositor work, and an adversarial red-team ratified the
contained `overlay_rgn` fold over the heavy uniform-window model. Lanes ran in parallel worktrees
(<=2 opus, <=6 sonnet), each graded independently on the main tree.

## Frictions (the traps that were caught, not shipped)

- **jmc5 was mis-filed** as "no exposure / region corruption". A live re-test pinned the real cause
  (stale desktop_update) and saved a wasted uniform-window architecture lane.
- **4w15's boot regressed** to two identical System-7 bars. Caught only by driving the boot
  screendump and looking (the WL-0066 lesson). Sent back; fixed by the HELLO-foreground arrangement.
- **TIER-C mis-calibration averted.** The jmc5 agent's TIER-C probed HELLO's frame, but on the
  INTEGRATED scene v6t2 repaints the deactivated HELLO -- the erased window is NOTES (raised, not
  re-invalidated). Blindly porting would have shipped a green-but-blind oracle (the exact heresy
  that let jmc5 ship). Re-calibrated EMPIRICALLY by driving fixed-vs-mutant images and sampling.
- **`git diff` misses untracked files.** pipa's new `ppm_flair_dc4v_check.c` was absent from its
  patch; the dc4v gate could not build until it was copied over. Going forward: check `git status`
  for `??` when integrating a worktree.
- **Latent Makefile bug (filed):** `KERNEL_DESKTOP_OBJ`'s prereqs reference later-defined vars ->
  they evaluate empty -> `desktop.o` is NOT auto-rebuilt on `desktop.c` edits (a false-green).
  Worked around with `rm -f build/*desktop*.o`; needs a real fix.
- **Clock-skew** false-greens on incremental builds -- resolved by the closing `make clean && make test`.

## Acceptance

Each fix was re-graded on the main tree by the orchestrator (not the subagent): its subsystem
oracle GREEN + every mutant RED + the kernel build + ASCII-clean. Key EMU gates: `test-copy-selfcopy`
(no data loss), `test-flair-dc4v` (modal survives a drag) + `IGNORE_OVERLAY` mutant, `test-flair-appswitch`
(6 legs incl TIER-C) + 5 mutants. pipa + 4w15 were additionally EYEBALLED (the post-drag frame is the
Office Space "Saving tables to disk..." still). **Full gate: `make clean && make test` = ALL GREEN, 291 host + 59 emu gates
("InitechDOS is rock solid").** The gate earned its keep: the FIRST run caught a cross-test
regression -- v6t2's deactivation-invalidate contaminated `test-process-update`'s cascaded
`NewWindow` construction (a DIFFERENT test than v6t2's author touched) -- fixed by the same
`WindowMgr_validate` baseline isolation `test_window.c` adopts, then re-run clean.

## Pointers

- Commits `1ff449f..16b02a7` on `command-com-default`.
- Follow-ups filed: `44ab` (region cap decision), `0i1m` (from_rects O(n^2)), `vj28` (COPY same-file
  edge cases), `hv7u` (full inactive chrome fidelity), `zn61` (modal input-blocking / true modality),
  `7tjp` (tenants band-2 dup Apple slot when NOTES active), the Makefile-build bug, and a permanent
  qi8v drag oracle.
- Lesson: `bd memories flair-compositor-oracle-gap-jmc5-qi8v-2026-07` (probe the region the bug
  ERASES, not the actor's).
- The re-flair epic `initech-qipc` step 7 (operational GUI) is materially advanced: the live desktop
  now composites correctly through drag, app-switch, and modal-occlusion, each behind a
  composited-pixel gate.
