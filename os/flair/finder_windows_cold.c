/* Ref: ADR-0014 DEC-14-8/-10; finder_windows.h section 12; PRD 6.5.
 * Destructive operations and their alert text, section COLD. Window painting,
 * event handling, enumeration callbacks and state remain resident. */
#include "finder_windows_private.h"
#include "chicago12.h"
/* 1 when `name83` is an item of `dir` (case-insensitive 8.3). */
static int fw_has_item(finder_shell_t *sh, uint16_t dir, const char *name83)
{
    fw_item_t it;
    for (int n = 0; fw_item(sh, dir, n, &it) > 0; n++)
        if (fw_ieq(it.name, name83)) return 1;
    return 0;
}

/* RETIRE every view of directory `dir`, which the purge has just REMOVED
 * (Sec 12; bead initech-uios, audit L002): DisposeWindow through the Window
 * Manager (no view state saved -- the folder is gone), the icon model and its
 * selection cleared, the slot flagged in purge_closed; and the kind=4 view
 * record keyed by the dead cluster dropped, so a folder later allocated into
 * that cluster inherits nothing. Returns 1 when a view record was dropped. */
/* The open window slots over directory `dir`, as a bit set. */
static unsigned fw_slots_of(const finder_shell_t *sh, uint16_t dir)
{
    unsigned m = 0u;
    for (int i = 0; i < sh->window_cap; i++)
        if (sh->windows[i].open && sh->windows[i].dir_start == dir)
            m |= 1u << i;
    return m;
}

static int fw_retire_dir(finder_shell_t *sh, uint16_t dir)
{
    uint16_t kept = 0u;
    int dropped;

    for (int i = 0; i < sh->window_cap; i++) {
        finder_window_t *w = &sh->windows[i];
        if (!w->open || w->is_root || w->dir_start != dir) continue;
        DisposeWindow(sh->wm, &w->rec);
        w->open   = 0u;
        w->view.n = 0u;
        finder_click_reset(&w->click);
        sh->purge_closed |= 1u << i;
    }
    for (uint16_t i = 0u; i < sh->n_views; i++)
        if (sh->views[i].dir_start != dir) sh->views[kept++] = sh->views[i];
    dropped = (kept != sh->n_views);
    sh->n_views = kept;
    return dropped;
}

/* THE DEPTH-FIRST PURGE of everything UNDER directory `root` (Sec 12). Shared
 * verbatim by Empty Trash (root = \TRASH) and by Delete Now (root = the folder
 * being deleted; bead initech-vnl4), so the two destructive paths cannot drift:
 * the same locked-descendant refusal, the same depth limit, the same retirement
 * of every view of a removed folder. `root` itself is never removed here. */
typedef struct fw_purge {
    uint16_t purged, refused;
    int      locked, views_dropped;
    unsigned touched;               /* surviving views whose listing changed */
} fw_purge_t;

static void fw_purge_tree(finder_shell_t *sh, uint16_t root, fw_purge_t *p)
{
    struct {
        uint16_t dir, skip;
        char     name[FINDER_DESK_NAME_MAX];   /* this folder's name in its parent */
    } st[FINDER_TRASH_DEPTH];
    int d = 0, rc;
    fw_item_t it;

    /* DEPTH-FIRST. Each level remembers how many leading items it had to
     * REFUSE (`skip`), so the walk always takes the first item it has not
     * given up on: delete it, or descend into it; an emptied level is
     * removed from its parent on the way back up. Every step deletes an
     * entry, refuses one (skip grows), descends or ascends, so it ends. */
    st[0].dir  = root;
    st[0].skip = 0u;
    for (;;) {
        rc = fw_item(sh, st[d].dir, (int)st[d].skip, &it);
        if (rc <= 0) {
            if (d == 0) break;
            /* The level is as empty as it will get: remove the folder. */
            rc = sh->fs.rmdir(sh->fs.user, st[d].name, st[d - 1].dir);
            d--;
            if (rc == (int)FINDER_WIN_OK) {
                p->purged = (uint16_t)(p->purged + 1u);
                p->touched |= fw_slots_of(sh, st[d].dir);
#if !defined(FINDER_WIN_MUT_PURGE_KEEPS_VIEWS)
                /* The folder is GONE: so is every view of it (audit L002). */
                p->views_dropped |= fw_retire_dir(sh, st[d + 1].dir);
#else
                (void)fw_retire_dir;
#endif
            } else {
                p->refused = (uint16_t)(p->refused + 1u);
                st[d].skip = (uint16_t)(st[d].skip + 1u);
            }
            continue;
        }
        /* Recheck each live entry, including descendants locked after staging.
         * Apple_Macintosh_System_Software_Users_Guide_V6.0.pdf pp. 88, 112.
         * L118: protection has its own result, retained origins and feedback. */
#ifndef FINDER_WIN_MUT_LOCKED_PURGE
        if ((it.attr & FINDER_ATTR_READONLY) != 0u) {
            p->locked = 1;
            p->refused = (uint16_t)(p->refused + 1u);
            st[d].skip = (uint16_t)(st[d].skip + 1u);
            continue;
        }
#endif
        if ((it.attr & (uint8_t)FINDER_ATTR_DIRECTORY) != 0u) {
            if (d + 1 >= FINDER_TRASH_DEPTH) {          /* too deep: refuse loud */
                p->refused = (uint16_t)(p->refused + 1u);
                st[d].skip = (uint16_t)(st[d].skip + 1u);
                continue;
            }
            d++;
            st[d].dir  = it.cl;
            st[d].skip = 0u;
            fw_copy83(st[d].name, it.name);
            continue;
        }
        if (sh->fs.unlink(sh->fs.user, it.name, st[d].dir) == (int)FINDER_WIN_OK) {
            p->purged = (uint16_t)(p->purged + 1u);
            p->touched |= fw_slots_of(sh, st[d].dir);
        } else {
            p->refused = (uint16_t)(p->refused + 1u);
            st[d].skip = (uint16_t)(st[d].skip + 1u);
        }
    }

}

finder_win_status_t finder_shell_empty_trash(finder_shell_t *sh,
                                             uint16_t *out_purged,
                                             uint16_t *out_refused,
                                             uint8_t *out_db_dirty)
{
    uint16_t kept = 0u;
    int td;
    fw_purge_t p;

    if (out_purged != (uint16_t *)0)   *out_purged = 0u;
    if (out_refused != (uint16_t *)0)  *out_refused = 0u;
    if (out_db_dirty != (uint8_t *)0)  *out_db_dirty = 0u;
    if (sh != (finder_shell_t *)0) sh->purge_closed = 0u;
    if (!fw_trash_ok(sh) ||
        sh->fs.unlink == (int (*)(void *, const char *, uint16_t))0 ||
        sh->fs.rmdir == (int (*)(void *, const char *, uint16_t))0)
        return FINDER_WIN_ERR_NULL;
    td = sh->fs.trash_dir(sh->fs.user);
    if (td < 0) return FINDER_WIN_ERR_NOTRASH;

    fw_zero(&p, (uint32_t)sizeof p);
    fw_purge_tree(sh, (uint16_t)td, &p);

    /* The kind=5 origins of purged items are stale; a refused item keeps its
     * own (it is still in \TRASH and can still be put back). */
    for (uint16_t i = 0u; i < sh->n_origins; i++)
        if (fw_has_item(sh, (uint16_t)td, sh->origins[i].name83))
            sh->origins[kept++] = sh->origins[i];
    if ((kept != sh->n_origins || p.views_dropped) && out_db_dirty != (uint8_t *)0)
        *out_db_dirty = 1u;
    sh->n_origins = kept;

    /* An open Trash window shows what is left -- and so does every SURVIVING
     * folder window the walk deleted something out of (a partial purge: the
     * folder stayed for a locked descendant, its removed children must not
     * stay on as icons naming freed clusters). A retired slot is closed and
     * no longer matches. */
    p.touched |= fw_slots_of(sh, (uint16_t)td);
    for (int i = 0; i < sh->window_cap; i++)
        if ((p.touched & (1u << i)) != 0u && sh->windows[i].open &&
            finder_win_populate(sh, i) == FINDER_WIN_OK)
            finder_win_invalidate_all(sh, i);
    (void)finder_shell_recount_trash(sh);
    if (sh->purge_closed != 0u) {
        /* No selection, open-predicate or group head may still name a
         * retired slot (the command context is recomputed from the live
         * windows only). */
        fw_refresh_group_head(sh);
        finder_shell_sync_ctx(sh);
    }
    if (out_purged != (uint16_t *)0)  *out_purged = p.purged;
    if (out_refused != (uint16_t *)0) *out_refused = p.refused;
    return p.locked ? FINDER_WIN_ERR_LOCKED : FINDER_WIN_OK;
}

/* DELETE NOW (bead initech-vnl4; audit docs/audits/2026-10-23-desktop
 * REPORT.md L001, P1): a volume that is FULL and has no \TRASH cannot stage
 * anything -- staging needs the Trash directory, and making it needs a free
 * cluster -- so without this verb the Finder offers no way at all to free
 * space. The period guidance for a full disk is "You can remove items from
 * the disk to free up more space" (Apple_Macintosh_System_Software_Users_
 * Guide_V6.0.pdf printed p. 262); the kernel asks first (the CAUTION alert,
 * "remove ... permanently", the Empty Trash confirm's own words, Sec 12) and
 * calls this ONLY on OK. Removes item `idx` of window `slot` IN PLACE: a
 * document by unlink, a folder by the SAME depth-first walk Empty Trash uses
 * (fw_purge_tree) and then rmdir. A locked item, or a locked descendant,
 * refuses FINDER_WIN_ERR_LOCKED and the folder stays (pp. 88, 112: unlock
 * before discarding). The window, and any view the walk touched, is
 * re-listed; every view of a removed folder is retired (audit L002). */
finder_win_status_t finder_shell_delete_item(finder_shell_t *sh, int slot,
                                             int idx, uint16_t *out_purged,
                                             uint8_t *out_db_dirty)
{
    finder_window_t *w;
    fw_item_t it;
    fw_purge_t p;
    finder_win_status_t st;
    char name[FINDER_DESK_NAME_MAX];
    uint16_t dir;
    int n, rc, found = 0;

    if (out_purged != (uint16_t *)0)   *out_purged = 0u;
    if (out_db_dirty != (uint8_t *)0)  *out_db_dirty = 0u;
    if (!fw_slot_ok(sh, slot) || !sh->have_fs ||
        sh->fs.enumerate == (int (*)(void *, uint16_t, finder_enum_cb, void *))0 ||
        sh->fs.unlink == (int (*)(void *, const char *, uint16_t))0 ||
        sh->fs.rmdir == (int (*)(void *, const char *, uint16_t))0)
        return FINDER_WIN_ERR_NULL;
    w = &sh->windows[slot];
    if (idx < 0 || idx >= (int)w->view.n) return FINDER_WIN_ERR_NULL;
    sh->purge_closed = 0u;
    /* The view must still BE its folder (Sec 10d). */
    st = finder_win_check_live(sh, slot);
    if (st != FINDER_WIN_OK) return st;
    dir = w->dir_start;
    fw_copy83(name, w->view.icons[idx].name);
    if (finder_win_is_service(dir, name)) return FINDER_WIN_ERR_SERVICE;
    /* The DISK says what the item is, not the icon (it may predate a lock). */
    for (n = 0; (rc = fw_item(sh, dir, n, &it)) > 0; n++)
        if (fw_ieq(it.name, name)) { found = 1; break; }
    if (rc < 0) return FINDER_WIN_ERR_ENUM;
    if (!found) return FINDER_WIN_ERR_MOVE;
    if ((it.attr & FINDER_ATTR_READONLY) != 0u) return FINDER_WIN_ERR_LOCKED;

    fw_zero(&p, (uint32_t)sizeof p);
    if ((it.attr & (uint8_t)FINDER_ATTR_DIRECTORY) != 0u) {
#ifndef FINDER_WIN_MUT_DELETE_NO_WALK
        /* Rule 6, test-finder-ops O18: skipping the walk cannot delete a tree. */
        fw_purge_tree(sh, it.cl, &p);
#endif
        if (!p.locked && p.refused == 0u) {
            if (sh->fs.rmdir(sh->fs.user, name, dir) == (int)FINDER_WIN_OK) {
                p.purged = (uint16_t)(p.purged + 1u);
#ifndef FINDER_WIN_MUT_DELETE_KEEP_VIEW
                /* O18 also grades this root retirement, outside the walk. */
                p.views_dropped |= fw_retire_dir(sh, it.cl);
#endif
            } else {
                p.refused = (uint16_t)(p.refused + 1u);
            }
        }
    } else if (sh->fs.unlink(sh->fs.user, name, dir) == (int)FINDER_WIN_OK) {
        p.purged = (uint16_t)(p.purged + 1u);
    } else {
        p.refused = (uint16_t)(p.refused + 1u);
    }

    p.touched |= fw_slots_of(sh, dir);
    for (int i = 0; i < sh->window_cap; i++)
        if ((p.touched & (1u << i)) != 0u && sh->windows[i].open &&
            finder_win_populate(sh, i) == FINDER_WIN_OK)
            finder_win_invalidate_all(sh, i);
    if (sh->purge_closed != 0u) {
        fw_refresh_group_head(sh);
        finder_shell_sync_ctx(sh);
    }
    if (out_purged != (uint16_t *)0)  *out_purged = p.purged;
    if (out_db_dirty != (uint8_t *)0) *out_db_dirty = (uint8_t)(p.views_dropped ? 1u : 0u);
    if (p.locked) return FINDER_WIN_ERR_LOCKED;
    return (p.refused != 0u) ? FINDER_WIN_ERR_MOVE : FINDER_WIN_OK;
}

/* libc-free decimal (Law 3); returns the digit count written. */
static int fw_utoa(char *dst, uint32_t v)
{
    char tmp[10];
    int n = 0, k = 0;
    do { tmp[n++] = (char)('0' + (v % 10u)); v /= 10u; } while (v != 0u);
    while (n > 0) dst[k++] = tmp[--n];
    return k;
}

static int fw_cat(char *dst, int at, const char *s)
{
    while (*s != '\0' && at < 200) dst[at++] = *s++;
    return at;
}

static int fw_alert_wrap(const char *msg,
                         char out[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX]);

int finder_trash_alert_text(uint16_t items, uint32_t bytes,
                            char out[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX])
{
    char msg[208];
    int  n = 0;
    int  one = (items == 1u);

    /* The corpus wording (Sec 12), "Trash" for "Wastebasket"; the SINGULAR
     * agreement ("1 item, which uses ... this item") is authored, no local
     * reference shows it. */
    n = fw_cat(msg, n, "The Trash contains ");
    n += fw_utoa(msg + n, items);
    n = fw_cat(msg, n, one ? " item, which uses " : " items, which use ");
    n += fw_utoa(msg + n, (bytes + 1023u) / 1024u);
    n = fw_cat(msg, n, one ? "K of disk space. Are you sure you want to remove "
                             "this item permanently?"
                           : "K of disk space. Are you sure you want to remove "
                             "these items permanently?");
    msg[n] = '\0';
    return fw_alert_wrap(msg, out);
}

/* Special > Exit to InitechDOS (bead initech-3p1a): the operator's wording
 * (2026-10-16), the SAME wrap. */
int finder_exit_dos_alert_text(char out[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX])
{
    return fw_alert_wrap("Are you sure you want to exit to InitechDOS?", out);
}

/* Delete Now (bead initech-vnl4, audit 2026-10-23 desktop L001): the confirm
 * a full volume with no \TRASH puts up instead of a dead-end refusal. The
 * question is the Empty Trash confirm's own ("Are you sure you want to remove
 * ... permanently?", Sec 12); the first sentence, naming why the Trash cannot
 * take the item, is authored -- no local Finder string exists for it. */
int finder_delete_now_alert_text(const char *name,
                                 char out[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX])
{
    char msg[208];
    int  n = 0;

    n = fw_cat(msg, n, "There is no room on the disk for the Trash. Are you sure you want to remove ");
    n = fw_cat(msg, n, name != (const char *)0 ? name : "");
    n = fw_cat(msg, n, " permanently?");
    msg[n] = '\0';
    return fw_alert_wrap(msg, out);
}

/* tdnl.61/.63/.62/.66/.72/.57. Wording is the bead/operator wording,
 * NOT claimed as extracted native strings: no corresponding alert text is
 * captured locally. Chrome reuses the independently graded Dialog Manager. */
int finder_error_alert_text(finder_win_status_t st, const char *name,
                            char out[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX])
{
    const char *msg;
    char replace[208];
#ifdef FINDER_MUT_NO_MOVE_ALERT
    if(st==FINDER_WIN_ERR_CYCLE || st==FINDER_WIN_ERR_SAMEDIR ||
       st==FINDER_WIN_ERR_SERVICE || st==FINDER_WIN_ERR_MOVE) return 0;
#endif
    switch(st) {
    case FINDER_WIN_ERR_DISKFULL:
#ifdef FINDER_MUT_NO_DISK_ALERT
        return 0;
#endif
        msg="The disk is full."; break;
    case FINDER_WIN_ERR_FULL:
        msg="The folder could not be opened because there is not enough memory to open another window."; break;
    case FINDER_WIN_ERR_MEMORY:
        msg="The folder could not be opened because there is not enough memory to display its contents."; break;
    case FINDER_WIN_ERR_ENUM:
        msg="The folder could not be opened because its contents could not be read."; break;
    case FINDER_WIN_ERR_STORAGE:
        msg="The disk could not be updated."; break;
    case FINDER_WIN_ERR_LAUNCH:
    case FINDER_WIN_ERR_CRASH:
#ifdef FINDER_MUT_NO_LAUNCH_ALERT
        return 0;
#endif
        msg=st==FINDER_WIN_ERR_CRASH ? "The application has unexpectedly quit." :
            "The application could not be opened."; break;
    case FINDER_WIN_ERR_EXISTS: {
        int n=fw_cat(replace,0,"An item named ");
        n=fw_cat(replace,n,name?name:"");
        n=fw_cat(replace,n," already exists in this location. Do you want to replace it with the one you're moving?");
        replace[n]='\0'; msg=replace; break;
    }
    case FINDER_WIN_ERR_CYCLE:
#ifdef FINDER_MUT_NO_MOVE_ALERT
        return 0;
#endif
        msg="A folder cannot be moved inside itself."; break;
    case FINDER_WIN_ERR_SERVICE:
        msg="This item is used by the Finder and cannot be moved."; break;
    case FINDER_WIN_ERR_SAMEDIR:
        msg="The item is already in this folder."; break;
    case FINDER_WIN_ERR_NOTRASH:
        /* bead initech-vnl4 (audit 2026-10-23 desktop L007): opening the
         * Trash on a volume with no \TRASH (it could not be made: the disk
         * is full) names the operation that failed, never the generic "could
         * not be moved" -- no move was asked for. Wording authored (no local
         * Finder string for it); the System 6 guide's own advice for a full
         * disk is "remove items from the disk to free up more space"
         * (Apple_Macintosh_System_Software_Users_Guide_V6.0.pdf printed
         * p. 262), which the Delete Now route (finder_shell_delete_item)
         * makes possible.
         * MUTANT FINDER_MUT_NOTRASH_GENERIC (Rule 6): the pre-fix fall-through
         * to the move wording. NEVER in a real build. */
#ifndef FINDER_MUT_NOTRASH_GENERIC
        msg="The Trash could not be opened because there is no room for it on the disk."; break;
#endif
    default: msg="The item could not be moved. The disk may be full or the folder may no longer be available."; break;
    }
    return fw_alert_wrap(msg,out);
}

/* GREEDY WORD WRAP of an alert message into FINDER_ALERT_TEXT_W columns
 * (shared by every Finder alert). */
static int fw_alert_wrap(const char *msg,
                         char out[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX])
{
    int line = 0, i = 0;

    for (int l = 0; l < FINDER_ALERT_LINES; l++) out[l][0] = '\0';
    /* GREEDY WORD WRAP on Chicago 12 advances: a word joins the line when
     * the line plus a space plus the word still fits FINDER_ALERT_TEXT_W. */
    while (msg[i] != '\0' && line < FINDER_ALERT_LINES) {
        int j = i, ww = 0, len = 0, lw = 0;
        char *o = out[line];
        while (o[len] != '\0') { lw += chicago12_advance((unsigned char)o[len]); len++; }
        while (msg[j] != '\0' && msg[j] != ' ') ww += chicago12_advance((unsigned char)msg[j++]);
        if (len != 0 && lw + chicago12_advance(' ') + ww > FINDER_ALERT_TEXT_W) {
            line++;                               /* the word opens a new line */
            continue;
        }
        if (len != 0) o[len++] = ' ';
        while (i < j && len < FINDER_ALERT_LINE_MAX - 1) o[len++] = msg[i++];
        o[len] = '\0';
        i = j;
        while (msg[i] == ' ') i++;
    }
    return (line < FINDER_ALERT_LINES) ? line + 1 : FINDER_ALERT_LINES;
}

/* ===========================================================================
 * THE COMMAND EXECUTION HOOK  (finder_cmd.h FinderCtx.exec)
 * ===========================================================================*/
