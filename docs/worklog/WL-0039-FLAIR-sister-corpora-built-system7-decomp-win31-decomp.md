<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# WL-0039 -- FLAIR sister ground-truth corpora BUILT (system7-decomp + win31-decomp); WL-0037 loop closed

**Type:** Cross-repo ground-truth build + handoff (no initech-os code change). **Date:** 2026-06-20.
**Branch:** command-com-default. **Continues / CLOSES the queued work of:** WL-0037 (the PARALLEL goldens-repo plan).
**Related beads:** `pvo4` (Mac ROM/goldens), `77wz` (Win SM_*), `q0gy` (86Box, still deferred per DEC-04).

## Context

WL-0037 audited FLAIR's spec/golden provenance and decided the heavy real-software ground truth lives in a PARALLEL/SISTER REPO (the `../dbase3-decomp` pattern), queuing "create + populate the sister GUI-ground-truth repo" for the next agent. The operator then directed splitting it into TWO sister corpora by source-OS. **That work is now done.** This shard records it so a future FLAIR agent does NOT re-create them.

## What exists now (two new sibling repos, NOT in this repo)

- **`../system7-decomp`** -- Apple Macintosh System 7 GUI ground truth. Read its `HANDOFF.md` + `CLAUDE.md`.
- **`../win31-decomp`** -- Microsoft Windows 3.1 GUI ground truth (the chimera accents). Read its `HANDOFF.md`.

Both mirror the `dbase3-decomp` spec+ground-truth-corpus pattern (CLAUDE/README/SOURCES/INDEX/GAPS/specs/OUTLINE/re/mint/tools; heavy ground truth gitignored). The seed brief `docs/research/gui-ground-truth.md` was split + deepened across the two.

## State delivered

- **system7-decomp:** 32/43 specs authored + adversarially verified (quickdraw 9, toolbox 8, chrome 9, desktop 6). The **Mac mint harness works** (Basilisk II + the operator-supplied Quadra 650 ROM, headless) -- 23 real **System 7.5.3** color-chrome screendumps + 22 harvested System-file resources, pixel-measured (pinstripe `#F3F3F3`/`#969696`, bevel `#DADAFF`/`#B3B3DA`, title bar ~18px, menu bar ~19-20px). Remaining: fonts/ (5) + resources/ (6).
- **win31-decomp:** 19/45 specs verified (user 11, gdi 8). **Windows 3.1 installed headless** under DOSBox-X; Program Manager + dialog + listbox-scrollbar goldens; the `.FON` fonts local. Remaining: chrome/ (10) + fonts/ (5) + shell/ (6) + ini-resources/ (5).

## Bead-relevant resolutions (for the `initech-rf2l` epic)

- **`pvo4` (Mac native-pixel goldens):** ROM gate RESOLVED (operator supplied the old-world Mac ROM set; Quadra 650 `F1ACAD13` is the Basilisk primary). Pinstripe/bevel/box shades MINTED (7.5.3). **Honesty caveat:** goldens are 7.5.3, not the 7.0/7.1 target -- geometry is shared, but the lavender bevel tinge wants a bootable 7.0/7.1 image to confirm (the 7.0.1 download was a model-locked "Minimal" disk). Law-2 correction: `wctb` -4096 is NOT in the System file.
- **`77wz` (Win SM_CYCAPTION / SM_CXVSCROLL):** Win 3.1 installed + captured; SM_CYCAPTION ~18px measured. **Depth caveat:** the base floppies install only the 16-color VGA driver (caption renders `#0000AA`, `#C0C0C0` dithered); FLAIR's indexed-8 target renders the true WIN.INI `#000080`/`#C0C0C0`. A 256-color re-mint (needs an S3 driver) gives the pixel-exact values.
- `q0gy` (86Box) unchanged -- still deferred per ADR-0004 DEC-04.

## The main-repo wiring NOT yet done (next FLAIR agent)

The `need_goldens`-style resolver for the GUI corpora is NOT yet added to this repo's Makefile. When the corpora goldens are ready to gate FLAIR's `test-chrome`/`fb-agree`, add `SYSTEM7_DECOMP ?= ../system7-decomp` + `WIN31_DECOMP ?= ../win31-decomp` env vars + a `need_flair_goldens` loud-skip (mirror the `DBASE3_DECOMP`/`need_goldens` pattern, Makefile ~627). Do this only once real pixel-diff goldens exist (a gate with no goldens loud-fails by design).

## Acceptance / pointers

- No initech-os code/spec changed. The sister corpora are the deliverable, in their own repos (commit there per their HANDOFFs).
- Two operator acquisitions remain (both abandonware-grey): a bootable 7.0/7.1 Mac image; a 256-color S3 Win 3.1 driver.
- Pointers: `../system7-decomp/HANDOFF.md`, `../win31-decomp/HANDOFF.md`, their `re/mint-results-00{1,2}.md`.
