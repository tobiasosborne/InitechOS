<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# WL-0079 -- THE DESKTOP IS PLATINUM: 3knt + i3si land together (DEC-10 executed end-to-end)

**Date:** 2026-08-16/17 (one orchestrated overnight session)
**Beads:** `initech-3knt` (CLOSED), `initech-i3si` (CLOSED), epic `initech-t1i0`
**Commits:** `d8d16e4` (slice 1), `8671799` (slice 2), `b880a62` (the joint Platinum
landing), `c2e469d` (FO-10.7 docs sweep)
**Certificate:** `make clean && make test` ALL GREEN -- **324 host + 85 emu** (3 Bochs
legs inside); 6 `record-flair` clips + repro-gate byte-identical.

## Context

DEC-10 (ADR-0004-AMENDMENT, RATIFIED 2026-08-16) flipped the FLAIR base era to
`ERA_SYS8_PLATINUM` per the operator's Law-4 frame ruling. WL-0078 minted the Mac OS 8.1
goldens and authored `../system7-decomp/specs/sys8/` (sampled-domain, capture+coords
cited). This session executed the whole re-key + render arc, oracle-first, per the
D-10.8 sequencing and the WL-0075 "gates land WITH the fixes" pattern.

## What changed (serial slices; codex exec gpt-5.6-sol xhigh lanes, orchestrator graded
every diff first-person and ran ALL emu/video proofs)

1. **Slice 1 (`d8d16e4`)** -- `spec/flair_skins.h`: ERA_SYS8_PLATINUM row APPENDED as
   BASE; Option B explicit default (`FLAIR_DEFAULT_ERA` + `flair_skin_default()`);
   `SKIN_FROZEN_DIGEST` **unchanged 0xDEF099AC** (D-10.5a accretion proof); frozen
   oracle re-keyed to strictly-stronger inversions (exact-3 rows, sole-Platinum-row,
   default-resolves legs); teal oracle extended to the Platinum identity slots; 2 NEW
   mutants RED-proven.
2. **Slice 2 (`8671799`)** -- canon `era: multi` (OQ-1); 11 Platinum **SAMPLED-domain**
   ramp indices `CIDX_PLAT_*` (OQ-3: zero non-neutral rows; accent = existing teal per
   OQ-2); generator RAMP_N 2->13; `test-color-canon` **LEG F** vs the independent sys8
   spec + the `CANON_MUTATE_PLAT` gamma-trap mutant (nominal-domain relapse #777777
   rejected).
3. **B1+B2 (`b880a62`)** -- Route 2 (OQ-7): `chrome_metrics.{h,json}` re-keyed (22px
   band, 12 stripes, 3 widgets L+4/R-32/R-16 top T+4, 4px raised body frame, 2px shadow
   notch; SYS7 values retained as `FLAIR_CHROME_SYS7_*`); `chrome_fidelity_golden.h`
   re-derived from window-chrome.md/scrollbars.md (35 `FG_SYS7_*` retained);
   `test_chrome_fidelity` re-keyed -- proven **RED-for-Platinum-reasons first** (14
   legs) against the pre-change renderer -- then `chrome.c` rendered the full delta
   through **11 new flair_look PARTs** (C-8 intact: zero literals, zero era branches;
   colorblind + mech-policy mutants green). 17 legs GREEN; **all 14 fidelity mutants**
   (10 re-pointed + COLLAPSE/RAMP/NOTCH/BODYBAR) RED-proven. Host scene tests re-keyed
   strictly-stronger (exact 22-row profiles).
4. **B3a (`b880a62`)** -- emu graders: only 3 of 12 gates needed re-keys (desktop legs
   c/d, drag LEG A, solid leg C) -- the WL-0076 behavioral-grading discipline paid off.
   All scene mutants re-proven under QEMU; tri-emulator agreement (QEMU + 3 Bochs legs).
5. **Videos (operator directive)** -- all 6 locked scripts recorded
   (solid_close/drag/switch/appswitch/clamp/raise -> build/clips/), `record-flair-repro`
   byte-identical; drag + raise clips frame-eyeballed: chrome moves with content intact,
   active/inactive transitions correct. NOTE: `appswitch` and `solid_switch` map to the
   same tenants trace (identical clips) -- script-map fact, not a bug.
6. **FO-10.7 (`c2e469d`)** -- nothing claims System-7-as-base anymore; OQ-8 clut
   annotation widened; OQ-9 Appearance aliases documented; chimera base origins
   re-tagged platinum (accents + intentional rows untouched); explicit TODO_GOLDEN
   deferrals for menu-face/control chrome.

## Frictions / traps (recorded in bd memory `system-8-pivot-re-key-lesson-2026-08`)

- **Value-coincidence false friends:** Platinum's dark stripe #969696 byte-aliases SYS7
  `CIDX_PIN_DARK`'s RGB; the well gray #C0C0C0 aliases win31 BTNFACE. Tells that pass by
  coincidence were re-ATTRIBUTED (sampled-domain provenance comments), never "deduped".
- **Stale-fixture trap:** a failing solid gate short-circuits before refreshing later
  legs' dumps -- the B3a lane correctly refused to sign off G/H against yesterday's
  SYS7 captures; refreshed dumps matched the sys8 profile exactly.
- **`codex exec resume --last`** rejects `-s`; use `-c sandbox_mode=workspace-write`.
  Prefer explicit session ids when other sessions may run codex on the host (a peer
  session raised a since-retracted steal hypothesis; `--last` is cwd-filtered).
- codex sandbox cannot open `../dbase3-decomp` fixtures read-write; it used the
  documented `DBASE3_DECOMP` override with a byte-identical copy -- orchestrator re-ran
  the literal vector unsandboxed.

## Follow-ups filed

`initech-chd4` (Route 1: era axis as data into flair_look/chrome.c -- OQ-7's funded
follow-up), `initech-sjvq` (Platinum menu-bar/panel chrome, operator Law-4 call on the
chimera bar), `initech-81ft` (Platinum control chrome incl. the unresolved pink alert
bevel). Also this session: 6 SAMIR gap beads under `initech-586` (aggregates/DDL/macros
P3; BROWSE/reports/ASSIST P4) from the dBASE completeness audit.

## Acceptance

`make clean && make test` ALL GREEN 324 host + 85 emu (see the certificate log);
`test-chrome-fidelity` 17/17 with 14/14 mutants; LEG F 11/11 vs specs/sys8; digest
0xDEF099AC standing; videos recorded + deterministic; operator Law-4 eyeball of the
booted 386 desktop still owed on the fresh screendump (clips ready in build/clips/).

## Pointers

ADR-0004-AMENDMENT-DEC-10 (Sec 4 re-key table = the map of everything touched);
`../system7-decomp/specs/sys8/INDEX-sys8.md`; WL-0078 (the mint); this session's briefs
in the session scratchpad (brief-slice1/2, brief-phaseB1/B2/B3a/B5).
