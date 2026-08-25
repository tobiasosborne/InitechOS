/*
 * os/flair/finder_windows.h -- the R3 Finder DISK WINDOWS (THE ARTIFACT).
 *
 * beads: initech-tdnl.10 (GUI remediation R3.3 "disk windows": double-click the
 *        volume icon and a real icon-view window opens over the REAL FAT12
 *        volume; double-click a folder inside it and that folder's window opens
 *        as a SPATIAL SINGLETON; Close disposes it and saves its view state;
 *        New Folder creates a REAL directory; Clean Up snaps row-major).
 *
 * WHAT THIS IS
 *   The Finder shell tenant (the FlairAppProcs vtable moved here from
 *   finder_desktop.c, which now owns only the desktop MODEL) plus the
 *   fixed-capacity table of open disk windows. It is where a FAT directory
 *   becomes a grid of icons and a WindowRecord.
 *
 * ---------------------------------------------------------------------------
 * THE LAYERING RULE, AND HOW THIS FILE OBEYS IT
 * ---------------------------------------------------------------------------
 * os/flair includes NO os/milton header -- so this file cannot call fat12_*
 * and cannot see a fat12_volume_t. Enumeration and directory creation arrive
 * through a CALLER-SUPPLIED BINDING (`finder_fs_t`): a small vtable of function
 * pointers the kernel wires to fat12_read_dir / fat12_mkdir in os/milton/kmain.c
 * and the host oracle wires to a hand-authored dirent stream. This is the
 * os/flair/stdfile.h precedent verbatim (FlairSFEnumProc: "the directory
 * provider (DECOUPLES the module from FAT I/O) ... THIS is what makes the module
 * host-testable"), and the same split R3.2 used for the DESKTOP.DB codec (bytes
 * in os/flair, FAT I/O in os/milton, kmain joins them).
 *
 * THE CALLBACK-LIFETIME CONTRACT IS CARRIED THROUGH VERBATIM. os/milton/fat12.h
 * :: fat12_dirent_cb says the entry "is valid only for the duration of the call
 * (it lives in the caller's sector_buf, which the next sector read overwrites)".
 * finder_dirent_t therefore hands over a `const char *name83` that is likewise
 * only valid DURING the call, and this module copies the name, the attribute,
 * the size and the start cluster into its own record BEFORE returning. The host
 * oracle proves it: its mock binding deliberately SCRIBBLES the name buffer the
 * moment the callback returns, so a retained pointer reads garbage and the
 * hand-authored expected listing goes RED.
 *
 * ---------------------------------------------------------------------------
 * WHY A DISK WINDOW REUSES THE DESKTOP MODEL UNCHANGED
 * ---------------------------------------------------------------------------
 * Design F2-5 made the R3.2 trackers PURE functions of numbers: hit test,
 * marquee, selection, double-click synthesis and drag commit read a
 * finder_desk_t (an icon array plus a `bounds` rect) and nothing else. A disk
 * window's content area is just a different `bounds`. So a window OWNS a
 * finder_desk_t whose bounds are its CONTENT rect, in the same global
 * coordinates, and every tracker and the painter work on it with zero change --
 * no surface parameter, no refactor of finder_desktop.c's logic, and no second
 * copy of the geometry. The only edit R3.3 needed on that file was extending
 * the kind->strike map (finder_desktop.c :: fd_strike) so the ONE painter can
 * draw folders and documents as well as the volume and the Trash.
 *
 * ---------------------------------------------------------------------------
 * THE PAINT SEAM IS THE ORDINARY TENANT UPDATE PATH -- NOT THE UNDERLAY
 * ---------------------------------------------------------------------------
 * Desktop icons paint through the WindowMgr desktop-underlay hook (design
 * F2-4 seam 1) because they live UNDER every window. Window icons are ordinary
 * OWNED-WINDOW CONTENT: damage goes in through WindowMgr_invalidate(wm, win,
 * rect), flair_route_updates delivers an updateEvt to the Finder (background
 * tenants receive updates -- process.h Sec 3), and this tenant's event() paints
 * clipped to contRgn INTERSECT updateRgn, exactly like os/apps/ref_tenant.c.
 * There is no second compositor and the WL-0076 damage contract is untouched.
 *
 * ---------------------------------------------------------------------------
 * SERIAL MARKERS (emitted by os/milton/kmain.c; os/flair files never call
 * serial_puts -- ASCII, one per line, grep-able)
 * ---------------------------------------------------------------------------
 *   FINDER-OPEN-VOLUME win=<slot> n=<icons>      root disk window opened
 *       *** RE-KEY (Rule 8): this REPLACES R3.2's "FINDER-OPEN-VOLUME NYI".
 *       The volume double-click now really opens a window and really
 *       enumerates the volume, so the marker carries the window slot and the
 *       icon count. The emu gate leg that asserted the NYI line is re-keyed to
 *       this one -- strictly stronger (it now also pins the enumerated count).
 *   FINDER-OPEN-FOLDER name=<n> win=<slot> singleton=<0|1>
 *                                                folder window opened, or the
 *                                                existing one brought forward
 *   FINDER-CLOSE-WINDOW win=<slot>               go-away on a Finder window
 *   FINDER-NEW-FOLDER name=<n> parent=<cluster>  a REAL directory was created
 *   FINDER-CLEANUP win=<slot> moved=<n>          Clean Up snapped n icons
 *   FINDER-DIR-FULL parent=<cluster>             mkdir refused: directory full
 *   FINDER-WIN-FULL                              every window slot is in use
 *   FINDER-WIN-ICONS-FULL win=<slot> dropped=<n> the per-window icon cap bit
 *   FINDER-WIN-SELECT win=<slot> name=<n> count=<sel>
 *   FINDER-WIN-DESELECT-ALL win=<slot>
 *   FINDER-WIN-MARQUEE win=<slot> n=<sel>
 *   FINDER-WIN-DRAG win=<slot> name=<n> x=<x> y=<y>
 * The window-surface gesture markers are DISTINCT from the desktop's
 * (FINDER-ICON-SELECT / -DESELECT-ALL / -MARQUEE / -DRAG) on purpose: the
 * gestures are the same code, but a gate must be able to tell WHICH surface
 * reported, and re-keying the locked R3.2 desktop markers to carry a surface
 * field would have been a gratuitous break.
 *
 * Ref: docs/design/GUI-remediation-R3-finder-design.md
 *        F1.1 (icon records; the kind heuristic; the fixed caps that fail loud;
 *          the callback-lifetime rule; VOLLABEL skipped by the Finder),
 *        F1.2 (32x32 sprites; the invisible grid; labels beneath),
 *        F1.3 (the LOCKED DESKTOP.DB record layout, kind=4 folder-view),
 *        F2.3/F2-4 (redraw discipline), F2-3 (close = DisposeWindow only for a
 *          Finder-owned window), F2-5 (pure trackers),
 *        F3.1 (the fat12 API map), F3.2/F3-3 (raw formatted 8.3 display),
 *        F3.3 (spatial singleton; the root window's frame-canonical title;
 *          per-folder view state keyed by dir_start; Clean Up = row-major),
 *        F5.2 (slice tdnl.10 scope + the SPATIAL_DUP mutant).
 *      docs/plans/GUI-remediation-plan.md R4.6 ("Drive A Files") -- the
 *        frame-canonical root-window title F3.3 defers to.
 *      os/flair/stdfile.h (the provider-binding precedent + the 13-byte
 *        formatted-8.3 display convention this file inherits),
 *      os/flair/finder_desktop.h (the icon model, the trackers, the painter and
 *        the DESKTOP.DB codec this file consumes),
 *      os/flair/finder_cmd.h (the ONE command spine; FinderCtx.exec is the hook
 *        this file binds), os/flair/window.h (NewDocumentWindow / SetWTitle /
 *        SelectWindow / DisposeWindow / WindowMgr_invalidate),
 *      os/apps/ref_tenant.c (the arena-carved WindowRecord + region bundles and
 *        the updateEvt repaint this file follows).
 *      CLAUDE.md Law 1, Law 2 (hand-authored goldens; the mock binding is the
 *        oracle's, never the artifact's), Law 3 (freestanding, dual-compile,
 *        caller-supplied storage), Rule 2 (fail loud: every cap says so on
 *        serial, never silently truncates), Rule 6 (mutation-proven), Rule 8
 *        (the DB layout is locked data; the marker re-key is deliberate),
 *        Rule 11 (deterministic), Rule 12 (ASCII-clean).
 *
 * DUAL-COMPILE: finder_windows.c compiles BOTH freestanding for the kernel and
 * hosted for harness/proptest/test_finder_windows.c. No libc, no malloc.
 */
#ifndef INITECH_OS_FLAIR_FINDER_WINDOWS_H
#define INITECH_OS_FLAIR_FINDER_WINDOWS_H

#include <stdint.h>

#include "surface.h"            /* bitmap_t                                    */
#include "region_algebra.h"     /* region_t / rgn_rect_t (-Ispec)               */
#include "region.h"             /* rgn_row_t (-Ios/flair/atkinson)              */
#include "window.h"             /* WindowMgr / WindowRecord                     */
#include "process.h"            /* FlairApp / FlairAppProcs                     */
#include "finder_desktop.h"     /* the icon model + trackers + the DB codec     */
#include "finder_cmd.h"         /* FinderCtx + finder_cmd_id (the exec hook)    */

/* ===========================================================================
 * 1. CAPACITIES  (design F1.1: fixed capacity, FAIL LOUD on overflow, Rule 2)
 * ===========================================================================*/

/* Open disk windows. Four is the deliberate V1 ceiling: a spatial Finder on a
 * 640x480 screen with 360x220 windows has room for a cascade of four before the
 * arrangement stops being legible, and every slot's storage (WindowRecord +
 * four region bundles + the icon array) is carved ONCE at launch so a mid-
 * session open can never fail on a heap condition. A fifth open reports
 * FINDER_WIN_ERR_FULL and kmain says FINDER-WIN-FULL on serial. */
#define FINDER_WIN_MAX             4

/* Icons per window (design F1.1: "64 desktop icons + 64 per open window").
 * The volume's root directory holds 224 entries (root_entry_count on the
 * flagship 1.44 MB image), so this cap CAN bite -- and when it does the window
 * keeps the first 64 and reports the remainder as `dropped`, loudly. It never
 * silently shows a partial directory (Rule 2). */
#define FINDER_WIN_ICONS_MAX      64

/* Region-bundle caps, the os/apps/ref_tenant.c values verbatim: a rectangular
 * document window's strucRgn/contRgn, a rect-damage updateRgn, and their
 * intersection all stay far under these, and a cap overflow FAILS LOUD inside
 * the region engine rather than truncating a region. */
#define FINDER_WIN_RGN_ROWS       32
#define FINDER_WIN_RGN_POOL      128

/* ===========================================================================
 * 2. THE INVISIBLE GRID  (design F1.2 / F3.3 "Clean Up ... plain row-major")
 * ---------------------------------------------------------------------------
 * PITCH is design F1.2's provisional 68 x 52, *golden-resolves* against the
 * sys8 captures. INSET_X is (68 - 32) / 2 == 18, i.e. the 32-px sprite is
 * CENTRED in its 68-px grid cell, so a label wider than the sprite spreads
 * symmetrically instead of hanging off one side. INSET_Y is a 4-px top margin
 * inside the content rect.
 *
 * Cell (row, col) of a window whose content rect is `c`:
 *      sprite_x = c.left + FINDER_GRID_INSET_X + col * FINDER_GRID_PITCH_X
 *      sprite_y = c.top  + FINDER_GRID_INSET_Y + row * FINDER_GRID_PITCH_Y
 * and the column count is (content_width - INSET_X) / PITCH_X, at least 1.
 * ROW-MAJOR means index i occupies col = i % cols, row = i / cols -- fill the
 * first row left-to-right, then the next. Deterministic (Rule 11) and fully
 * hand-computable, which is how the oracle grades it.
 *
 * A cell below the content rect is NOT clipped away: finder_desk_drag_commit's
 * clamp keeps every icon inside `bounds`, so a directory with more icons than
 * fit simply stacks them on the last legal row. Scrolling is DEFERRED (no List
 * Manager, no scrollbar wiring this slice; the view record reserves the bits).
 * ===========================================================================*/
#define FINDER_GRID_PITCH_X       68
#define FINDER_GRID_PITCH_Y       52
#define FINDER_GRID_INSET_X       18
#define FINDER_GRID_INSET_Y        4

/* ===========================================================================
 * 3. DEFAULT WINDOW GEOMETRY
 * ---------------------------------------------------------------------------
 * A window with no saved view record opens at a CASCADED default derived from
 * its slot index -- deterministic, so the same action sequence always produces
 * the same frame (Rule 11).
 *
 *   frame(slot) = (L + slot*DX, T + slot*DY) .. (+W, +H)
 *
 * The base rect sits clear of the desktop's icon column (the volume and Trash
 * sprites occupy x [584,616), and their labels start at x 577), so opening the
 * root window never hides the icon you just double-clicked.
 *
 * THE LOAD-BEARING INVARIANT (and it is NOT about these numbers): no disk
 * window exists at boot. Windows open ONLY on a double-click, so every locked
 * emu trace that never double-clicks renders a bit-identical frame to before
 * this slice. That is what makes these defaults free to choose -- a window may
 * legitimately cover another gate's probe point, because that gate's trace
 * never opens one.
 * ===========================================================================*/
#define FINDER_WIN_DEFAULT_L      20
#define FINDER_WIN_DEFAULT_T      60
#define FINDER_WIN_DEFAULT_W     360
#define FINDER_WIN_DEFAULT_H     220
#define FINDER_WIN_CASCADE_DX     20
#define FINDER_WIN_CASCADE_DY     20

/* The root (volume) window's title. Design F3.3: "the root volume window takes
 * the frame-canonical name (the 'Drive A Files' convention of the remediation
 * plan R4.6 -- the File Manager *is* the disk window)". A subfolder's window is
 * titled with its raw formatted 8.3 name (F3-3: no synthetic long names). */
#define FINDER_WIN_ROOT_TITLE   "Drive A Files"

/* ===========================================================================
 * 4. THE NEW-FOLDER NAME LADDER  (LOCKED convention, design F5.3 / F1.4)
 * ---------------------------------------------------------------------------
 * F5.3 asks New Folder to create an "untitled-analog 8.3 dir with name field
 * ready for inline rename". Inline rename is tdnl.11; what THIS slice owes is a
 * deterministic, collision-free, 8.3-LEGAL default name.
 *
 * THE CONVENTION, LOCKED HERE:
 *      first free name is           "NEWFOLD"                 (7 chars, no ext)
 *      on collision, lowest free of "NEWFOL02" .. "NEWFOL99"  (6 + 2 digits)
 * i.e. the base is truncated to 6 characters and a 2-digit counter appended,
 * counting from 02 so the ladder reads as a continuation of the un-suffixed
 * first name. LOWEST-FREE-FIRST mirrors F1.4's Trash-collision scheme (base
 * truncated + numeric suffix, lowest counter first) and the FAT allocator's own
 * "lowest-free-first" philosophy -- the same deterministic instinct in three
 * places. 99 taken names exhausts the ladder and FAILS LOUD
 * (FINDER_WIN_ERR_NAMES); it never invents a 100th spelling.
 *
 * This is a DEFINED convention (Rule 8: locked, deliberate, documented), not a
 * claimed period restoration -- period Macs had HFS long names and could simply
 * say "untitled folder"; 8.3 cannot, and F3.4 already set the precedent of
 * inventing and locking a convention rather than pretending one was attested.
 * ===========================================================================*/
#define FINDER_NEWFOLDER_BASE   "NEWFOLD"
#define FINDER_NEWFOLDER_STEM   "NEWFOL"
#define FINDER_NEWFOLDER_MAX_N  99

/* ===========================================================================
 * 5. THE FAT BINDING SEAM  (see the banner: os/flair names no os/milton symbol)
 * ===========================================================================*/

/* The two FAT attribute bits the Finder reads. Their VALUES are
 * spec/dos_structs.h's DIR_ATTR_VOLLABEL / DIR_ATTR_DIRECTORY; they are
 * restated here rather than included because spec/dos_structs.h is the DOS
 * on-disk layout and dragging it into the Toolbox would blur the boundary this
 * file exists to keep. The DRIFT TOOTH is a _Static_assert in the binding TU
 * (os/milton/kmain.c), where BOTH headers are legitimately in scope -- so a
 * renumbering can never diverge silently (Law 1 / Rule 2). */
#define FINDER_ATTR_VOLLABEL   0x08u
#define FINDER_ATTR_DIRECTORY  0x10u

/* One directory entry, as seen DURING the enumeration callback.
 *
 * LIFETIME: `name83` points at the binding's scratch and is valid ONLY for the
 * duration of the call (the fat12_dirent_cb contract, carried through verbatim
 * -- see the banner). Copy, never retain. */
typedef struct finder_dirent {
    const char *name83;        /* NUL-terminated, fat12_format_83 shape        */
    uint8_t     attribute;     /* the raw FAT attribute byte                   */
    uint32_t    size;          /* file size in bytes (0 for a directory)       */
    uint16_t    start_cluster; /* the entry's first data cluster               */
} finder_dirent_t;

/* Return 0 to continue the enumeration, non-zero to stop early (the value is
 * propagated as the enumerate() return -- the fat12_dirent_cb rule). */
typedef int (*finder_enum_cb)(const finder_dirent_t *e, void *user);

/* The binding. The kernel wires these to fat12_read_dir / fat12_mkdir; the host
 * oracle wires them to a hand-authored dirent stream. */
typedef struct finder_fs {
    /* Enumerate the directory whose first data cluster is `dir_start` (0 == the
     * fixed root), invoking `cb` per surviving entry. Returns 0 on a full scan,
     * the callback's non-zero value on an early stop, or a NEGATIVE value on a
     * backend error. */
    int (*enumerate)(void *fs_user, uint16_t dir_start,
                     finder_enum_cb cb, void *cb_user);

    /* Create the subdirectory `name83` under `parent_dir_start` (0 == root).
     * Returns FINDER_WIN_OK, FINDER_WIN_ERR_DIRFULL when the parent cannot take
     * another entry, or FINDER_WIN_ERR_MKDIR for any other backend refusal --
     * the BINDING maps its backend's error codes into these, so this layer
     * never spells a FAT12_ERR_* value. */
    int (*mkdir)(void *fs_user, const char *name83, uint16_t parent_dir_start);

    void *user;                /* the binding's opaque cookie                  */
} finder_fs_t;

/* DEVIATION FROM THE BRIEF, STATED: the brief's example binding also carried a
 * volume_label() entry. It has no consumer this slice -- design F3.3 rules the
 * ROOT window's title is the frame-canonical FINDER_WIN_ROOT_TITLE, not the
 * volume label, and the label itself already reaches the Finder as the desktop
 * volume ICON's name (kmain reads it off the disk at mount time, R3.2). A third
 * path to the same string would be an unused seam, and this file does not ship
 * speculative API (Law 1). */

/* ===========================================================================
 * 6. STATUS CODES
 * ===========================================================================*/
typedef enum finder_win_status {
    FINDER_WIN_OK            =  0,
    FINDER_WIN_ERR_NULL      = -1,  /* NULL shell / no binding / bad slot      */
    FINDER_WIN_ERR_FULL      = -2,  /* every window slot is in use             */
    FINDER_WIN_ERR_ENUM      = -3,  /* the binding's enumerate() failed        */
    FINDER_WIN_ERR_DIRFULL   = -4,  /* mkdir: the parent directory is full     */
    FINDER_WIN_ERR_MKDIR     = -5,  /* mkdir: any other backend refusal        */
    FINDER_WIN_ERR_NAMES     = -6   /* the NEWFOLD.. ladder is exhausted       */
} finder_win_status_t;

/* ===========================================================================
 * 7. THE RECORDS
 * ===========================================================================*/

/* A region plus its caller-supplied backing (the engine never mallocs). */
typedef struct finder_win_rgn {
    region_t  r;
    rgn_row_t rows[FINDER_WIN_RGN_ROWS];
    int16_t   pool[FINDER_WIN_RGN_POOL];
} finder_win_rgn_t;

typedef struct finder_window {
    uint8_t              open;        /* 1 == this slot holds a live window    */
    uint8_t              is_root;     /* 1 == the volume's root window         */
    uint16_t             dir_start;   /* the folder's first cluster (0 == root)*/
    char                 name83[FINDER_DESK_NAME_MAX];  /* the folder's name   */
    uint16_t             view_bits;   /* the kind=4 record's view bits         */
    uint16_t             dropped;     /* entries the icon cap refused          */
    WindowRecord         rec;         /* the window itself                     */
    finder_win_rgn_t     rs, rc, ru;  /* struc / cont / update                 */
    finder_win_rgn_t     rk;          /* the updateEvt paint-clip scratch      */
    finder_desk_t        view;        /* the icon model; bounds == content rect*/
    finder_desk_icon_t   icons[FINDER_WIN_ICONS_MAX];
    finder_click_track_t click;       /* this surface's double-click tracker   */
} finder_window_t;

/* ---------------------------------------------------------------------------
 * THE COMMAND OUTCOME  (how a Layer-3 command reports to the Layer-1 kernel)
 *
 * finder_dispatch calls the shell's exec hook from deep inside kmain's keyDown
 * handling. The hook cannot emit serial (os/flair files never call serial_puts)
 * and cannot drive the repaint cycle (that needs the live offscreen and
 * present). So it records WHAT HAPPENED here, and kmain reads it back the
 * instant finder_dispatch returns, emits the marker and runs the DQ2 cycle.
 * One command, one outcome, one report -- no hidden second channel.
 * ------------------------------------------------------------------------- */
typedef struct finder_cmd_outcome {
    uint8_t             valid;    /* 1 == a command ran since the last read    */
    finder_cmd_id       id;       /* which one                                 */
    finder_win_status_t status;   /* FINDER_WIN_OK or the refusal              */
    int16_t             slot;     /* the affected window slot, or -1           */
    int16_t             moved;    /* Clean Up: icons that actually moved        */
    uint16_t            parent;   /* New Folder: the parent's first cluster    */
    uint8_t             singleton;/* Open: 1 == an existing window was raised  */
    char                name83[FINDER_DESK_NAME_MAX];  /* New Folder / Open    */
} finder_cmd_outcome_t;

/* The shell tenant's whole per-instance state; ONE allocation from the tenant's
 * RECORDS arena at launch (ADR-0013 AC-2: the shell reads only records-arena
 * storage during teardown, so tenant death survives a scribbled DATA arena). */
typedef struct finder_shell {
    finder_desk_t        desk;                              /* R3.2 desktop    */
    finder_desk_icon_t   desk_icons[FINDER_DESK_MAX_ICONS];
    /* NOTE: the DESKTOP surface's double-click tracker is NOT here -- it stays
     * the kmain-owned global R3.2 put it in, so the desktop gesture path is
     * byte-for-byte the code that is already gated green. Each WINDOW carries
     * its own tracker (below), which is what makes a double-click in one window
     * independent of a click in another. */
    finder_window_t      windows[FINDER_WIN_MAX];
    finder_view_rec_t    views[FINDER_DB_MAX_VIEWS];        /* kind=4 records  */
    uint16_t             n_views;
    WindowMgr           *wm;
    const bitmap_t      *surface;
    FlairApp            *app;
    finder_fs_t          fs;
    FinderCtx           *ctx;      /* the command context (kmain owns it)      */
    uint8_t              have_fs;  /* 1 once a binding is installed            */
    finder_cmd_outcome_t last;     /* the most recent command's outcome        */
} finder_shell_t;

/* ===========================================================================
 * 8. THE TENANT  (design F2-1; the vtable R3.2 kept in finder_desktop.c)
 * ---------------------------------------------------------------------------
 * open()  carves ONE finder_shell_t from records_arena, builds the R3.2 desktop
 *         model into it (finder_desk_tenant_build -- same two calls, same
 *         order, same seeded coordinates, so the boot frame is unchanged),
 *         attaches every window slot's regions, and stores the shell in
 *         self->userData. It creates NO window: a disk window opens only on a
 *         double-click, so the boot scene is byte-identical to R3.2's.
 * event() handles updateEvt (repaint a disk window's content, clipped to
 *         contRgn INTERSECT updateRgn -- the ref_tenant discipline) and keyDown
 *         (the Cmd-chord path through the ONE command spine). It deliberately
 *         IGNORES mouseDown: a click that starts a marquee or a drag has to be
 *         tracked to mouseUp through WaitNextEvent, which is a live-pump
 *         concern and lives in kmain beside flair_live_do_drag -- exactly where
 *         every other tracked gesture already lives.
 * close() is absent: the Finder is always resident (design F2-1).
 * ===========================================================================*/
extern const FlairAppProcs finder_shell_procs;

/* Recover the shell from a launched Finder tenant (NULL-safe, magic-checked). */
finder_shell_t *finder_shell_of(FlairApp *app);

/* The R3.2 desktop model inside a shell (NULL-safe). */
finder_desk_t *finder_shell_desk(finder_shell_t *sh);

/* Install the FAT binding. Until this is called every disk-window operation
 * returns FINDER_WIN_ERR_NULL rather than pretending to succeed (Rule 2). */
void finder_shell_bind_fs(finder_shell_t *sh, const finder_fs_t *fs);

/* Bind the command context the key path dispatches through, and install this
 * shell as its execution hook (FinderCtx.exec/shell). */
void finder_shell_bind_ctx(finder_shell_t *sh, FinderCtx *ctx);

/* Take (and CLEAR) the last command outcome. Returns NULL when no command has
 * run since the previous take -- so a caller can never report the same command
 * twice, which on a serial trace would read as two commands (Rule 2). */
const finder_cmd_outcome_t *finder_shell_take_outcome(finder_shell_t *sh);

/* Recompute the FinderCtx predicate state (selection_count / front_is_diskwin)
 * from the live shell. Called before every dispatch so the enablement
 * predicates read real state, exactly as F4.4 requires. */
void finder_shell_sync_ctx(finder_shell_t *sh);

/* ===========================================================================
 * 9. PURE HELPERS  (graded directly by the host oracle)
 * ===========================================================================*/

/* The icon-kind heuristic (design F1.1):
 *      attribute & FINDER_ATTR_DIRECTORY   -> FINDER_ICON_FOLDER
 *      name83 ends ".EXE" (case-insensitive)-> FINDER_ICON_APP
 *      otherwise                            -> FINDER_ICON_FILE
 * The extension test is COSMETIC ONLY (ADR-0003-AMENDMENT-DEC-08a.4 makes
 * dispatch content-based); it decides which sprite is drawn, nothing else. */
finder_icon_kind_t finder_win_kind_of(const char *name83, uint8_t attribute);

/* 1 when the Finder must SKIP this entry (design F1.1: volume-label entries are
 * skipped by the Finder, because the FAT layer deliberately passes them through
 * -- fat12.h :: fat12_read_root_dir "Volume-label and directory entries are NOT
 * filtered here"). Deleted / free / LFN entries are already gone by then.
 *
 * SCOPE NOTE, STATED (Law 1): F1.1 rules out the VOLUME LABEL and nothing else,
 * so HIDDEN entries -- \DESKTOP.DB and \TRASH on the flagship volume -- DO
 * appear in the root window this slice. Period Finders hid their Desktop file
 * and period DOS DIR hid hidden entries, so a hidden-skip is a plausible later
 * ruling; making it here would be a DESIGN change dressed as an implementation
 * detail, so it is recorded instead of silently applied. */
int finder_win_skip_entry(uint8_t attribute);

/* The default cascaded frame for `slot` (Sec 3). Pure. */
rgn_rect_t finder_win_default_frame(int slot);

/* Columns that fit across a content rect `c` (at least 1). Pure. */
int finder_win_grid_cols(rgn_rect_t c);

/* The row-major grid cell for icon index `i` inside content rect `c`. Pure. */
void finder_win_grid_origin(rgn_rect_t c, int i, int16_t *out_x, int16_t *out_y);

/* Build the next free New Folder name for a directory whose existing names are
 * `existing[0..n)`. Writes a NUL-terminated 8.3 name into `out` (which must
 * hold FINDER_DESK_NAME_MAX bytes) and returns FINDER_WIN_OK, or
 * FINDER_WIN_ERR_NAMES when the whole ladder is taken. Pure; the comparison is
 * case-insensitive because FAT 8.3 names are. */
finder_win_status_t finder_win_next_folder_name(
        const char (*existing)[FINDER_DESK_NAME_MAX], int n, char *out);

/* ===========================================================================
 * 10. WINDOW OPERATIONS
 * ===========================================================================*/

/* The slot holding the window for `dir_start`, or -1. This IS the spatial
 * singleton rule (design F3.3): reopening a folder finds its window instead of
 * making a second one. */
int finder_win_find(const finder_shell_t *sh, uint16_t dir_start);

/* The slot owning `w`, or -1 (used to demux a go-away / content click). */
int finder_win_slot_of(const finder_shell_t *sh, const WindowRecord *w);

/* The FRONTMOST open disk window's slot in the WindowMgr z-order, or -1. */
int finder_win_front_slot(const finder_shell_t *sh);

/* Open (or raise) the window for a directory.
 *
 *   dir_start : the folder's first cluster; 0 == the volume root
 *   name83    : the folder's formatted 8.3 name; ignored when is_root
 *   is_root   : 1 for the volume window (title FINDER_WIN_ROOT_TITLE)
 *   out_slot  : receives the slot (may be NULL)
 *   out_singleton : receives 1 when an EXISTING window was raised instead of a
 *                   new one being built (may be NULL)
 *
 * A raise is SelectWindow only -- no re-enumeration, no re-layout, so the
 * user's arrangement inside that window survives (the spatial-Finder promise).
 * A fresh open enumerates through the binding, lays the icons out row-major on
 * the grid, restores the saved frame origin from the kind=4 view record when
 * there is one, and seeds a full-content update. */
finder_win_status_t finder_win_open(finder_shell_t *sh, uint16_t dir_start,
                                    const char *name83, uint8_t is_root,
                                    int *out_slot, int *out_singleton);

/* Close a Finder-owned window: SAVE the view state (design F3.3), then
 * DisposeWindow ONLY -- never the tenant-terminate path, because the owner is
 * the always-resident Finder (design F2-3, the anti-8fhu refinement). */
finder_win_status_t finder_win_close(finder_shell_t *sh, int slot);

/* Re-enumerate a window from the volume and re-lay it out on the grid. Used by
 * open() and after New Folder. Sets w->dropped when the icon cap bit. */
finder_win_status_t finder_win_populate(finder_shell_t *sh, int slot);

/* Create a REAL directory in `slot`'s folder with the next free ladder name
 * (Sec 4), then re-populate the window. `out_name83` (may be NULL) receives the
 * created name; `out_parent` (may be NULL) receives the parent cluster, so the
 * caller can emit FINDER-NEW-FOLDER / FINDER-DIR-FULL without re-deriving it. */
finder_win_status_t finder_win_new_folder(finder_shell_t *sh, int slot,
                                          char *out_name83,
                                          uint16_t *out_parent);

/* Snap every icon in `slot` to its row-major grid cell (design F3.3). Returns
 * the number of icons that actually MOVED (0 means the window was already
 * clean). Invalidation is the caller's -- finder_win_invalidate_all. */
int finder_win_cleanup(finder_shell_t *sh, int slot);

/* Damage the whole content of `slot` through the ONE window-damage entry point
 * (WindowMgr_invalidate); the tenant's updateEvt then repaints it. */
void finder_win_invalidate_all(finder_shell_t *sh, int slot);

/* Damage exactly two icon cells (old then new -- the accumulation IS the union,
 * with no over-repaint of the rectangle spanning both; ADR-0004 D-5). */
void finder_win_invalidate(finder_shell_t *sh, int slot,
                           rgn_rect_t old_rect, rgn_rect_t new_rect);

/* Paint a window's content: the content fill, then the icons, both clipped.
 * Deterministic (Rule 11). Exposed so the oracle can render without a pump. */
void finder_win_paint(const finder_window_t *w, const bitmap_t *dst,
                      const region_t *clip);

/* ===========================================================================
 * 11. THE kind=4 VIEW RECORDS  (design F1.3 / F3.3; the codec is in
 *     finder_desktop.c -- this is the in-memory set the codec serialises)
 * ===========================================================================*/

/* Seed sh->views from a DESKTOP.DB image (boot). Returns the number of kind=4
 * records taken; a corrupt image contributes none (the caller regenerates
 * defaults loudly -- design F1.3). Records past FINDER_DB_MAX_VIEWS are
 * dropped, oldest-first-wins, which is deterministic. */
uint16_t finder_shell_load_views(finder_shell_t *sh, const uint8_t *buf,
                                 uint32_t len);

/* Upsert one view record keyed by dir_start. Returns 1 when the set CHANGED
 * (so the caller knows to commit), 0 when it was already exactly this. */
int finder_shell_note_view(finder_shell_t *sh, uint16_t dir_start,
                           const char *name83, uint16_t x, uint16_t y,
                           uint16_t view_bits);

/* The stored record for dir_start, or NULL. */
const finder_view_rec_t *finder_shell_find_view(const finder_shell_t *sh,
                                                uint16_t dir_start);

/* Record `slot`'s CURRENT window frame origin + view bits into the view set.
 * Returns 1 when the set changed (commit needed). Called on close and after a
 * window move. */
int finder_shell_save_view(finder_shell_t *sh, int slot);

#endif /* INITECH_OS_FLAIR_FINDER_WINDOWS_H */
