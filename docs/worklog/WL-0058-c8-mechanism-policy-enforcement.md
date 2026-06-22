<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0058 -- C-8 mechanism/policy enforcement: flair_look_pixel + the two structural oracles (epic initech-qipc step 5)

**Type:** implementation shard (step 5 -- the X-Window-style mechanism/policy split made structurally true).
**Date:** 2026-06-23.
**Bead:** `initech-6bq2` (epic `initech-qipc` step 5).
**Commit:** (this shard's commit).

## Context -- why this shard exists

With the desktop correctly teal and value-graded (steps 1-4), step 5 binds the
C-8 cut-line: the imaging MECHANISM never names a color; it names a palette
INDEX/PART and resolves to a pixel ONLY through one policy seam. Before this,
the only mechanism color-literal left was `desktop.c:74` (`INITECH_DESKTOP_BG_RGB`).

## What changed (orchestrated: 1 opus lane for the source; I owned the Makefile + grading)

- `os/flair/flair_look.{h,c}` (NEW) -- the policy seam: a `FLAIR_PART` enum
  (CONTENT/FRAME/TEXT/DESKTOP/MENUBAR/CAPTION_NAVY/BTNFACE/PIN_LIGHT/PIN_DARK +
  the two bevel rows) + `flair_look_pixel(port, PART)` / `flair_look_pixel_depth(bpp, PART)`:
  a PURE-DATA PART->idx map -> `flair_canon_rgb(idx)` -> device quantize. It is the
  ONLY site besides clut.h that turns an index into a color (ARB-3).
- chrome/control/dialog re-routed so the decoration reads color via the seam
  (`cfill`/`ctrl_px`/`dlg_px` -> flair_look_pixel); `desktop.c:74` fixed to resolve
  through `flair_look_pixel_depth(bpp, FLAIR_PART_DESKTOP)` -- the mechanism now
  names NO color. RENDERED OUTPUT byte-identical (a seam refactor, not a value
  change; the chrome PPM is cmp-identical pre/post).
- `harness/proptest/test_mech_policy.c` (NEW) -- the C-8 SOURCE scanner: ZERO
  0xRRGGBB / INITECH_*_RGB / index->RGB switch in the mechanism file set
  (surface/blitter/window/event/desktop + chrome/control/dialog decoration);
  allowlist EXACTLY clut.h + the flair_look resolver TU. Mutation-proven
  (`-DMECH_POLICY_MUTANT` + a planted-literal fixture -> RED).
- `harness/proptest/test_flair_mechanism_colorblind.c` + `flair_look_sentinel_stub.c`
  (NEW) -- compile chrome against a SENTINEL policy stub (magenta for every PART);
  render; assert EVERY chrome pixel == sentinel (no color originates in the
  mechanism). Mutation-proven (`-DFLAIR_COLORBLIND_MUTANT` -> a hardcoded color ->
  the sentinel render goes RED).
- `docs/adr/ADR-0004-FLAIR-Toolbox-Architecture.md`: binding constraint C-8 added
  to Sec 5.1 (after C-7).
- Makefile (orchestrator): `flair_look.c` wired into the 5 decoration link sets
  (CHROME/CONTROL/DIALOG/SHELL/DRAG) + a `KERNEL_FLAIRLOOK_OBJ` kernel obj +
  KERNEL_FLAIR_OBJS; the 2 new gates + their mutants wired into TEST_UNIT_GATES + .PHONY.

## Frictions / lessons

- **THE LESSON: a subagent's "all oracles green" can be a STALE-BINARY false
  green.** The lane (correctly, per its brief) left the Makefile wiring to me and
  reported the 7 render oracles "green via make" -- but it had verified them with
  a TEMPORARY link-set wiring it then reverted, so the binaries in build/ were
  built WITH flair_look.c linked while the committed Makefile had no reference to
  it. `make test-chrome` reran the stale binary and passed. Rule 4 (re-run the
  oracle yourself) + a forced clean rebuild (`rm build/test_dialog && make
  test-dialog`) exposed the truth instantly: `undefined reference to
  flair_look_pixel` in dlg_px/ctrl_px/cfill. NEVER trust a green that a `make
  clean` could turn red -- after a new linked TU, force the clean rebuild. (This
  is exactly the discipline the FLAIR heresy purge was about: the oracle is the
  truth only when you actually ran it against the current tree.)
- The decoration (chrome/control/dialog) is BELOW the C-8 cut-line too (it names a
  PART, never a color), so the scanner grades all eight files; menu.c/window.c do
  NOT call the seam (verified) so their link sets are untouched.

## Acceptance (Law 2)

- test-mech-policy GREEN + mutant RED (planted literal caught); test-flair-
  mechanism-colorblind GREEN + mutant RED (hardcoded decoration color caught).
- All render oracles byte-identical GREEN on a FORCED CLEAN rebuild (test-chrome/
  control/dialog/shell/drag/flair-desktop) -- the seam refactor changed no pixel.
- Kernel image builds with flair_look.o linked (`_kernel_end=0x34f00` < 0x40000).
- Full clean aggregate (`make clean && make test`): GREEN -- **263 host + 43 emu**
  (was 259; +4 = the two C-8 oracles + their mutants).
- ASCII-clean (Rule 12); C-8 in ADR-0004 Sec 5.1.

## Pointers

- Seam: `os/flair/flair_look.{h,c}`. Oracles: `harness/proptest/test_mech_policy.c`,
  `test_flair_mechanism_colorblind.c` + `flair_look_sentinel_stub.c` +
  `fixtures/mech_policy_mutant.c`.
- Design: ADR-0004-AMENDMENT-DEC-09 (ARB-3 Sec 3.3, DEC-09-D4 Sec 3.10, C-8 Sec 5.1).
- Roadmap: `bd show initech-qipc`; NEXT = `initech-m6qx` (step 6: era-layering
  registry flair_skins.h + skin oracles) -> `5l5z` (step 7: live event loop).
- Deferred (FO-5): relocating cfill/crect/cframe into a mechanism TU -- the C-8
  oracles now guard that future move.
- Prior: WL-0057 (the render flip + de-heresy).
