/*
 * os/flair/finder_desktop.h -- the R3 Finder DESKTOP MANAGER (THE ARTIFACT).
 *
 * beads: initech-tdnl.9 (GUI remediation R3.2 "DesktopMgr": the desktop icon
 *        layer -- volume + Trash icons, selection, marquee, icon drag, the
 *        double-click synthesiser, and the DESKTOP.DB record codec).
 *
 * WHAT THIS IS
 *   The Finder's desktop-side data model and its PURE trackers, plus the ONE
 *   painter that puts icons on the desktop through the WindowMgr underlay seam.
 *   It owns NO storage (all arrays are caller-supplied, Law 3), calls NO serial
 *   (Layer 3/5 files never do -- kmain emits the markers, the finder_cmd.h
 *   banner rule), touches NO FAT (os/flair must not include os/milton headers),
 *   and names NO color (C-8: every tone resolves through flair_look_pixel_depth).
 *
 * THE FIVE RULES IT OBEYS
 *   1. TRACKERS ARE PURE FUNCTIONS OF A POINT SEQUENCE (design F2-5, the
 *      flair_menu_track precedent). finder_band_rect / finder_desk_marquee_select
 *      / finder_click_classify / finder_desk_drag_commit take numbers and return
 *      numbers; the live pump feeds them real EventRecords, the host oracle
 *      (harness/proptest/test_finder_desktop.c) feeds them hand-authored ones.
 *      Nothing here needs an emulator to grade.
 *   2. THE ICON LAYER HOOKS EXACTLY ONE PAINT SEAM (design F2-4): the WindowMgr
 *      `desktop_underlay` callback invoked by os/flair/desktop.c immediately
 *      after its base desktop fill, with THAT site's clip. Icons are UNDER
 *      windows by construction, because the clip only ever contains pixels no
 *      window owns. There is no second compositor, and shell_render's static
 *      path is untouched.
 *   3. INVALIDATION GOES EXCLUSIVELY THROUGH WindowMgr_invalidate_desktop
 *      (old rect, then new rect -- the accumulation IS the union). Icons never
 *      poke a window's updateRgn and never blit outside a paint callback.
 *   4. ALL DRAWING IS CLIPPED. Sprite pixels go through finder_icon_draw (which
 *      honours the mask AND the clip); the label band goes through
 *      blitter_fill_rect_clipped; the label glyphs go through a per-pixel walk
 *      that tests the clip exactly like finder_icon.c does. A destination pixel
 *      is written IFF the clip contains it.
 *   5. FIXED CAPACITY, FAIL LOUD (design F1.1; Rule 2). finder_desk_add returns
 *      -1 when the caller-supplied array is full; the caller panics.
 *
 * THE DESKTOP IS FLAT TEAL THIS SLICE. The Platinum desktop PATTERN is a canon
 * ruling still pending, so the underlay draws icons ONLY -- it never touches the
 * background fill desktop.c already performed (design F2.3 "Pattern-vs-teal
 * note"; the pattern lands with the ruling, at the same seam, with no further
 * plumbing).
 *
 * FONT NOTE (a correction to the design document): design F1.2 / correction 0.1
 * assumed no Geneva strike exists and raised F1-4 as an open policy question.
 * IT DOES EXIST -- os/flair/text.h has FONT_GENEVA9 over the hand-authored
 * spec/assets/geneva9.h strike (GENEVA9_CELL_H == 11). So this slice uses
 * GENEVA for icon labels, which is the period-correct Finder label font, and
 * F1-4's Chicago-substitution fallback is NOT needed. Recorded here because the
 * design doc is wrong on the point and a later reader would otherwise "fix" it.
 *
 * ------------------------------------------------------------------------
 * SERIAL MARKERS (emitted by os/milton/kmain.c; the emu-wiring slice keys its
 * gates on these EXACT lines -- ASCII, one per line, grep-able)
 * ------------------------------------------------------------------------
 *   FINDER-DESKTOP-ICONS n=<count>       once, when the Finder tenant opens
 *   FINDER-ICON-SELECT name=<name> count=<sel>   an icon became selected
 *   FINDER-ICON-DESELECT-ALL             a click on bare desktop cleared it
 *   FINDER-MARQUEE n=<selected>          a rubber-band completed
 *   FINDER-ICON-DRAG name=<name> x=<gx> y=<gy>   drop committed (new origin)
 *   FINDER-ICON-DRAG-REVERT              drop refused (undraggable icon)
 *   FINDER-OPEN-TRASH NYI                double-click on the Trash icon
 *   DESKTOP-DB-SAVE n=<records>          positions committed to \DESKTOP.DB
 *
 * R3.3 RE-KEY (bead initech-tdnl.10): the volume double-click no longer says
 * "FINDER-OPEN-VOLUME NYI" -- it OPENS the root disk window and says
 *   FINDER-OPEN-VOLUME win=<slot> n=<icons>
 * See os/flair/finder_windows.h for that marker and the rest of the disk-window
 * set. The Trash double-click stays NYI until the tdnl.11 file-ops slice.
 *
 * Ref: docs/design/GUI-remediation-R3-finder-design.md F1.1 (icon records +
 *        fixed caps), F1.2 (32x32 sprites, labels centered beneath, hit test =
 *        sprite UNION label, topmost wins by record order), F1.3 (the DESKTOP.DB
 *        24-byte record layout -- LOCKED, little-endian positional assembly),
 *        F1.5 (trash-origin forward compatibility), F2.2 (double-click
 *        synthesis off EventRecord.when, rubber band, icon drag), F2.3/F2-4
 *        (the three seams), F2-5 (pure trackers + save-under, no XOR blit).
 *      os/flair/finder_icon.h (the strike blitter + mask hit test),
 *        spec/assets/desk_icons.h (the LOCKED strikes),
 *        os/flair/window.h (WindowMgr.desktop_underlay + invalidate_desktop),
 *        os/flair/flair_look.h (the ONE policy seam),
 *        os/flair/text.h + spec/assets/geneva9.h (the label font),
 *        os/flair/finder_cmd.h (FinderCtx.selection_count, fed from here).
 *      CLAUDE.md Law 1 (ground truth), Law 2 (hand-authored goldens; the DB
 *        expected bytes are authored in the oracle, NEVER computed from this
 *        writer -- HER-02), Law 3 (freestanding + dual-compile), Rule 1
 *        (red->green), Rule 2 (fail loud), Rule 6 (mutation-proven), Rule 8
 *        (the DB record layout is locked data), Rule 11 (deterministic --
 *        the underlay must be byte-reproducible or the solid leg-D whole-frame
 *        identity gate breaks), Rule 12 (ASCII-clean).
 *
 * DUAL-COMPILE (the window.c / finder_icon.c pattern): finder_desktop.c compiles
 * BOTH freestanding for the kernel and hosted for the property suite. No libc,
 * no malloc; all storage is caller-supplied.
 */
#ifndef INITECH_OS_FLAIR_FINDER_DESKTOP_H
#define INITECH_OS_FLAIR_FINDER_DESKTOP_H

#include <stdint.h>

#include "surface.h"            /* bitmap_t                                   */
#include "region_algebra.h"     /* region_t / rgn_rect_t (-Ispec)              */
#include "window.h"             /* WindowMgr (underlay install + invalidate)   */
#include "process.h"            /* FlairApp / FlairAppProcs (the shell tenant) */

/* ===========================================================================
 * 1. LOCKED-ISH GEOMETRY  (design F1.2; provisional numbers are golden-resolves)
 * ===========================================================================*/

/* Sprite cell: the classic ICN# 32x32 (design F1.2; uncontroversial). */
#define FINDER_ICON_DIM            32

/* Label metrics. GENEVA9_CELL_H is 11 (spec/assets/geneva9.h); one pixel of
 * padding above and below gives a 13-row band. PAD_X pads each side of the
 * measured string so the band is not flush against the glyphs. GAP is the
 * vertical gap between sprite bottom and band top. All *golden-resolves*
 * against the sys8 captures (design F1.2). */
#define FINDER_LABEL_GAP           2
#define FINDER_LABEL_PAD_X         3
#define FINDER_LABEL_ROWS          11   /* == GENEVA9_CELL_H; asserted in .c   */
#define FINDER_LABEL_H             (FINDER_LABEL_ROWS + 2)

/* Full cell height: sprite + gap + label band. */
#define FINDER_CELL_H              (FINDER_ICON_DIM + FINDER_LABEL_GAP + \
                                    FINDER_LABEL_H)

/* Default placement offsets from the usable desktop bounds (design F1.1:
 * volume top-right, Trash bottom-right). Chosen so neither cell touches any
 * probe point of any locked emu trace/grader on the 640x480 tenants image --
 * see the placement note in os/milton/kmain.c and the bead report. With the
 * usable desktop [0,40)-(640,480) these resolve to:
 *     VOLUME sprite (584,48)..(616,80)
 *     TRASH  sprite (584,404)..(616,436)
 * (labels are centered under each sprite; the Trash label ends at y=451, which
 * is above the locked cursor PARK rect at (620,460)..(636,476)). */
#define FINDER_DESK_MARGIN_R       24   /* right margin to the sprite's right  */
#define FINDER_DESK_MARGIN_T        8   /* top margin below the two menu bars  */
#define FINDER_DESK_TRASH_BOTTOM   76   /* bounds.bottom - this = sprite top   */

/* Interaction constants (design F2-5). FINDER_DBLCLICK_TICKS is PROVISIONAL at
 * 20 ticks (1/3 s on the 100 Hz PIT), *golden-resolves* against a period Mouse
 * control-panel capture, and becomes the R4.5 Mouse panel's slider variable.
 *
 * -D-OVERRIDABLE, and ONLY for the demo RECORD image (beads initech-tdnl.10),
 * exactly as os/milton/kmain.c's FLAIR_TEN_TICK_BUDGET and
 * FLAIR_LIVE_DRAG_TRACK_TICKS are: record-flair dumps one PPM per injected
 * event, which stretches the gap between the two clicks of a double past 20
 * ticks, so a replay of a LOCKED trace would degrade every double-click into
 * two singles and the clip would show a gesture that never happened. The
 * $(BUILD)/flair_tenants_mut_recorddbl.img variant widens it; the DEFAULT build
 * is byte-identical at 20 and every gate still grades that one.
 *
 * THIS IS NOT THE "RAISE THE CONSTANT TO GO GREEN" MOVE the traces file warns
 * against (spec/flair_desktop_icons_traces.mk trace 5): no oracle grades the
 * record image, and if a GATE ever needs the interval widened the fix is still
 * the harness or the trace, never this number. */
#ifndef FINDER_DBLCLICK_TICKS
#define FINDER_DBLCLICK_TICKS      20u
#endif
#define FINDER_DBLCLICK_SLOP        4   /* max |dh|,|dv| between the two clicks */
#define FINDER_DRAG_SLOP            3   /* movement below this is still a click */

/* Fixed capacities (design F1.1: fail loud on overflow, Rule 2). 13 + NUL is
 * the fat12_format_83 shape ("NAME.EXT" is at most 12 chars; 13 matches
 * FAT12_NAME83_MAX so a later item-icon slice needs no widening). */
#define FINDER_DESK_NAME_MAX       14
#define FINDER_DESK_MAX_ICONS      16

/* ===========================================================================
 * 2. THE ICON RECORD  (design F1.1, narrowed to what R3.2 actually holds)
 * ---------------------------------------------------------------------------
 * DEVIATION FROM F1.1, STATED: F1.1's sketch carries dir_start / slot /
 * origin_dir / surface for the disk-window and Trash-staging slices (tdnl.10 /
 * .11). R3.2 ships ONLY desktop icons, so those fields would be dead weight
 * that the DB codec would have to invent values for. They arrive with the
 * slices that can populate them honestly (Law 1); the DB record layout ALREADY
 * reserves dir_start, so adding them is not a format change.
 * ===========================================================================*/

/* The icon kinds. VOLUME/TRASH are the R3.2 DESKTOP icons; FOLDER/FILE/APP are
 * the R3.3 DISK-WINDOW icons (design F1.1: DIR_ATTR_DIRECTORY => folder, an 8.3
 * name ending ".EXE" => app, everything else => document).
 *
 * VALUE RULE (bead initech-tdnl.10): the two DESKTOP kinds coincide with their
 * DESKTOP.DB record kinds by R3.2's design (1 = volume-pos, 2 = trash-pos), and
 * finder_desk_db_encode writes `ic->kind` straight into the record's kind byte.
 * The three WINDOW kinds are therefore placed DELIBERATELY ABOVE the whole
 * record-kind range (1..5, design F1.3) so an icon kind can never be mistaken
 * for a record kind and a window icon can never encode as a plausible-but-wrong
 * DB record. Window icons are never encoded at all -- a disk window persists a
 * kind=4 VIEW record, not per-item positions -- and this numbering makes that a
 * fact of the data, not a fact of the control flow (Rule 2). */
typedef enum finder_icon_kind {
    FINDER_ICON_NONE   = 0,
    FINDER_ICON_VOLUME = 1,   /* == DESKTOP.DB record kind 1 (volume-pos)      */
    FINDER_ICON_TRASH  = 2,   /* == DESKTOP.DB record kind 2 (trash-pos)       */
    FINDER_ICON_FOLDER = 16,  /* disk window: a subdirectory (F1.1)            */
    FINDER_ICON_FILE   = 17,  /* disk window: a document (F1.1)                */
    FINDER_ICON_APP    = 18   /* disk window: an ".EXE" application (F1.1)     */
} finder_icon_kind_t;

typedef struct finder_desk_icon {
    uint8_t  kind;          /* finder_icon_kind_t                              */
    uint8_t  selected;      /* 0/1                                             */
    uint8_t  draggable;     /* 0 == fixed position (the Trash, design F1.1)    */
    uint8_t  reserved;      /* keeps the record 4-byte aligned; always 0        */
    int16_t  x;             /* sprite top-left, desktop (== global) coords     */
    int16_t  y;
    char     name[FINDER_DESK_NAME_MAX];   /* NUL-terminated label text        */
    /* R3.3 (bead initech-tdnl.10). F1.1's original sketch carried dir_start /
     * slot / origin_dir; R3.2 shipped none of them because a desktop-only slice
     * could not have populated them honestly (Law 1). THIS is the field the
     * disk-window slice can: for a FOLDER icon it is that folder's OWN first
     * data cluster -- the key its window is opened under and the key its kind=4
     * view record is stored under (design F3.3 "Keyed by dir_start"). It is 0
     * for every other kind, and the DESKTOP.DB codec does not read it (a
     * desktop record's dir_start field is the CONTAINING directory, always the
     * root, always 0). */
    uint16_t dir_start;
} finder_desk_icon_t;

typedef struct finder_desk {
    finder_desk_icon_t *icons;   /* CALLER-SUPPLIED array (Law 3)              */
    uint16_t            n;       /* live count                                 */
    uint16_t            cap;     /* capacity of icons[]                        */
    rgn_rect_t          bounds;  /* the usable desktop rect (below both bars)  */
} finder_desk_t;

/* ===========================================================================
 * 3. LIFECYCLE + RECORD BUILDING
 * ===========================================================================*/

/* Bind `fd` to caller storage and the usable desktop rect. Zero icons. */
void finder_desk_init(finder_desk_t *fd, finder_desk_icon_t *storage,
                      uint16_t cap, rgn_rect_t usable);

/* Append one icon. Returns its index, or -1 when the array is FULL (the caller
 * fails loud -- Rule 2 / design F1.1). `name` is copied and NUL-terminated;
 * NULL means the empty label. */
int finder_desk_add(finder_desk_t *fd, finder_icon_kind_t kind,
                    const char *name, int16_t x, int16_t y, uint8_t draggable);

/* Seed the two default desktop icons (design F1.1): the volume top-right and
 * the Trash bottom-right of `fd->bounds`, using the margins above. The volume
 * label is set separately (finder_desk_set_name) because only the kernel knows
 * the mounted volume's label. Returns 0 on success, -1 if the array is full. */
int finder_desk_seed_defaults(finder_desk_t *fd);

/* Replace icon `idx`'s label text. No-op on a bad index. */
void finder_desk_set_name(finder_desk_t *fd, int idx, const char *name);

/* Set icon `idx`'s dir_start (R3.3: a FOLDER icon's own first cluster; see the
 * field comment). No-op on a bad index. Separate from finder_desk_add so the
 * R3.2 call sites keep their exact signature. */
void finder_desk_set_cluster(finder_desk_t *fd, int idx, uint16_t cluster);

/* Find the first icon of `kind`, or -1. */
int finder_desk_find_kind(const finder_desk_t *fd, finder_icon_kind_t kind);

/* ===========================================================================
 * 4. GEOMETRY  (design F1.2)
 * ---------------------------------------------------------------------------
 * The label band is centered under the sprite and CLAMPED into fd->bounds so a
 * long label can never run off the screen edge. The hit / damage rect is the
 * sprite rect UNION the label rect.
 * ===========================================================================*/
rgn_rect_t finder_desk_sprite_rect(const finder_desk_t *fd, int idx);
rgn_rect_t finder_desk_label_rect (const finder_desk_t *fd, int idx);
rgn_rect_t finder_desk_cell_rect  (const finder_desk_t *fd, int idx);

/* Rect helpers (half-open [left,right) x [top,bottom), the region_algebra.h
 * convention). Exported because the oracle grades them directly. */
int        finder_rect_contains  (rgn_rect_t r, int16_t h, int16_t v);
int        finder_rect_intersects(rgn_rect_t a, rgn_rect_t b);

/* finder_desk_hit -- point -> icon index, TOPMOST WINS, or -1.
 *
 * Icons paint in RECORD ORDER (index 0 first), so a LATER record is on top;
 * the scan therefore runs from the last record BACKWARD. The target is the
 * SPRITE rect UNION the LABEL rect (design F1.2), NOT their bounding box: the
 * bbox would swallow the two dead corners beside a centered label and select an
 * icon from bare desktop under its shoulder. finder_desk_cell_rect (the bbox)
 * is the DAMAGE rect only. */
int finder_desk_hit(const finder_desk_t *fd, int16_t h, int16_t v);

/* ===========================================================================
 * 5. SELECTION  (design F1.1 "Selection state")
 * ===========================================================================*/
void     finder_desk_select_only  (finder_desk_t *fd, int idx);  /* plain click */
void     finder_desk_select_extend(finder_desk_t *fd, int idx);  /* shift-click */
void     finder_desk_deselect_all (finder_desk_t *fd);
uint16_t finder_desk_selection_count(const finder_desk_t *fd);

/* ===========================================================================
 * 6. THE RUBBER BAND  (design F2.2 / F2-5: a PURE function of a point pair)
 * ---------------------------------------------------------------------------
 * finder_band_rect normalises the anchor and the current point into a HALF-OPEN
 * rect that CONTAINS BOTH endpoint pixels: left = min(h0,h1), right =
 * max(h0,h1) + 1 (and likewise vertically). The +1 is the load-bearing part --
 * without it the band excludes its own right/bottom edge pixel and an icon
 * whose left edge is exactly under the release point is missed. That is the
 * FINDER_DESK_MUT_MARQUEE_OFFBYONE mutant.
 *
 * finder_desk_marquee_select REPLACES the selection with every icon whose
 * SPRITE rect or LABEL rect intersects the band (the same target the hit test
 * uses, never the bounding box), and returns the resulting selected count.
 * ===========================================================================*/
rgn_rect_t finder_band_rect(int16_t h0, int16_t v0, int16_t h1, int16_t v1);
uint16_t   finder_desk_marquee_select(finder_desk_t *fd, rgn_rect_t band);

/* ===========================================================================
 * 7. DOUBLE-CLICK SYNTHESIS  (design F2.2 / F2-5)
 * ---------------------------------------------------------------------------
 * Finder state, keyed off EventRecord.when -- deliberately NOT in the Event
 * Manager (Layer 4 defines no click-interval concept and must not gain one).
 * A click is a DOUBLE iff the tracker is armed, the target is the SAME icon,
 * the tick delta is <= FINDER_DBLCLICK_TICKS, and both coordinates are within
 * FINDER_DBLCLICK_SLOP. A double DISARMS the tracker, so three clicks in a row
 * are double + single, never two overlapping doubles.
 * ===========================================================================*/
typedef enum finder_click_kind {
    FINDER_CLICK_SINGLE = 0,
    FINDER_CLICK_DOUBLE = 1
} finder_click_kind_t;

typedef struct finder_click_track {
    uint32_t last_when;
    int16_t  last_h;
    int16_t  last_v;
    int16_t  last_target;   /* icon index, or -1 for "no target"               */
    uint8_t  armed;
    uint8_t  reserved;
} finder_click_track_t;

void                finder_click_reset(finder_click_track_t *t);
finder_click_kind_t finder_click_classify(finder_click_track_t *t,
                                          int16_t target,
                                          int16_t h, int16_t v, uint32_t when);

/* ===========================================================================
 * 8. ICON DRAG  (design F2.2: reposition + persist; revert otherwise)
 * ===========================================================================*/
typedef enum finder_drop {
    FINDER_DROP_NOOP   = 0,   /* nothing moved and nothing to say              */
    FINDER_DROP_MOVED  = 1,   /* committed; *out_old / *out_new are the damage */
    FINDER_DROP_REVERT = 2    /* refused (fixed-position icon)                 */
} finder_drop_t;

/* The CLAMPED outline rect for a live drag of icon `idx` by (dh,dv): the cell
 * rect translated by the clamped delta. Pure; used by the save-under tracker. */
rgn_rect_t finder_desk_drag_outline(const finder_desk_t *fd, int idx,
                                    int16_t dh, int16_t dv);

/* Commit a drop. Clamps the new sprite origin so the WHOLE cell stays inside
 * fd->bounds. `out_old` / `out_new` receive the cell rects before and after (so
 * the caller can invalidate old then new -- the union). Both may be NULL. */
finder_drop_t finder_desk_drag_commit(finder_desk_t *fd, int idx,
                                      int16_t dh, int16_t dv,
                                      rgn_rect_t *out_old, rgn_rect_t *out_new);

/* ===========================================================================
 * 9. THE DESKTOP.DB RECORD CODEC  (design F1.3 -- LOCKED layout, Rule 8)
 * ---------------------------------------------------------------------------
 * The FILE lives on MILTON's FAT volume and os/flair may not include os/milton
 * headers, so the split is: THIS module owns the BYTES (pure, host-gradable),
 * os/milton/desktop_db.c owns the FAT read/write of those bytes, and kmain
 * joins them. (Deviation from the brief's "record I/O in desktop_db.c" -- the
 * I/O is there; the codec has to live where the icon records live.)
 *
 * Layout, little-endian positional assembly, 8-byte header + 24-byte records:
 *     0  'I' 'D' 'B' '1'
 *     4  uint16 version = 1
 *     6  uint16 n_records
 *     8  records[n], each:
 *          +0  uint8  kind      (1=volume-pos, 2=trash-pos, 3=item-pos,
 *                                4=folder-view, 5=trash-origin)
 *          +1  uint8  flags
 *          +2  uint16 dir_start (containing dir cluster; 0 = root)
 *          +4  char   name83[13]
 *          +17 uint16 grid_x
 *          +19 uint16 grid_y
 *          +21 uint16 view_bits
 *          +23 uint8  pad = 0
 *
 * DEVIATION, STATED: F1.3 names the position fields "grid_x, grid_y" (an
 * invisible-grid CELL index). R3.2 ships FREE dragging (F2.2: "desktop =>
 * reposition"), so there is no grid to index yet; the fields carry the sprite
 * origin IN PIXELS. Same width, same offsets, same file version -- Clean Up
 * (tdnl.12+) introduces the grid and can reinterpret or convert without a
 * format bump. Origins are clamped >= 0 before encoding, so the unsigned field
 * is always exact.
 * ===========================================================================*/
#define FINDER_DB_HEADER_SIZE   8u
#define FINDER_DB_RECORD_SIZE   24u
#define FINDER_DB_VERSION       1u

/* Record-count capacity. R3.2 shipped desktop icon records only, so the cap WAS
 * FINDER_DESK_MAX_ICONS. R3.3 (bead initech-tdnl.10) adds the kind=4 per-folder
 * VIEW records (design F1.3 / F3.3), so the image can now hold both classes and
 * the cap is their sum. This is a capacity EXTENSION of an already-locked
 * layout, not a format change: the magic, the version, the 8-byte header and
 * the 24-byte record shape are untouched, and a count beyond the cap is still
 * FINDER_DB_ERR_COUNT (fail loud, Rule 2). */
/* The five record kinds of the LOCKED layout (design F1.3), named so the codec
 * never spells a bare literal. R3.2 wrote 1/2; R3.3 adds 4. 3 and 5 belong to
 * the tdnl.11 file-ops slice and are still only reserved values. */
#define FINDER_DB_KIND_VOLUME   1u   /* volume-pos   (desktop icon)            */
#define FINDER_DB_KIND_TRASH    2u   /* trash-pos    (desktop icon)            */
#define FINDER_DB_KIND_ITEM     3u   /* item-pos     (reserved, tdnl.11)       */
#define FINDER_DB_KIND_VIEW     4u   /* folder-view  (per-folder window state) */
#define FINDER_DB_KIND_ORIGIN   5u   /* trash-origin (reserved, F1.5)          */

#define FINDER_DB_MAX_VIEWS     8u
#define FINDER_DB_MAX_RECORDS   ((uint32_t)FINDER_DESK_MAX_ICONS + \
                                 FINDER_DB_MAX_VIEWS)
#define FINDER_DB_MAX_BYTES     (FINDER_DB_HEADER_SIZE + \
                                 FINDER_DB_MAX_RECORDS * \
                                 FINDER_DB_RECORD_SIZE)

typedef enum finder_db_status {
    FINDER_DB_OK        =  0,
    FINDER_DB_ERR_LEN   = -1,   /* shorter than the header / wrong total size  */
    FINDER_DB_ERR_MAGIC = -2,
    FINDER_DB_ERR_VER   = -3,
    FINDER_DB_ERR_COUNT = -4    /* n_records beyond FINDER_DESK_MAX_ICONS      */
} finder_db_status_t;

/* Structural validation of a DB image. Pure. */
finder_db_status_t finder_desk_db_validate(const uint8_t *buf, uint32_t len);

/* Serialise every desktop icon as a positional record. Returns the byte count
 * written, or 0 when `cap` is too small (the caller fails loud). */
uint32_t finder_desk_db_encode(const finder_desk_t *fd, uint8_t *buf,
                               uint32_t cap);

/* Apply the positions in a DB image to the matching icons (matched by record
 * kind for the singleton volume/Trash icons). Unknown kinds are ignored, not
 * an error. Positions are clamped into fd->bounds exactly like a drop, so a
 * DB authored against a different screen can never strand an icon off-screen.
 * Returns FINDER_DB_OK, or the validation status (fd untouched) on corruption
 * -- the caller then regenerates defaults loudly (design F1.3). */
finder_db_status_t finder_desk_db_apply(finder_desk_t *fd, const uint8_t *buf,
                                        uint32_t len);

/* ---------------------------------------------------------------------------
 * kind=4 -- THE PER-FOLDER VIEW RECORD (design F1.3 record kind 4 "folder-view",
 * F3.3 "per-folder DB records (kind=4): grid origin, view bits ... Keyed by
 * dir_start. Loaded at window open; saved on close/move").
 *
 * DEVIATION, STATED (bead initech-tdnl.10): F3.3 asks the record to carry grid
 * ORIGIN, view bits AND scroll offsets, and the brief additionally asks for the
 * window SIZE. The LOCKED 24-byte record (F1.3) has exactly two uint16 position
 * fields and one uint16 view_bits -- there is no field for a size and no field
 * for a scroll offset. So this slice persists:
 *      grid_x, grid_y  <- the window FRAME's top-left corner, in pixels
 *      view_bits       <- bit0 = list view (always 0 this slice; the List/
 *                         scrollbar wiring is deferred), bits 1..15 reserved 0
 * and the window SIZE stays the deterministic default the open path derives.
 * Widening the record is a deliberate Rule 8 act with its own issue -- silently
 * bumping the format to make one feature fit is exactly what Rule 8 forbids.
 * The reserved view_bits are written and read back verbatim, so the field is
 * already load-bearing for the slice that does add list view.
 * ---------------------------------------------------------------------------*/
typedef struct finder_view_rec {
    uint16_t dir_start;                  /* the folder's first cluster (0=root) */
    char     name83[FINDER_DESK_NAME_MAX];/* the folder's 8.3 name ("" for root)*/
    uint16_t x;                          /* window FRAME left                   */
    uint16_t y;                          /* window FRAME top                    */
    uint16_t view_bits;                  /* bit0 = list view; others reserved 0 */
} finder_view_rec_t;

/* Serialise the desktop icons (kind 1/2, exactly as finder_desk_db_encode) and
 * then `n_views` kind=4 view records, into ONE image. Record ORDER is icons
 * first, views second, each in array order -- deterministic (Rule 11), so the
 * same session state always produces the same bytes. Returns the byte count
 * written, or 0 when `cap` is too small or the record count exceeds
 * FINDER_DB_MAX_RECORDS (the caller fails loud). `views` may be NULL iff
 * n_views == 0, in which case the output is byte-identical to
 * finder_desk_db_encode -- the R3.2 golden still holds. */
uint32_t finder_desk_db_encode_all(const finder_desk_t *fd,
                                   const finder_view_rec_t *views,
                                   uint16_t n_views,
                                   uint8_t *buf, uint32_t cap);

/* Look up the kind=4 record for `dir_start` in a DB image. Returns 1 and fills
 * *out when found, 0 when absent, and a NEGATIVE finder_db_status_t when the
 * image does not validate (the caller then regenerates defaults loudly). The
 * FIRST matching record wins (the encoder never writes duplicates). */
int finder_desk_db_find_view(const uint8_t *buf, uint32_t len,
                             uint16_t dir_start, finder_view_rec_t *out);

/* ===========================================================================
 * 10. PAINTING + THE UNDERLAY SEAM  (design F2-4)
 * ===========================================================================*/

/* Draw every icon whose cell rect meets `clip` (NULL clip == no extra clip).
 * Deterministic: same state + same clip => same pixels, byte for byte (Rule 11;
 * the solid leg-D whole-frame identity gate depends on it). */
void finder_desk_paint(const finder_desk_t *fd, const bitmap_t *dst,
                       const region_t *clip);

/* Install `fd` as `wm`'s desktop underlay, so desktop_paint_all /
 * desktop_paint_damage draw the icons right after their base fill, with that
 * site's clip. Idempotent. `fd` must outlive `wm`. */
void finder_desk_install_underlay(finder_desk_t *fd, WindowMgr *wm);

/* Invalidate an icon's damage through the ONE entry point (design F2-4 seam 2):
 * WindowMgr_invalidate_desktop(old) then (new) -- the accumulation IS the
 * union, with no over-repaint of the rectangle spanning both. */
void finder_desk_invalidate(WindowMgr *wm, rgn_rect_t old_rect,
                            rgn_rect_t new_rect);

/* ===========================================================================
 * 11. THE DESKTOP HALF OF THE SHELL TENANT  (design F2-1: the Finder is an
 *     always-resident, compiled-in App Contract tenant, launched FIRST)
 * ---------------------------------------------------------------------------
 * R3.2 shipped the whole tenant here (finder_desk_procs / finder_desk_of).
 * R3.3 (bead initech-tdnl.10) MOVED the FlairAppProcs vtable to
 * os/flair/finder_windows.c, because the tenant now owns disk windows as well
 * as the desktop and its event() must paint updateEvts -- and a tenant whose
 * event() is a no-op cannot do that. What stays HERE is the desktop MODEL
 * build, called by that vtable's open():
 *
 *   finder_desk_tenant_build(fd, storage, cap, lp) ==
 *       finder_desk_init(fd, storage, cap, lp->bounds) + seed_defaults(fd)
 *
 * i.e. exactly the two calls R3.2's open() made, in the same order, producing
 * the same two icons at the same coordinates -- so the boot frame is unchanged
 * by the move (the load-bearing invariant of this slice). The CALLER supplies
 * the storage, as always (Law 3).
 *
 * Returns 0 on success, -1 when the storage is unusable or the array is full.
 * ===========================================================================*/
int finder_desk_tenant_build(finder_desk_t *fd, finder_desk_icon_t *storage,
                             uint16_t cap, const FlairLaunchParams *lp);

#endif /* INITECH_OS_FLAIR_FINDER_DESKTOP_H */
