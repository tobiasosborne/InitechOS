<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0061 -- window-chrome fidelity: title text + knockout (the biggest visual win)

**Type:** implementation shard (oracle-first / RED->GREEN; the title-text element).
**Date:** 2026-06-25.
**Bead:** `initech-lxg9` (epic umbrella `initech-hmll`).
**Commit:** (this shard's commit). Prior: WL-0060 (the pinstripe-phase element).

## Context

The window-chrome fidelity oracle (`test-chrome-fidelity`, WL-0060) graded the
pinstripe phase. Its biggest deferred element was the TITLE: `chrome.c:279` drew
the title bar BLANK (a documented Font-Manager deferral), so the live windows had
no names -- the single most visible gap vs a real System 7 window. This shard draws
the centered Chicago title over a knocked-out light gap and grades it.

## What changed

- **`os/flair/chrome.c` + `chrome.h`:** `flair_draw_document_window` gains a
  `const char *title`. The title is drawn CENTERED in Chicago over a KNOCKOUT light
  panel (System 7 suppresses the racing stripe under the title; golden
  s7_doc_window.png: a centered #F3F3F3 gap with black glyphs;
  ../system7-decomp/specs/chrome/title-bar.md Sec 3). `surface_blit` writes the
  glyph-cell background OPAQUELY, so `text_draw(fg=ink, bg=light)` paints the
  knockout AND the glyphs in one pass. Both ink + knockout resolve through the C-8
  policy seam (`flair_look_pixel(FLAIR_PART_TEXT/PIN_LIGHT)`) -- NEVER a color
  literal -- so `test-flair-mechanism-colorblind` stays green. `text_draw`/
  `text_measure` are header-only inlines (no new link dep). Mutant
  `CHROME_FID_MUT_NO_TITLE` (blank bar) added.
- **`os/flair/window.c` (root-cause fix):** `NewWindow` now defaults `titleHandle`
  to "" (it left it UNINITIALIZED; latent bug my change exposed -- any window whose
  record was not zeroed would render a garbage title). IM-I "NewWindow leaves "".
- **`os/flair/desktop.c`:** `paint_window_chrome` passes the window's existing
  `w->titleHandle` (the live windows already carried titles).
- **`harness/proptest/test_chrome_fidelity.c` + `spec/chrome_fidelity_golden.h`:**
  render with a title; the PHASE scan moved to a clear column (right of the close
  box, LEFT of the centered title); two NEW title legs vs the golden -- (5) FIGURE
  ink (idx 4 = CIDX_TITLE_INK) present in the centered region, (6) KNOCKOUT (zero
  dark idx-8 under the title). Both RED on the blank-bar render, GREEN now;
  mutation-proven by CHROME_FID_MUT_NO_TITLE.
- **Scan moves (the title now occupies bar-center):** `test_shell.c` + the live
  `ppm_flair_check.c` leg (c) + its HER-02 demo pinstripe relation now scan a clear
  column (W1_L+24). `ppm_flair_check` leg (c) also gained a LIVE-desktop title-ink
  tripwire (the booted OS title must render). `test_chrome.c`/`test_drag.c` pass ""
  (they grade geometry, not the title -- no move needed; test_drag is untitled via
  NewWindow). `test-flair-mechanism-colorblind` passes a real title to exercise the
  seam.

## Why

Title text is the largest single fidelity gap (a blank title bar reads nothing like
System 7). The knockout-via-opaque-bg approach reuses the existing surface_blit
semantics and the C-8 seam, so it is faithful AND policy-clean. The NewWindow fix
is root-cause (Rule 3), not a band-aid around the symptom.

## Frictions / lessons

- **The title sits at bar-center -- exactly where 5 oracles scanned the pinstripe.**
  Adding the title forced moving those scans to a clear column. A grep (Rule 3/4)
  was essential: the HER-02 demo's pinstripe relation ALSO scanned mid_x and was
  caught only by the emu run (it failed shade_ok on the title ink) -- moving leg(c)
  was not enough.
- **Flaky `test-kernel-repro` in the INCREMENTAL aggregate** went RED (byte 400)
  once, then GREEN standalone + on a CLEAN aggregate (sha256 stable, 0 diff). The
  WL-0028 lesson stands: authoritative = `make clean && make test-unit`.
- **C-8 seam is load-bearing for text:** drawing the title with a raw color literal
  would have tripped the colorblind oracle. Resolving fg/bg via flair_look_pixel
  keeps the title sentinel-clean under the stub.
- Chicago is the fixed-cell 8x16 v0 strike (text.h), not proportional Chicago 12 --
  a known font-fidelity approximation, separate from this element.

## Acceptance

- `make clean && make test-unit`: **ALL GREEN -- 271 host gates.** test-chrome-fidelity
  7/0 (phase + title legs); CHROME_FID_MUT_PHASE + CHROME_FID_MUT_NO_TITLE both RED.
- colorblind 2/0 (title seam-clean), test-chrome 29/0, test-flair-shell 38/0,
  test-drag 18/0, test-window 15/0, test-kernel-repro green.
- **Rule 5:** `test-flair-desktop` (QEMU) PASS -- live title ink 121px in the
  centered region, `_kernel_end=0x35b20 < 0x40000` (the Chicago strike in chrome.o
  added ~3.6 KiB); `test-flair-desktop-bochs` PASS.
- Law 4: live screendump eyeballed -- W0 "untitled-1", W1 "Saving tables to disk"
  (the canon front-window title), both centered Chicago over a light knockout gap.
- ASCII-clean (Rule 12).

## Pointers

- Drawer: `os/flair/chrome.c` (title block). Oracle: `harness/proptest/test_chrome_fidelity.c`
  + `spec/chrome_fidelity_golden.h`. Live tripwire: `tools/ppm_flair_check.c` leg (c).
- Ground truth: `../system7-decomp/specs/chrome/title-bar.md` Sec 3 + `pinstripe.md`.
- Remaining fidelity elements (umbrella `initech-hmll`): `92li` bevel+recomp,
  `ts3t` close/zoom box, `jh7m` scrollbar, `54nw` drop shadow.
