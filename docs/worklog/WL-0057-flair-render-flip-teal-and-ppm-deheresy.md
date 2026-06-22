<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0057 -- the FLAIR render flip to the Initech teal canon + the ppm_flair_check de-heresy (epic initech-qipc step 4)

**Type:** implementation shard (the render flip -- step 4, the payoff of the oracle-first canon chain).
**Date:** 2026-06-23.
**Bead:** `initech-7x9k` (epic `initech-qipc` step 4). Two bisectable lanes.
**Commits:** `892e9d7` (Lane 1, the render flip) + the Lane 2 commit (the ppm de-heresy).
**Follow-up filed:** `initech-r8hv` (FO-3 completeness -- the MILTON text-console teal flip, an operator Law-4 judgment call).

## Context -- why this shard exists

The oracle-first foundation (n79q region spine, h714 canon module, mwpw value
oracle) was complete and green. This step is the PAYOFF: flip the booted FLAIR
desktop from the operator-condemned "very wrong" palette (preview.webp grays +
seafoam) to the locked Initech Color Canon, and end the HER-02 by-construction
color grade in the live screendump oracle. Per DEC-09-D3 this is a DELIBERATE
Rule-8 VALUE change (not a refactor): the five index->RGB switches carried THREE
different desktop values, and three sites change value at the flip.

## What changed (orchestrated: 2 opus lanes; I owned grading + Makefile + the screendump)

**Lane 1 (`892e9d7`) -- the render flip:**
- The 3 Manager switches (`chrome_pal_rgb`/`ctrl_pal_rgb`/`dlg_pal_rgb`) + the
  factory `render_palette_rgb` re-bodied to `return flair_canon_rgb(index)`. The
  CIDX_* macro(color_canon.h)-vs-enum(chrome.c) collision was resolved by dropping
  chrome.c's local enum and using the canon macros. ALL geometry untouched.
- `spec/assets/palette.h`: `flair_palette_rgb` is now a THIN ALIAS to
  `flair_canon_rgb` (via the generator `tools/palette_extract.c` + a deterministic
  regen); preview samples demoted to `INITECH_*_FRAME_V0` provenance.
- `os/boot/stage2.asm`: SEAFOAM_R/G/B -> TEAL_R/G/B (0x8D/0xDC/0xDC).
- `test_palette_seafoam.c` re-keyed seafoam->teal.
- RESULT: the index->pixel map is byte-identical across all sites (fb-agree); the
  booted-386 FLAIR desktop renders the Office Space frame in the CORRECT colors --
  visually confirmed (build/flair_desktop.ppm: teal #8DDCDC desktop sampled at the
  pixel, two chimera menu bars, two crisp-WHITE pinstripe windows, the "Saving
  tables to disk..." modal occluding them).

**Lane 2 (the de-heresy) -- ppm_flair_check re-key (ADR-0010 CD-5):**
- `tools/ppm_flair_check.c`: struck the HER-02 by-construction framing (expected ==
  flair_palette_rgb, the same function the kernel renders from); re-keyed the
  expected colors onto `flair_canon_rgb` whose VALUES are now independently
  decomp-graded by `test-color-canon`; kept VERBATIM every value-INDEPENDENT
  structural probe (two-bar Apple-slot ink-density tell, the period-2 pinstripe
  alternation as the value-free relation `shade[k]!=shade[k-1] && shade[k]==shade[k-2]`,
  1px-frame, centered-modal/7px-border, z-order occlusion); kept the +/-2 tolerance
  (justification: capture noise, not a value fudge). ppm_flair_check is now the
  STRUCTURE oracle ONLY -- never the value authority.
- Added the `-DPPM_FLAIR_HER02_DEMO` build: proves the structure is value-BLIND (a
  teal->seafoam perturbation leaves the structural relations GREEN) while
  test-color-canon CATCHES it (RED) -- the heresy made mechanically visible.
- `tools/ppm_seafoam_check.c` neutered to an inert RETIRED stub (CD-6/CD-7).
- Makefile (orchestrator): CD-5 item 5 -- `test-color-canon`(+`-gen`) run FIRST as
  a HARD precondition of `test-flair-desktop` (the value oracle gates the structure
  oracle); the HER-02 demo wired as the `[+]` step of test-flair-desktop;
  ppm_seafoam_check retired from `factory`; the ppm_flair_check build dep + the
  stale by-construction comments corrected.

## Why (the principle: end HER-02, P3)

HER-02 -- the canonical anti-oracle the Heresy Purge was convened against -- was a
by-construction color grade: ppm_flair_check computed its expected RGB from the
exact function the artifact rendered from, so a wrong palette flowed identically
into both the pixel and the "expected" and the diff could never bite on color.
That is what let the preview.webp palette earn a green ratification it did not
deserve. Now the VALUE authority is `test-color-canon` (independent decomp
goldens); ppm_flair_check grades only structure; and the live desktop renders the
canon teal. The cross-oracle property is mechanical: CANON_MUTATE_TEAL reddens
test-color-canon while the structure stays green.

## Frictions / lessons

- **The decisive Law-4 proof is the screendump, not the oracle text.** I converted
  the booted-386 dump to PNG and looked: the teal Office Space frame. Sampling the
  bg pixel = (141,220,220) = #8DDCDC. The structural oracles were green either way;
  the eyeball is what confirms the operator's "very wrong" is fixed.
- **The 5 switches carried 3 desktop values** (seafoam #6FA08E in palette/render;
  preview gray #73696C in chrome/control/dialog). The collapse onto flair_canon_rgb
  UNIFIED them -- proof the by-construction tangle hid real divergence.
- **CIDX_* macro vs enum:** color_canon.h's index macros collide with chrome.c's
  enum on inclusion; the fix is to drop the local enum (the canon owns the index
  names now).
- **Scope discipline at a judgment boundary:** FO-3 also asks to flip the kmain
  pre-FLAIR TEXT-console fill, but that surface is graded by a DIFFERENT oracle
  (test-tracer-boot/ppm_text_check asserts seafoam) and whether the DOS console
  should be teal is an operator Law-4 call. The FLAIR desktop fully repaints (no
  seam under it), so I landed the ratified FLAIR flip and FILED the text-console
  flip as `initech-r8hv` rather than silently re-key a third oracle.

## Acceptance (Law 2 + Law 4)

- 9 render oracles GREEN: test-chrome/control/dialog/shell/fbagree/color-canon/
  flair-desktop + test-palette-seafoam(re-keyed)/-mutant.
- test-flair-desktop: structural checks GREEN on the TEAL desktop; the 3 structural
  mutants RED; the [+] HER-02 boundary step green; test-color-canon runs first as
  the precondition.
- The seafoam/preview VALUE grep over the render path is EMPTY (only provenance).
- palette.h regen deterministic/generator-true; kernel FLAIR objs build -Werror;
  build/flair_desktop.img boots teal on QEMU.
- Law 4 (the operator's judge): the screendump IS the Office Space frame in the
  Initech colors (presented for the operator's async eyeball).
- Full clean aggregate (`make clean && make test`): GREEN -- **259 host + 43 emu**;
  the Bochs leg (`make test-flair-desktop-bochs`): PASS (the flair_desktop kernel
  boots under Bochs through its fail-loud guard; the composed 640x480 present is
  QEMU-only by the dual-depth strategy of record -- Bochs has no 640x480 LFB).
- ASCII-clean (Rule 12).

## Pointers

- Render path: `os/flair/{chrome,control,dialog}.c`, `harness/render/render.c`,
  `spec/assets/palette.h` (+ generator `tools/palette_extract.c`), `os/boot/stage2.asm`.
- Oracle: `tools/ppm_flair_check.c` (re-keyed) + `tools/ppm_seafoam_check.c` (retired).
- The screendump: `build/flair_desktop.ppm` (the teal Office Space frame).
- Design: ADR-0004-AMENDMENT-DEC-09 (DEC-09-D3, OD-4-REVOKED), ADR-0010 (CD-5/6/7).
- Roadmap: `bd show initech-qipc`; NEXT = `initech-6bq2` (step 5: mechanism/policy
  enforcement oracles C-8 -- flair_look_pixel + test-mech-policy +
  test-flair-mechanism-colorblind) -> `m6qx` (era registry) -> `5l5z` (live loop).
- Follow-up: `initech-r8hv` (the MILTON text-console teal flip, operator Law-4).
- Prior: WL-0056 (canon module + value oracle), WL-0055 (region spine).
