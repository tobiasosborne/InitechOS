<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0053 -- The Initech Color canon: FLAIR palette source corrected to the decomp goldens (a DECISION shard; re30.4 implements next session)

**Type:** decision/continuity shard (no code landed -- the implementation is the
NEXT session's re30.4). **Date:** 2026-06-21.
**Beads:** initech-hfeg (P1, the directive), initech-re30.4 (re-oriented),
initech-fgs1 (P3 hardware era), initech-2gva/6rim/5dr8 (prior re30.2/3 follow-ups).
**Memory:** `bd memories initech-color` -> `initech-color-canon-operator-2026-06-21-definitive`.

## Why this shard exists

After re30.3 shipped the live desktop, the operator inspected the render and
ruled the **palette was "very wrong."** Root cause (Law 1, verified): the FLAIR
chrome colors in `spec/assets/palette.h` are MEASURED SAMPLES from the low-res
`spec/assets/preview.webp` Office Space MOCK-UP frame (a warm-cast, dim CRT
photo) -- NOT authentic period color. So the window "white" was a muddy
`#7F7F86` (a 50% gray-purple), pinstripe was dark `#6B6B74/#8A8A93`, the menu
bar `#67696C` -- the windows rendered muddy-gray instead of crisp Mac white.

A committee (re30.4-palette-source) had, one step earlier, ruled to UNIFY onto
that same `palette.h` semantic source -- preserving the wrong reference. The
operator OVERRODE it: **FLAIR is graded against the painstakingly-built sister-
repo goldens/specs (`../system7-decomp`, `../win31-decomp`); those color schemes
are canonical.** (The committee + I had anchored on the preview.webp "seafoam"
as ratified canon; the operator is the human Law-4 judge and outranks that.)

## The Initech Color canon (operator, DEFINITIVE)

The signature **"Initech teal"** = VIC-20 cyan (hardware colour 3):
- **SCREEN / phosphor value = `#8DDCDC`** (operator motion read, H180 S53 L71).
  Alt VIC decodes: `#85D4DC` (Lospec), `#87D6DD` (azulianblue) -- prefer the
  NTSC 6560 / PAL 6561 decode if matching a real VIC. `#8DDCDC` is canonical.
- **PIGMENT value = `#57B19F`** -- PRINTED PROPS ONLY (box, manuals, floppy
  labels; pre-CRT). The composite/CRT shader carries pigment toward the screen
  cyan; never use `#57B19F` for the on-screen render.

**The palette RULE for FLAIR:**
1. **System 7 colors are GOLDEN** (System 8/Platinum accretes later under the
   same rule), graded vs the system7-decomp goldens.
2. **Replace ALL System-7 LAVENDER tinge** (`#DADAFF`, `#CCCCFF`, `#B3B3DA`,
   `#8787B3`, `#333366` -- the 3D bevel/groove highlights+shadows on title bars,
   dialog grooves, close/zoom boxes, scrollbars) **with Initech teal `#8DDCDC`**
   (preserve the 3D by using teal-light `#8DDCDC` + a darkened teal shadow,
   approx `#4E9BA3`, same hue).
3. **Desktop background = `#8DDCDC`** -- this SUPERSEDES ADR-0004 OD-4
   (seafoam `#6FA08E`). The teal is what endows InitechOS with its character.
4. **Win 3.1 colors** (win31-decomp) for the **additional Windows/Photoshop
   chrome bits** (BTNFACE `#C0C0C0`, navy `#000080`, etc.).

## The approved per-index re-derivation (operator: "sounds good")

| idx | role | was (preview.webp) | -> CANON | source |
|----|------|------|------|--------|
| 0 | ink/frame/text | `#000000` | `#000000` | System 7 |
| 1 | window/content white | `#7F7F86` | `#FFFFFF` | System 7 wContentColor |
| 2 | desktop bg | `#6FA08E` seafoam | `#8DDCDC` | **Initech teal** (supersedes OD-4) |
| 3 | menu-bar bg | `#67696C` | `#FFFFFF` | System 7 (white bar, black baseline) |
| 4 | title ink | `#525A63` | `#000000` | System 7 wTextColor |
| 5 | accent (progress/pie/sel) | `#1E2F87` | `#000080` | Win 3.1 navy |
| 6 | control face | `#BFBFBF` | `#C0C0C0` | Win 3.1 BTNFACE |
| 7 | pinstripe light | `#6B6B74` | `#F3F3F3` | System 7 (rendered golden) |
| 8 | pinstripe dark | `#8A8A93` | `#969696` | System 7 (rendered golden) |

Plus the lavender->teal bevel/groove swap (chrome.c/control.c/dialog.c bevel
logic): light tinge -> `#8DDCDC`, dark tinge -> darkened teal `~#4E9BA3`.

## Canonical SYSTEM 7 chrome values (from system7-decomp, for the implementer)

- window content WHITE `#FFFFFF`; window frame `#000000` 1px.
- ACTIVE title bar pinstripe: period-2 `$FF00` HilitePattern, light `#F3F3F3`
  / dark `#969696` (softened grays, rendered-golden authority, era-stable
  7.0.1-7.5.3); bevels were lavender `#DADAFF`/`#B3B3DA` -> NOW TEAL.
- menu bar: white `#FFFFFF` fill, `#000000` bottom rule + text (no system mctb).
- modal dBoxProc: 7px border outer-black / groove (lavender `#DADAFF`/`#8787B3`
  -> NOW TEAL) / inner-black / `#FFFFFF` interior; default-button ring `#000000`.
- close/zoom box: dark frame `#545487`, bevel lavender -> teal, interior
  `#C0C0C0` idle.
Win 3.1 (win31-decomp) "Windows Default": BTNFACE `#C0C0C0`, BTNSHADOW
`#808080`, BTNHIGHLIGHT `#FFFFFF` (FLAT, not Win95 double-bevel), ACTIVECAPTION
navy `#000080`, WINDOW `#FFFFFF`, WINDOWFRAME `#000000`. (Index space stops at
20; `#DFDFDF` COLOR_3DLIGHT is a Win95 import -- FORBIDDEN, Law 3.)

## Next session (re30.4, re-oriented)

Implement the canon: re-value `palette.h`/the FLAIR palette module to the table
above; swap chrome lavender -> teal in chrome/control/dialog; re-point the
single palette authority (the re30.4 unification still happens -- kill the 5
drifted switches, dual-compile, centralize the bpp branch) to the CANON not the
preview.webp samples; **re-calibrate the oracles** (`ppm_flair_check`,
test-chrome/control/dialog, and `test-palette-seafoam` -> assert the Initech
teal `#8DDCDC`, not seafoam) to grade against the decomp goldens; amend ADR-0004
OD-4 (seafoam -> Initech teal). Then a fresh 386 screendump for the operator's
Law-4 verdict. The committee re30.4-palette-source ruling is SUPERSEDED by
this operator directive (clut.h stays the device CLUT; the chrome source is the
decomp canon, not palette.h preview-samples).

## Frictions / lesson (saved to bd memory)

I (and a committee) twice anchored on the `preview.webp` mock-up samples as
"ratified canon" and reasoned hard from the wrong premise; the operator
corrected it. LESSON: for any FLAIR color/chrome question, the authority is the
`../system7-decomp` + `../win31-decomp` goldens/specs (rendered-golden pixels =
Law 2 truth), NOT the preview.webp samples (which are a dim CRT-photo mock-up,
kept only for provenance). The Initech teal `#8DDCDC` is the load-bearing
identity color.
