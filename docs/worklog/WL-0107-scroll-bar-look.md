# WL-0107 -- Scroll-bar appearance and resize consistency

## Context

Coding lane `lane-scroll`, based on `7bec622` from `BASE_COMMIT.txt`, for
initech-tdnl.96 (M007), .97 (M008), .101 (M013), .82 (J01), and .86 (J05).
The supplied bead texts replace tracker access: `bd` is unavailable. The lane
writes only this sibling clone, commits through `build/lane.git`, and does
not push. The other lanes' text/menu and DOS console sources are untouched.

Read CLAUDE.md, PRD 6.2-6.4, LOOK pass 6 REPORT/TRIAGE, pass 4 J01/J05,
WL-0093, the locked chrome data, both actual drawers, window/Finder scroll
paths, the existing host/emulator gates and recording machinery. References
are the sibling system7-decomp corpus, especially sys8/scrollbars.md and
s8_doc_window_inactive.png.

## What changed

- Independent literal crops cover both thumbs and all four enabled arrow
  tiles. The host gate decodes the original PNG with libpng and verifies the
  transcription before comparing the real chrome and Control Manager drawers
  at 8 and 32 bits. Both drawers are retained and graded by the same oracle.
- The sampled thumb classes H/P/S/R map to white / existing teal / existing
  dark / existing dark. Indexed output uses #FFFFFF / #8DDCDC / #3F3F3F;
  direct colour uses #FFFFFF / #8DDCDC / #4E9BA3. The original classes are
  #DADAFF / #B3B3FF / #8787DA / #5454B3, with 49/102/26/28 pixels and five
  #F3F3F3 caps. The crop fixes four seven-pixel dark grips, their six-pixel
  highlight companions, and the face-coloured off-diagonal tile corners.
- Scroll models cache their document extents. Grow, zoom, restore and unshade
  re-evaluate the range before chrome painting, using the same state as input.
  Finder re-list explicitly damages both bars even if the value stays zero.
  Disabled horizontal fill now erases all 14 rows, including the old well's
  final row (an additional defect caught by the new emulator oracle).
- Grow clamps to 96x88. The measured bar requires
  1+14+1+15+1+14+1=47 pixels; FLAIR's vertical reservation is 41 pixels and
  horizontal reservation is 25 pixels. The geometric floor is 72x88; the
  existing 96px width policy is retained. Both minimum-size bars are pixel
  graded with enabled thumbs and arrows; the vertical thumb has zero travel.
  Control Manager drawing also accepts the same 47px minimum bar.
- `test-flair-scroll-look` drives four prefixes of one locked trace: shrink
  360x220 to 128x120 (enabled horizontal bar), scroll by16, grow back (both
  disabled), request60x40 (clamped96x88), and click the visible minimum down
  arrow (value10). Full disabled bands and independent enabled crops are
  graded, with no FLAIR-SCROLL-IGNORED in the trace.
- New host/emulator gates and mutants are wired into TEST_UNIT_GATES and
  TEST_EMU_GATES. `scroll_look` is wired into RECORD_SCRIPTS using the existing
  long double-click recording image. Both mutant targets require a passing
  clean baseline first.

## Why

The thumb geometry already existed, but indexed colour collapsed every role
onto the single teal index. This was a policy-mapping defect rather than
missing geometry. The locked indexed palette contains one teal, white,
navy, black and grays; it cannot supply three solid teal RGB shades. The
palette-preserving three-tone substitution is explicit and authored, not
claimed to be a captured Mac teal theme. Colour authority is
spec/assets/color_canon.json entries/derived_rows and DEC-10 OQ-3; structural
truth is sys8/scrollbars.md 2.2/2.4 and the historical PNG. No palette changed.

Finder content paints after chrome. Re-evaluating the range during that paint
made the already-drawn bar stale. Updating the cached range in the geometry
transaction fixes the ordering, while re-list damage fixes content changes.

The requested corpus contains no measured Finder grow minimum. The period
rule is documented in refs/MacintoshToolboxEssentials.pdf printed p.4-57:
applications supply grow limits sufficient for structure and scroll bars.
Printed p.4-58 requires erasing/redrawing bars after resize. Its sample uses
64px PORT limits, not a measured Finder frame size. StandardWDEF_a.txt
1291-1343 draws the supplied outline and imposes no minimum. Thus 96x88 is
honestly derived from measured parts, not presented as a historical capture.

## Frictions

- The hard palette restriction prevents a solid three-teal-shade live ramp;
  the indexed fallback uses dark gray. The direct-colour path retains the
  existing dark teal. A future exact multi-teal live ramp needs a separately
  authorized palette or framebuffer change.
- No Mac OS 8 pressed-state golden exists (scrollbars.md 6). Existing authored
  pressed-arrow behaviour is retained; disabled/hollow states remain covered
  by the existing fidelity and control gates.
- The first emulator run exposed the unpainted disabled bottom row, reproduced
  in a second isolated run, then fixed without relaxing the oracle.
- One invocation accidentally started two new emulator targets concurrently.
  It was interrupted and its evidence discarded. All final emulator evidence
  below comes from isolated sequential gates with INITECH_QMP_STDIO=1.
- `record-flair`, Bochs and bead status updates were not run, as instructed.
  Rule-14 clips therefore remain for PM acceptance; these beads are not claimed
  closed. No full test/test-unit/test-emu vector was run.

## Acceptance

Every make invocation used the absolute SYSTEM7_DECOMP, WIN31_DECOMP,
DBASE3_DECOMP, LOTUS123_DECOMP and WORDPERFECT51_DECOMP roots under the sibling
Projects directory; all five were present. No SKIP option was used. Host
builds used at most -j2; emulator gates used -j1 and ran one at a time.

RED first: build/scrollbar-look-red.log reports missing thumb classes and
2/196 arrow differences; build/winscroll-red.log reports W7 stale resize and
W8 insufficient minimum. Final evidence:

| Target | Final evidence |
| --- | --- |
| test-scrollbar-look | `test-scrollbar-look: 10 checks, 0 failures`; `>>> test-scrollbar-look: green (both depths, both drawers; six independent crops)` |
| test-scrollbar-look-mutant | `>>> test-scrollbar-look-mutant: SB_LOOK_MUT_RAMP_FLAT correctly RED` |
| test-scrollbar-look-mutant | `>>> test-scrollbar-look-mutant: SB_LOOK_MUT_GRIPS_MISSING correctly RED` |
| test-scrollbar-look-mutant | `>>> test-scrollbar-look-mutant: SB_LOOK_MUT_CORNERS_BEVELLED correctly RED` |
| test-winscroll | `test-winscroll: 34 checks, 0 failures`; `>>> test-winscroll: green` |
| test-winscroll-mutant | `WINDOW_MUT_SCROLL_STALE_RESIZE correctly RED` on `W7 resize re-evaluates enabled ranges before any paint` |
| test-winscroll-mutant | `WINDOW_MUTATE_GROW_NO_MIN correctly RED` on `W8 minimum grow is 96x88: measured bar anatomy fits` |
| test-winscroll-mutant | `>>> test-winscroll-mutant: green (five mutants RED for the named reason)` including the three existing mutants |
| test-flair-scroll-look | `>>> test-flair-scroll-look: green (shrink enabled; click scrolls16; restore disabled; minimum96x88 draws all parts; no SCROLL-IGNORED)` |
| test-flair-scroll-look-mutant | `>>> test-flair-scroll-look-mutant: WINDOW_MUT_SCROLL_STALE_RESIZE correctly RED (enabled thumb missing after shrink)` |
| test-flair-scroll-look-mutant | `>>> test-flair-scroll-look-mutant: SB_LOOK_MUT_RAMP_FLAT correctly RED (independent thumb crop)` |
| test-flair-scroll-look-mutant | `>>> test-flair-scroll-look-mutant: WINDOW_MUTATE_GROW_NO_MIN correctly RED (undrawn bar and wrong frame)` |
| test-chrome / test-chrome-mutant | `test-chrome: 55 checks, 0 failures`; all four named mutants correctly RED |
| test-chrome-fidelity / test-chrome-fidelity-mutant | `test-chrome-fidelity: 28 checks, 0 failures`; all existing named mutants correctly RED, including CHROME_FID_MUT_SCROLL_FLAT |
| test-control | `test-control: 163 checks, 0 failures`; `>>> test-control: green` |
| test-flair-scroll | `>>> test-flair-scroll: green` (arrow, page, thumb, hold, scrolled selection, fitting bars; no separate arrow/page/thumb make targets exist) |
| test-flair-scroll-mutant | `>>> test-flair-scroll-mutant: green (both mutants RED for the named reason)` |
| test-flair-window-ops | `>>> test-flair-window-ops: all three R1.2 emulator legs green` |
| test-flair-desktop | `VERDICT : PASS -- the LIVE FLAIR chimera desktop is the Office Space frame` |
| test-flair-disk-windows | `VERDICT : PASS -- the booted Finder opens the REAL volume in a REAL window` |
| test-flair-solid | `VERDICT : PASS -- the live desktop honours the ONE repaint contract` |
| test-spec | `VERDICT : PASS -- ADR-0003 spec-data parses, sizes hold, banner verbatim, INT 25h/26h locked` |
| test-makefile-vars | `>>> test-makefile-vars: green` |
| test-kernel-repro | `>>> test-kernel-repro: green -- kernel.bin reproducible (sha256 acaca40fdaed0cada0b80212de5fe1d1f9804617ff9b956362d619b04c31daea)` |
| test-kernel-headroom | `>>> test-kernel-headroom: 149 kernels measured; window 196608 B (384 sectors); floor 4096 B`; `>>> test-kernel-headroom: green` |

Base headroom was independently measured from an archive of7bec622 under
build/scroll-base (same targeted headroom gate,146 kernels). Flagship:
173576B loaded /23032B headroom ->174152B /22456B. Largest variant in both
runs: kernel_flairtenants_mut_no_raise_on_title.elf,
174696B /21912B ->175272B /21336B binding. Growth is576B, below3072B.

The dated2026-10-15 chrome_fidelity_golden.h extension adds independently
sourced thumb highlight/dark classes and population documentation; existing
face index2 is unchanged. The old grow endpoint assertions and record marker
are deliberately re-keyed96x64->96x88 in test_window.c,
ppm_flair_window_ops_check.c, flair_solid_traces.mk and Makefile. The locked
chrome_metrics.json/.h policy floor and every palette file are unchanged.
New independent locked data: scrollbar_look_golden.h and
flair_scroll_look_traces.mk. ASCII scan and git diff --check pass.

## Pointers

- Host oracle: harness/proptest/test_scrollbar_look.c;
  spec/scrollbar_look_golden.h; test-winscroll W7/W8.
- Emulator: spec/flair_scroll_look_traces.mk;
  tools/ppm_flair_scroll_look_check.c; Makefile's new gate block.
- Evidence: build/scrollbar-look-final.log, build/scroll-host-final.log,
  build/scroll-look-acceptance.log, build/scroll-regression.log,
  build/test-flair-*.scroll.log, build/scroll-base-headroom.log,
  build/scroll-kernel-headroom-final.log, build/scroll-kernel-repro-final.log.
  Native dumps: build/scroll_look_{shrink,click,restore,min}.ppm.
- Handoff: build/lane.bundle, requiring base7bec622, branch lane-scroll.
  The PM should record scroll_look plus the existing scroll_arrow/page/thumb
  clips on the integrated image before Rule-14 acceptance and bead closure.
  No clips were generated in this lane. The original .git is untouched.
