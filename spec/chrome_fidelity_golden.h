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
 * TITLE-BAR BEVEL ROWS + EXACTLY-15-ROW INTERIOR (beads initech-92li).
 *
 * Source: ../system7-decomp/specs/chrome/window-frame.md Sec 2a (the x=400
 *   vertical scan, top frame at y=164) + Sec 2b (the bevel "groove") +
 *   pinstripe.md Geometry ("title interior height 15 px y=166..180; bevel rows 1
 *   px top + 1 px bottom") + refs/StandardWDEF_a.txt L709-744 (the wLTinge0 top/
 *   left highlight _Line and wLTinge4 bottom/right shadow _Line, drawn 1px inside
 *   the title FrameRect after _InsetRect OneOne, only when wHilited).
 *
 * THE MECHANISM (window-frame.md Sec 2a, x=400 vertical scan):
 *     y=164  #000000  top window frame (black)
 *     y=165  #DADAFF  bevel-hi highlight (wLTinge0)      -- NOT a stripe
 *     y=166..180       15 pinstripe rows (FG_TITLE_INTERIOR_PATTERN)
 *     y=181  #B3B3DA  bevel-lo shadow (wLTinge4)         -- NOT a stripe
 *     y=182  #000000  the SHARED frame line (bottom of the title FrameRect AND
 *                     top of the content-body FrameRect)
 *   So the pinstripe run is bounded ABOVE by the bevel-hi row and BELOW by the
 *   bevel-lo row, and the contiguous LIGHT/DARK run is EXACTLY 15.
 *
 * THE TELL THIS CATCHES. FLAIR previously drew 19 ALL-stripe rows with NO bevel
 * rows (the contiguous L/D run was the full band, > 15, with no distinct bevel
 * row-class bounding it).  The measurable, recolor-invariant tells: (a) the
 * contiguous stripe run is EXACTLY 15 rows, (b) the row immediately ABOVE it is
 * the bevel-hi role (idx 2), and (c) the row immediately BELOW it is the bevel-lo
 * role (idx 4) -- a DISTINCT 3rd/4th row-class, neither L (idx 7) nor D (idx 8).
 *
 * RECOLOR-INVARIANCE: graded by INDEX class only.  bevel-hi #DADAFF (wLTinge0) ->
 *   FLAIR_PART_BEVEL_LIGHT canon TEAL -> 8bpp idx 2 (the SAME WL-0053 lavender->
 *   teal recolor the close/zoom box bevel uses, beads initech-ts3t); bevel-lo
 *   #B3B3DA (wLTinge4) -> FLAIR_PART_BEVEL_SHADOW canon teal-dark -> 8bpp idx 4.
 *   The exact #DADAFF/#B3B3DA -> teal canon values are test-color-canon's job,
 *   OUT OF SCOPE here.  These are the same idx 2 / idx 4 classes FG_BOX_BEVEL_IDX
 *   / FG_BOX_DARK_IDX already pin for the box gadget.
 * ========================================================================= */

/* The bevel HIGHLIGHT row (wLTinge0 #DADAFF) classifies as FLAIR_PART_BEVEL_LIGHT
 * -> 8bpp idx 2.  Ref: window-frame.md Sec 2b golden y=165 #DADAFF; the WL-0053
 * lavender->teal canon (== FG_BOX_BEVEL_IDX). */
#define FG_TITLE_BEVEL_HI_IDX   2   /* bevel-hi -> BEVEL_LIGHT canon teal, 8bpp idx 2 */

/* The bevel SHADOW row (wLTinge4 #B3B3DA) classifies as FLAIR_PART_BEVEL_SHADOW
 * -> 8bpp idx 4.  Ref: window-frame.md Sec 2b golden y=181 #B3B3DA; the canon
 * teal-dark recolor (== FG_BOX_DARK_IDX). */
#define FG_TITLE_BEVEL_LO_IDX   4   /* bevel-lo -> BEVEL_SHADOW canon teal-dark, idx 4 */

/* The SHARED frame line below the bevel-lo (golden y=182 #000000) classifies as
 * FLAIR_PART_FRAME -> 8bpp idx 0 (== FG_SHADOW_INK_IDX).  Ref: window-frame.md
 * Sec 2a "the shared line: bottom of the title-bar FrameRect AND top of the
 * content-body FrameRect". */
#define FG_TITLE_SHARED_FRAME_IDX  0   /* shared frame line -> CIDX_BLACK (idx 0) */

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

/* ===========================================================================
 * CLOSE / ZOOM BOX (the title-bar goAway + zoom gadgets).
 *
 * Source: ../system7-decomp/specs/chrome/close-zoom-box.md (pixel-measured from
 *   ../system7-decomp/goldens/captures/s7_doc_window.png close box x=361..371,
 *   y=168..178; s7_scrollbar_active.png zoom box x=393..403, y=31..41) +
 *   refs/StandardWDEF_a.txt (PlotGoAway left+=9 @1675-1678; PlotZoom
 *   left:=right-20 @1682-1693; dest top = struct.top + wBoxDelta + 1 @1705-1707;
 *   box-height derivation (titleHgt-13)/2 @344-346; PlotZoom nested-square glyph
 *   @1695).
 *
 * THE MECHANISM. The active title bar carries two small 3-D beveled square
 * gadgets: the CLOSE (goAway) box on the LEFT, the ZOOM box on the RIGHT.  Each
 * RENDERS 11x11 (the WDEF derives a 13 px box but the CopyBits dest is inset and
 * the lavender bevel sits inside the dark frame -- LAW 2, the golden wins).  The
 * close box left edge is struct.left + 9; the zoom box left edge is
 * struct.right - 20.  The box top is struct.top + wBoxDelta + 1 where
 * wBoxDelta = (titleHgt-13)/2 (= 3 for the 19 px bar), i.e. box top = frame_top + 4.
 *
 * Each gadget is a DOUBLE-BEVELED square (close-zoom-box.md ASCII diagram):
 *   dark OUTER top/left frame, an inner lavender bevel HIGHLIGHT just inside
 *   top/left, an inner-right + inner-bottom DARK ring, a lavender bottom/right,
 *   and a 7x7 GRAY recessed face.  At least THREE distinct tonal roles
 *   (dark outline / light bevel / gray face) in that arrangement.  The ZOOM box
 *   additionally carries the inner nested-square "little dude" glyph (dark figure
 *   inside the face); the CLOSE box interior is plain gray (no glyph).
 *
 * THE TELL THIS CATCHES. FLAIR previously drew both boxes as a FLAT 1px frame,
 * 13x13, inset fr+3 (=4) from each corner -- wrong size, wrong offsets, NO bevel
 * (one tonal role, not three), and the zoom box identical to the close box (no
 * glyph).  The measurable, recolor-invariant tells are: size (11 not 13), the +9
 * / -20 offsets, the >=3 tonal roles per box, and the zoom box's extra interior
 * dark structure vs the close box.
 *
 * RECOLOR-INVARIANCE: graded by INDEX class + relations only.  The dark outline
 * is the dark bevel-SHADOW role (FLAIR_PART_BEVEL_SHADOW -> 8bpp idx 4); the bevel
 * highlight is the canon TEAL bevel-LIGHT role (FLAIR_PART_BEVEL_LIGHT -> 8bpp
 * idx 2 = CIDX_DESKTOP -- the WL-0053 lavender->teal recolor of the wLTinge0
 * #DADAFF bevel); the face is the GRAY control role (idx 6, #C0C0C0).  No RGB
 * literals; the exact #545487/#DADAFF/#C0C0C0 -> teal canon values are
 * test-color-canon's job, OUT OF SCOPE here.
 * ========================================================================= */

/* The rendered gadget size: 11x11 (whole gadget incl bevel).  NOT the WDEF 13 px
 * derivation.  Ref: close-zoom-box.md Geometry (golden 11x11). */
#define FG_BOX_RENDER_SIZE   11

/* Horizontal offsets from the window struct frame.  Close box LEFT edge =
 * struct.left + 9 (PlotGoAway 'moveq #9,D1' WDEF @1675-1678); zoom box LEFT edge =
 * struct.right - 20 (PlotZoom 'left:=right; moveq #-20,D1' WDEF @1682-1693).
 * Ref: close-zoom-box.md FLAIR mapping (close X = struct.left+9; zoom X = struct.right-20). */
#define FG_CLOSE_BOX_LEFT_OFF    9    /* struct.left + 9  -> close box left edge */
#define FG_ZOOM_BOX_RIGHT_OFF   20    /* struct.right - 20 -> zoom box left edge */

/* The double-bevel exhibits >=3 distinct tonal roles per box (dark outline vs
 * light bevel vs gray face).  A flat 1px frame has ONE.  Ref: close-zoom-box.md
 * ASCII bevel diagram (D dark / ^ lavender / g gray). */
#define FG_BOX_MIN_TONAL_ROLES   3

/* Recolor-invariant tonal-role index classes for the box gadget (System-7 shade
 * roles, mapped to the FLAIR canon INDEX the recolor-invariant role resolves to in
 * 8bpp; NOT teal/lavender RGBs).  Ref: close-zoom-box.md Rendered colors table +
 * the WL-0053 lavender->teal canon (color_canon.h):
 *   dark outline #545487 -> BEVEL_SHADOW canon teal-dark #4E9BA3 -> 8bpp idx 4;
 *   bevel highlight #DADAFF -> BEVEL_LIGHT canon teal #8DDCDC -> 8bpp idx 2;
 *   gray face #C0C0C0 -> the GRAY control role -> idx 6. */
#define FG_BOX_DARK_IDX     4    /* dark outline -> BEVEL_SHADOW teal-dark, 8bpp idx 4 */
#define FG_BOX_BEVEL_IDX    2    /* bevel highlight -> BEVEL_LIGHT canon teal, 8bpp idx 2 */
#define FG_BOX_FACE_IDX     6    /* 7x7 recessed gray face (#C0C0C0 control role)  */

/* The ZOOM box carries an inner nested-square glyph (extra interior dark figure)
 * that the CLOSE box lacks: the zoom box has STRICTLY MORE interior dark pixels in
 * the face region than the close box.  Ref: close-zoom-box.md (PlotZoom nested-
 * square glyph @1695; "the zoom box would carry the nested-square 'little dude'
 * glyph ... the close box interior carries no glyph"). */
#define FG_ZOOM_HAS_NESTED_GLYPH   1

/* ===========================================================================
 * VERTICAL SCROLLBAR (inactive, no thumb).
 *
 * Source: ../system7-decomp/specs/chrome/scrollbar.md (pixel-measured from
 *   ../system7-decomp/goldens/captures/s7_about.png, right gutter of the
 *   "About This Macintosh" window, x=494..509, y=159..217) +
 *   refs/StandardWDEF_a.txt (scrollBarSize EQU 16 @73; WDEF gutter-divider
 *   lines @1330-1338 + @1310-1318).
 *
 * GEOMETRY (scrollbar.md Geometry table).
 *   Total band width: 16 px (scrollBarSize; x=494..509 in the golden).
 *   Structure left-to-right: 1 px black gutter-DIVIDER (WDEF draws this
 *   vertical line, @1332), 14 px interior (drawable track + arrows), 1 px
 *   black window-FRAME line (shared with window-frame.md).
 *   Up-arrow box: 16 px square (top edge BLACK, box face y=160..173, lower
 *   separator GRAY #969696 at y=174).
 *   Down-arrow box: symmetric at the bottom (upper separator GRAY #969696 at
 *   y=202, box face y=203..216, bottom edge BLACK at y=217).
 *   Page track: the stretch between the two separator lines (y=175..201 in
 *   the golden; varies with window height).
 *
 * RENDERED COLORS (scrollbar.md "Rendered colors" table + Law-2 note).
 *   All fills are SOLID (NO dither, NO #969696 sub-pixels in the track --
 *   verified x=495..508, y=176..200 uniformly #F3F3F3 in the golden).
 *
 *   element                   | hex     | idx (8bpp) | FLAIR_PART
 *   --------------------------+---------+------------+---------------------
 *   outer box edges + divider | #000000 | 0          | FLAIR_PART_FRAME
 *   inner box/track separators| #969696 | 8          | FLAIR_PART_PIN_DARK
 *   arrow-box face + page track| #F3F3F3| 7          | FLAIR_PART_PIN_LIGHT
 *   arrow-glyph outline       | #969696 | 8          | FLAIR_PART_PIN_DARK
 *
 * NOTE: the #F3F3F3/#969696 pair is "the same light/dark RGBs seen in the
 * title-bar pinstripe" (scrollbar.md Rendered colors).  They are the
 * PINSTRIPE shades (idx 7/8), NOT the lavender bevel roles (idx 2/4).
 *
 * ARROW GLYPH (scrollbar.md arrow-glyph shape section, inactive rendering).
 *   Each arrow box (14 px wide interior) carries a hollow-outlined triangle-
 *   on-stem glyph in #969696 (FLAIR_PART_PIN_DARK, idx 8) on a #F3F3F3
 *   face.  The up-arrow apex is ~3 px from the box top; the down-arrow is
 *   the vertical mirror.  The inactive bar dimmed arrows are gray outlines
 *   (NOT solid black -- that is the ACTIVE/enabled state, golden-resolves).
 *   Minimum PIN_DARK pixels per arrow box (interior, excluding outer edge):
 *   the glyph outline spans roughly 10 rows x ~4 columns = ~14 pixels;
 *   we assert >= 8 to be tolerant of exact anchor pixel shifts.
 *
 * INACTIVE BAR: no thumb (the "About This Macintosh" window does not
 * overflow its content; the active-bar thumb + dithered track are a
 * separate golden (s7_scrollbar_active.png) and are deferred to the Control
 * Manager CDEF implementation (not the WDEF chrome; golden-resolves for the
 * active state).
 *
 * SEPARATOR TOPOLOGY (the key fidelity bug).
 *   The two rows between the arrow boxes and the track (y=174 and y=202 in
 *   the golden) are SOLID #969696 (PIN_DARK, idx 8) -- NOT black (FRAME,
 *   idx 0).  The outer top (y=159) and bottom (y=217) box-edge rows ARE
 *   solid black (FRAME, idx 0).  FLAIR previously drew all four edges of
 *   each arrow box with cframe(..., FLAIR_PART_FRAME) -- making both the
 *   outer edges AND the inner separator the same black, which is WRONG.
 *
 * RECOLOR-INVARIANCE.
 *   All graded by INDEX CLASS (idx 0/7/8), not by RGB.  The gray/near-white
 *   pair is invariant under the Initech teal recolor (which only touches the
 *   lavender bevel roles, not the pinstripe roles; test-color-canon's job).
 * ========================================================================= */

/* Outer edge ink (top + bottom outer rows of each arrow box = window-frame
 * black = CIDX_BLACK).  Ref: scrollbar.md Geometry + Rendered colors
 * (x=494,x=509 col y=159..217; rows y=159 & y=217 = #000000). */
#define FG_SB_OUTER_EDGE_IDX    0   /* FLAIR_PART_FRAME -> CIDX_BLACK (idx 0) */

/* Inner separator ink (the two rows between an arrow box and the page track).
 * Ref: scrollbar.md Rendered colors (rows y=174 & y=202 = solid #969696 =
 * NOT black; "inactive-dimmed separator lines"). */
#define FG_SB_SEPARATOR_IDX     8   /* FLAIR_PART_PIN_DARK -> CIDX_PIN_DARK (idx 8) */

/* Arrow-box face + page-track fill (SOLID, no dither).
 * Ref: scrollbar.md Rendered colors + Law-2 note ("the empty page track and
 * arrow-box faces are SOLID #F3F3F3; no dither"). */
#define FG_SB_FACE_IDX          7   /* FLAIR_PART_PIN_LIGHT -> CIDX_PIN_LIGHT (idx 7) */

/* Arrow glyph ink (hollow triangle-on-stem outline, INACTIVE/dimmed = gray).
 * Same shade as the inner separator.
 * Ref: scrollbar.md "arrow glyph (up/down triangle) #969696 (150) outline". */
#define FG_SB_GLYPH_IDX         8   /* FLAIR_PART_PIN_DARK -> CIDX_PIN_DARK (idx 8) */

/* Minimum PIN_DARK glyph pixels per arrow box (the inactive outlined triangle
 * spans ~10 rows x ~4 cols; we require >=8 to tolerate pixel-exact anchor
 * shifts while still catching a box with NO glyph at all).
 * Ref: scrollbar.md arrow-glyph shape ASCII diagram. */
#define FG_SB_GLYPH_MIN_PX      8

/* Inactive bar carries NO thumb.
 * Ref: scrollbar.md Geometry ("INACTIVE (no-thumb)"); active thumb is
 * golden-resolves (s7_scrollbar_active.png). */
#define FG_SB_INACTIVE_NO_THUMB 1

#endif /* INITECH_SPEC_CHROME_FIDELITY_GOLDEN_H */
