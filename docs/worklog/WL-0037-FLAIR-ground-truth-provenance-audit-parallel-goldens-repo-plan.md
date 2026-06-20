<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# WL-0037 -- FLAIR ground-truth provenance audit + the PARALLEL goldens-repo plan (next-agent handoff)

**Type:** Provenance audit + handoff (no code change). **Date:** 2026-06-20. **Branch:** command-com-default (pushed).
**Epic:** `initech-rf2l` (coordinates `pvo4`/`77wz`/`q0gy`/`u9gf`). **Continues:** WL-0034..0036.

## Context

Operator asked "what sources did you use to create spec/goldens?" -- a Law-1 provenance audit of the FLAIR work. This shard records the honest accounting AND the decision that the heavy real-software ground-truth lives in a PARALLEL/SISTER REPO (the `../dbase3-decomp` pattern), not the main repo. This is queued work for the next agent.

## What each FLAIR spec/golden was actually built from (audited from the files)

| Artifact | Source | Status |
|---|---|---|
| `spec/chrome_metrics.json` native (title 19 / scrollbar 16 / menu 20 / dialog 7 / frame 1 / WBoxDelta 13 / icon 20 / pinstripe period 2 / shade idx 7,8) | Apple **System 7.0 WDEF assembly source `StandardWDEF.a`** + `GetMBarHeight` (Toolbox-128), real developer.apple.com URLs per value | Cited; values match the documented constants. **GAP:** see Honesty Gap 1. A few (Toolbox-303/313 scrollbar/frame) are brief-sourced (IM PDF not text-extracted). |
| `grafport.h` / `imaging.h` / `window_record.h` / `event_model.h` | **Verbatim Inside Macintosh** (Toolbox Essentials + Vols I-VI) field names / part-codes / transfer modes / WDEF variants | Cited in-header; brief-sourced (well-known IM constants, PDFs not cached). |
| `chicago8x16.h`, `geneva9.h` | **HAND-AUTHORED clean-room** (NOT extracted; macfonts/BDF clones are reference-only, never shipped) | Honest, Law-1 labeled. |
| `cursors.h` (hourglass) | **HAND-AUTHORED clean-room canon** (our invention; NOT the real System-7 watch, NOT from ROM/frame) | Honest. |
| `menu_canon.h` ("File Edit Image Layer Select View Window Help"), FILE COPY "Saving tables to disk..." | The **prop's verbatim strings from the frame** (PRD Appendix A) | Locked canon. |
| `palette.json` `colors` anchors | **Sampled from the frame photo** `preview.webp` via `palette_extract.c` | Acknowledged unreliable (compressed CRT photo). |
| `palette.json` `canonical` desktop_bg = SEAFOAM | `stage2.asm` SEAFOAM_R/G/B (the boot/oracle value, ADR-0004 OD-4) | Authoritative. |
| `region_algebra.h` | **NO external golden possible** (QuickDraw region body unpublished); oracle = intrinsic homomorphism property test. Only op NAMES + Rect order from IM | By design (ADR-0005). |

## The two HONESTY GAPS (Law 1 / Rule 4)

1. **Sources cited but NOT cached locally.** `chrome_metrics.json` `_provenance` says the WDEF source was "FETCHED AND VERIFIED", but **`refs/` does not exist** -- the subagent fetched via WebFetch (returns content, persists nothing) and verified in-transcript, then wrote nothing to disk. So the citations are URLs + an unrepeatable in-transcript fetch, NOT local files we can diff against. The VALUES are the well-documented authentic System-7 constants (correct), but the "first-hand verified this session" wording overstates what is locally auditable / reproducible. FIX: cache the WDEF listing + IM PDFs (into a gitignored `refs/` or the sister repo) and soften the wording to match.
2. **NO real-era-software screenshot goldens exist yet** -- the operator's gold standard. Everything pixel-perfect is flagged `golden_resolves` (pinstripe shade RGBs from wctb -4096, the real content-white, close/zoom interior, grow fill, Win31 SM_CYCAPTION/SM_CXVSCROLL) and is GATED on a real boot. So far the goldens are: source-cited geometry, hand-authored clean-room assets, the (unreliable) frame photo, and intrinsic property oracles -- NOT yet real System-7/Win-3.1 captures.

## The decision: heavy ground-truth lives in a PARALLEL / SISTER REPO

Mirror the SAMIR pattern exactly. `../dbase3-decomp` holds the copyrighted dBASE goldens + a dosbox-x mint harness; Makefile `$(call need_goldens,<gate>)` resolves `DBASE3_DECOMP`; absent -> loud-skip (Makefile ~622-626; HANDOFF ~370). Do the same for the GUI:
- A **sister GUI-ground-truth repo** holds: the fetched Inside Macintosh + WDEF reference PDFs/listings; the Mac 68K ROM + System 7 disk image; the Win 3.1 image; the Basilisk II / dosbox-x headless MINT HARNESS; and the REAL-SOFTWARE SCREENSHOT GOLDENS (+ derived golden_resolves values).
- The MAIN repo gains a `need_goldens`-style helper + a `FLAIR_GOLDENS` env var; FLAIR fidelity gates resolve it and LOUD-SKIP if absent. NOTHING copyrighted or large enters the main repo (ROMs/images stay gitignored + external, per the existing `.gitignore` `*.ROM` / `refs/`).

## Next-agent task list (epic `initech-rf2l`)

1. Cache the WDEF listing + IM PDFs locally (gitignored `refs/` or the sister repo); correct `chrome_metrics.json` `_provenance` wording (cited + values match documented constants; local cache via the goldens repo). (Honesty Gap 1.)
2. Create + populate the sister GUI-ground-truth repo; wire the main-repo `need_goldens`/`FLAIR_GOLDENS` helper + loud-skip.
3. Win 3.1 chrome golden via dosbox-x (NO ROM; `apt install xvfb xdotool`) -> resolve `win31_sm_cycaption`/`sm_cxvscroll` (`77wz`).
4. Mac System-7 goldens via Basilisk II (gated on the Mac ROM, `pvo4`) -> resolve pinstripe shade RGBs (wctb -4096), content-white, close/zoom interior.
5. Feed the resolved `golden_resolves` values back into the locked spec + mutation-prove; then the `bmih` content-white fix can use the REAL white.

## Acceptance / state

- No code changed this shard; provenance audited; honesty gaps recorded; epic `rf2l` filed (parents `pvo4`/`77wz`). Main-repo gates remain 216 host + 35 QEMU + Bochs GREEN (WL-0036).
- Pushed to origin/command-com-default.
