/*
 * spec/ssim_params.h -- FLAIR SSIM guide parameters: the LOCKED contract.
 *
 * LOCKED spec-data (CLAUDE.md Rule 8; PRD Sec 6.3 -- "the Toolbox layer").
 * beads: initech-k8o5.4 (FLAIR event + window model as spec-data).
 *
 * ==========================================================================
 * WARNING: SSIM IS A GUIDE, NEVER A GATE.
 *
 * ADR-0004 Decision D-8 (Oracle Vector, RATIFIED 2026-06-19):
 *   "ssim | GUIDE ONLY -- never gates | Per-window structural similarity vs
 *    the frame fixture; reported to point agents toward fidelity (Law 4,
 *    PRD Sec 3, Sec 8). Structurally a guide; it is NOT summed into a reward
 *    and never blocks a merge."
 *
 * ADR-0004 Amendment AM-6 (Ratification Record Sec 8.2):
 *   "The 'each emulator vs its own host-model prediction, NOT a cross-
 *    emulator byte-CRC' definition in D-8/Sec 4.5 is preserved verbatim
 *    and may only be changed by a deliberate ADR amendment."
 *
 * ADR-0004 Rationale 4.4 (why SSIM is a guide, not a gate):
 *   "A numeric SSIM cutoff would either reject authentic frames (the still is
 *    compressed, low-res) or be set so loose it proves nothing. The judge is
 *    a person who used the era's software saying 'yes, that's it' (Law 4,
 *    PRD Sec 3). SSIM is reported per-window to steer agents; the STRUCTURAL
 *    oracles (chrome metrics, canon bytes, event replay) are what actually
 *    gate. Recording this prevents a future agent from 'tightening' SSIM into
 *    a hard cutoff and weakening the real signal."
 *
 * A future agent MUST NOT:
 *   - Use any SSIM value in this header as a hard pass/fail threshold.
 *   - Sum per-window SSIM into a composite reward or merge-gate.
 *   - Tighten the SSIM constants below to block a merge.
 *   - Revert the cross-emulator agreement definition to a naive byte-CRC
 *     (a Stop condition; ADR-0004 AM-6, D-8).
 *
 * Turning SSIM into a hard gate is an explicit STOP CONDITION in CLAUDE.md
 * ("you are about to weaken an oracle" -- a SSIM hard cutoff is a weaker,
 * more brittle oracle than test-chrome/canon; escalate to the user).
 * ==========================================================================
 *
 * PURPOSE:
 * This header defines the parameters used by the per-window SSIM guide
 * (the `make ssim` target in the harness, ADR-0004 D-8). It specifies:
 *   1. The SSIM window size (Gaussian kernel size) and sigma.
 *   2. Named per-window crop rectangles in 640x480 native coordinate space
 *      that carve the relevant windows out of a screendump for comparison
 *      against crops of the fixture (spec/assets/preview.webp).
 *   3. A luminance data-range constant.
 *
 * WHY CROP RECTS INSTEAD OF FULL-FRAME SSIM:
 * Per-window SSIM is the mandate (ADR-0004 D-8: "per-window structural
 * similarity"). Full-frame SSIM conflates unrelated regions; a wrong title
 * bar would be diluted by a correct desktop background. Per-window crops
 * isolate what changed and give actionable signals ("FILE COPY dialog SSIM
 * improved from 0.72 to 0.89 after this commit; the title bar is still 0.61").
 *
 * SOURCE / LAW-1 CITATIONS:
 *
 *   ADR-0004 D-8 (RATIFIED 2026-06-19): "ssim | GUIDE ONLY -- never gates."
 *   ADR-0004 AM-6 (ibid.): cross-emulator definition frozen.
 *   ADR-0004 Rationale 4.4 (ibid.): why SSIM cannot be a gate.
 *
 *   docs/research/gui-ground-truth.md Sec 3.2 (Law-1 callout for the frame):
 *     "the still in spec/assets/preview.webp is a CRT photo ... too low-res/
 *      compressed to recover Chicago/Geneva strikes by clustering."
 *   gui-ground-truth.md Sec 3.4 (resolved contradiction):
 *     "Do NOT back-compute native pixels from the frame photo. Use WDEF/IM
 *      native constants for oracle datums; keep frame px only as annotation."
 *     "The frame photo is not linearly scalable to native px."
 *   gui-ground-truth.md Sec 5.1 (minting note):
 *     "SSIM is a per-window GUIDE, not a gate. The hard gates are test-chrome
 *      (structural, vs chrome_metrics v1 + fixture crops), canon (frozen
 *      bytes), test-event, fb-agree, drag-gate (AM-8)."
 *   gui-ground-truth.md Sec 8 (open risk):
 *     "Frame photo is not linearly scalable to native px."
 *     "Close/zoom/grow box exact rendered geometry is golden-resolves."
 *     "Pinstripe shade RGBs (wctb -4096) unextracted."
 *
 *   CLAUDE.md Law 4: "SSIM is a *guide* the harness reports per-window to
 *     push you toward it, **not a hard numeric gate**."
 *   CLAUDE.md Hallucination-risk callout: "The frame is a Photoshop mock-up,
 *     not a real OS screenshot. Some 'tells' are deliberately inconsistent."
 *
 *   PRD Sec 3 (fidelity bar): "The judge is a person who used early-90s
 *     Mac+DOS software saying 'yes, that's it'."
 *   PRD Sec 8 (oracle infra / gate vector): SSIM is the guide lane only.
 *
 * DUAL-COMPILE: freestanding (gcc -m32 -ffreestanding -nostdlib -std=c11)
 * AND hosted (cc -std=c11). Only <stdint.h> + <stddef.h>. No host malloc;
 * no libc beyond stdint/stddef. Rule 11 (reproducible). ASCII-clean (Rule 12).
 * Changing this file is a deliberate, beads-tracked Rule 8 act.
 */
#ifndef INITECH_SPEC_SSIM_PARAMS_H
#define INITECH_SPEC_SSIM_PARAMS_H

#include <stdint.h>
#include <stddef.h>

/* ===========================================================================
 * 1. SSIM ALGORITHM CONSTANTS
 * ---------------------------------------------------------------------------
 * Ref: Wang et al., "Image Quality Assessment: From Error Visibility to
 *      Structural Similarity," IEEE TIP 2004 (the SSIM paper; external ref).
 *      ImageMagick `compare -metric SSIM` uses an 8x8 or 11x11 sliding window
 *      with a uniform or Gaussian kernel; we use the Gaussian/11x11 variant
 *      to match the scikit-image default (optional; ImageMagick is the primary
 *      implementation per gui-ground-truth.md Sec 5.2).
 *
 * These are INFORMATIONAL for the harness; they are NOT threshold gates.
 * The harness REPORTS the computed SSIM score for agent steering; it does
 * NOT fail on a score below any threshold (ADR-0004 D-8 / AM-6).
 * ===========================================================================*/

/* SSIM window (Gaussian kernel) size in pixels. Standard value: 11.
 * Must be odd (symmetric kernel). Source: Wang et al. 2004 (standard 11x11).
 * Used by: `compare -metric SSIM -define ssim:radius=<N>` or scikit-image
 * `ssim(..., win_size=FLAIR_SSIM_WINDOW_SIZE)`.                              */
#define FLAIR_SSIM_WINDOW_SIZE  11u    /* 11x11 Gaussian window (Wang 2004)  */

/* SSIM Gaussian sigma. Standard value: 1.5 for 11x11 (Wang et al. 2004).
 * Encoded here as 1/10th integer (i.e. value 15 = 1.5) to stay in integer
 * math; the harness divides by 10.0 to recover 1.5 for floating-point calls.
 * Used by: scikit-image `ssim(..., sigma=FLAIR_SSIM_SIGMA_TENTHS / 10.0)`.  */
#define FLAIR_SSIM_SIGMA_TENTHS  15u   /* sigma = 1.5 (15/10); Wang 2004    */

/* Luminance data range for 8-bit-per-channel images. 255 for 8-bpp palette-
 * indexed or 8-bpp R/G/B channel. The SSIM L^2 denominator uses (2^bits - 1).
 * Ref: Wang et al. 2004; scikit-image `ssim(..., data_range=255)`.           */
#define FLAIR_SSIM_DATA_RANGE  255u    /* 8-bpp channel range (0..255)       */

/* ===========================================================================
 * 2. PER-WINDOW CROP RECTANGLES (native 640x480 coordinate space)
 * ---------------------------------------------------------------------------
 * These crop rectangles define the pixel regions in a 640x480 QEMU screendump
 * that will be compared via SSIM against the corresponding crop of the frame
 * fixture (spec/assets/preview.webp, 1456x819 -- a CRT photo, NOT a linear
 * scale of 640x480; see gui-ground-truth.md Sec 3.4 warning).
 *
 * IMPORTANT: The frame fixture is a CRT PHOTO at 1456x819. It is NOT a linear
 * upscale of a 640x480 framebuffer. As gui-ground-truth.md Sec 3.4 explains:
 *   "The implied frame upscale (24/19 ~= 1.26x) does NOT match a naive
 *    640->1456 (2.275x) scale, because the OS window in the frame is not
 *    full-screen -- the photo is not a linear upscale of a 640x480 buffer."
 * Therefore, the crop rectangles below are in TWO coordinate spaces:
 *   - `native`: 640x480 InitechOS framebuffer coords (what the SSIM harness
 *     uses to crop a live screendump for the "actual" side of the comparison).
 *   - `fixture`: 1456x819 preview.webp coords (what the harness uses to crop
 *     the reference fixture for the "expected" side of the comparison).
 * The harness scales neither; it crops from each image in its own coordinate
 * space and passes both crops to the SSIM function (which accepts arbitrary
 * same-size images if the crops happen to be the same pixel count after resize,
 * or simply compares the shapes qualitatively as the guide it is).
 *
 * STATUS CONVENTION:
 *   FLAIR_SSIM_STATUS_GOLDEN_RESOLVED = fixture crop was sourced from a real
 *     screendump or from a precisely measured reference; the rect is definitive.
 *   FLAIR_SSIM_STATUS_TODO_GOLDEN     = the rect is a PLACEHOLDER requiring a
 *     real screendump or emulator boot to confirm. The harness must log a
 *     warning when it encounters a TODO rect; it must NOT assert on the SSIM
 *     score for that window (a placeholder rect produces meaningless SSIM).
 *
 * ALL NATIVE CROP RECTS ARE CURRENTLY STATUS_TODO_GOLDEN.
 *
 * Rationale: The fixture is a CRT photo (gui-ground-truth.md Sec 3.4; see
 * above). The native 640x480 crop rects cannot be derived from the fixture by
 * simple scaling. They must be measured from an actual InitechOS boot screendump
 * (captured via `make run` + QMP screendump + crop measurement). Because FLAIR
 * window rendering has not yet landed (beads initech-k8o5, pre-M3), no
 * screendump yet exists. These are TODO golden-resolves per gui-ground-truth.md
 * Sec 8 ("Close/zoom/grow box exact rendered geometry is golden-resolves").
 *
 * FIXTURE CROP RECTS (preview.webp 1456x819) are similarly TODO because the
 * fixture is a hand-taken CRT photo with perspective distortion, bezels, and
 * CRT curvature (gui-ground-truth.md Sec 3.4). They are included as rough
 * visual-region annotations only; a future agent with the real Init window
 * layout must confirm/replace them.
 *
 * DO NOT FABRICATE PRECISE COORDINATES. The prior session that authored
 * gui-ground-truth.md explicitly called out that exact crop coords are
 * golden-resolves: "Close/zoom/grow box exact rendered geometry is golden-
 * resolves" (Sec 8). A wrong crop rect gives a misleading SSIM score.
 * ===========================================================================*/

/*
 * flair_ssim_crop_t -- a named per-window crop rectangle for SSIM comparison.
 *
 * Two crop rects are carried: one in native 640x480 space (for the live
 * screendump) and one in fixture 1456x819 space (for preview.webp). The
 * harness crops both and computes SSIM between the two crops.
 *
 * Fields:
 *   name            -- short ASCII label for logging (e.g. "DESKTOP").
 *   status          -- FLAIR_SSIM_STATUS_* flag (0=TODO, 1=golden-resolved).
 *   native_x, native_y, native_w, native_h  -- crop in 640x480 screendump px.
 *   fixture_x, fixture_y, fixture_w, fixture_h -- crop in 1456x819 px.
 */
typedef struct flair_ssim_crop {
    const char *name;          /* short label (ASCII; Rule 12)               */
    uint8_t     status;        /* 0 = TODO_GOLDEN; 1 = GOLDEN_RESOLVED       */
    /* native 640x480 crop (source: live QEMU screendump) */
    uint16_t    native_x;
    uint16_t    native_y;
    uint16_t    native_w;
    uint16_t    native_h;
    /* fixture 1456x819 crop (source: spec/assets/preview.webp) */
    uint16_t    fixture_x;
    uint16_t    fixture_y;
    uint16_t    fixture_w;
    uint16_t    fixture_h;
} flair_ssim_crop_t;

/* Status codes for flair_ssim_crop_t.status. */
#define FLAIR_SSIM_STATUS_TODO_GOLDEN     0u  /* placeholder; needs real screendump */
#define FLAIR_SSIM_STATUS_GOLDEN_RESOLVED 1u  /* sourced from real measurement      */

/*
 * FLAIR_SSIM_CROP_TABLE -- the per-window crop table.
 *
 * Each entry is one named window region. The SSIM harness iterates this table,
 * crops both the live screendump and the fixture, computes SSIM, and reports
 * the score alongside the window name and its status. Entries with
 * status=TODO_GOLDEN produce a "TODO: not yet measured" log line and are
 * SKIPPED for scoring (the SSIM of a random crop vs a random crop is
 * meaningless and must not mislead agents).
 *
 * Window regions included:
 *
 *   DESKTOP      -- the full visible desktop background (640x480 minus the
 *                   menu bar), before any windows. Tests the seafoam teal
 *                   background color (ADR-0004 OD-4; SEAFOAM canon R=0x6F
 *                   G=0xA0 B=0x8E; beads initech-ch81).
 *
 *   MENU_BAR     -- the 640x20 menu bar at the top of the screen (20 px tall;
 *                   ADR-0004 D-7; gui-ground-truth.md Sec 3.3 native_value=20
 *                   from GetMBarHeight and WDEF). Tests the Chicago 12 menu
 *                   items against the Photoshop menu-bar canon (FROZEN, ADR-0004
 *                   AM-4, beads initech-zaqj).
 *
 *   FLAIR_WINDOW_GENERIC -- a representative document window (title bar,
 *                   close/zoom boxes, drag region, content). Coordinates depend
 *                   on the specific window shown in the frame (the frame shows
 *                   several overlapping windows including an InitechCalc-style
 *                   spreadsheet and the FILE COPY dialog). Since the exact
 *                   window layout in InitechOS at first-boot is not yet fixed
 *                   (apps are M5+ per PRD; a test window will be open for M3
 *                   drag-gate testing per beads initech-87a), both coords are
 *                   TODO golden-resolves. Update this entry once the M3
 *                   drag-gate screendump is captured (ADR-0004 AM-8).
 *
 *   FILE_COPY_DIALOG -- the "Saving tables to disk..." modal dialog box (PRD
 *                   Appendix B, Sec 6.5; the comedic centerpiece). dBoxProc
 *                   variant, 7-px fancy border (gui-ground-truth.md Sec 3.3
 *                   WDEF dBoxBorderSize=7). The frame shows this dialog
 *                   prominently; the fixture crop is clearly identifiable in
 *                   preview.webp. But the exact pixel coords in both the native
 *                   640x480 space and the 1456x819 fixture space are
 *                   golden-resolves: the fixture is a distorted CRT photo
 *                   (gui-ground-truth.md Sec 3.4 warning), and the native
 *                   position depends on the FLAIR desktop layout at the test
 *                   screendump time (PRD Appendix B places it center-screen;
 *                   exact bounds = TODO until the dialog renders).
 *
 *   TODO_GOLDEN note: ALL entries are TODO_GOLDEN for the reasons above.
 *   The harness must:
 *     - Log "SSIM GUIDE [<name>]: TODO golden -- skipping score" for each.
 *     - NOT assert or gate on any SSIM score.
 *     - NOT block a merge on any SSIM value.
 *   When a golden-resolve lands (a real screendump is captured and the crop
 *   is measured), update the entry to GOLDEN_RESOLVED and record the bead
 *   number and the screendump commit in a comment below. That is a Rule 8 act.
 */

/* Number of windows in the crop table. */
#define FLAIR_SSIM_CROP_COUNT  4u

/*
 * The crop table is defined as a static const array for inclusion in C units
 * that need to iterate it (the harness). Because the native/fixture coords
 * are all TODO, placeholder zeros are used. The harness detects zeros + TODO
 * status and skips scoring.
 *
 * TO RESOLVE A TODO:
 *   1. Boot InitechOS in QEMU (`make run`) after the relevant rendering lands.
 *   2. Capture a screendump via QMP (`{"execute":"screendump","arguments":
 *      {"filename":"/tmp/flair_screen.ppm"}}`).
 *   3. Measure the pixel bounds of the target window in the 640x480 PPM.
 *   4. Measure the corresponding region in preview.webp (1456x819) -- note
 *      that perspective/CRT distortion means the fixture crop will be
 *      irregular; crop the best-matching rectangular approximation.
 *   5. Update this entry: set native_* and fixture_* to the measured values,
 *      set status = FLAIR_SSIM_STATUS_GOLDEN_RESOLVED.
 *   6. Record the bead + screendump commit in a comment here.
 *   This is a Rule 8 deliberate act. Do NOT fabricate coords; wrong coords
 *   produce misleading SSIM scores that steer agents toward plausible nonsense.
 */
static const flair_ssim_crop_t flair_ssim_crops[FLAIR_SSIM_CROP_COUNT] = {
    {
        /* DESKTOP: the full seafoam background area (below menu bar).
         * native: 640x460 region starting at y=20 (below the 20-px menu bar).
         * The native coords here are PARTIALLY RESOLVED for the top/left/width
         * (the desktop is always x=0, y=20, w=640 in a 640x480 framebuffer);
         * height is 640x480 minus 20-px menu bar = 460 px. This is a geometric
         * consequence of the screen size (OD-3 640x480) and menu bar height
         * (gui-ground-truth.md Sec 3.3 native=20), not a screendump measurement.
         * Marked TODO_GOLDEN because the fixture crop (in the distorted CRT photo)
         * cannot be reliably measured without the Mac ROM / Basilisk II mint
         * (gui-ground-truth.md Sec 6 P1 acquisition path, gated on ROM).        */
        "DESKTOP",
        FLAIR_SSIM_STATUS_TODO_GOLDEN,
        /* native 640x480 */  0, 20, 640, 460,
        /* fixture 1456x819 */
        /* TODO: crop the background region visible in preview.webp behind
         * the windows -- but the CRT photo occludes much of it and the exact
         * region depends on window layout. Placeholder zeros. */
        0, 0, 0, 0
    },
    {
        /* MENU_BAR: the top 640x20 pixels (ADR-0004 D-7; native value 20 px
         * from GetMBarHeight, gui-ground-truth.md Sec 3.3).
         * native: always (0, 0, 640, 20) -- a geometric consequence.
         * fixture: TODO -- CRT photo distortion means the fixture menu bar
         * region must be measured from preview.webp rather than computed.
         * The fixture crop also tests the Photoshop menu string canon
         * (FLAIR_MENU_BAR_CANON; spec/assets/menu_canon.h; FROZEN per
         * ADR-0004 AM-4). Screendump measurement needed once a font renders.   */
        "MENU_BAR",
        FLAIR_SSIM_STATUS_TODO_GOLDEN,
        /* native 640x480 */   0, 0, 640, 20,
        /* fixture 1456x819 */
        /* TODO: measure in preview.webp. The menu bar appears at the top of
         * the Mac window chrome in the frame; but the frame is a layered
         * Photoshop mock-up (CLAUDE.md hallucination-risk callout) and the
         * Mac window does not fill the frame. Placeholder zeros.               */
        0, 0, 0, 0
    },
    {
        /* FLAIR_WINDOW_GENERIC: a representative document window for chrome
         * testing (title bar, close/zoom boxes, frame, scrollbar).
         * TODO -- native coords depend on where the test window is placed for
         * the M3 drag-gate screendump (beads initech-87a, ADR-0004 AM-8).
         * Once the drag-gate passes and a screendump exists, measure the
         * title-bar area (top 19 px of the window body per gui-ground-truth.md
         * Sec 3.3 WDEF minTitleH=19) and the full chrome frame.
         * Update bead: initech-87a (the drag-gate is the earliest human-
         * verifiable Law-4 fidelity moment; ADR-0004 AM-8).                   */
        "FLAIR_WINDOW_GENERIC",
        FLAIR_SSIM_STATUS_TODO_GOLDEN,
        /* native 640x480 */  0, 0, 0, 0,  /* TODO: measure after M3 drag-gate */
        /* fixture 1456x819 */ 0, 0, 0, 0  /* TODO: measure from preview.webp  */
    },
    {
        /* FILE_COPY_DIALOG: the "Saving tables to disk..." modal dialog box.
         * PRD Appendix B, Sec 6.5 (the comedic centerpiece). dBoxProc variant
         * (FLAIR_WDEF_DBOX_PROC=1; 7-px fancy border per WDEF dBoxBorderSize=7,
         * gui-ground-truth.md Sec 3.3). The dialog is placed center-screen per
         * PRD Appendix B; exact pixel bounds are a golden-resolve.
         * fixture: the dialog is visible in preview.webp (the frame still);
         * but exact pixel coords in the 1456x819 photo are golden-resolves
         * (CRT distortion, Sec 3.4 warning). Measure once Basilisk II boots
         * (gui-ground-truth.md Sec 6 P1 acquisition; or measure directly from
         * the InitechOS screendump once the FILE COPY app lands in M5+).      */
        "FILE_COPY_DIALOG",
        FLAIR_SSIM_STATUS_TODO_GOLDEN,
        /* native 640x480 */
        /* TODO: PRD Appendix B says center-screen; dialog is approximately
         * 350x200 px (estimate only -- actual size depends on FLAIR chrome
         * rendering of dBoxProc + content). Placeholder zeros.               */
        0, 0, 0, 0,
        /* fixture 1456x819 */
        /* TODO: the "Saving tables to disk..." dialog is visible in the frame
         * still but the exact crop in the 1456x819 photo needs measurement.   */
        0, 0, 0, 0
    }
};

/* ===========================================================================
 * 3. COMPILE-TIME CONTRACT CHECKS
 * ===========================================================================*/

/* SSIM window size must be a positive odd number. */
_Static_assert(FLAIR_SSIM_WINDOW_SIZE > 0u,
               "SSIM window size must be positive");
_Static_assert((FLAIR_SSIM_WINDOW_SIZE & 1u) == 1u,
               "SSIM window size must be odd (symmetric Gaussian kernel)");

/* Sigma encoding: FLAIR_SSIM_SIGMA_TENTHS must be positive. */
_Static_assert(FLAIR_SSIM_SIGMA_TENTHS > 0u,
               "SSIM sigma_tenths must be positive (1.5 encoded as 15)");

/* Data range for 8-bpp. */
_Static_assert(FLAIR_SSIM_DATA_RANGE == 255u,
               "SSIM data range must be 255 for 8-bpp channels");

/* Crop table has the expected count. */
_Static_assert(FLAIR_SSIM_CROP_COUNT == 4u,
               "flair_ssim_crops must have FLAIR_SSIM_CROP_COUNT (4) entries");

/* Status codes are distinct. */
_Static_assert(FLAIR_SSIM_STATUS_TODO_GOLDEN     == 0u,
               "FLAIR_SSIM_STATUS_TODO_GOLDEN must be 0");
_Static_assert(FLAIR_SSIM_STATUS_GOLDEN_RESOLVED == 1u,
               "FLAIR_SSIM_STATUS_GOLDEN_RESOLVED must be 1");
_Static_assert(FLAIR_SSIM_STATUS_TODO_GOLDEN != FLAIR_SSIM_STATUS_GOLDEN_RESOLVED,
               "TODO and GOLDEN_RESOLVED status codes must differ");

/* flair_ssim_crop_t has the required fields (size sanity). */
_Static_assert(sizeof(flair_ssim_crop_t) > 0u,
               "flair_ssim_crop_t must be non-empty");

/* The crop table itself is the right size. */
_Static_assert(sizeof(flair_ssim_crops) ==
               FLAIR_SSIM_CROP_COUNT * sizeof(flair_ssim_crop_t),
               "flair_ssim_crops size must match FLAIR_SSIM_CROP_COUNT entries");

#endif /* INITECH_SPEC_SSIM_PARAMS_H */
