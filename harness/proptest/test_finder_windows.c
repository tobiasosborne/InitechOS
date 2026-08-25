/*
 * test_finder_windows.c -- the R3.3 Finder DISK WINDOWS oracle (THE ORACLE).
 *
 * beads: initech-tdnl.10 (GUI remediation R3.3 "disk windows").
 *
 * Ref: os/flair/finder_windows.h (the contract under test, the LOCKED grid and
 *        name-ladder conventions, the binding seam and its callback-lifetime
 *        rule, and every stated deviation),
 *      docs/design/GUI-remediation-R3-finder-design.md F1.1 (the kind
 *        heuristic, the VOLLABEL skip, the fixed caps, the callback-copy rule),
 *        F1.2/F3.3 (the invisible grid + row-major Clean Up), F1.3 (the LOCKED
 *        24-byte DESKTOP.DB record, kind=4 folder-view), F3.3 (spatial
 *        singleton, per-folder view state keyed by dir_start), F5.2 (the
 *        SPATIAL_DUP mutant),
 *      harness/proptest/test_finder_desktop.c (the arena-backed WindowMgr scene
 *        pattern and the hand-authored-golden discipline this file follows).
 *      CLAUDE.md Law 2 (the oracle is the truth -- every expectation below is
 *        HAND-AUTHORED, never computed by the code it grades; HER-02), Rule 1
 *        (red->green), Rule 2 (fail loud), Rule 6 (mutation-proven), Rule 11,
 *        Rule 12 (ASCII-clean).
 *
 * WHAT IT GRADES
 *   L1 KIND HEURISTIC. A hand-written table of (name, attribute) -> icon kind:
 *      directory bit wins over everything; ".EXE" (either case) is an app;
 *      ".EXEC"/"EXE"/"X.EXE" edge cases; everything else a document.
 *   L2 VOLUME-LABEL SKIP. The Finder -- not the FAT layer -- drops the volume
 *      label; a directory and a plain file are kept.
 *   L3 THE GRID. Column count and row-major cell origins hand-computed from the
 *      LOCKED pitch/inset against a HAND-AUTHORED content rect, so the
 *      expectations borrow nothing from the Window Manager.
 *   L4 THE NAME LADDER. "NEWFOLD" first; then the lowest free of NEWFOL02..99;
 *      near-misses ("NEWFOL01", "NEWFOLDX", "NEWFOL02.TXT") do NOT consume a
 *      rung; a saturated ladder FAILS LOUD.
 *   L5 ENUMERATION -> RECORDS, through a MOCK binding whose dirent name buffer
 *      is DELIBERATELY SCRIBBLED the instant each callback returns. Grades the
 *      resulting listing (order, names, kinds, folder clusters) against a
 *      hand-authored expectation -- which is simultaneously the use-after-return
 *      tooth: a retained name pointer reads 'Z's and every name check goes RED.
 *   L6 SPATIAL SINGLETON + CAPS. Reopening a folder RAISES its window and
 *      consumes no second slot; four windows fill the table and the fifth is
 *      FINDER_WIN_ERR_FULL; a 70-entry directory keeps 64 icons and reports 6
 *      dropped.
 *   L7 CLEAN UP. Scatter the icons, snap, and compare against hand-computed
 *      row-major coordinates plus the exact moved count.
 *   L8 THE kind=4 CODEC. encode_all against a HAND-AUTHORED 80-byte image
 *      (2 desktop records + 1 view record), the keyed lookup, and the
 *      shell's in-memory view set loaded back from those same bytes.
 *   L9 VIEW PERSISTENCE. Move a window, close it (which saves), reopen it, and
 *      assert the frame came back to the SAVED origin, not the cascade default.
 *
 * INDEPENDENCE (Law 2 / HER-02 boundary)
 *   - Every kind, name, coordinate, count and DB byte below is written out by
 *     hand from the design document and the header contract.
 *   - The grid legs are graded against a HAND-AUTHORED content rect. Where a
 *     leg necessarily uses a REAL window, the expectation is hand-authored
 *     ARITHMETIC on that window's content rect (which the Window Manager
 *     produces and test-window/test-chrome already grade) -- the grid math, the
 *     thing under test here, is never asked to check itself.
 *
 * MUTANTS (Rule 6), all -D knobs on the IMPLEMENTATION TU:
 *   FINDER_WIN_MUT_SINGLETON_DUP    -- L6's singleton property collapses.
 *   FINDER_WIN_MUT_VOLLABEL_SHOWN   -- L2/L5's listings gain the volume label.
 *   FINDER_WIN_MUT_CLEANUP_UNSORTED -- L7's snapped coordinates mirror.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "region_algebra.h"    /* the LOCKED region contract (-Ispec)          */
#include "region.h"            /* rgn_row_t caps (-Ios/flair/atkinson)         */
#include "surface.h"           /* bitmap_t (-Ios/flair)                        */
#include "window.h"            /* WindowMgr + MoveWindow (-Ios/flair)          */
#include "finder_desktop.h"    /* the icon model + the DB codec                */
#include "finder_windows.h"    /* the module under test                        */
#include "test_assert.h"       /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)     */

TEST_HARNESS();

/* ===========================================================================
 * Shared scaffolding.
 * ===========================================================================*/

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
 * THE MOCK BINDING -- and the use-after-return tooth.
 *
 * Each entry's formatted name is copied into ONE shared scratch buffer, handed
 * to the callback, and then -- the instant the callback returns -- OVERWRITTEN
 * with 'Z'. That is a faithful, hostile model of the real
 * fat12_dirent_cb contract ("valid only for the duration of the call ... the
 * next sector read overwrites it"). Any name the module retained by POINTER
 * rather than by VALUE reads as 'ZZZZZZZZZZZZ' by the time L5 inspects it.
 * ------------------------------------------------------------------------- */
typedef struct mock_entry {
    const char *name;
    uint8_t     attr;
    uint32_t    size;
    uint16_t    cluster;
} mock_entry_t;

typedef struct mock_dir {
    uint16_t            dir_start;
    const mock_entry_t *ents;
    int                 n;
} mock_dir_t;

typedef struct mock_fs {
    const mock_dir_t *dirs;
    int               n_dirs;
    char              scratch[FINDER_DESK_NAME_MAX];
    int               fail;          /* 1 == enumerate() reports a backend error */
    int               mkdir_rc;      /* what mkdir() returns                     */
    char              mkdir_name[FINDER_DESK_NAME_MAX];
    uint16_t          mkdir_parent;
    int               mkdir_calls;
} mock_fs_t;

static int mock_enumerate(void *user, uint16_t dir_start,
                          finder_enum_cb cb, void *cb_user)
{
    mock_fs_t *m = (mock_fs_t *)user;
    const mock_dir_t *d = NULL;

    if (m->fail) return -1;
    for (int i = 0; i < m->n_dirs; i++)
        if (m->dirs[i].dir_start == dir_start) { d = &m->dirs[i]; break; }
    if (d == NULL) return 0;                 /* an empty directory              */

    for (int i = 0; i < d->n; i++) {
        finder_dirent_t e;
        int rc;

        memset(m->scratch, 0, sizeof m->scratch);
        strncpy(m->scratch, d->ents[i].name, sizeof m->scratch - 1);

        e.name83        = m->scratch;
        e.attribute     = d->ents[i].attr;
        e.size          = d->ents[i].size;
        e.start_cluster = d->ents[i].cluster;
        rc = cb(&e, cb_user);

        /* THE TOOTH: the borrowed buffer dies here, exactly as the real sector
         * buffer does on the next read. */
        memset(m->scratch, 'Z', sizeof m->scratch - 1);
        m->scratch[sizeof m->scratch - 1] = '\0';

        if (rc != 0) return rc;
    }
    return 0;
}

static int mock_mkdir(void *user, const char *name83, uint16_t parent)
{
    mock_fs_t *m = (mock_fs_t *)user;
    m->mkdir_calls++;
    memset(m->mkdir_name, 0, sizeof m->mkdir_name);
    strncpy(m->mkdir_name, name83, sizeof m->mkdir_name - 1);
    m->mkdir_parent = parent;
    return m->mkdir_rc;
}

typedef struct scene {
    WindowMgr      wm;
    rgn_store_t    desk, sa, sb, sc;
    finder_shell_t sh;
    mock_fs_t      mock;
    finder_fs_t    fs;
} scene_t;

static void scene_init(scene_t *s, const mock_dir_t *dirs, int n_dirs)
{
    rgn_rect_t frame = mk_rect(0, 0, 480, 640);

    memset(s, 0, sizeof *s);
    store_attach(&s->desk); store_attach(&s->sa);
    store_attach(&s->sb);   store_attach(&s->sc);
    WindowMgr_init(&s->wm, frame, &s->desk.r, &s->sa.r, &s->sb.r, &s->sc.r);

    s->sh.wm      = &s->wm;
    s->sh.surface = NULL;
    s->sh.app     = NULL;
    finder_desk_init(&s->sh.desk, s->sh.desk_icons,
                     (uint16_t)FINDER_DESK_MAX_ICONS, frame);

    s->mock.dirs     = dirs;
    s->mock.n_dirs   = n_dirs;
    s->mock.mkdir_rc = (int)FINDER_WIN_OK;
    s->fs.enumerate  = mock_enumerate;
    s->fs.mkdir      = mock_mkdir;
    s->fs.user       = &s->mock;
    finder_shell_bind_fs(&s->sh, &s->fs);
}

/* ===========================================================================
 * L1 -- THE ICON-KIND HEURISTIC  (design F1.1, hand table)
 * ===========================================================================*/
static void leg_kind(void)
{
    struct { const char *name; uint8_t attr; finder_icon_kind_t want; const char *why; }
    T[] = {
      { "APPS",       0x10u, FINDER_ICON_FOLDER, "L1 the directory bit is a FOLDER" },
      { "SETUP.EXE",  0x10u, FINDER_ICON_FOLDER, "L1 the directory bit BEATS the .EXE suffix" },
      { "SETUP.EXE",  0x20u, FINDER_ICON_APP,    "L1 a plain .EXE is an APPLICATION" },
      { "setup.exe",  0x00u, FINDER_ICON_APP,    "L1 the .EXE test is case-INsensitive" },
      { "SETUP.eXe",  0x20u, FINDER_ICON_APP,    "L1 ... in either direction, per character" },
      { "README.TXT", 0x20u, FINDER_ICON_FILE,   "L1 a plain file is a DOCUMENT" },
      { "DESKTOP.DB", 0x02u, FINDER_ICON_FILE,   "L1 a HIDDEN file is still a document (F1.1 rules out only the volume label)" },
      { "EXE",        0x20u, FINDER_ICON_FILE,   "L1 a bare \"EXE\" name is NOT an app (no dot)" },
      { ".EXE",       0x20u, FINDER_ICON_APP,    "L1 a dot-EXE with an empty base still ends .EXE" },
      { "A.EXEC",     0x20u, FINDER_ICON_FILE,   "L1 \".EXEC\" does not end in \".EXE\"" },
      { "",           0x20u, FINDER_ICON_FILE,   "L1 an empty name is a document, never a crash" },
    };

    for (unsigned i = 0; i < sizeof T / sizeof T[0]; i++)
        CHECK(finder_win_kind_of(T[i].name, T[i].attr) == T[i].want, T[i].why);
}

/* ===========================================================================
 * L2 -- THE VOLUME-LABEL SKIP  (design F1.1)
 * ===========================================================================*/
static void leg_skip(void)
{
    CHECK(finder_win_skip_entry(0x08u) == 1,
          "L2 a VOLUME-LABEL entry is skipped BY THE FINDER (the FAT layer "
          "deliberately passes it through)");
    CHECK(finder_win_skip_entry(0x10u) == 0, "L2 a directory is kept");
    CHECK(finder_win_skip_entry(0x20u) == 0, "L2 a plain file is kept");
    CHECK(finder_win_skip_entry(0x02u) == 0,
          "L2 a HIDDEN entry is KEPT this slice (the stated F1.1 scope note)");
}

/* ===========================================================================
 * L3 -- THE INVISIBLE GRID  (design F1.2 pitch 68x52; header inset 18,4)
 * ---------------------------------------------------------------------------
 * HAND-AUTHORED content rect (100,50)..(400,250):
 *   width 300; usable = 300 - 18 = 282; cols = 282 / 68 = 4.
 *   cell(i): x = 100 + 18 + (i % 4) * 68,  y = 50 + 4 + (i / 4) * 52
 * ===========================================================================*/
static void leg_grid(void)
{
    rgn_rect_t c = mk_rect(50, 100, 250, 400);
    struct { int i; int16_t x, y; } W[] = {
        { 0, 118,  54 }, { 1, 186,  54 }, { 2, 254,  54 }, { 3, 322,  54 },
        { 4, 118, 106 }, { 7, 322, 106 }, { 8, 118, 158 }, { 11, 322, 158 },
    };

    CHECK(finder_win_grid_cols(c) == 4,
          "L3 a 300-px content rect fits 4 columns at the LOCKED 68-px pitch");
    CHECK(finder_win_grid_cols(mk_rect(0, 0, 100, 60)) == 1,
          "L3 a rect too narrow for one pitch still reports ONE column "
          "(never zero -- a zero would divide by zero in the row-major map)");
    CHECK(finder_win_grid_cols(mk_rect(0, 0, 100, 86)) == 1,
          "L3 exactly one pitch past the inset is one column");
    CHECK(finder_win_grid_cols(mk_rect(0, 0, 100, 154)) == 2,
          "L3 two pitches past the inset is two columns");

    for (unsigned k = 0; k < sizeof W / sizeof W[0]; k++) {
        int16_t x = -1, y = -1;
        finder_win_grid_origin(c, W[k].i, &x, &y);
        CHECK(x == W[k].x && y == W[k].y,
              "L3 the row-major cell origin matches the hand-computed grid");
    }

    /* The default frames cascade by exactly (20,20) per slot, and their size is
     * constant -- the header's Sec 3 contract, hand-restated. */
    for (int slot = 0; slot < FINDER_WIN_MAX; slot++) {
        rgn_rect_t f = finder_win_default_frame(slot);
        CHECK(f.left == (int16_t)(20 + slot * 20) &&
              f.top  == (int16_t)(60 + slot * 20),
              "L3 the default frame cascades 20 px per slot");
        CHECK((f.right - f.left) == 360 && (f.bottom - f.top) == 220,
              "L3 every default frame is the same 360x220 size");
    }
}

/* ===========================================================================
 * L4 -- THE NEW-FOLDER NAME LADDER  (the LOCKED convention, header Sec 4)
 * ===========================================================================*/
static void ladder_case(const char *const *names, int n,
                        const char *want, const char *why)
{
    char table[128][FINDER_DESK_NAME_MAX];
    char out[FINDER_DESK_NAME_MAX];
    finder_win_status_t st;

    memset(table, 0, sizeof table);
    for (int i = 0; i < n; i++) {
        strncpy(table[i], names[i], FINDER_DESK_NAME_MAX - 1);
        table[i][FINDER_DESK_NAME_MAX - 1] = '\0';
    }
    memset(out, 0, sizeof out);
    st = finder_win_next_folder_name(
            (const char (*)[FINDER_DESK_NAME_MAX])table, n, out);
    CHECK(st == FINDER_WIN_OK && strcmp(out, want) == 0, why);
}

static void leg_ladder(void)
{
    static const char *e0[] = { "README.TXT", "APPS" };
    static const char *e1[] = { "NEWFOLD" };
    static const char *e2[] = { "NEWFOLD", "NEWFOL02" };
    static const char *e3[] = { "NEWFOLD", "NEWFOL03" };
    static const char *e4[] = { "newfold" };
    static const char *e5[] = { "NEWFOL01", "NEWFOLDX", "NEWFOL02.TXT" };
    static const char *e6[] = { "NEWFOLD", "NEWFOL02", "NEWFOL03", "NEWFOL05" };

    ladder_case(NULL, 0, "NEWFOLD",
                "L4 an empty directory gets the un-suffixed NEWFOLD");
    ladder_case(e0, 2, "NEWFOLD",
                "L4 unrelated names do not consume a rung");
    ladder_case(e1, 1, "NEWFOL02",
                "L4 NEWFOLD taken -> the ladder starts at 02");
    ladder_case(e2, 2, "NEWFOL03",
                "L4 lowest-free-first walks up the ladder");
    ladder_case(e3, 2, "NEWFOL02",
                "L4 lowest-free-first fills a GAP before extending");
    ladder_case(e4, 1, "NEWFOL02",
                "L4 the collision test is case-insensitive (FAT 8.3 is)");
    ladder_case(e5, 3, "NEWFOLD",
                "L4 near-misses (01, a longer stem, an extension) consume NO rung");
    ladder_case(e6, 4, "NEWFOL04",
                "L4 the first free rung above a run of taken ones");

    /* Saturation FAILS LOUD (Rule 2): it never invents a 100th spelling. */
    {
        char table[100][FINDER_DESK_NAME_MAX];
        char out[FINDER_DESK_NAME_MAX];
        int n = 0;
        memset(table, 0, sizeof table);
        strcpy(table[n++], "NEWFOLD");
        for (int i = 2; i <= 99; i++) {
            table[n][0] = 'N'; table[n][1] = 'E'; table[n][2] = 'W';
            table[n][3] = 'F'; table[n][4] = 'O'; table[n][5] = 'L';
            table[n][6] = (char)('0' + i / 10);
            table[n][7] = (char)('0' + i % 10);
            table[n][8] = '\0';
            n++;
        }
        CHECK(finder_win_next_folder_name(
                  (const char (*)[FINDER_DESK_NAME_MAX])table, n, out) ==
              FINDER_WIN_ERR_NAMES,
              "L4 a saturated ladder is FINDER_WIN_ERR_NAMES, never a made-up name");
    }
}

/* ===========================================================================
 * L5 -- ENUMERATION -> ICON RECORDS  (through the hostile mock binding)
 * ===========================================================================*/

/* The hand-authored root directory. Deliberately shaped like the flagship
 * volume plus an application and a volume label. */
static const mock_entry_t ROOT_ENTS[] = {
    { "INITECH",    0x08u,   0u,  0u },   /* the VOLUME LABEL -- must be SKIPPED */
    { "README.TXT", 0x20u, 114u,  5u },
    { "APPS",       0x10u,   0u,  7u },
    { "DESKTOP.DB", 0x22u,  56u,  9u },
    { "TRASH",      0x12u,   0u, 11u },
    { "SETUP.EXE",  0x20u, 999u, 13u },
};
static const mock_entry_t APPS_ENTS[] = {
    { "CALC.EXE",   0x20u, 200u, 21u },
    { "SUB",        0x10u,   0u, 23u },
};
static const mock_dir_t MOCK_DIRS[] = {
    { 0u,  ROOT_ENTS, (int)(sizeof ROOT_ENTS / sizeof ROOT_ENTS[0]) },
    { 7u,  APPS_ENTS, (int)(sizeof APPS_ENTS / sizeof APPS_ENTS[0]) },
};

static void leg_enumerate(void)
{
    static scene_t s;
    int slot = -1, singleton = -1;
    /* HAND-AUTHORED expected listing: the volume label dropped, everything else
     * in on-disk order, with the kind heuristic applied by hand. */
    struct { const char *name; finder_icon_kind_t kind; uint16_t cluster; } W[] = {
        { "README.TXT", FINDER_ICON_FILE,   0u },
        { "APPS",       FINDER_ICON_FOLDER, 7u },
        { "DESKTOP.DB", FINDER_ICON_FILE,   0u },
        { "TRASH",      FINDER_ICON_FOLDER, 11u },
        { "SETUP.EXE",  FINDER_ICON_APP,    0u },
    };
    finder_desk_t *v;
    rgn_rect_t content;
    int cols;

    scene_init(&s, MOCK_DIRS, (int)(sizeof MOCK_DIRS / sizeof MOCK_DIRS[0]));

    CHECK(finder_win_open(&s.sh, 0u, "", 1u, &slot, &singleton) == FINDER_WIN_OK,
          "L5 the root disk window opens");
    CHECK(slot == 0 && singleton == 0,
          "L5 the first open takes slot 0 and is NOT a singleton raise");
    CHECK(strcmp(s.sh.windows[0].rec.titleHandle, "Drive A Files") == 0,
          "L5 the ROOT window carries the frame-canonical title (design F3.3)");

    v = &s.sh.windows[0].view;
    CHECK(v->n == 5u,
          "L5 five entries survive the volume-label skip (6 on disk, 1 dropped)");
    CHECK(s.sh.windows[0].dropped == 0u, "L5 nothing hit the icon cap");

    for (unsigned i = 0; i < sizeof W / sizeof W[0] && i < v->n; i++) {
        CHECK(strcmp(v->icons[i].name, W[i].name) == 0,
              "L5 the listed name matches the hand-authored expectation "
              "(and was COPIED, not retained -- the mock scribbles the buffer)");
        CHECK(v->icons[i].kind == (uint8_t)W[i].kind,
              "L5 the icon kind matches the hand-applied heuristic");
        CHECK(v->icons[i].dir_start == W[i].cluster,
              "L5 a FOLDER carries its own first cluster; everything else 0");
    }

    /* The freshly populated window is already CLEAN: hand-authored arithmetic
     * on the window's own content rect (which the Window Manager produced and
     * test-window/test-chrome grade -- the GRID math is what is under test). */
    content = region_get_bbox(s.sh.windows[0].rec.contRgn);
    cols = ((int)content.right - (int)content.left - 18) / 68;
    if (cols < 1) cols = 1;
    for (int i = 0; i < (int)v->n; i++) {
        int16_t wx = (int16_t)((int)content.left + 18 + (i % cols) * 68);
        int16_t wy = (int16_t)((int)content.top  +  4 + (i / cols) * 52);
        CHECK(v->icons[i].x == wx && v->icons[i].y == wy,
              "L5 a freshly opened window is already snapped row-major");
    }

    /* A backend failure is reported, not papered over, and leaves NO window. */
    {
        static scene_t f;
        int fslot = -1;
        scene_init(&f, MOCK_DIRS, 2);
        f.mock.fail = 1;
        CHECK(finder_win_open(&f.sh, 0u, "", 1u, &fslot, NULL) ==
              FINDER_WIN_ERR_ENUM,
              "L5 an enumerate() failure is FINDER_WIN_ERR_ENUM");
        CHECK(f.sh.windows[0].open == 0u,
              "L5 ... and leaves no half-built window claiming to show a directory");
    }
}

/* ===========================================================================
 * L6 -- SPATIAL SINGLETON + THE CAPS  (design F3.3 / F1.1; mutant SPATIAL_DUP)
 * ===========================================================================*/

/* A 70-entry directory: the per-window cap is 64, so 6 must be reported. */
static mock_entry_t BIG_ENTS[70];
static mock_dir_t   BIG_DIRS[1];

static void leg_singleton(void)
{
    static scene_t s;
    int a = -1, b = -1, sa = -1, sb = -1;
    int open_count;

    scene_init(&s, MOCK_DIRS, 2);

    CHECK(finder_win_open(&s.sh, 0u, "", 1u, &a, &sa) == FINDER_WIN_OK,
          "L6 the root window opens");
    CHECK(finder_win_open(&s.sh, 7u, "APPS", 0u, &b, &sb) == FINDER_WIN_OK,
          "L6 a folder window opens");
    CHECK(a == 0 && b == 1 && sa == 0 && sb == 0,
          "L6 two DIFFERENT directories take two different slots");

    /* THE SINGLETON PROPERTY (design F3.3): reopening brings the EXISTING
     * window forward -- same slot, singleton=1, no third slot consumed. */
    {
        int c = -1, sc = -1;
        CHECK(finder_win_open(&s.sh, 7u, "APPS", 0u, &c, &sc) == FINDER_WIN_OK,
              "L6 reopening an open folder succeeds");
        CHECK(c == 1, "L6 ... in the SAME slot");
        CHECK(sc == 1, "L6 ... and reports singleton=1 (a raise, not an open)");
    }
    open_count = 0;
    for (int i = 0; i < FINDER_WIN_MAX; i++) if (s.sh.windows[i].open) open_count++;
    CHECK(open_count == 2,
          "L6 exactly TWO windows exist after three opens of two directories "
          "(the SPATIAL_DUP mutant makes this three)");
    CHECK(finder_win_find(&s.sh, 7u) == 1,
          "L6 the lookup keyed by dir_start finds the folder's window");
    CHECK(finder_win_find(&s.sh, 4242u) == -1,
          "L6 an unopened directory has no window");
    CHECK(finder_win_slot_of(&s.sh, &s.sh.windows[1].rec) == 1,
          "L6 a WindowRecord demuxes back to its slot");

    /* The window table is FIXED CAPACITY and fails loud when full. */
    {
        int d = -1;
        CHECK(finder_win_open(&s.sh, 21u, "SUBA", 0u, &d, NULL) == FINDER_WIN_OK,
              "L6 a third window opens");
        CHECK(finder_win_open(&s.sh, 23u, "SUBB", 0u, &d, NULL) == FINDER_WIN_OK,
              "L6 a fourth window opens (the table is full)");
        CHECK(finder_win_open(&s.sh, 25u, "SUBC", 0u, &d, NULL) ==
              FINDER_WIN_ERR_FULL,
              "L6 the FIFTH is FINDER_WIN_ERR_FULL -- fail loud, never a "
              "silently dropped open (Rule 2)");
    }

    /* The per-window icon cap: 70 entries -> 64 kept, 6 reported. */
    {
        static scene_t big;
        static char names[70][FINDER_DESK_NAME_MAX];
        int slot = -1;

        for (int i = 0; i < 70; i++) {
            snprintf(names[i], sizeof names[i], "F%02d.TXT", i);
            BIG_ENTS[i].name    = names[i];
            BIG_ENTS[i].attr    = 0x20u;
            BIG_ENTS[i].size    = 1u;
            BIG_ENTS[i].cluster = (uint16_t)(100 + i);
        }
        BIG_DIRS[0].dir_start = 0u;
        BIG_DIRS[0].ents      = BIG_ENTS;
        BIG_DIRS[0].n         = 70;

        scene_init(&big, BIG_DIRS, 1);
        CHECK(finder_win_open(&big.sh, 0u, "", 1u, &slot, NULL) == FINDER_WIN_OK,
              "L6 a 70-entry directory still opens");
        CHECK(big.sh.windows[0].view.n == (uint16_t)FINDER_WIN_ICONS_MAX,
              "L6 the window keeps exactly FINDER_WIN_ICONS_MAX icons");
        CHECK(big.sh.windows[0].dropped == 6u,
              "L6 ... and REPORTS the 6 it could not show (never silent)");
        CHECK(strcmp(big.sh.windows[0].view.icons[0].name, "F00.TXT") == 0 &&
              strcmp(big.sh.windows[0].view.icons[63].name, "F63.TXT") == 0,
              "L6 the kept 64 are the FIRST 64 in on-disk order");
    }
}

/* ===========================================================================
 * L7 -- CLEAN UP  (design F3.3 row-major; mutant CLEANUP_UNSORTED)
 * ===========================================================================*/
static void leg_cleanup(void)
{
    static scene_t s;
    int slot = -1;
    finder_desk_t *v;
    rgn_rect_t content;
    int cols;
    int moved;

    scene_init(&s, MOCK_DIRS, 2);
    CHECK(finder_win_open(&s.sh, 0u, "", 1u, &slot, NULL) == FINDER_WIN_OK,
          "L7 the root window opens");
    v = &s.sh.windows[0].view;

    CHECK(finder_win_cleanup(&s.sh, 0) == 0,
          "L7 Clean Up on an ALREADY clean window moves nothing");

    /* Scatter every icon to a hand-picked non-grid position. */
    for (int i = 0; i < (int)v->n; i++) {
        v->icons[i].x = (int16_t)(200 + i);
        v->icons[i].y = (int16_t)(150 + 2 * i);
    }
    moved = finder_win_cleanup(&s.sh, 0);
    CHECK(moved == (int)v->n, "L7 every scattered icon reports as MOVED");

    content = region_get_bbox(s.sh.windows[0].rec.contRgn);
    cols = ((int)content.right - (int)content.left - 18) / 68;
    if (cols < 1) cols = 1;
    for (int i = 0; i < (int)v->n; i++) {
        int16_t wx = (int16_t)((int)content.left + 18 + (i % cols) * 68);
        int16_t wy = (int16_t)((int)content.top  +  4 + (i / cols) * 52);
        CHECK(v->icons[i].x == wx && v->icons[i].y == wy,
              "L7 Clean Up snaps ROW-MAJOR in record order (the "
              "CLEANUP_UNSORTED mutant mirrors this)");
    }

    /* Selected-items-first is explicitly NOT period (design F3.3): selecting a
     * later icon must not change where anything lands. */
    {
        int16_t was_x = v->icons[0].x, was_y = v->icons[0].y;
        finder_desk_select_only(v, 3);
        for (int i = 0; i < (int)v->n; i++) { v->icons[i].x = 300; v->icons[i].y = 300; }
        (void)finder_win_cleanup(&s.sh, 0);
        CHECK(v->icons[0].x == was_x && v->icons[0].y == was_y,
              "L7 a selection does NOT reorder the snap (plain row-major)");
    }
}

/* ===========================================================================
 * L8 -- THE kind=4 CODEC  (design F1.3; HAND-AUTHORED bytes, Law 2 / HER-02)
 * ---------------------------------------------------------------------------
 * 8-byte header + THREE 24-byte records = 80 bytes:
 *   header : 'I','D','B','1'  ver=1 (LE)  n=3 (LE)
 *   rec 0  : kind 1 (volume-pos), dir_start 0, "INITECH", x=584 (0x0248),
 *            y=48 (0x0030), view_bits 0, pad 0
 *   rec 1  : kind 2 (trash-pos),  dir_start 0, "Trash",   x=584, y=404 (0x0194)
 *   rec 2  : kind 4 (folder-view),dir_start 7 (0x0007), "APPS", x=100 (0x0064),
 *            y=80 (0x0050), view_bits 0, pad 0
 * ===========================================================================*/
static const uint8_t DB_GOLDEN[80] = {
    /* header */
    'I','D','B','1', 0x01,0x00, 0x03,0x00,
    /* rec 0 -- volume-pos "INITECH" @ (584,48) */
    0x01, 0x00, 0x00,0x00,
    'I','N','I','T','E','C','H', 0x00,0x00,0x00,0x00,0x00,0x00,
    0x48,0x02, 0x30,0x00, 0x00,0x00, 0x00,
    /* rec 1 -- trash-pos "Trash" @ (584,404) */
    0x02, 0x00, 0x00,0x00,
    'T','r','a','s','h', 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x48,0x02, 0x94,0x01, 0x00,0x00, 0x00,
    /* rec 2 -- folder-view for cluster 7 ("APPS") @ (100,80) */
    0x04, 0x00, 0x07,0x00,
    'A','P','P','S', 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x64,0x00, 0x50,0x00, 0x00,0x00, 0x00
};

static void leg_db(void)
{
    finder_desk_t fd;
    finder_desk_icon_t icons[FINDER_DESK_MAX_ICONS];
    finder_view_rec_t views[2];
    finder_view_rec_t got;
    uint8_t buf[FINDER_DB_MAX_BYTES];
    uint32_t n;
    static scene_t s;

    memset(icons, 0, sizeof icons);
    finder_desk_init(&fd, icons, (uint16_t)FINDER_DESK_MAX_ICONS,
                     mk_rect(40, 0, 480, 640));
    (void)finder_desk_add(&fd, FINDER_ICON_VOLUME, "INITECH", 584, 48, 1u);
    (void)finder_desk_add(&fd, FINDER_ICON_TRASH,  "Trash",   584, 404, 0u);

    memset(views, 0, sizeof views);
    views[0].dir_start = 7u;
    strcpy(views[0].name83, "APPS");
    views[0].x = 100u; views[0].y = 80u; views[0].view_bits = 0u;

    memset(buf, 0xAA, sizeof buf);
    n = finder_desk_db_encode_all(&fd, views, 1u, buf, (uint32_t)sizeof buf);
    CHECK(n == 80u, "L8 encode_all writes 8 + 3*24 = 80 bytes");
    CHECK(memcmp(buf, DB_GOLDEN, sizeof DB_GOLDEN) == 0,
          "L8 the encoded image is BYTE-IDENTICAL to the hand-authored golden");

    /* With NO view records the output must still be the R3.2 shape, so the
     * locked 56-byte on-disk assertion of the emu gate is untouched. */
    memset(buf, 0, sizeof buf);
    n = finder_desk_db_encode_all(&fd, NULL, 0u, buf, (uint32_t)sizeof buf);
    CHECK(n == 56u, "L8 with no view records the image is the R3.2 56 bytes");
    CHECK(buf[6] == 0x02 && buf[7] == 0x00,
          "L8 ... and its record count is 2, exactly as before");
    CHECK(memcmp(buf + 8, DB_GOLDEN + 8, 48) == 0,
          "L8 ... with the two desktop records byte-identical");

    CHECK(finder_desk_db_encode_all(&fd, views, 1u, buf, 79u) == 0u,
          "L8 encode_all refuses a too-small buffer (never overflows, Rule 2)");

    /* The keyed lookup. */
    memset(&got, 0xAA, sizeof got);
    CHECK(finder_desk_db_find_view(DB_GOLDEN, sizeof DB_GOLDEN, 7u, &got) == 1,
          "L8 the kind=4 record for cluster 7 is found");
    CHECK(got.x == 100u && got.y == 80u && got.view_bits == 0u &&
          strcmp(got.name83, "APPS") == 0,
          "L8 ... and decodes to the hand-authored fields");
    CHECK(finder_desk_db_find_view(DB_GOLDEN, sizeof DB_GOLDEN, 9u, &got) == 0,
          "L8 an absent key is 0 (absent), not an error");
    {
        uint8_t bad[80];
        memcpy(bad, DB_GOLDEN, sizeof bad);
        bad[0] = 'X';
        CHECK(finder_desk_db_find_view(bad, sizeof bad, 7u, &got) ==
              (int)FINDER_DB_ERR_MAGIC,
              "L8 a corrupt image reports the validation status, not a hit");
    }
    CHECK(finder_desk_db_validate(DB_GOLDEN, sizeof DB_GOLDEN) == FINDER_DB_OK,
          "L8 a 3-record image validates (the capacity extension is live)");

    /* The shell's in-memory view set, loaded from those same bytes. */
    scene_init(&s, MOCK_DIRS, 2);
    CHECK(finder_shell_load_views(&s.sh, DB_GOLDEN, sizeof DB_GOLDEN) == 1u,
          "L8 the shell takes exactly ONE kind=4 record from the image");
    CHECK(s.sh.n_views == 1u, "L8 ... and holds one view record");
    CHECK(finder_shell_find_view(&s.sh, 7u) != NULL &&
          finder_shell_find_view(&s.sh, 7u)->x == 100u,
          "L8 ... keyed by dir_start, with the decoded origin");
    CHECK(finder_shell_find_view(&s.sh, 0u) == NULL,
          "L8 the desktop records did NOT become view records");
    {
        uint8_t bad[80];
        memcpy(bad, DB_GOLDEN, sizeof bad);
        bad[4] = 0x02;   /* a future version */
        CHECK(finder_shell_load_views(&s.sh, bad, sizeof bad) == 0u,
              "L8 a corrupt image contributes NO view records (regenerate-default)");
        CHECK(s.sh.n_views == 0u, "L8 ... and clears the set");
    }
}

/* ===========================================================================
 * L9 -- VIEW PERSISTENCE ACROSS CLOSE + REOPEN  (design F3.3)
 * ===========================================================================*/
static void leg_persist(void)
{
    static scene_t s;
    int slot = -1;
    rgn_rect_t f0, f1, f2;

    scene_init(&s, MOCK_DIRS, 2);
    CHECK(finder_win_open(&s.sh, 7u, "APPS", 0u, &slot, NULL) == FINDER_WIN_OK,
          "L9 a folder window opens");
    f0 = WindowFrameRect(&s.sh.windows[slot].rec);
    CHECK(f0.left == 20 && f0.top == 60,
          "L9 with no saved record it opens at the slot-0 cascade default");

    MoveWindow(&s.wm, &s.sh.windows[slot].rec, 150, 200);
    f1 = WindowFrameRect(&s.sh.windows[slot].rec);
    CHECK(f1.left == 150 && f1.top == 200, "L9 the window really moved");

    CHECK(finder_win_close(&s.sh, slot) == FINDER_WIN_OK, "L9 the window closes");
    CHECK(s.sh.windows[slot].open == 0u, "L9 ... and its slot is free");
    CHECK(s.sh.n_views == 1u, "L9 closing SAVED a view record");
    CHECK(finder_shell_find_view(&s.sh, 7u) != NULL &&
          finder_shell_find_view(&s.sh, 7u)->x == 150u &&
          finder_shell_find_view(&s.sh, 7u)->y == 200u,
          "L9 ... carrying the moved frame origin, keyed by dir_start");
    CHECK(strcmp(finder_shell_find_view(&s.sh, 7u)->name83, "APPS") == 0,
          "L9 ... and the folder's 8.3 name");

    slot = -1;
    CHECK(finder_win_open(&s.sh, 7u, "APPS", 0u, &slot, NULL) == FINDER_WIN_OK,
          "L9 the folder reopens");
    f2 = WindowFrameRect(&s.sh.windows[slot].rec);
    CHECK(f2.left == 150 && f2.top == 200,
          "L9 ... at the SAVED origin, not the cascade default");
    CHECK((f2.right - f2.left) == (f0.right - f0.left) &&
          (f2.bottom - f2.top) == (f0.bottom - f0.top),
          "L9 ... at the DEFAULT size (the locked 24-byte record has no size "
          "field -- the stated deviation, finder_desktop.h Sec 9)");

    /* An idempotent save reports NO change, so the caller never rewrites the
     * DESKTOP.DB for nothing. */
    CHECK(finder_shell_note_view(&s.sh, 7u, "APPS", 150u, 200u, 0u) == 0,
          "L9 re-noting an identical record reports no change");
    CHECK(finder_shell_note_view(&s.sh, 7u, "APPS", 151u, 200u, 0u) == 1,
          "L9 a different origin DOES report a change");
}

/* ===========================================================================
 * L10 -- NEW FOLDER through the binding  (the ladder + the error mapping)
 * ===========================================================================*/
static void leg_new_folder(void)
{
    static scene_t s;
    int slot = -1;
    char name[FINDER_DESK_NAME_MAX];
    uint16_t parent = 0xFFFFu;

    scene_init(&s, MOCK_DIRS, 2);
    CHECK(finder_win_open(&s.sh, 7u, "APPS", 0u, &slot, NULL) == FINDER_WIN_OK,
          "L10 a folder window opens");

    memset(name, 0, sizeof name);
    CHECK(finder_win_new_folder(&s.sh, slot, name, &parent) == FINDER_WIN_OK,
          "L10 New Folder succeeds");
    CHECK(strcmp(name, "NEWFOLD") == 0,
          "L10 ... with the LOCKED un-suffixed first ladder name");
    CHECK(parent == 7u, "L10 ... created in the WINDOW'S directory");
    CHECK(s.mock.mkdir_calls == 1 && strcmp(s.mock.mkdir_name, "NEWFOLD") == 0 &&
          s.mock.mkdir_parent == 7u,
          "L10 ... via exactly ONE mkdir on the binding, with that name+parent");

    /* A full parent maps to the DIR_FULL status the caller reports loudly. */
    s.mock.mkdir_rc = (int)FINDER_WIN_ERR_DIRFULL;
    CHECK(finder_win_new_folder(&s.sh, slot, name, &parent) ==
          FINDER_WIN_ERR_DIRFULL,
          "L10 a full parent directory surfaces as FINDER_WIN_ERR_DIRFULL");
    s.mock.mkdir_rc = (int)FINDER_WIN_ERR_MKDIR;
    CHECK(finder_win_new_folder(&s.sh, slot, name, &parent) ==
          FINDER_WIN_ERR_MKDIR,
          "L10 any other backend refusal surfaces as FINDER_WIN_ERR_MKDIR");

    /* No binding at all is a refusal, never a pretend-success. */
    {
        static scene_t nb;
        scene_init(&nb, MOCK_DIRS, 2);
        nb.sh.have_fs = 0u;
        CHECK(finder_win_new_folder(&nb.sh, 0, name, &parent) ==
              FINDER_WIN_ERR_NULL,
              "L10 with no FAT binding every disk verb refuses (Rule 2)");
    }
}

/* ===========================================================================
 * L11 -- THE COMMAND SPINE, END TO END  (design F4.4; the exec hook + the
 *        keyboard lookup that the live Ctrl chord rides)
 * ---------------------------------------------------------------------------
 * The live path is: kmain decodes the modifier -> finder_cmd_key_lookup(ch) ->
 * finder_cmd_result(row) -> finder_dispatch(ctx, result, "key") -> the shell's
 * exec hook -> a finder_cmd_outcome_t that kmain reads back and reports.
 *
 * EVERYTHING EXCEPT THE MODIFIER DECODE IS GRADED HERE. The decode itself lives
 * in kmain (not host-buildable) and cannot be driven from the emulator either:
 * the QMP rail sends one qcode at a time (harness/emu/qemu.c :: qmp_send_key)
 * and has no way to HOLD Ctrl across a keypress -- the same limitation
 * spec/flair_desktop_icons_traces.mk already records for shift-click. Stated,
 * not silently skipped (Law 1).
 * ===========================================================================*/
static void leg_cmd_spine(void)
{
    static scene_t s;
    FinderCtx ctx;
    const finder_cmd_t *c;
    const finder_cmd_outcome_t *o;
    int slot = -1;

    /* The keyboard lookup: case-insensitive, and it finds the SAME rows the
     * mouse path finds by (menu,item). */
    c = finder_cmd_key_lookup('N');
    CHECK(c != NULL && c->id == FCMD_NEW_FOLDER, "L11 Cmd-N is New Folder");
    CHECK(finder_cmd_key_lookup('n') == c, "L11 ... case-insensitively");
    CHECK(finder_cmd_key_lookup('W') != NULL &&
          finder_cmd_key_lookup('W')->id == FCMD_CLOSE_WINDOW,
          "L11 Cmd-W is Close Window");
    CHECK(finder_cmd_key_lookup('O')->id == FCMD_OPEN, "L11 Cmd-O is Open");
    CHECK(finder_cmd_key_lookup('Q') == NULL,
          "L11 an unbound chord finds NOTHING (kmain then routes it onward)");
    CHECK(finder_cmd_key_lookup(0) == NULL,
          "L11 a zero cmd_char never matches the rows that carry none");
    CHECK(finder_cmd_result(c) ==
          (((uint32_t)(uint16_t)c->menu_id << 16) | c->item_1based),
          "L11 the row packs back into the IM result word");
    CHECK(finder_cmd_lookup(c->menu_id, c->item_1based) == c,
          "L11 ... which the mouse path's lookup resolves to the SAME row");
    CHECK(finder_cmd_result(NULL) == 0u,
          "L11 a NULL row packs to 0 -- IM's \"nothing chosen\", ignored by dispatch");

    /* The hook: dispatch really reaches the shell and really creates a folder. */
    scene_init(&s, MOCK_DIRS, 2);
    memset(&ctx, 0, sizeof ctx);
    finder_shell_bind_ctx(&s.sh, &ctx);
    CHECK(ctx.exec != NULL && ctx.shell == (void *)&s.sh,
          "L11 binding the context installs the shell as the execution hook");

    CHECK(finder_win_open(&s.sh, 7u, "APPS", 0u, &slot, NULL) == FINDER_WIN_OK,
          "L11 a folder window opens");
    finder_shell_sync_ctx(&s.sh);
    CHECK(ctx.front_is_diskwin == 1u,
          "L11 ... and the Close Window predicate now reads TRUE");

    CHECK(finder_shell_take_outcome(&s.sh) == NULL,
          "L11 no command has run yet, so there is no outcome to report");

    finder_dispatch(&ctx, finder_cmd_result(c), "key");
    o = finder_shell_take_outcome(&s.sh);
    CHECK(o != NULL, "L11 dispatching Cmd-N produced an outcome");
    CHECK(o->id == FCMD_NEW_FOLDER && o->status == FINDER_WIN_OK,
          "L11 ... reporting a successful New Folder");
    CHECK(strcmp(o->name83, "NEWFOLD") == 0 && o->parent == 7u,
          "L11 ... with the ladder name and the front window's directory");
    CHECK(s.mock.mkdir_calls == 1,
          "L11 ... via exactly one mkdir on the binding");
    CHECK(finder_shell_take_outcome(&s.sh) == NULL,
          "L11 taking the outcome CLEARS it (a command is never reported twice)");

    /* Clean Up rides the same spine. */
    {
        finder_desk_t *v = &s.sh.windows[slot].view;
        for (int i = 0; i < (int)v->n; i++) { v->icons[i].x = 500; v->icons[i].y = 400; }
        finder_dispatch(&ctx, ((uint32_t)515u << 16) | 1u, "mouse");  /* Special > Clean Up */
        o = finder_shell_take_outcome(&s.sh);
        CHECK(o != NULL && o->id == FCMD_CLEANUP && o->status == FINDER_WIN_OK,
              "L11 the MOUSE path reaches the same hook (Special > Clean Up)");
        CHECK(o->moved > 0, "L11 ... and really snapped the scattered icons");
    }

    /* A DISABLED command never reaches the hook: Close Window with no disk
     * window front is refused by the predicate inside finder_dispatch. */
    {
        static scene_t d;
        FinderCtx dctx;
        memset(&dctx, 0, sizeof dctx);
        scene_init(&d, MOCK_DIRS, 2);
        finder_shell_bind_ctx(&d.sh, &dctx);
        finder_shell_sync_ctx(&d.sh);
        CHECK(dctx.front_is_diskwin == 0u, "L11 with no window front the predicate is FALSE");
        finder_dispatch(&dctx, finder_cmd_result(finder_cmd_key_lookup('W')), "key");
        CHECK(finder_shell_take_outcome(&d.sh) == NULL,
              "L11 a predicate-disabled command never reaches the shell hook");
    }
}

int main(void)
{
    leg_kind();
    leg_skip();
    leg_grid();
    leg_ladder();
    leg_enumerate();
    leg_singleton();
    leg_cleanup();
    leg_db();
    leg_persist();
    leg_new_folder();
    leg_cmd_spine();
    return TEST_SUMMARY("test-finder-windows");
}
