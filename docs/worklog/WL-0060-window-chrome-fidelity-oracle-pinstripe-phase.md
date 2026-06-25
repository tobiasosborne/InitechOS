<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0060 -- the FLAIR window-chrome FIDELITY oracle (first element: pinstripe phase)

**Type:** implementation shard (oracle-first / RED->GREEN; closes a real Law-2 gap).
**Date:** 2026-06-25.
**Bead:** `initech-hmll` (FLAIR window-chrome fidelity oracle). Follow-ups:
`initech-lxg9/92li/ts3t/jh7m/54nw`.
**Commit:** (this shard's commit).

## Context

Operator observation: the live FLAIR windows "do NOT look like System 7 or 8,"
and a suspicion that "the goldens do not enforce the windows are correct" -- the
operator believed goldens were pixel-matched against ground-truth screenshots.

An audit (two workflows + first-person verification) confirmed the suspicion:
**no green gate enforced window APPEARANCE against ground truth.** `test-color-canon`
grades color VALUES (independent decomp), `test-chrome` grades metric NUMBERS vs
FLAIR's OWN `chrome_metrics.h` ("STRUCTURAL compare, not SSIM"; ADR-0004 D-8),
`ppm_flair_check` grades scene topology calibrated to `test_shell.c`. The real
System-7 window captures (`../system7-decomp/goldens/captures/*.png`) were NOT
diffed against any FLAIR render; SSIM (`harness/ssim.c`) is unbuilt. So a window
with the right palette + metric numbers but the wrong appearance passed everything.

Sharpest demonstration: `ppm_flair_check` leg (c) asserts the title-bar pinstripe
"alternates period-2." FLAIR free-ran the fill from the title top -> `L,D,L,D` --
which IS period-2, so it PASSED -- but the real System-7 racing stripe is
PHASE-LOCKED (patAlign mod-8), landing doubled-LIGHT pairs at the band edges
(`L,L,D,...,D,L,L`; `../system7-decomp/specs/chrome/pinstripe.md`, golden
`s7_doc_window.png` x=450 y=166..180). The "period-2" check literally could not
tell FLAIR's wrong phase from the right one.

## What changed (oracle-first, operator-ratified GREEN)

- **`spec/chrome_fidelity_golden.h` (NEW, locked):** the INDEPENDENT System-7 chrome
  golden, pixel-measured from `../system7-decomp` (the captures + the pixel-verified
  `specs/chrome/*.md`), keyed to System-7 shade INDICES / row-class patterns --
  recolor-invariant (mechanism only; the teal canon is graded by `test-color-canon`).
  NOT derived from `chrome_metrics.h` (the HER-02 by-construction trap). First
  element: the title-bar pinstripe phase pattern `LLDLDLDLDLDLDLL`.
- **`harness/proptest/test_chrome_fidelity.c` (NEW) + `test-chrome-fidelity` gate:**
  drives the REAL `chrome.c` through the host render skeleton, scans the title-bar
  stripe column on the 8bpp pass, asserts the phase-lock signature (doubled-LIGHT
  pairs at both band edges) vs the golden. Mutation-proven by `CHROME_FID_MUT_PHASE`
  (revert to the free-running fill -> RED). Landed RED on the old render, then GREEN.
- **`os/flair/chrome.c`:** phase-locked the pinstripe fill -- LIGHT at both band
  edges + odd interior rows, DARK on even interior rows -> the golden's doubled-light
  pairs. (Bevel rows + the exact 15-row interior decomposition are deferred to `92li`.)
- **5 strict-period-2 assertions amended -> phase-AGNOSTIC "striped" checks** (the
  fidelity oracle now solely owns phase; operator-ratified since `test_chrome.c` is the
  ADR-0004 D-8 hard gate): `test_chrome.c`, `test_shell.c`, `test_drag.c`,
  `ppm_flair_check.c` leg (c) + the HER-02 demonstration relation. (`test_drag.c` was
  found only by a defensive grep -- Rule 3/4.)
- **Makefile:** new gate + mutant, wired into `TEST_UNIT_GATES`.

## Why

The re-flair purge fixed COLOR but never added an APPEARANCE oracle. Strengthening
an oracle is sanctioned (CLAUDE.md); the independent golden already existed in
`../system7-decomp` (pixel-measured). The amended period-2 assertions were not
"weakened" -- they asserted something FALSE about real System 7 (they encoded the
free-running bug), so correcting them to the phase-locked ground truth is the fix.

## Frictions / lessons

- **The frame is a low-res reference; my own eyeball was wrong.** I initially called
  FLAIR's "full-height period-2 stripes" a deviation -- the decomp pixel-scan proved
  the stripes are CORRECT and only the PHASE is wrong. Law 1 (ground truth before
  code) caught it before any edit. The oracle replaces the eyeball precisely here.
- **Blast radius via the live desktop:** `chrome.c` renders the live kernel desktop
  (`desktop.c:119` <- `shell_render`), so the change is Rule-5 (tri-emulator) and
  touched `ppm_flair_check` (incl. the HER-02 demo). A grep found a 5th period-2
  site (`test_drag.c`) the per-file analysis missed.
- **Two prior workflows failed on over-strict StructuredOutput schemas;** the
  re-run with FLAT schemas (string + string-array only) succeeded. Lesson: keep
  workflow schemas flat.
- **Scope discipline:** kept the band HEIGHT unchanged (phase-only) to minimize
  blast radius; the bevel + 15-row recomposition (which shifts `content_top`) is a
  separate increment (`92li`).

## Acceptance (Law 2 / Rule 5 / Rule 6)

- `make test-unit`: **ALL GREEN -- 271 host gates** (was 269; +2 fidelity gate +
  mutant). `test-chrome-fidelity` GREEN; `CHROME_FID_MUT_PHASE` correctly RED.
- Amended gates green: `test-chrome` 29/0, `test-flair-shell` 38/0, `test-drag` 18/0.
- **Rule 5:** `make test-flair-desktop` (QEMU) PASS -- `(c) two-shade stripe`,
  `ppm_flair_check PASS`, HER-02 boundary PASS, VERDICT PASS; `test-flair-desktop-bochs`
  PASS (boots the fallback, no triple-fault).
- Live screendump eyeballed (Law 4): teal desktop + 2 menu bars + 2 pinstripe windows
  + FILE COPY modal -- the frame, unregressed.
- ASCII-clean (Rule 12).

## Honest gap (Law 1)

This closes the ENFORCEMENT gap and fixes the PHASE; it is correctness, not a
dramatic visual change. The windows still lack the bigger System-7 elements --
title text, the 3-D bevel, scrollbar arrows/thumb, the drop shadow -- now filed as
`initech-lxg9` (title text -- biggest visual win), `92li`, `ts3t`, `jh7m`, `54nw`,
each grading the REAL `chrome.c` against a decomp-pixel-measured golden.

## Pointers

- Golden: `spec/chrome_fidelity_golden.h`. Oracle: `harness/proptest/test_chrome_fidelity.c`.
- Drawer: `os/flair/chrome.c` (pinstripe loop). Amended oracles: `test_chrome.c`,
  `test_shell.c`, `test_drag.c`, `tools/ppm_flair_check.c`.
- Ground truth: `../system7-decomp/specs/chrome/pinstripe.md` + `window-frame.md`;
  captures `goldens/captures/s7_doc_window.png`.
- Roadmap: `bd show initech-hmll` (umbrella) + its 5 dependents.
- Prior: WL-0059 (era-layering skin registry).
