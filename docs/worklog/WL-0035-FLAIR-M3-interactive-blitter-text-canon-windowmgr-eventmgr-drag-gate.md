<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# WL-0035 -- FLAIR M3 interactive core: blitter + text + canon + Window/Event Managers + the DRAG GATE (a draggable System-7 desktop)

**Milestone:** M3 (FLAIR Toolbox) -- drawing primitives + the interactive core + the drag-gate capstone (ADR-0004 AM-8).
**Date:** 2026-06-20. **Branch:** command-com-default (pushed to origin). **Continues:** WL-0034.

## Context

After WL-0034 (ADRs ratified, ground truth, surface + heap + the chrome oracle), the operator's standing directive held: the GUI is the crown jewel, built on real measurements, 100% usable, an obvious System-7+Windows chimera. This session drove the M3 drawing primitives and the interactive core to a **live, draggable, oracle-enforced System-7 desktop** (host-rendered; in-OS wiring is the next arc). Orchestrated as parallel disjoint waves (1 opus + N sonnet), orchestrator-owned Makefile integration + independent re-grading + commit/push-per-wave.

## What changed (3 commits: c54a80a, 41f6f66, 00e3c1e)

**Wave 4 (c54a80a) -- drawing primitives + fidelity.**
- `i50` BLITTER `os/flair/blitter.{c,h}`: region-clipped blit/fill (a pixel writes IFF in blit-rect AND in clip-region) -- the load-bearing clip primitive over surface (no 2nd pixel path). `test-blitter` 11 checks vs an INDEPENDENT region rasterize; IGNORE_CLIP/OFF_BY_ONE mutants.
- `kg5` TEXT `os/flair/text.{c,h}` + `spec/assets/geneva9.h`: hand-authored proportional Geneva-9 + `text_measure`(sum of advances)/`text_draw`/`text_center_in`. `test-text` 27 checks; FIXED_PITCH mutant.
- `k8o5.10` CANON `harness/proptest/test_canon.c`: 67 checks -- hourglass-not-watch, Photoshop menu, PC LOAD LETTER, pie==116, 570-; WATCH/FIX_MENU mutants (Law 4). Points to test-canon-y2k/-salami for the app-rendered canon.
- `ch81` SEAFOAM: `palette.json` desktop_bg reconciled to SEAFOAM canon. **Root fix in the GENERATOR** `tools/palette_extract.c`: it now emits `INITECH_<NAME>_*` for the `canonical` block too, so generated `palette.h` carries `INITECH_DESKTOP_BG_*`=seafoam (render/chrome consume it) + `_FRAME_V0_*`=gray provenance. `test-palette-seafoam` exact-== + mutant. The chrome render desktop is now SEAFOAM.

**Wave 5 (41f6f66) -- the interactive core (freestanding artifact, host-tested, kernel-droppable).**
- `9qf` WINDOW MANAGER `os/flair/window.{c,h}`: z-order, NewWindow/Dispose/Show/Hide/Select, MoveWindow/DragWindow, FindWindow (IM part-codes). visible = strucRgn DIFF union-of-fronts; MoveWindow computes the DiffRgn DAMAGE (D-5: only newly-exposed dirtied, no over-repaint). `test-window` 15 checks BIT-EXACT vs an INDEPENDENT per-pixel owner-grid (non-vacuous: 162 overlaps / 165 L-shaped / 300 stacks); ZORDER/OVERPAINT mutants.
- `8b7` EVENT MANAGER `os/flair/event.{c,h}`: ISR-enqueue-only SPSC ring (D-4) + WaitNextEvent cooking raw input -> EventRecords in task context + cooperative yield hook + cursor tracking. `test-event` 240 checks = recorded raw trace replays to a DETERMINISTIC event sequence (D-8); DROP_SYNTH/STALE_WHERE mutants. (autoKey deferred -> follow-up.)

**Wave 6 (00e3c1e) -- THE DRAG GATE CAPSTONE (87a / AM-8).**
- `os/flair/desktop.{c,h}`: the D-5 minimal-repaint COMPOSITOR (`desktop_paint_all` + `desktop_paint_damage`), reusing the GrafPort visRgn INTERSECT clipRgn seam.
- `test-drag`: builds a seafoam desktop + 3 overlapping System-7 windows, drives a DRAG of the front window through the FULL event->window->compositor loop, and asserts at the PIXEL level (18 checks vs independent owner-grids): (a) incremental == from-scratch full repaint bit-exact; (b) the pixels that change == the computed update regions (NO over-repaint -- the headline D-5 payoff); (c) the vacated area re-exposes the windows behind; (d) chrome geometry holds. SKIP_EXPOSED/NO_CLIP mutants. **Visually audited** (`build/drag_before/after.ppm`): a real draggable Mac desktop with correct clipping + re-exposure.

## Why

This makes "the GUI is built on real measurements" mechanical from the metrics (WDEF source -> chrome_metrics -> test-chrome) all the way up to interaction (drag -> DiffRgn damage -> pixel-level no-over-repaint). The region engine's internal format still has no golden; everything ON SCREEN and every interaction now does.

## Frictions / lessons

- **GENERATED files + make clean (Law 2).** The seafoam fix passed targeted but FAILED `make clean` because `palette.h` is GENERATED from `palette.json` (an agent hand-edited the generated file). Root-fixed in the generator. ALWAYS `make clean && make test-unit` before trusting -- it caught a stale-artifact false-green exactly as CLAUDE.md warns.
- **Orchestrator owns the Makefile.** With 4 agents each adding a gate, none touch the Makefile; each delivers source + self-verifies by direct gcc + hands back its gate recipe; the orchestrator wires all gates in one pass + re-grades. No shared-seam race.
- **Independent ground truth is the discipline.** Every oracle (blitter, window, drag) builds its expected result a DIFFERENT way (rasterize / owner-grid) than the code under test, so a shared bug cannot mask. Look at the screendump (Law 4): the drag PPMs confirmed the windows before trusting green.

## Acceptance

- `make clean && make test-unit`: **208 host gates ALL GREEN** (WL-0034 left 194; +14 = blitter/text/canon/palette-seafoam/window/event/drag x2 each).
- Boot path + emu UNTOUCHED this WL (no FLAIR module linked into the kernel image yet; test-kernel-repro green). The 35 QEMU + Bochs boot gates from WL-0034 stand.
- 3 commits pushed to origin/command-com-default. FLAIR epic 22/33 (66%).

## Pointers / next

- **THE NEXT ARC -- the in-OS desktop.** Everything above is HOST-rendered (the oracles). Wire the FLAIR stack (surface/heap/region/blitter/chrome/text/window/event/desktop) INTO the kernel and boot to a live seafoam desktop in the emulator. Needs: `26d` PS/2 mouse (IRQ12, dual-PIC EOI, Bochs-verified) + the hourglass cursor blit; a desktop kmain path; an emu gate (boot -> desktop screendump). This is where the host-tested logic becomes the actual product (operator's "100% usable").
- **M4 Managers:** `n3e` Menu (Photoshop-exact bar), `8h9` Control (buttons/scrollbars/progress), `qcc` Dialog (modal FILE COPY), `k8o5.12` manager set, `859` desktop shell -> reproduce the frame.
- **Fidelity follow-ups (filed):** palette unification chrome.c/render.c -> palette.h single-source + content-body WHITE (currently gray); autoKey auto-repeat; `u9gf` Bochs RFB pixel capture (fb-agree Bochs leg).
- **OPERATOR-gated (non-blocking):** `pvo4` Mac 68K ROM (Basilisk II rendered System-7 SSIM goldens + exact pinstripe shade RGBs), `77wz` Win 3.1 via dosbox-x (`apt install xvfb xdotool`), `q0gy` 86Box (blocks M4 sign-off). Tier-1 (WDEF-source metrics) is live; Tier-2 (pixel-perfect screenshot goldens) waits on these.
