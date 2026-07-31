# WL-0076 — FLAIR solidity Wave A: the ONE repaint contract lands (gofc + rqz5), test-flair-solid goes green-and-biting

**Date:** 2026-07-31 · **Session:** first-person Wave A (operator-directed: NO delegation — every ruling, edit, and verification on the main thread)
**Baseline:** `09687ed` — WL-0075's ALL GREEN 320 host + 83 emu certificate.
**Epic:** `initech-av7s` (label `flair-solidity`).

## Context

WL-0075 left the SOLIDITY arc staged: three painters with no shared
invariant, solid_check legs A/B/C RED-proven, a design committee stopped
mid-run on DQ1–DQ10, and Wave A specified (`gofc` + `rqz5` + wire the
gate). The operator directed this session to proceed with the work
entirely first-person, and to hunt for latent defects in the existing
code while landing it. The stopped committee was not resumed: the DQ
proposals were already grounded in cited forensics, so they were ratified
solo and recorded as the binding record on the epic's design field
(`bd show initech-av7s`).

## What changed

1. **DQ1–DQ10 ratified** (epic design field). Highlights: DQ1 validation
   ownership — `desktop_paint_damage` clears ONLY `desktop_update` (which
   it services); window-damage validation moves to the content phase
   (`flair_route_updates` validates EVERY damaged visible window it walks,
   unowned included — absorbing/superseding `0zxp`) or the new explicit
   `desktop_validate_all`. DQ2 pump order on every damaging dispatch:
   WM-op → chrome → content → present. DQ3: `reaffirm_active` seeds
   strucRgn bbox on BOTH hilited transitions. DQ4 deferred to the Wave B
   `8fhu` lane; DQ5–DQ8 ratified as proposed for Wave B; DQ9 gate shape +
   DQ10 non-goals ratified. One naming deviation from the WL-0075 draft:
   the leg-C knob is `WINDOW_MUTATE_NO_ACTIVATE_INVAL` (window.c's
   established prefix), not `FLAIR_MUTATE_*`.

2. **`rqz5` — the activation seed** (`os/flair/window.c/h`):
   `reaffirm_active` now seeds the 0→1 transition symmetrically with the
   v6t2 1→0 half; `SelectWindow`'s raise-exposure note and the
   NewWindow/ShowWindow "caller seeds" contracts rewritten to match. New
   mutant `WINDOW_MUTATE_NO_ACTIVATE_INVAL`.

3. **`gofc` — the update contract** (`os/flair/desktop.c/h`,
   `process.c/h`, `os/milton/kmain.c`): `desktop_paint_damage` no longer
   validates window updateRgns; new `desktop_validate_all`;
   `flair_route_updates` validates all walked windows;
   `flair_live_ctx_t` gains a guarded `plist` member armed at the launch
   site; `flair_live_content_phase` (route-or-validate, with the
   `FLAIR_LIVE_MUTATE_NO_ROUTE_ON_CHROME` mutant) now runs on the
   inDrag/inGoAway dispatches; the switch block drops its redundant
   contRgn re-seed; `flair_desktop_run` and the tenants launch site close
   their scene-build books with `desktop_validate_all`.

4. **`test-flair-solid` + `test-flair-solid-mutant`** wired into
   `TEST_EMU_GATES` (85 emu gates now): three locked-trace boots graded by
   `ppm_flair_solid_check` A/B/C; mutants `no_route_on_chrome` (kills legs
   A+B) and `no_activate_inval` (kills leg C; a NEW window.c
   tenants-mutant-image template) proven RED against a green clean
   baseline. No separate Bochs leg: the gate shares `FLAIRTENANTS_IMG`
   with `test-flair-appswitch`, whose `-bochs` leg already proves the
   image's pre-tenants fail-loud differential.

5. **Host contracts strengthened** (the DQ3 blast radius, update =
   strengthen): `test_window.c` gains a directed + randomized (800-stack,
   owner-grid-exact) activation-seed property and the HideWindow damage
   golden now unions the successor's activation seed;
   `test_process_update.c`'s unowned window WU flips from
   "skipped-untouched" to "validated, no delivery" with the mutant
   expectations extended; `test_drag.c`/`test_shell.c` adopt the
   paint-then-`desktop_validate_all` pump idiom.

## Defects found while landing (the operator asked for these)

- **`initech-vtdo` (P1, FIXED here):** `ref_tenant.c` painted at its
  LAUNCH-time cached content rect forever — `paint_content`,
  `accent_rect` (graded HELLO accent used ABSOLUTE demo coords), and
  `marker_rect` all read the open-time snapshot, so a moved tenant
  reconstructed content at the OLD position (solid leg B caught it: the
  moved window's newly-arrived L-strip stayed WDEF white). Fixed root
  cause: all drawn rects derive from the live `contRgn`
  (`content_rect(p)`); the graded accent is carried as an offset from the
  boot content origin so the boot frame is bit-identical. Invisible
  pre-DQ2 because tenants were never routed content after a drag at all.
- **`initech-wlzp` (P2, open):** `updateRgn ⊆ visible(w)` is an UNSTATED
  load-bearing invariant — the tenant updateEvt paint clips to
  `contRgn ∩ updateRgn` only, so any stale seed surviving a z-order
  change lets a background tenant stomp the foreground (the launch site
  was a live instance: NOTES's brief-front full-struct seed survived
  HELLO launching over it; caught by appswitch TIER-A mid-landing, fixed
  narrowly at the launch site). Proper fix: BeginUpdate-fidelity clip at
  delivery. Bead carries the full analysis.
- **`initech-j0vt` (P2, open):** the tenants-scene cross-menu restore
  path (`flair_live_do_menu` → `shell_render`) repaints tenant windows
  chrome-only and never routes content — a band-1 cross-menu drag wipes
  both tenants. Deferred to the Wave B menu lane (t1rv/7tjp/b3hl).
- Stale comment corrected in `ref_tenant.c` (claimed the renderer draws
  no active/inactive chrome distinction — predates a9iq/hv7u and rqz5).

## The mutant that stopped biting (Rule 6 honesty)

`DESKTOP_MUTATE_NO_PAINTALL_CLEAR` (the jmc5/qi8v stale-`desktop_update`
emu mutant) went silent TWICE during the landing: first masked by the new
`desktop_validate_all` second backstop (fixed by sharing the knob across
BOTH books-closing paths — the winh × ojxn double-backstop lesson), then
structurally healed by the DQ3 activation seed itself (every switch now
repaints both tenants' full chrome, so the 2-tenant scene cannot show the
stomp). The class needs a window that is NEITHER activated NOR damaged in
the stomping repaint: it is now proven by a 3-window directed host case —
`test_drag.c` leg (e) (hide a ghost → a window ARRIVES on the stale
footprint → damage only a bystander → the arriver is stomped) — via
`test-drag-mutant`, and the emu image is retired with an OMISSION note at
the instantiation site. `ppm_flair_appswitch_check` TIER-C remains a live
invariant check.

## Acceptance

- `make test-flair-solid`: legs A/B/C PASS on the booted 386, each frame
  eyeballed first-person (Law 4): close-expose repainted, dragged content
  + accent + marker at the new position, full-width active title on the
  raised window with no stale band.
- `make test-flair-solid-mutant`: clean baseline GREEN ×3;
  `no_route_on_chrome` RED on A and B; `no_activate_inval` RED on C.
- `make test-flair-appswitch` + `-mutant` (4 emu mutants) GREEN;
  drag/menu/crossdrag/desktop/live emu gates GREEN; all touched host
  suites GREEN with all mutants biting (window ×4, drag ×3,
  process-update ×2, process-activate ×2).
- Full `make clean && make test` certificate: **ALL GREEN 320 host + 85
  emu** (test-flair-solid + test-flair-solid-mutant join the vector; the
  three Bochs legs ran inside it, Rule 5).
- Rule 12: all touched files ASCII-clean.

## Pointers

- Rulings: `bd show initech-av7s` (design field). Drive report:
  `docs/FLAIR-solidity-drive-2026-07-21.md`.
- Gate: Makefile `test-flair-solid` / `test-flair-solid-mutant`;
  grader `tools/ppm_flair_solid_check.c`; locked traces
  `spec/flair_solid_traces.mk`.
- Beads closed: `gofc`, `rqz5`, `0zxp` (superseded by DQ1), `vtdo`.
  Filed: `wlzp`, `j0vt`.
- Next: Wave B lanes per drive report §7 (B1 chrome.c, B2 ref_tenant
  titles, B3 kmain interaction incl. DQ5–DQ8), then `883x`/`r8r7`
  cluster; B8 seed (`ogxv`) still queued behind the arc; `altq` still
  needs the operator's `sudo apt install fpc`.
