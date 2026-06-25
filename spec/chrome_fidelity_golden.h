/*
 * spec/chrome_fidelity_golden.h -- INDEPENDENT System-7 window-chrome golden.
 *
 * beads: initech-hmll (FLAIR window-chrome FIDELITY oracle). Ref: CLAUDE.md Law 2
 *        ("the oracle is the truth"; "an oracle that computes its expected values
 *        from the SAME source the artifact renders from is NOT an oracle" -- the
 *        HER-02 by-construction heresy), Law 4 ("look like the frame"), Law 1
 *        (ground truth: every datum cites a local source), Rule 8 (locked
 *        spec-data), Rule 11 (deterministic), Rule 12 (ASCII).
 *
 * WHY THIS EXISTS / WHAT IT IS NOT.
 *   The existing chrome oracles grade FLAIR against itself: test-chrome asserts
 *   the rendered pixels match the #defines in spec/chrome_metrics.h -- the SAME
 *   header chrome.c renders FROM (a STRUCTURAL compare, not SSIM; ADR-0004 D-8) --
 *   and ppm_flair_check asserts a period-2 ALTERNATION whose probe geometry mirrors
 *   test_shell.c's own scene. Neither grades FLAIR's window APPEARANCE against the
 *   REAL System 7. So a window that looks nothing like System 7 but uses the right
 *   palette indices and metric numbers passes every gate.
 *
 *   This header is the missing INDEPENDENT golden: it encodes the System-7 chrome
 *   structure as PIXEL-MEASURED from real System 7 screendumps in the sister
 *   ground-truth repo ../system7-decomp (the goldens/captures PNG screendumps),
 *   distilled in ../system7-decomp/specs/chrome (the .md specs). NOT derived from
 *   chrome_metrics.h /
 *   chrome_metrics.json -- that is the whole point (Law 2). chrome.c renders from
 *   chrome_metrics.h; the fidelity oracle grades the render against THIS, a source
 *   distinct from the render.
 *
 * RECOLOR-INVARIANCE (mechanism vs policy; ADR-0004-AMENDMENT-DEC-09 C-8).
 *   FLAIR deliberately recolors the desktop and the System-7 lavender bevels to the
 *   Initech teal canon -- that COLOR POLICY is graded by test-color-canon and is
 *   OUT OF SCOPE here. This golden encodes only MECHANISM: geometry, pattern,
 *   phase, and shade-RELATIONS (light-vs-dark-vs-frame), every one of which survives
 *   any monochrome-preserving recolor. So the data below is keyed to System-7 shade
 *   INDICES (the wctb id=0 anchors) and structural row classes, never to teal/
 *   lavender RGBs.
 *
 * STATUS: this is the FIRST increment (the title-bar pinstripe PHASE element).
 *   Further elements (close/zoom box geometry, scrollbar arrow/separator structure,
 *   drop shadow, grow box, title text knockout) accrete here as their oracle legs
 *   land -- each cited to its ../system7-decomp/specs/chrome .md source.
 */
#ifndef INITECH_SPEC_CHROME_FIDELITY_GOLDEN_H
#define INITECH_SPEC_CHROME_FIDELITY_GOLDEN_H

/* ===========================================================================
 * TITLE-BAR PINSTRIPE PHASE (the "racing stripe").
 *
 * Source: ../system7-decomp/specs/chrome/pinstripe.md (pixel-measured from
 *         ../system7-decomp/goldens/captures/s7_doc_window.png, active doc window,
 *         column x=450, rows y=164..182) + refs/StandardWDEF_a.txt
 *         (HilitePattern dc.w $FF00 x4 @2206; patAlign origin-mod-8 phase lock
 *         @891-894; wTitleBarLight=7 / wTitleBarDark=8 @77-78).
 *
 * THE MECHANISM. The active title interior is _FillRect'd with the 1-bpp pattern
 * HilitePattern = $FF00,$FF00,$FF00,$FF00 -- 8 rows of {all-ones, all-zeros}, i.e.
 * a PERIOD-2 HORIZONTAL stripe. Crucially the WDEF sets patAlign to the window's
 * STRUCTURE ORIGIN mod 8 BEFORE filling, phase-locking the stripe to the window so
 * it does not crawl as the window moves. That phase lock lands a LIGHT row at BOTH
 * interior boundaries, producing DOUBLED-LIGHT row pairs at the top and bottom of
 * the stripe band -- the measured pattern, top-to-bottom over the 15-row interior:
 *
 *     L L D L D L D L D L D L D L L      (9 light, 6 dark)
 *
 * (pinstripe.md Geometry table + the y=165..181 scan: y=166/167 both #F3F3F3,
 *  alternating to y=179/180 both #F3F3F3; bevel rows #DADAFF @165 / #B3B3DA @181
 *  bound the interior and are NOT stripes.)
 *
 * THE TELL THIS CATCHES. A naive renderer free-runs the period-2 fill from the
 * title-bar TOP -- L,D,L,D,... -- which is ALSO "period-2 alternation" and so PASSES
 * the existing test-chrome period_ok + ppm_flair_check leg (c). But it has NO two
 * adjacent equal rows, so it lacks the phase-locked doubled-LIGHT pairs. That is the
 * measurable, recolor-invariant phase bug this golden pins.
 * ========================================================================= */

/* System-7 wctb id=0 shade-index anchors for the stripe (decomp-sourced; NOT from
 * chrome_metrics.h). Used to CLASSIFY a rendered row as light/dark; the EXPECTED
 * pattern (below) is the independent golden. Ref: pinstripe.md FLAIR mapping +
 * StandardWDEF_a.txt @77-78; re/mint-results-006.md (wctb id=0). */
#define FG_STRIPE_LIGHT_IDX   7   /* wTitleBarLight -> rendered #F3F3F3 (idx 7) */
#define FG_STRIPE_DARK_IDX    8   /* wTitleBarDark  -> rendered #969696 (idx 8) */

/* The measured 15-row title-INTERIOR stripe pattern, top-to-bottom. 'L'=light row
 * (idx 7), 'D'=dark row (idx 8). The doubled-light pairs at index [0,1] and
 * [13,14] are the patAlign mod-8 phase-lock signature. */
#define FG_TITLE_INTERIOR_PATTERN  "LLDLDLDLDLDLDLL"
#define FG_TITLE_INTERIOR_ROWS     15   /* strlen(FG_TITLE_INTERIOR_PATTERN)   */
#define FG_TITLE_INTERIOR_LIGHT_N   9   /* count('L')                          */
#define FG_TITLE_INTERIOR_DARK_N    6   /* count('D')                          */

/* The phase-lock SIGNATURE the oracle asserts (recolor-invariant, and absent from
 * any free-running alternation):
 *   - the stripe run BEGINS with a doubled-LIGHT pair (interior rows 0,1 = L,L),
 *   - the stripe run ENDS   with a doubled-LIGHT pair (interior rows 13,14 = L,L),
 *   - therefore >=1 pair of adjacent equal rows exists (a free-running L,D,L,D...
 *     has zero). */
#define FG_PHASE_DOUBLED_LIGHT_AT_EDGES  1

/* ===========================================================================
 * WINDOW DROP SHADOW + BODY SINGLE-LINE FRAME.
 *
 * Source: ../system7-decomp/specs/chrome/window-frame.md Sec 1 (native-px
 *   geometry), Sec 2a (horizontal scan y=300 proves single-line body frame),
 *   Sec 4 (shadow clipped in s7_doc_window.png; golden-resolves: s7_get_info.png
 *   for the on-screen right edge) + refs/StandardWDEF_a.txt L515 (drop-shadow
 *   factor D4 = OneOne = (1,1) for documentProc varCode 0) + L578-594 (the
 *   L-shape paint: MoveTo(right,top+shadow); LineTo(right,bottom);
 *   LineTo(left+shadow,bottom) in wFrameColor = black).
 *
 * THE MECHANISM (WDEF; StandardWDEF_a.txt).
 *   L515: `move.l OneOne,D4` -- the shadow factor for documentProc (varCode 0)
 *   is (1,1): offset 1 px right, 1 px down.  L578-594: the shadow L is painted
 *   with _PenSize = D4 (1x1) and wFrameColor (black, idx 0):
 *     down the RIGHT column just OUTSIDE the window frame (x = right+0, i.e.
 *     right through right+shadow-1 = right+0 -- one column, at x = right, from
 *     y = top+shadow = top+1 to y = bottom inclusive), and
 *     across the BOTTOM row just OUTSIDE the window frame (y = bottom, from
 *     x = left+shadow = left+1 to x = right+shadow-1 = right+0).
 *   In half-open [left,right) x [top,bottom) coordinate space (the window
 *   occupies [left,right) x [top,bottom)):
 *     shadow column: x = right,   y in [top+1, bottom+1)   (down right edge)
 *     shadow row:    y = bottom,   x in [left+1, right+1)   (across bottom)
 *   The top-right corner (right, top) and bottom-left corner (left, bottom) are
 *   NOT shadowed (the L misses them by the +1 offset).
 *
 * BODY EDGE IS SINGLE-LINE (window-frame.md Sec 2a).
 *   Horizontal scan at y=300 (a content row, below the title bar):
 *     x=352 = black (#000000) -- the 1px outer frame line.
 *     x=353 = white (#FFFFFF) -- CONTENT begins IMMEDIATELY; NO second ink line.
 *   This matches the WDEF: the body is ONE _FrameRect; the lavender bevel _Lines
 *   (wLTinge0/wLTinge4) are drawn only on the TITLE-BAR interior.  Any inner
 *   groove running down the body left/right/bottom edges is a FIDELITY BUG.
 *
 * RECOLOR-INVARIANCE: shade RELATIONS only -- shadow ink = FRAME (black, idx 0);
 *   body inner pixel = CONTENT (white, idx 1).  No RGB literals.
 * ========================================================================= */

/* Shadow ink index (wFrameColor = black = CIDX_BLACK).
 * Ref: window-frame.md Sec 1 / Sec 3 (wFrameColor -- frame/content fore); NOT
 * from chrome_metrics.h. */
#define FG_SHADOW_INK_IDX    0   /* wFrameColor -> CIDX_BLACK (idx 0) */

/* Shadow offset: (1,1) -- 1 px to the right, 1 px down.
 * Ref: refs/StandardWDEF_a.txt L515 `move.l OneOne,D4` for varCode 0 documentProc. */
#define FG_SHADOW_OFFSET     1   /* 1 px at (1,1): documentProc only (varCode 0) */

/* Body-no-groove fact: on a content row (below the title bar), the pixel one
 * column INSIDE the outer left frame is CONTENT (white, idx 1) -- NOT a second
 * inked groove line.  Ref: window-frame.md Sec 2a horizontal scan y=300. */
#define FG_BODY_NO_GROOVE    1   /* 1 = inner pixel is CONTENT, not a groove */

/* Content body pixel index (the one column inside the outer frame on a content
 * row must be this).  Ref: window-frame.md Sec 2a x=353 = white = CIDX_WHITE. */
#define FG_BODY_INNER_IDX    1   /* CIDX_WHITE (idx 1) */

/* ===========================================================================
 * TITLE TEXT + KNOCKOUT (the centered window name).
 *
 * Source: ../system7-decomp/specs/chrome/title-bar.md Sec 3 + pinstripe.md.
 *   The window title is drawn CENTERED in the bar in Chicago, black (wTextColor
 *   #000000), with the racing stripe SUPPRESSED under it -- a centered LIGHT
 *   (#F3F3F3) knockout gap with black glyphs (golden s7_doc_window.png: the
 *   "System7_5_3" glyph run on a #F3F3F3 gap; pinstripe.md y=168 horizontal scan:
 *   the dark row is broken by the centered title gap #F3F3F3 at x=519..618). The
 *   centering indent clamps right of the go-away box (title-bar.md x=left+32).
 *
 * Two recolor-invariant structural facts (graded by INDEX class, not RGB):
 *   (a) FIGURE ink (wTextColor; FLAIR CIDX_TITLE_INK = idx 4) appears in the
 *       centered title region -- the glyphs are drawn (not a blank bar).
 *   (b) under the centered title the DARK stripe (idx 8) is SUPPRESSED to the
 *       light knockout panel -- ZERO dark pixels in the centered title cell rows.
 * ========================================================================= */
#define FG_TITLE_INK_IDX        4   /* wTextColor -> CIDX_TITLE_INK (idx 4, black) */
#define FG_TITLE_KNOCKOUT_IDX   7   /* the suppressed-stripe gap -> light (idx 7)  */

#endif /* INITECH_SPEC_CHROME_FIDELITY_GOLDEN_H */
