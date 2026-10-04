/*
 * test_finder_ops.c -- the R3.4a Finder FILE-OPERATIONS oracle (THE ORACLE).
 *
 * beads: initech-34dh (drag-move + drag-to-Trash staging; reslice 1/3 of
 *        initech-tdnl.11).
 *
 * Ref: os/flair/finder_ops.h (the contract under test: the drop-target table,
 *        the refusal ladder and its ORDER, Trash staging, kind=5 origins),
 *      os/flair/finder_icon.h :: finder_icon_draw_hilite (the highlighted
 *        drop-target look; IM VI p. 2-19..2-20),
 *      os/flair/finder_desktop.h (the kind=5 field mapping),
 *      docs/design/GUI-remediation-R3-finder-design.md F1.4 / F1.5 / F2.2,
 *      harness/proptest/test_finder_windows.c (the scene + mock-binding
 *        pattern this file follows).
 *      CLAUDE.md Law 2 (every expectation HAND-AUTHORED; HER-02), Rule 1,
 *        Rule 6 (mutation-proven), Rule 12.
 *
 * THE MOCK VOLUME. A tiny mutable directory model stands in for the FAT
 * backend: enumerate walks it, move transplants an entry between two of its
 * directories (refusing a taken name with ERR_EXISTS, exactly the contract
 * finder_windows.h states), unlink deletes, trash_name returns the name when
 * free and otherwise the HAND-WRITTEN rule trunc5 + "001" (the only collision
 * these legs create). It deliberately does NOT detect cycles: the Finder's own
 * folder-into-itself rung is what O5 grades, so the NO_CYCLE mutant has
 * nothing to hide behind. The REAL backend's cycle walk, suffix ladder and
 * start_cluster preservation are graded by test-fat12-move (tdnl.26) and by
 * the emulator gate's mtools differential.
 *
 * THE SCENE (hand-derived from finder_windows.h Sec 2/3 + CalcDocContentRect):
 *   slot 0 = APPS   frame (20,60)..(380,280)  content (21,82)..(359,259)
 *   slot 1 = ROOT   frame (40,80)..(400,300)  content (41,102)..(379,279)
 *   (content bottom = frame.bottom - 21 since bead initech-tdnl.35: contRgn
 *   stops at the horizontal scroll bar, window.c CalcDocContentRect)
 *   ROOT icons row-major: README.TXT (59,106)  APPS (127,106)
 *                         DESKTOP.DB (195,106) TRASH (263,106)
 *   APPS icons:           TENANTFX.EXE (39,86)
 *   ROOT is in FRONT; APPS shows only its left strip x [21,40).
 *   desktop: volume sprite (584,8), Trash sprite (584,404) (seed_defaults on
 *   the (0,0)..(640,480) bounds: margin_r 24, margin_t 8, trash_bottom 76).
 *
 * LEGS
 *   O1 resolve: every row of the drop-target table, incl. "the dragged
 *      folder itself is NOT a folder target" and a WINDOW-RELATIVE re-hit after
 *      the root window is moved.
 *   O2 hilite: track on/off/same + the painted pixels of the highlighted
 *      folder strike (face -> #777777 idx 119, shade -> #3F3F3F idx 63, ink
 *      stays black 0) against the plain strike (face 1, shade 6).
 *   O3 move into a folder icon; the other icons do NOT re-flow.
 *   O4 move between windows; lands where dropped, relative to the content.
 *   O5 the refusal ladder: samedir, cycle, exists, err -- and NOTHING changes.
 *   O6 Trash staging: transplanted (not deleted), origin recorded; a name
 *      collision suffixes (NEWFO001) and sets the renamed flag.
 *   O7 the kind=5 codec against a hand-authored 80-byte image + reload.
 *   O8 moving an item back OUT of \TRASH drops its origin record.
 *   O13 the Trash window (bead initech-tdnl.39): lists the staged items,
 *      titled "Trash", singleton; put back = a plain move into the lowest
 *      FREE cell of the open root window; no \TRASH -> ERR_NOTRASH.
 *
 * MUTANTS: FINDER_OPS_MUT_TRASH_NO_STAGE (O6), FINDER_OPS_MUT_NO_CYCLE (O5),
 *          FINDER_OPS_MUT_NO_ORIGIN (O6/O7), FINDER_DESK_MUT_NO_HILITE (O2),
 *          FINDER_WIN_MUT_TRASH_ROOT + FINDER_OPS_MUT_COUNT_CELL (O13).
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "region_algebra.h"
#include "region.h"
#include "surface.h"
#include "window.h"
#include "finder_desktop.h"
#include "finder_windows.h"
#include "finder_ops.h"
#include "finder_cmd.h"
#include "test_assert.h"

TEST_HARNESS();

static rgn_rect_t mk_rect(int16_t t, int16_t l, int16_t b, int16_t r)
{
    rgn_rect_t x; x.top = t; x.left = l; x.bottom = b; x.right = r; return x;
}

enum { ST_ROWS = 64, ST_POOL = 512 };
typedef struct rgn_store {
    region_t  r;
    rgn_row_t rows[ST_ROWS];
    int16_t   pool[ST_POOL];
} rgn_store_t;

static void store_attach(rgn_store_t *s)
{
    s->r.rows = s->rows; s->r.cap_rows = ST_ROWS;
    s->r.x_pool = s->pool; s->r.x_pool_cap = ST_POOL;
    region_set_empty(&s->r);
}

/* ---------------------------------------------------------------------------
 * The mutable mock volume.
 * ------------------------------------------------------------------------- */
enum { MV_DIRS = 4, MV_ENTS = 26 };   /* 26: leg O11 root, 24 shown + 2 hidden */
enum { CL_ROOT = 0, CL_APPS = 2, CL_TRASH = 3, CL_NEWFOLD = 9 };

typedef struct mv_ent { char name[14]; uint8_t attr; uint16_t cluster; uint32_t size; } mv_ent_t;
typedef struct mv_dir { uint16_t cl; int n; mv_ent_t e[MV_ENTS]; } mv_dir_t;

typedef struct mock {
    mv_dir_t d[MV_DIRS];
    char     scratch[FINDER_DESK_NAME_MAX];
    int      move_calls, unlink_calls;
    int      force_move_rc;      /* != 0 -> move() returns this, changes nothing */
    int      no_trash;           /* 1 -> trash_dir() reports none               */
    const char *refuse;          /* unlink of this name fails (O14)             */
    char     log[256];           /* O14: the purge ops, in order                */
} mock_t;

static mv_dir_t *mv_dir(mock_t *m, uint16_t cl)
{
    for (int i = 0; i < MV_DIRS; i++) if (m->d[i].cl == cl) return &m->d[i];
    return NULL;
}
static int mv_find(const mv_dir_t *d, const char *n)
{
    for (int i = 0; i < d->n; i++) if (strcmp(d->e[i].name, n) == 0) return i;
    return -1;
}
static void mv_add(mv_dir_t *d, const char *n, uint8_t attr, uint16_t cl)
{
    memset(&d->e[d->n], 0, sizeof d->e[d->n]);
    strncpy(d->e[d->n].name, n, 13); d->e[d->n].attr = attr;
    d->e[d->n].cluster = cl; d->n++;
}
static void mv_del(mv_dir_t *d, int i)
{
    for (int k = i; k + 1 < d->n; k++) d->e[k] = d->e[k + 1];
    d->n--;
}
static int mv_has(mock_t *m, uint16_t cl, const char *n)
{
    mv_dir_t *d = mv_dir(m, cl);
    return d != NULL && mv_find(d, n) >= 0;
}

static int m_enum(void *u, uint16_t cl, finder_enum_cb cb, void *cu)
{
    mock_t *m = (mock_t *)u;
    mv_dir_t *d = mv_dir(m, cl);
    if (d == NULL) return 0;
    for (int i = 0; i < d->n; i++) {
        finder_dirent_t e; int rc;
        memset(m->scratch, 0, sizeof m->scratch);
        strncpy(m->scratch, d->e[i].name, sizeof m->scratch - 1);
        e.name83 = m->scratch; e.attribute = d->e[i].attr; e.size = d->e[i].size;
        e.start_cluster = d->e[i].cluster;
        rc = cb(&e, cu);
        memset(m->scratch, 'Z', sizeof m->scratch - 1);
        if (rc != 0) return rc;
    }
    return 0;
}
static int m_mkdir(void *u, const char *n, uint16_t p) { (void)u; (void)n; (void)p; return 0; }
static int m_move(void *u, const char *src, uint16_t sd, const char *dst, uint16_t dd)
{
    mock_t *m = (mock_t *)u;
    mv_dir_t *s = mv_dir(m, sd), *d = mv_dir(m, dd);
    const char *nn = (dst != NULL && dst[0] != '\0') ? dst : src;
    int i;
    m->move_calls++;
    if (m->force_move_rc != 0) return m->force_move_rc;
    if (s == NULL || d == NULL) return (int)FINDER_WIN_ERR_MOVE;
    if (sd == dd) return (int)FINDER_WIN_ERR_SAMEDIR;
    i = mv_find(s, src);
    if (i < 0) return (int)FINDER_WIN_ERR_MOVE;
    if (mv_find(d, nn) >= 0) return (int)FINDER_WIN_ERR_EXISTS;
    mv_add(d, nn, s->e[i].attr, s->e[i].cluster);
    d->e[d->n - 1].size = s->e[i].size;      /* a move keeps the bytes      */
    mv_del(s, i);
    return 0;
}
static int m_trash_name(void *u, const char *n, uint16_t dir, char *out)
{
    mock_t *m = (mock_t *)u;
    if (!mv_has(m, dir, n)) { strcpy(out, n); return 0; }
    /* the only collision these legs create is an extension-less name */
    memset(out, 0, FINDER_DESK_NAME_MAX);
    strncpy(out, n, 5); strcat(out, "001");
    return 0;
}
static int m_trash_dir(void *u) { return ((mock_t *)u)->no_trash ? -1 : CL_TRASH; }
static int m_unlink(void *u, const char *n, uint16_t dir)
{
    mock_t *m = (mock_t *)u;
    mv_dir_t *d = mv_dir(m, dir);
    int i;
    m->unlink_calls++;
    if (d == NULL || (i = mv_find(d, n)) < 0) return (int)FINDER_WIN_ERR_MOVE;
    if (m->refuse != NULL && strcmp(n, m->refuse) == 0) return (int)FINDER_WIN_ERR_MOVE;
    snprintf(m->log + strlen(m->log), sizeof m->log - strlen(m->log),
             "U:%s@%u ", n, (unsigned)dir);
    mv_del(d, i);
    return 0;
}
/* O14: remove an EMPTY subdirectory; a non-empty one is refused NOTEMPTY,
 * exactly the finder_fs_t rmdir contract. */
static int m_rmdir(void *u, const char *n, uint16_t dir)
{
    mock_t *m = (mock_t *)u;
    mv_dir_t *d = mv_dir(m, dir), *sub;
    int i;
    if (d == NULL || (i = mv_find(d, n)) < 0) return (int)FINDER_WIN_ERR_MOVE;
    sub = mv_dir(m, d->e[i].cluster);
    if (sub != NULL && sub->n != 0) return (int)FINDER_WIN_ERR_NOTEMPTY;
    snprintf(m->log + strlen(m->log), sizeof m->log - strlen(m->log),
             "R:%s@%u ", n, (unsigned)dir);
    mv_del(d, i);
    return 0;
}

/* ---------------------------------------------------------------------------
 * The scene.
 * ------------------------------------------------------------------------- */
enum { SCRW = 640, SCRH = 480 };
typedef struct scene {
    WindowMgr      wm;
    rgn_store_t    desk, sa, sb, sc;
    finder_shell_t sh;
    mock_t         mock;
    finder_fs_t    fs;
    uint8_t        px[SCRW * SCRH];
    bitmap_t       bm;
} scene_t;

static scene_t S;   /* large; static storage */

static void scene_init(scene_t *s)
{
    rgn_rect_t frame = mk_rect(0, 0, SCRH, SCRW);
    int slot = -1, single = 0;

    memset(s, 0, sizeof *s);
    store_attach(&s->desk); store_attach(&s->sa);
    store_attach(&s->sb);   store_attach(&s->sc);
    WindowMgr_init(&s->wm, frame, &s->desk.r, &s->sa.r, &s->sb.r, &s->sc.r);
    s->sh.wm = &s->wm;
    finder_desk_init(&s->sh.desk, s->sh.desk_icons,
                     (uint16_t)FINDER_DESK_MAX_ICONS, frame);
    finder_desk_seed_defaults(&s->sh.desk);

    s->mock.d[0].cl = CL_ROOT;  s->mock.d[1].cl = CL_APPS;
    s->mock.d[2].cl = CL_TRASH; s->mock.d[3].cl = CL_NEWFOLD;
    mv_add(&s->mock.d[0], "README.TXT", 0x20u, 5);
    mv_add(&s->mock.d[0], "APPS",       0x10u, CL_APPS);
    mv_add(&s->mock.d[0], "DESKTOP.DB", 0x22u, 6);
    mv_add(&s->mock.d[0], "TRASH",      0x10u, CL_TRASH);
    mv_add(&s->mock.d[1], "TENANTFX.EXE", 0x20u, 7);

    s->fs.enumerate  = m_enum;
    s->fs.mkdir      = m_mkdir;
    s->fs.move       = m_move;
    s->fs.trash_name = m_trash_name;
    s->fs.trash_dir  = m_trash_dir;
    s->fs.unlink     = m_unlink;
    s->fs.rmdir      = m_rmdir;
    s->fs.user       = &s->mock;
    finder_shell_bind_fs(&s->sh, &s->fs);

    s->bm.base = s->px; s->bm.pitch = SCRW; s->bm.width = SCRW;
    s->bm.height = SCRH; s->bm.bpp = 8u; s->bm.bytes_per_pixel = 1u;

    (void)finder_win_open(&s->sh, CL_APPS, "APPS", 0u, &slot, &single);  /* 0 */
    (void)finder_win_open(&s->sh, CL_ROOT, "", 1u, &slot, &single);      /* 1 */
}

static int idx_of(const finder_desk_t *fd, const char *n)
{
    for (int i = 0; i < (int)fd->n; i++) if (strcmp(fd->icons[i].name, n) == 0) return i;
    return -1;
}

#define ROOT 1
#define APPW 0

/* ===========================================================================
 * O1 -- TARGET RESOLUTION
 * ===========================================================================*/
static void leg_resolve(void)
{
    finder_tgt_t t;
    int readme, apps;

    scene_init(&S);
    readme = idx_of(&S.sh.windows[ROOT].view, "README.TXT");
    apps   = idx_of(&S.sh.windows[ROOT].view, "APPS");
    CHECK(readme == 0 && apps == 1, "O1 scene: the root window lists README.TXT then APPS");
    CHECK(S.sh.windows[ROOT].view.icons[1].x == 127 && S.sh.windows[ROOT].view.icons[1].y == 106,
          "O1 scene: APPS sits at the hand-derived cell (127,106)");

    t = finder_ops_resolve(&S.sh, ROOT, readme, 143, 122);
    CHECK(t.kind == FINDER_TGT_FOLDER && t.slot == ROOT && t.idx == 1 && t.dir == CL_APPS,
          "O1 over the APPS folder icon -> FOLDER (into APPS)");
    t = finder_ops_resolve(&S.sh, ROOT, apps, 143, 122);
    CHECK(t.kind == FINDER_TGT_REPOSITION,
          "O1 a folder dragged over ITSELF is a reposition, never a folder target");
    t = finder_ops_resolve(&S.sh, ROOT, readme, 300, 250);
    CHECK(t.kind == FINDER_TGT_REPOSITION && t.slot == ROOT,
          "O1 bare content of the icon's OWN window -> REPOSITION");
    t = finder_ops_resolve(&S.sh, ROOT, readme, 30, 200);
    CHECK(t.kind == FINDER_TGT_WINDOW && t.slot == APPW && t.dir == CL_APPS,
          "O1 the visible strip of ANOTHER disk window -> WINDOW (into its dir)");
    t = finder_ops_resolve(&S.sh, ROOT, readme, 600, 420);
    CHECK(t.kind == FINDER_TGT_TRASH && t.slot == -1 && t.idx == 1,
          "O1 over the desktop Trash icon -> TRASH");
    t = finder_ops_resolve(&S.sh, ROOT, readme, 600, 24);
    CHECK(t.kind == FINDER_TGT_VOLUME && t.dir == 0u,
          "O1 over the volume icon -> VOLUME (the root)");
    t = finder_ops_resolve(&S.sh, ROOT, readme, 500, 360);
    CHECK(t.kind == FINDER_TGT_NONE, "O1 bare desktop -> NONE");
    t = finder_ops_resolve(&S.sh, ROOT, readme, 200, 90);
    CHECK(t.kind == FINDER_TGT_NONE, "O1 a window TITLE BAR -> NONE");

    /* WINDOW-RELATIVE: move the root window by (+100,+60). The APPS icon is
     * now on screen at (227,166)+(16,16); the hit must follow the window. */
    MoveWindow(&S.wm, &S.sh.windows[ROOT].rec, 140, 140);
    t = finder_ops_resolve(&S.sh, ROOT, readme, 243, 182);
    CHECK(t.kind == FINDER_TGT_FOLDER && t.idx == 1,
          "O1 after a window move the folder target follows the WINDOW (relative hit)");
}

/* ===========================================================================
 * O2 -- THE HIGHLIGHTED DROP TARGET
 * ===========================================================================*/
static uint8_t pix(int x, int y) { return S.px[y * SCRW + x]; }

static void leg_hilite(void)
{
    finder_tgt_t cur, nx;
    finder_window_t *rw;

    scene_init(&S);
    rw = &S.sh.windows[ROOT];
    memset(&cur, 0, sizeof cur); cur.idx = -1; cur.slot = -1;

    nx = finder_ops_resolve(&S.sh, ROOT, 0, 143, 122);
    CHECK(finder_ops_track_hilite(&S.sh, &cur, &nx) == 1, "O2 entering a folder target changes the highlight");
    CHECK(rw->view.icons[1].hilite == 1, "O2 the APPS folder is now highlighted");
    CHECK(finder_ops_track_hilite(&S.sh, &cur, &nx) == 0, "O2 staying on it changes nothing");

    memset(S.px, 0xEE, sizeof S.px);
    finder_win_paint(rw, &S.bm, NULL);
    /* APPS sprite at (127,106). Folder map (spec/assets/finder_icons.h):
     * row 10 col 10 'w', row 25 col 10 'g', row 7 col 10 '#'. */
    CHECK(pix(137, 116) == 119, "O2 highlighted FACE pixel is #777777 (ramp idx 119)");
    CHECK(pix(137, 131) == 63,  "O2 highlighted SHADE pixel is #3F3F3F (ramp idx 63)");
    CHECK(pix(137, 113) == 0,   "O2 the INK outline stays black");
    /* README.TXT (59,106) is NOT a target: its face stays white (idx 1). The
     * document strike's body: row 10 col 12 is white (finder_icons.h DOC). */
    CHECK(pix(59 + 12, 106 + 10) == 1, "O2 a non-target icon keeps its white face");

    nx = finder_ops_resolve(&S.sh, ROOT, 0, 300, 250);
    CHECK(finder_ops_track_hilite(&S.sh, &cur, &nx) == 1, "O2 leaving the folder changes the highlight");
    CHECK(rw->view.icons[1].hilite == 0, "O2 ... and clears it");
    memset(S.px, 0xEE, sizeof S.px);
    finder_win_paint(rw, &S.bm, NULL);
    CHECK(pix(137, 116) == 1 && pix(137, 131) == 6,
          "O2 the un-highlighted folder is white face / #C0C0C0 shade again");

    nx = finder_ops_resolve(&S.sh, ROOT, 0, 600, 420);
    (void)finder_ops_track_hilite(&S.sh, &cur, &nx);
    CHECK(S.sh.desk.icons[1].hilite == 1, "O2 the desktop Trash highlights too");
}

/* ===========================================================================
 * O3 -- MOVE INTO A FOLDER ICON
 * ===========================================================================*/
static void leg_into_folder(void)
{
    finder_tgt_t t;
    finder_move_result_t r;
    finder_window_t *rw;
    finder_win_status_t st;

    scene_init(&S);
    rw = &S.sh.windows[ROOT];
    t  = finder_ops_resolve(&S.sh, ROOT, 0, 143, 122);
    st = finder_ops_drop(&S.sh, ROOT, 0, &t, 127, 106, &r);
    CHECK(st == FINDER_WIN_OK && r.status == FINDER_WIN_OK && r.op == FINDER_OP_MOVE,
          "O3 README.TXT dropped on APPS commits a MOVE");
    CHECK(r.from_dir == 0u && r.to_dir == CL_APPS && strcmp(r.name, "README.TXT") == 0,
          "O3 the result names the item, from=0 and to=APPS");
    CHECK(mv_has(&S.mock, CL_APPS, "README.TXT") && !mv_has(&S.mock, CL_ROOT, "README.TXT"),
          "O3 the VOLUME now has README.TXT in APPS and not in the root");
    CHECK(rw->view.n == 1 && idx_of(&rw->view, "README.TXT") < 0,
          "O3 the root window lost exactly that icon (2 shown -> 1; tdnl.56)");
    CHECK(rw->view.icons[0].x == 127 && rw->view.icons[0].y == 106,
          "O3 the remaining icons do NOT re-flow (APPS stays at (127,106))");
    CHECK(r.dst_slot == APPW && idx_of(&S.sh.windows[APPW].view, "README.TXT") == 1,
          "O3 the open APPS window shows the arrival");
    CHECK(S.sh.windows[APPW].view.icons[1].x == 107 && S.sh.windows[APPW].view.icons[1].y == 86,
          "O3 ... in the next free grid cell (39+68, 86)");
    CHECK(S.sh.n_origins == 0u && r.db_dirty == 0u, "O3 a plain move records no origin");
}

/* ===========================================================================
 * O4 -- MOVE BETWEEN WINDOWS
 * ===========================================================================*/
static void leg_between(void)
{
    finder_tgt_t t;
    finder_move_result_t r;
    finder_window_t *aw, *rw;
    int k;

    scene_init(&S);
    aw = &S.sh.windows[APPW]; rw = &S.sh.windows[ROOT];
    /* drag TENANTFX.EXE out of the APPS window into the ROOT window body */
    t = finder_ops_resolve(&S.sh, APPW, 0, 300, 250);
    CHECK(t.kind == FINDER_TGT_WINDOW && t.slot == ROOT && t.dir == 0u,
          "O4 the root window body is a WINDOW target for an APPS icon");
    /* RE-KEYED (tdnl.35, stated): the drop point was (284,234); its 47-row
     * cell (to 281) now crosses ROOT's content bottom 279 (the horizontal
     * scroll bar is no longer content), so it would be clamped and this leg
     * would stop grading "no clamp". (284,214) keeps the cell inside. */
    CHECK(finder_ops_drop(&S.sh, APPW, 0, &t, 284, 214, &r) == FINDER_WIN_OK,
          "O4 the drop commits");
    CHECK(mv_has(&S.mock, CL_ROOT, "TENANTFX.EXE") && !mv_has(&S.mock, CL_APPS, "TENANTFX.EXE"),
          "O4 the volume moved TENANTFX.EXE from APPS to the root");
    CHECK(aw->view.n == 0u, "O4 the APPS window is empty");
    k = idx_of(&rw->view, "TENANTFX.EXE");
    CHECK(k == 2 && rw->view.icons[k].kind == FINDER_ICON_APP,
          "O4 the root window gained it as an APPLICATION icon");
    CHECK(rw->view.icons[k].x == 284 && rw->view.icons[k].y == 214,
          "O4 ... exactly where it was dropped (content-relative, no clamp)");
}

/* ===========================================================================
 * O5 -- THE REFUSAL LADDER (and nothing changes)
 * ===========================================================================*/
static void refused(const finder_tgt_t *t, int slot, int idx,
                    finder_win_status_t want, const char *reason, const char *why)
{
    finder_move_result_t r;
    uint16_t n0 = S.sh.windows[slot].view.n;
    int moves0 = S.mock.move_calls;
    int nroot = S.mock.d[0].n, napps = S.mock.d[1].n, ntr = S.mock.d[2].n;
    finder_win_status_t st = finder_ops_drop(&S.sh, slot, idx, t, 0, 0, &r);

    CHECK(st == want && r.status == want, why);
    CHECK(strcmp(finder_ops_reason(st), reason) == 0, "O5 the reason string is the locked spelling");
    CHECK(S.sh.windows[slot].view.n == n0, "O5 a refusal leaves the window model untouched");
    CHECK(S.mock.d[0].n == nroot && S.mock.d[1].n == napps && S.mock.d[2].n == ntr,
          "O5 a refusal leaves the volume untouched");
    (void)moves0;
}

static void leg_ladder(void)
{
    finder_tgt_t t;

    scene_init(&S);
    t = finder_ops_resolve(&S.sh, ROOT, 0, 600, 24);          /* volume = root */
    refused(&t, ROOT, 0, FINDER_WIN_ERR_SAMEDIR, "samedir",
            "O5 a root item onto the volume icon is refused SAMEDIR");
    CHECK(S.mock.move_calls == 0, "O5 samedir is decided by the Finder, before the backend");

    /* cycle: the APPS folder dropped into the APPS window body */
    t = finder_ops_resolve(&S.sh, ROOT, 1, 30, 200);
    CHECK(t.kind == FINDER_TGT_WINDOW && t.dir == CL_APPS, "O5 (setup) the APPS window is the target");
    refused(&t, ROOT, 1, FINDER_WIN_ERR_CYCLE, "cycle",
            "O5 a folder dropped into ITSELF is refused CYCLE");
    CHECK(S.mock.move_calls == 0, "O5 the folder-into-itself rung never reaches the backend");

    /* exists: a README.TXT already waits in APPS */
    mv_add(&S.mock.d[1], "README.TXT", 0x20u, 8);
    t = finder_ops_resolve(&S.sh, ROOT, 0, 143, 122);
    refused(&t, ROOT, 0, FINDER_WIN_ERR_EXISTS, "exists",
            "O5 a taken name at the destination is refused EXISTS");
    mv_del(&S.mock.d[1], 1);

    /* err: the backend fails for any other reason */
    S.mock.force_move_rc = (int)FINDER_WIN_ERR_MOVE;
    refused(&t, ROOT, 0, FINDER_WIN_ERR_MOVE, "err", "O5 any other backend refusal is ERR");
    S.mock.force_move_rc = 0;

    /* err: a volume with no \TRASH refuses the Trash drop loudly */
    S.mock.no_trash = 1;
    t = finder_ops_resolve(&S.sh, ROOT, 0, 600, 420);
    refused(&t, ROOT, 0, FINDER_WIN_ERR_MOVE, "err", "O5 no \\TRASH -> the Trash drop is ERR, never a guess");
    S.mock.no_trash = 0;
}

/* ===========================================================================
 * O12 -- THE IDENTITY GUARD (beads initech-tdnl.73 / .75; audit pass 3 H03 +
 * H05). The root \TRASH and \DESKTOP.DB are not listed (test-finder-windows
 * L5b), so no gesture can press them -- but the move/trash path must refuse
 * them BY IDENTITY even if a model reaches them (a stale view, a later
 * command): reason "service", the backend never called, nothing changes.
 * Nor may a move CREATE one: a user's TRASH folder dropped into the root.
 * Mutant FINDER_OPS_MUT_NO_SERVICE_GUARD goes RED here.
 * ===========================================================================*/
static void leg_service_guard(void)
{
    finder_tgt_t t;
    finder_desk_t *rv;
    int ti, di;

    scene_init(&S);
    rv = &S.sh.windows[ROOT].view;
    CHECK(idx_of(rv, "TRASH") < 0 && idx_of(rv, "DESKTOP.DB") < 0 && rv->n == 2u,
          "O12 (setup) the root view lists neither TRASH nor DESKTOP.DB");
    /* A path that reaches them anyway: append both to the root model, as the
     * pre-tdnl.56 listing had them (cells 2 and 3). */
    ti = finder_desk_add(rv, FINDER_ICON_FOLDER, "TRASH", 243, 86, 1u);
    finder_desk_set_cluster(rv, ti, CL_TRASH);
    di = finder_desk_add(rv, FINDER_ICON_FILE, "DESKTOP.DB", 175, 86, 1u);
    CHECK(ti >= 0 && di >= 0, "O12 (setup) the stale entries are in the model");

    /* the audit's H03 move: \TRASH dropped onto the APPS folder icon */
    t = finder_ops_resolve(&S.sh, ROOT, ti, 143, 122);
    CHECK(t.kind == FINDER_TGT_FOLDER && t.dir == CL_APPS, "O12 (setup) APPS is the target");
    refused(&t, ROOT, ti, FINDER_WIN_ERR_SERVICE, "service",
            "O12 the root TRASH dropped on APPS is refused SERVICE (H03)");
    /* the audit's H05 move: \DESKTOP.DB dropped onto the desktop Trash */
    t = finder_ops_resolve(&S.sh, ROOT, di, 600, 420);
    CHECK(t.kind == FINDER_TGT_TRASH, "O12 (setup) the Trash is the target");
    refused(&t, ROOT, di, FINDER_WIN_ERR_SERVICE, "service",
            "O12 the root DESKTOP.DB dropped on the Trash is refused SERVICE (H05)");
    CHECK(S.mock.move_calls == 0 && S.mock.unlink_calls == 0,
          "O12 the service refusals never reach the backend");
    CHECK(mv_has(&S.mock, CL_ROOT, "TRASH") && mv_has(&S.mock, CL_ROOT, "DESKTOP.DB") &&
          !mv_has(&S.mock, CL_APPS, "TRASH") && !mv_has(&S.mock, CL_TRASH, "DESKTOP.DB"),
          "O12 the volume still holds \\TRASH and \\DESKTOP.DB at the root");

    /* the DESTINATION identity: a user's own TRASH folder inside APPS dragged
     * into the root window body would become a second root TRASH */
    scene_init(&S);
    mv_add(&S.mock.d[1], "TRASH", 0x10u, CL_NEWFOLD);
    (void)finder_win_populate(&S.sh, APPW);
    ti = idx_of(&S.sh.windows[APPW].view, "TRASH");
    CHECK(ti >= 0, "O12 (setup) a TRASH folder inside APPS IS listed (not the service)");
    t = finder_ops_resolve(&S.sh, APPW, ti, 300, 250);
    CHECK(t.kind == FINDER_TGT_WINDOW && t.dir == CL_ROOT, "O12 (setup) the root window body is the target");
    refused(&t, APPW, ti, FINDER_WIN_ERR_SERVICE, "service",
            "O12 a move that would CREATE a root TRASH is refused SERVICE");
    CHECK(S.mock.move_calls == 0, "O12 ... before the backend");
}

/* ===========================================================================
 * O6 -- TRASH STAGING
 * ===========================================================================*/
static void leg_trash(void)
{
    finder_tgt_t t;
    finder_move_result_t r;

    scene_init(&S);
    t = finder_ops_resolve(&S.sh, ROOT, 0, 600, 420);
    CHECK(finder_ops_drop(&S.sh, ROOT, 0, &t, 0, 0, &r) == FINDER_WIN_OK && r.op == FINDER_OP_TRASH,
          "O6 README.TXT dropped on the Trash is STAGED");
    CHECK(mv_has(&S.mock, CL_TRASH, "README.TXT"),
          "O6 the volume holds README.TXT inside \\TRASH (transplanted, NOT deleted)");
    CHECK(!mv_has(&S.mock, CL_ROOT, "README.TXT"), "O6 ... and no longer in the root");
    CHECK(S.mock.unlink_calls == 0, "O6 staging never deletes");
    CHECK(r.renamed == 0u && strcmp(r.as, "README.TXT") == 0, "O6 no collision -> no rename");
    CHECK(S.sh.n_origins == 1u && S.sh.origins[0].dir_start == 0u &&
          strcmp(S.sh.origins[0].name83, "README.TXT") == 0 && S.sh.origins[0].flags == 0u &&
          r.db_dirty == 1u,
          "O6 a kind=5 origin {dir 0, README.TXT, flags 0} is recorded");

    /* collision: a NEWFOLD is already in the Trash; trash another one */
    mv_add(&S.mock.d[2], "NEWFOLD", 0x10u, 10);
    mv_add(&S.mock.d[0], "NEWFOLD", 0x10u, CL_NEWFOLD);
    (void)finder_win_populate(&S.sh, ROOT);
    {
        int k = idx_of(&S.sh.windows[ROOT].view, "NEWFOLD");
        CHECK(k >= 0, "O6 (setup) the second NEWFOLD is listed");
        CHECK(finder_ops_drop(&S.sh, ROOT, k, &t, 0, 0, &r) == FINDER_WIN_OK,
              "O6 a colliding folder still stages");
    }
    CHECK(r.renamed == 1u && strcmp(r.as, "NEWFO001") == 0,
          "O6 the collision is suffixed trunc5+001 -> NEWFO001 (TRASH-RENAME)");
    CHECK(mv_has(&S.mock, CL_TRASH, "NEWFO001") && mv_has(&S.mock, CL_TRASH, "NEWFOLD"),
          "O6 the Trash now holds BOTH folders");
    CHECK(S.sh.n_origins == 2u && strcmp(S.sh.origins[1].name83, "NEWFO001") == 0 &&
          S.sh.origins[1].flags == FINDER_ORIGIN_FLAG_RENAMED,
          "O6 the second origin carries the staged name and the renamed flag");

    /* A target whose directory IS \TRASH stages exactly like the desktop
     * Trash (origin recorded), never a plain move. RE-KEYED at tdnl.56: the
     * root's own TRASH folder icon is no longer listed, so the property is
     * graded through a WINDOW on \TRASH (the shape the desktop Trash's Open
     * will take) with the APPS folder dropped into its body. */
    {
        int slot = -1, k = idx_of(&S.sh.windows[ROOT].view, "APPS");
        CHECK(finder_win_open(&S.sh, CL_TRASH, "TRASH", 0u, &slot, NULL) == FINDER_WIN_OK,
              "O6 (setup) a window on \\TRASH opens (front)");
        t = finder_ops_resolve(&S.sh, ROOT, k, 300, 250);
        CHECK(t.kind == FINDER_TGT_WINDOW && t.dir == CL_TRASH, "O6 (setup) the \\TRASH window body is a window target");
        CHECK(finder_ops_drop(&S.sh, ROOT, k, &t, 300, 250, &r) == FINDER_WIN_OK &&
              r.op == FINDER_OP_TRASH && S.sh.n_origins == 3u &&
              mv_has(&S.mock, CL_TRASH, "APPS"),
              "O6 a drop into the \\TRASH window is Trash STAGING (origin recorded)");
    }
}


/* ===========================================================================
 * O7 -- THE kind=5 CODEC
 * ===========================================================================*/
static const uint8_t DB7[] = {
    'I','D','B','1', 0x01,0x00, 0x03,0x00,
    /* volume-pos (584,8) */
    0x01,0x00, 0x00,0x00, 'V','O','L',0,0,0,0,0,0,0,0,0,0, 0x48,0x02, 0x08,0x00, 0x00,0x00, 0x00,
    /* trash-pos (584,404) */
    0x02,0x00, 0x00,0x00, 'T','r','a','s','h',0,0,0,0,0,0,0,0, 0x48,0x02, 0x94,0x01, 0x00,0x00, 0x00,
    /* trash-origin: flags 1, origin dir 2, name NEWFO001 */
    0x05,0x01, 0x02,0x00, 'N','E','W','F','O','0','0','1',0,0,0,0,0, 0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,
};

static void leg_codec(void)
{
    uint8_t buf[FINDER_DB_MAX_BYTES];
    finder_origin_rec_t o, back[4];
    finder_desk_t fd; finder_desk_icon_t ic[4];
    uint32_t n;

    finder_desk_init(&fd, ic, 4, mk_rect(0, 0, 480, 640));
    finder_desk_seed_defaults(&fd);
    finder_desk_set_name(&fd, 0, "VOL");
    memset(&o, 0, sizeof o);
    o.dir_start = 2; o.flags = FINDER_ORIGIN_FLAG_RENAMED; strcpy(o.name83, "NEWFO001");
    memset(buf, 0xCC, sizeof buf);
    n = finder_desk_db_encode_full(&fd, NULL, 0, &o, 1, buf, sizeof buf);
    CHECK(n == sizeof DB7 && memcmp(buf, DB7, sizeof DB7) == 0,
          "O7 encode_full == the hand-authored 80-byte image (icons, then kind=5)");
    CHECK(finder_desk_db_get_origins(DB7, sizeof DB7, back, 4) == 1 &&
          back[0].dir_start == 2 && back[0].flags == 1 && strcmp(back[0].name83, "NEWFO001") == 0,
          "O7 the kind=5 record decodes back");
    scene_init(&S);
    CHECK(finder_shell_load_origins(&S.sh, DB7, sizeof DB7) == 1 && S.sh.n_origins == 1,
          "O7 the shell reloads its origin set at boot");
    CHECK(finder_desk_db_encode_full(&fd, NULL, 0, &o, 1, buf, 79u) == 0u,
          "O7 a short buffer is refused (fail loud)");

    /* a staged item reaches the encoder (end-to-end: drop -> origins -> bytes) */
    scene_init(&S);
    {
        finder_tgt_t t = finder_ops_resolve(&S.sh, ROOT, 0, 600, 420);
        finder_move_result_t r;
        (void)finder_ops_drop(&S.sh, ROOT, 0, &t, 0, 0, &r);
    }
    n = finder_desk_db_encode_full(&S.sh.desk, NULL, 0, S.sh.origins, S.sh.n_origins,
                                   buf, sizeof buf);
    CHECK(n == 80u && buf[56] == 0x05 && buf[57] == 0x00 && buf[58] == 0x00 &&
          memcmp(buf + 60, "README.TXT", 11) == 0,
          "O7 the staged README.TXT serialises as kind=5 {flags 0, dir 0, README.TXT}");
}

/* ===========================================================================
 * O8 -- OUT OF THE TRASH AGAIN
 * ===========================================================================*/
static void leg_untrash(void)
{
    finder_tgt_t t;
    finder_move_result_t r;
    int slot = -1, single = 0, k;

    scene_init(&S);
    t = finder_ops_resolve(&S.sh, ROOT, 0, 600, 420);
    (void)finder_ops_drop(&S.sh, ROOT, 0, &t, 0, 0, &r);
    CHECK(S.sh.n_origins == 1u, "O8 (setup) one staged item");
    (void)finder_win_close(&S.sh, APPW);
    CHECK(finder_win_open(&S.sh, CL_TRASH, "TRASH", 0u, &slot, &single) == FINDER_WIN_OK,
          "O8 (setup) open the TRASH window");
    k = idx_of(&S.sh.windows[slot].view, "README.TXT");
    CHECK(k >= 0, "O8 (setup) the Trash window lists README.TXT");
    t.kind = FINDER_TGT_VOLUME; t.slot = -1; t.idx = 0; t.dir = 0u;
    CHECK(finder_ops_drop(&S.sh, slot, k, &t, 0, 0, &r) == FINDER_WIN_OK && r.op == FINDER_OP_MOVE,
          "O8 dragging it onto the volume is a plain move back to the root");
    CHECK(mv_has(&S.mock, CL_ROOT, "README.TXT") && S.sh.n_origins == 0u && r.db_dirty == 1u,
          "O8 ... and its origin record is dropped");
}

/* ===========================================================================
 * O13 -- THE TRASH WINDOW (bead initech-tdnl.39; audit F04). Hand-derived
 * from finder_windows.h Sec 2/3 + CalcDocContentRect: the scene's two windows
 * hold slots 0 and 1, so the Trash window takes slot 2 at the cascaded frame
 * (20+2*20, 60+2*20) = (60,100)..(420,320), content (61,122)..(399,299); its
 * cell 0 sprite is (61+18, 122+4) = (79,126).
 *   - it lists EXACTLY the staged items (README.TXT, a document), titled
 *     "Trash", flagged is_trash, keyed by \TRASH's cluster;
 *   - the root window still hides TRASH (the 37334ee rule is untouched);
 *   - a second open raises it (one window over \TRASH, singleton=1);
 *   - dragging README.TXT out of it onto the volume puts it back: a plain
 *     move to the root, the kind=5 origin dropped, the Trash window empty;
 *     the open root window shows it in the LOWEST FREE cell (59,106) -- not
 *     on APPS, which kept cell 1 (127,106) when README left (no re-flow);
 *   - a volume with no \TRASH refuses the open (ERR_NOTRASH), no window.
 * MUTANTS: FINDER_WIN_MUT_TRASH_ROOT (the window lists the root) and
 * FINDER_OPS_MUT_COUNT_CELL (the put-back lands on APPS) go RED here.
 * ===========================================================================*/
static int count_open(void)
{
    int n = 0;
    for (int i = 0; i < FINDER_WIN_MAX; i++) n += S.sh.windows[i].open ? 1 : 0;
    return n;
}

static void leg_trash_window(void)
{
    finder_tgt_t t;
    finder_move_result_t r;
    finder_window_t *tw, *rw;
    int slot = -1, single = -1, slot2 = -1, k, ntrash = 0;

    scene_init(&S);
    rw = &S.sh.windows[ROOT];
    t = finder_ops_resolve(&S.sh, ROOT, 0, 600, 420);
    CHECK(finder_ops_drop(&S.sh, ROOT, 0, &t, 0, 0, &r) == FINDER_WIN_OK &&
          S.sh.n_origins == 1u, "O13 (setup) README.TXT staged into \\TRASH");

    CHECK(finder_win_open_trash(&S.sh, &slot, &single) == FINDER_WIN_OK &&
          slot == 2 && single == 0, "O13 the Trash opens a NEW window in slot 2");
    tw = &S.sh.windows[2];
    CHECK(tw->open && tw->is_trash == 1u && tw->is_root == 0u &&
          strcmp(tw->rec.titleHandle, "Trash") == 0,
          "O13 the window is the Trash window, titled \"Trash\"");
    CHECK(tw->dir_start == CL_TRASH && tw->view.n == 1u &&
          strcmp(tw->view.icons[0].name, "README.TXT") == 0 &&
          tw->view.icons[0].kind == FINDER_ICON_FILE,
          "O13 the Trash window lists exactly the staged README.TXT");
    CHECK(tw->view.icons[0].x == 79 && tw->view.icons[0].y == 126,
          "O13 ... in cell 0 of the slot-2 content, sprite (79,126)");
    CHECK(rw->view.n == 1u && idx_of(&rw->view, "TRASH") < 0 &&
          idx_of(&rw->view, "DESKTOP.DB") < 0,
          "O13 the root window still lists neither TRASH nor DESKTOP.DB");

    CHECK(finder_win_open_trash(&S.sh, &slot2, &single) == FINDER_WIN_OK &&
          slot2 == 2 && single == 1, "O13 a second open RAISES the Trash window");
    for (int i = 0; i < FINDER_WIN_MAX; i++)
        if (S.sh.windows[i].open && S.sh.windows[i].dir_start == CL_TRASH) ntrash++;
    CHECK(ntrash == 1 && count_open() == 3, "O13 ... and there is still ONE window over \\TRASH");

    /* PUT BACK: README.TXT dragged out of the Trash window onto the volume. */
    t = finder_ops_resolve(&S.sh, 2, 0, 600, 24);
    CHECK(t.kind == FINDER_TGT_VOLUME, "O13 (setup) the volume icon is the target");
    CHECK(finder_ops_drop(&S.sh, 2, 0, &t, 0, 0, &r) == FINDER_WIN_OK &&
          r.op == FINDER_OP_MOVE && r.from_dir == CL_TRASH && r.to_dir == 0u,
          "O13 dragging it out onto the volume is a plain MOVE from \\TRASH to the root");
    CHECK(mv_has(&S.mock, CL_ROOT, "README.TXT") && !mv_has(&S.mock, CL_TRASH, "README.TXT"),
          "O13 the volume has README.TXT back in the root, gone from \\TRASH");
    CHECK(S.sh.n_origins == 0u && r.db_dirty == 1u,
          "O13 its kind=5 origin is dropped (DB dirty)");
    CHECK(tw->view.n == 0u, "O13 the Trash window is empty");
    k = idx_of(&rw->view, "README.TXT");
    CHECK(k >= 0 && rw->view.icons[k].x == 59 && rw->view.icons[k].y == 106,
          "O13 the root window shows it in the lowest FREE cell (59,106), not on APPS");
    CHECK(rw->view.icons[idx_of(&rw->view, "APPS")].x == 127,
          "O13 ... and APPS kept its cell (127,106)");

    /* No \TRASH on the volume: refused loudly, nothing opened. */
    (void)finder_win_close(&S.sh, 2);
    S.mock.no_trash = 1;
    slot = 7;
    CHECK(finder_win_open_trash(&S.sh, &slot, &single) == FINDER_WIN_ERR_NOTRASH &&
          slot == -1 && count_open() == 2,
          "O13 a volume with no \\TRASH refuses the open (ERR_NOTRASH), no window");
    S.mock.no_trash = 0;
}

/* ===========================================================================
 * O14 -- EMPTY TRASH (bead initech-6k12). Hand-authored throughout:
 *   - the count is every item at every depth: README.TXT (114 B), the APPS
 *     folder, and APPS\TENANTFX.EXE (684 B) = 3 items, 798 B;
 *   - Special > Empty Trash (menu 515 item 2) through the ONE spine reports 3
 *     and deletes NOTHING -- the purge is the alert's OK, not the dispatch;
 *   - the alert's words, wrapped by hand at 285 px from the corpus Chicago 12
 *     advances (../system7-decomp specs/fonts/chicago.md):
 *       "The Trash contains 3 items, which use 1K"   269 px
 *       "of disk space. Are you sure you want to"    262 px
 *       "remove these items permanently?"            228 px
 *   - the purge is DEPTH-FIRST: unlink README.TXT in \TRASH (3), descend into
 *     APPS (2), unlink TENANTFX.EXE, back up, remove APPS from \TRASH;
 *   - an item that will not go is refused, and so is the folder holding it.
 * MUTANT: FINDER_WIN_MUT_EMPTY_NO_CONFIRM (the purge at dispatch) goes RED.
 * ===========================================================================*/
static void leg_empty_trash(void)
{
    FinderCtx fx;
    const finder_cmd_outcome_t *o;
    finder_tgt_t t;
    finder_move_result_t r;
    uint16_t purged = 9u, refused = 9u;
    uint8_t dirty = 0u;
    int slot = -1, k;
    char lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX];

    scene_init(&S);
    S.mock.d[0].e[0].size = 114u;            /* README.TXT                   */
    S.mock.d[1].e[0].size = 684u;            /* APPS\TENANTFX.EXE            */
    memset(&fx, 0, sizeof fx);
    finder_shell_bind_ctx(&S.sh, &fx);
    CHECK(finder_shell_recount_trash(&S.sh) == 0 && S.sh.trash_items == 0u &&
          S.sh.desk.trash_full == 0u && fx.trash_nonempty == 0u,
          "O14 an empty Trash: no items, the EMPTY icon, Empty Trash dark");

    t = finder_ops_resolve(&S.sh, ROOT, 0, 600, 420);
    CHECK(finder_ops_drop(&S.sh, ROOT, 0, &t, 0, 0, &r) == FINDER_WIN_OK &&
          S.sh.trash_items == 1u && S.sh.desk.trash_full == 1u &&
          S.sh.trash_icon_dirty == 1u,
          "O14 the first staged item turns the Trash FULL (icon flagged for the pump)");
    /* The painter draws the FULL strike: its lid row 2 runs ink from col 10
     * (desk_icons.h FULL map), where the EMPTY strike's row 2 is clear (only
     * the handle, cols 13..18). Trash sprite at (584,404): pixel (594,406). */
    memset(S.px, 0xEE, sizeof S.px);
    finder_desk_paint(&S.sh.desk, &S.bm, NULL);
    CHECK(pix(594, 406) == 0, "O14 the desktop Trash is PAINTED full (lid ink at row 2 col 10)");
    S.sh.trash_icon_dirty = 0u;
    k = idx_of(&S.sh.windows[ROOT].view, "APPS");
    t = finder_ops_resolve(&S.sh, ROOT, k, 600, 420);
    CHECK(finder_ops_drop(&S.sh, ROOT, k, &t, 0, 0, &r) == FINDER_WIN_OK &&
          S.sh.trash_items == 3u && S.sh.trash_bytes == 798u &&
          S.sh.trash_icon_dirty == 0u,
          "O14 the Trash counts 3 items at every depth, 798 bytes; still FULL, no flip");
    finder_shell_sync_ctx(&S.sh);
    CHECK(fx.trash_nonempty == 1u, "O14 TRASH_NONEMPTY follows the count");

    finder_dispatch(&fx, ((uint32_t)515u << 16) | 2u, "mouse");
    o = finder_shell_take_outcome(&S.sh);
    CHECK(o != NULL && o->id == FCMD_EMPTY_TRASH && o->status == FINDER_WIN_OK &&
          o->moved == 3, "O14 Special > Empty Trash reaches the shell and reports 3 items");
    CHECK(S.mock.unlink_calls == 0 && mv_has(&S.mock, CL_TRASH, "README.TXT") &&
          mv_has(&S.mock, CL_TRASH, "APPS") && mv_has(&S.mock, CL_APPS, "TENANTFX.EXE"),
          "O14 dispatch alone deletes nothing -- the purge waits for the alert's OK");

    CHECK(finder_trash_alert_text(3u, 798u, lines) == 3 &&
          strcmp(lines[0], "The Trash contains 3 items, which use 1K") == 0 &&
          strcmp(lines[1], "of disk space. Are you sure you want to") == 0 &&
          strcmp(lines[2], "remove these items permanently?") == 0 && lines[3][0] == '\0',
          "O14 the alert's words, wrapped at 285 px of Chicago 12 (hand-derived)");
    CHECK(finder_trash_alert_text(1u, 114u, lines) == 3 &&
          strcmp(lines[0], "The Trash contains 1 item, which uses 1K") == 0 &&
          strcmp(lines[1], "of disk space. Are you sure you want to") == 0 &&
          strcmp(lines[2], "remove this item permanently?") == 0,
          "O14 the singular agreement (1 item, which uses ... this item)");

    CHECK(finder_win_open_trash(&S.sh, &slot, NULL) == FINDER_WIN_OK &&
          S.sh.windows[slot].view.n == 2u,
          "O14 (setup) the Trash window lists README.TXT and APPS");
    S.mock.log[0] = '\0';
    CHECK(finder_shell_empty_trash(&S.sh, &purged, &refused, &dirty) == FINDER_WIN_OK &&
          purged == 3u && refused == 0u && dirty == 1u,
          "O14 OK purges 3, refuses 0, and drops the origin records (DB dirty)");
    CHECK(strcmp(S.mock.log, "U:README.TXT@3 U:TENANTFX.EXE@2 R:APPS@3 ") == 0,
          "O14 the purge is DEPTH-FIRST: README.TXT, then APPS's file, then APPS");
    CHECK(S.mock.d[2].n == 0 && S.mock.d[1].n == 0 && S.sh.n_origins == 0u,
          "O14 \\TRASH is empty, the folder's contents are gone, no origin is left");
    CHECK(S.sh.windows[slot].view.n == 0u && S.sh.trash_items == 0u &&
          S.sh.desk.trash_full == 0u && S.sh.trash_icon_dirty == 1u,
          "O14 the open Trash window empties and the icon flips back to EMPTY");
    finder_shell_sync_ctx(&S.sh);
    CHECK(fx.trash_nonempty == 0u, "O14 Empty Trash goes dark again");
    memset(S.px, 0xEE, sizeof S.px);
    finder_desk_paint(&S.sh.desk, &S.bm, NULL);
    CHECK(pix(594, 406) == 0xEE && pix(597, 406) == 0,
          "O14 ... and is PAINTED empty again (row 2: only the handle, cols 13..18)");

    /* An item that will not go. */
    scene_init(&S);
    mv_add(&S.mock.d[0], "NEWFOLD", 0x10u, CL_NEWFOLD);
    mv_add(&S.mock.d[3], "STUCK.TXT", 0x20u, 11);
    (void)finder_win_populate(&S.sh, ROOT);
    k = idx_of(&S.sh.windows[ROOT].view, "NEWFOLD");
    t = finder_ops_resolve(&S.sh, ROOT, k, 600, 420);
    (void)finder_ops_drop(&S.sh, ROOT, k, &t, 0, 0, &r);
    k = idx_of(&S.sh.windows[ROOT].view, "README.TXT");
    (void)finder_ops_drop(&S.sh, ROOT, k, &t, 0, 0, &r);
    CHECK(S.sh.trash_items == 3u && S.sh.n_origins == 2u,
          "O14 (setup) NEWFOLD (holding STUCK.TXT) and README.TXT staged");
    S.mock.refuse = "STUCK.TXT";
    CHECK(finder_shell_empty_trash(&S.sh, &purged, &refused, &dirty) == FINDER_WIN_OK &&
          purged == 1u && refused == 2u,
          "O14 a file that will not go is REFUSED, and so is its folder (purged=1 refused=2)");
    CHECK(mv_has(&S.mock, CL_TRASH, "NEWFOLD") && mv_has(&S.mock, CL_NEWFOLD, "STUCK.TXT") &&
          !mv_has(&S.mock, CL_TRASH, "README.TXT") && S.sh.n_origins == 1u &&
          strcmp(S.sh.origins[0].name83, "NEWFOLD") == 0 &&
          S.sh.trash_items == 2u && S.sh.desk.trash_full == 1u,
          "O14 the refused folder keeps its origin record and the Trash stays FULL");
    S.mock.refuse = NULL;
}

/* ===========================================================================
 * O9 -- THE ICONS FOLLOW THE WINDOW (bead initech-tdnl.34; audit F01)
 *
 * Every expectation is HAND-DERIVED from the window's NEW content origin plus
 * the grid (finder_windows.h Sec 2: inset 18/4, pitch 68x52) -- never read
 * back out of the model under test. CalcDocContentRect(frame) = (left+1,
 * top+22) .. (right-21, bottom-21) (spec/chrome_metrics.h: frame 1, title 22,
 * body bar 4, scroll bar 16; the bottom edge stops at the horizontal scroll
 * bar since bead initech-tdnl.35 -- RE-KEYED from bottom-1, stated).
 *   ROOT moved to (140,140): content (141,162)..(479,339)
 *     README.TXT sprite (141+18, 162+4)      = (159,166)
 *     APPS       sprite (141+18+68, 162+4)   = (227,166)
 *   zoomed (window.c ZoomWindow over the 640x480 desktop, margins 4/40/4/4):
 *     frame (4,40)..(636,476), content (5,62)..(615,475)
 *     APPS sprite (5+18+68, 62+4) = (91,66)
 * Every consumer of the stored cells is graded: paint, hit-test, the rubber
 * band, the cell rect (the drag outline's start + the zoom-back destination),
 * the clamp + drag commit, Clean Up, New-Folder re-populate, grow, zoom,
 * restore, collapse and expand.
 * MUTANT: FINDER_WIN_MUT_ABS_COORDS (the pre-fix absolute model) goes RED.
 * ===========================================================================*/
static void leg_follow(void)
{
    finder_window_t *rw;
    finder_desk_t *v;
    rgn_rect_t cell;
    int16_t x, y;
    rgn_rect_t oc, nc;

    scene_init(&S);
    rw = &S.sh.windows[ROOT];
    MoveWindow(&S.wm, &rw->rec, 140, 140);

    /* paint: the icons are drawn inside the NEW content, not at the old cells */
    memset(S.px, 0xEE, sizeof S.px);
    finder_win_paint(rw, &S.bm, NULL);
    CHECK(pix(227 + 10, 166 + 7) == 0 && pix(227 + 10, 166 + 10) == 1,
          "O9 paint: APPS is drawn at the moved cell (227,166) (ink row 7, face row 10)");
    CHECK(pix(127 + 10, 106 + 7) == 0xEE && pix(127 + 10, 106 + 10) == 0xEE,
          "O9 paint: nothing is drawn at APPS's OLD cell (127,106) outside the moved content");

    /* hit-test + selection through THE accessor every gesture uses */
    v = finder_win_view(&S.sh, ROOT);
    CHECK(v != NULL && finder_desk_hit(v, 243, 182) == 1,
          "O9 hit: a click on the moved APPS centre (243,182) hits APPS");
    CHECK(finder_desk_hit(v, 143, 122) == -1,
          "O9 hit: the OLD APPS centre (143,122) is not an icon any more");
    CHECK(finder_desk_marquee_select(v, finder_band_rect(220, 160, 265, 200)) == 1 &&
          v->icons[1].selected == 1 && v->icons[0].selected == 0,
          "O9 rubber band: a band around the moved APPS cell selects exactly APPS");
    finder_desk_deselect_all(v);

    /* the cell rect: the drag outline's start and the zoom-back destination */
    cell = finder_desk_cell_rect(v, 1);
    CHECK(cell.top == 166 && cell.left <= 227 && cell.right >= 227 + 32 &&
          cell.bottom == 166 + 47,
          "O9 cell rect (outline start / zoom-back home) is the moved cell, top 166, bottom 213");

    /* the clamp + a same-window drag commit hold the icon in the NEW content */
    x = 0; y = 0;
    finder_desk_clamp(v, &x, &y);
    CHECK(x == 141 && y == 162, "O9 clamp: the content's top-left is the moved (141,162)");
    CHECK(finder_desk_drag_commit(v, 0, 1000, 1000, &oc, &nc) == FINDER_DROP_MOVED &&
          v->icons[0].x == 479 - 32 && v->icons[0].y == 339 - 47,
          "O9 drag commit clamps into the moved content: README.TXT at (447,292)");

    /* Clean Up snaps onto the grid of the CURRENT content */
    CHECK(finder_win_cleanup(&S.sh, ROOT) == 1 &&
          rw->view.icons[0].x == 159 && rw->view.icons[0].y == 166 &&
          rw->view.icons[1].x == 227 && rw->view.icons[1].y == 166,
          "O9 Clean Up re-grids onto the moved content: README (159,166), APPS (227,166), moved=1");

    /* grow: the origin stays, the clamp follows the new size */
    SizeWindow(&S.wm, &rw->rec, 200, 150);   /* frame (140,140)..(340,290) */
    v = finder_win_view(&S.sh, ROOT);
    x = 1000; y = 1000;
    finder_desk_clamp(v, &x, &y);
    CHECK(v->icons[1].x == 227 && v->icons[1].y == 166 && x == 319 - 32 && y == 269 - 47,
          "O9 grow: icons keep their place; the clamp is the GROWN content (319,269)");
    SizeWindow(&S.wm, &rw->rec, 360, 220);   /* back to the default size */

    /* zoom, then restore */
    (void)ZoomWindow(&S.wm, &rw->rec);
    v = finder_win_view(&S.sh, ROOT);
    CHECK(finder_desk_hit(v, 91 + 16, 66 + 16) == 1 && v->icons[1].x == 91 && v->icons[1].y == 66,
          "O9 zoom: APPS follows the zoomed content to (91,66)");
    memset(S.px, 0xEE, sizeof S.px);
    finder_win_paint(rw, &S.bm, NULL);
    CHECK(pix(91 + 10, 66 + 7) == 0, "O9 zoom: APPS is PAINTED at (91,66)");
    (void)ZoomWindow(&S.wm, &rw->rec);
    v = finder_win_view(&S.sh, ROOT);
    CHECK(finder_desk_hit(v, 243, 182) == 1 && v->icons[1].x == 227,
          "O9 restore: APPS is back at (227,166)");

    /* collapse leaves the model alone; expand re-bases nothing it need not */
    (void)CollapseWindow(&S.wm, &rw->rec);
    v = finder_win_view(&S.sh, ROOT);
    CHECK(v->bounds.left == 141 && v->bounds.top == 162 && v->bounds.right == 479 &&
          v->bounds.bottom == 339 && v->icons[1].x == 227,
          "O9 collapse: an EMPTY content region does not collapse the model's bounds");
    (void)CollapseWindow(&S.wm, &rw->rec);
    v = finder_win_view(&S.sh, ROOT);
    CHECK(finder_desk_hit(v, 243, 182) == 1, "O9 expand: APPS is hit where it is drawn");

    /* New Folder (re-populate) lays the grid over the moved content */
    MoveWindow(&S.wm, &rw->rec, 40, 80);
    CHECK(finder_win_populate(&S.sh, ROOT) == FINDER_WIN_OK &&
          rw->view.icons[1].x == 127 && rw->view.icons[1].y == 106,
          "O9 re-populate after moving back to (40,80): APPS on the original grid (127,106)");
    MoveWindow(&S.wm, &rw->rec, 140, 140);
    CHECK(finder_win_populate(&S.sh, ROOT) == FINDER_WIN_OK &&
          rw->view.icons[1].x == 227 && rw->view.icons[1].y == 166,
          "O9 re-populate in the moved window: APPS on the moved grid (227,166)");

    /* a drop INTO a folder of the moved window (the emu gate's last leg) */
    {
        finder_tgt_t t = finder_ops_resolve(&S.sh, ROOT, 0, 243, 182);
        finder_move_result_t r;
        CHECK(t.kind == FINDER_TGT_FOLDER && t.idx == 1 &&
              finder_ops_drop(&S.sh, ROOT, 0, &t, 227, 166, &r) == FINDER_WIN_OK &&
              mv_has(&S.mock, CL_APPS, "README.TXT"),
              "O9 README.TXT dropped on the moved APPS moves into APPS");
    }
}

/* ===========================================================================
 * O10 -- SELECT ALL + ARRANGE (BY NAME) through THE command spine
 * (beads initech-tdnl.36 / .67; audit F03 / G10). Hand-derived: the root
 * window (slot 1, content (41,102)) lists README.TXT, APPS in that order (the
 * mock's DESKTOP.DB and TRASH are the Finder's own and hidden since tdnl.56);
 * by NAME (case-insensitive 8.3 order) it is APPS, README.TXT, so the cells
 * (59,106) (127,106) go to APPS, README.TXT -- both icons move.
 * MUTANT FINDER_WIN_MUT_ARRANGE_NOOP (rank by listing order) goes RED.
 * ===========================================================================*/
static void leg_select_arrange(void)
{
    FinderCtx fx;
    finder_window_t *rw;
    const finder_cmd_outcome_t *o;

    scene_init(&S);
    rw = &S.sh.windows[ROOT];
    memset(&fx, 0, sizeof fx);
    finder_shell_bind_ctx(&S.sh, &fx);

    finder_shell_sync_ctx(&S.sh);
    finder_dispatch(&fx, ((uint32_t)513u << 16) | 6u, "mouse");   /* Select All */
    o = finder_shell_take_outcome(&S.sh);
    CHECK(o != NULL && o->id == FCMD_SELECT_ALL && o->slot == ROOT && o->moved == 2 &&
          finder_desk_selection_count(&rw->view) == 2u,
          "O10 Edit > Select All selects BOTH shown icons of the front window (F03)");

    finder_shell_sync_ctx(&S.sh);
    finder_dispatch(&fx, ((uint32_t)514u << 16) | 12u, "mouse");  /* Arrange   */
    o = finder_shell_take_outcome(&S.sh);
    CHECK(o != NULL && o->id == FCMD_ARRANGE_BY_NAME && o->status == FINDER_WIN_OK &&
          o->moved == 2,
          "O10 View > Arrange (by Name) ran on the front window and moved 2 icons (G10)");
    CHECK(rw->view.n == 2u &&
          rw->view.icons[1].x == 59  && rw->view.icons[1].y == 106 &&   /* APPS       */
          rw->view.icons[0].x == 127 && rw->view.icons[0].y == 106,     /* README.TXT */
          "O10 the icons are in NAME order on the grid: APPS, README.TXT");

    /* An unimplemented command never executes silently: dispatched anyway
     * (the bar would never hand it out), the shell declines it. */
    CHECK(finder_shell_implements(FCMD_GET_INFO) == 0 &&
          finder_shell_implements(FCMD_RESTART) == 0 &&
          finder_shell_implements(FCMD_SELECT_ALL) == 1 &&
          finder_shell_implements(FCMD_ARRANGE_BY_NAME) == 1,
          "O10 the shell's execution table: Select All + Arrange yes, Get Info + Restart no");
}

/* ===========================================================================
 * O11 -- THE ICON VIEW SCROLLS (bead initech-tdnl.35; audit F02)
 *
 * The root gets 22 more files, F00.TXT..F21.TXT (24 SHOWN entries -- the
 * mock's DESKTOP.DB and TRASH are hidden since tdnl.56, so two more files
 * than before keep every number below -- 6 grid rows), re-populated. Every number by hand (finder_windows.h Sec 2/3/10c,
 * winscroll.h, CalcDocContentRect):
 *   ROOT frame (40,80)..(400,300); content (41,102)..(379,279): 177 high.
 *   rows at doc y 4 + 52r; last row r=5 -> cell bottom 4+260+47 = 311,
 *   + inset 4 = vertical content 315 -> max 315-177 = 138, page 177-16 = 161;
 *   horizontal: col 3 x 18+204 = 222, + 32 + 18 = 272 <= 338 -> max 0.
 *   F18.TXT is index 20 (row 5, col 0; README, APPS, F00.. -> F18 at 2+18): unscrolled sprite (59, 102+264=366),
 *   outside the content. Three down-arrow steps -> value 48 -> (59,318);
 *   a page down -> 48+161 clamps to 138 -> (59,228), cell 228..275 inside.
 * MUTANT: FINDER_WIN_MUT_SCROLL_PAINT_ONLY (paint shifts, the model does not)
 * draws F18 at (59,228) but the click there misses it -> "O11 hit" RED.
 * ===========================================================================*/
static void leg_scroll(void)
{
    finder_window_t *rw;
    finder_desk_t *v;
    char nm[14];
    int k;

    scene_init(&S);
    rw = &S.sh.windows[ROOT];
    for (int i = 0; i < 22; i++) {
        snprintf(nm, sizeof nm, "F%02d.TXT", i);
        mv_add(&S.mock.d[0], nm, 0x20u, (uint16_t)(20 + i));
    }
    CHECK(finder_win_populate(&S.sh, ROOT) == FINDER_WIN_OK && rw->view.n == 24u,
          "O11 scene: the root lists 24 entries");
    k = idx_of(&rw->view, "F18.TXT");
    CHECK(k == 20 && rw->view.icons[k].x == 59 && rw->view.icons[k].y == 366,
          "O11 F18.TXT laid out at (59,366), below the 177-px content");
    CHECK(WindowScrollOf(&S.wm, &rw->rec) == &rw->scroll &&
          rw->scroll.axis[WSCROLL_V].max == 138 &&
          rw->scroll.axis[WSCROLL_V].page == 161 &&
          rw->scroll.axis[WSCROLL_H].max == 0,
          "O11 the window's bars: vertical max 138 / page 161, horizontal DISABLED");

    for (int i = 0; i < 3; i++) (void)wscroll_step(&rw->scroll.axis[WSCROLL_V], 21);
    v = finder_win_view(&S.sh, ROOT);
    CHECK(rw->scroll.axis[WSCROLL_V].value == 48 && v->icons[k].y == 318 &&
          v->icons[0].y == 106 - 48,
          "O11 three down-arrow steps: value 48, every icon 48 px up (F18 at 318)");
    (void)wscroll_step(&rw->scroll.axis[WSCROLL_V], 23);
    v = finder_win_view(&S.sh, ROOT);
    CHECK(rw->scroll.axis[WSCROLL_V].value == 138 && v->icons[k].x == 59 &&
          v->icons[k].y == 228,
          "O11 page down clamps at 138: F18 at (59,228)");

    /* paint, clipped to the content as the updateEvt path clips it */
    memset(S.px, 0xEE, sizeof S.px);
    finder_win_paint(rw, &S.bm, rw->rec.contRgn);
    CHECK(pix(59 + 10, 228 + 2) == 0,
          "O11 paint: F18.TXT's page top edge (DOC row 2) is drawn at the scrolled cell");
    CHECK(pix(59 + 10, 366 + 2) == 0xEE && pix(69, 79) == 0xEE,
          "O11 paint: nothing at the unscrolled cell, nothing above the content");

    /* hit-test, rubber band, Clean Up -- all through the one accessor */
    v = finder_win_view(&S.sh, ROOT);
    CHECK(finder_desk_hit(v, 59 + 16, 228 + 16) == k,
          "O11 hit: a click on F18 where it is DRAWN (75,244) hits F18");
    CHECK(finder_desk_marquee_select(v, finder_band_rect(50, 226, 100, 270)) == 1 &&
          v->icons[k].selected == 1,
          "O11 rubber band around the drawn F18 cell selects exactly F18");
    finder_desk_deselect_all(v);
    CHECK(finder_win_cleanup(&S.sh, ROOT) == 0 && rw->view.icons[k].y == 228 &&
          finder_win_doc_rect(rw).top == 102 - 138,
          "O11 Clean Up of a scrolled window keeps the DOCUMENT grid (moved=0)");

    /* the window grows: the range shrinks and the value re-clamps */
    SizeWindow(&S.wm, &rw->rec, 360, 300);   /* content 257 high: max 315-257 = 58 */
    v = finder_win_view(&S.sh, ROOT);
    CHECK(rw->scroll.axis[WSCROLL_V].max == 58 &&
          rw->scroll.axis[WSCROLL_V].value == 58 && v->icons[k].y == 366 - 58,
          "O11 grow: max 58, value re-clamped to 58, F18 at 308");
}

int main(void)
{
    leg_scroll();
    leg_follow();
    leg_select_arrange();
    leg_resolve();
    leg_hilite();
    leg_into_folder();
    leg_between();
    leg_ladder();
    leg_service_guard();
    leg_trash();
    leg_codec();
    leg_untrash();
    leg_trash_window();
    leg_empty_trash();
    return TEST_SUMMARY("test_finder_ops");
}
