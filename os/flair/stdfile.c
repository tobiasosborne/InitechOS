/*
 * os/flair/stdfile.c -- FLAIR Standard File (SFGetFile) implementation
 * (first cut: the navigate/select/return logic).
 *
 * beads: initech-gymo (FLAIR Phase 4.5 -- SFGetFile first cut).
 * Ref:   stdfile.h (the full Law-1 citation block: IM-I Standard File Package,
 *        the SFGetFile/SFReply model, ADR-0013 Sec 3.6, and the local building
 *        blocks os/flair/list.h + os/flair/dialog.h + os/milton/fat12.h).
 *        CLAUDE.md Law 2 (oracle truth, never by-construction), Law 3 (artifact
 *        C, freestanding, no libc), Rule 2 (fail loud), Rule 6 (mutation-proven),
 *        Rule 11 (deterministic), Rule 12 (ASCII-clean).
 *
 * THE FILE LIST IS BUILT ON THE REAL FlairList (os/flair/list.{c,h}): see
 * sf_rebuild_list below -- FlairList_init + FlairLAddRow + FlairLSetCell. We do
 * NOT reinvent a list; the List Manager is a reusable platform service.
 *
 * MUTANT COVERAGE (Rule 6 -- each #ifdef must COMPILE and go RED):
 *
 *   SF_MUT_RETURN_WRONG_CELL
 *     FlairSF_doOpen returns the FIXED entry index 0 regardless of the actual
 *     selection. Oracle step 5 selects row 2 ("TPS.TXT") and expects doOpen to
 *     return "TPS.TXT"; the mutant returns entries[0] = "REPORT.DBF" -> RED.
 *
 *   SF_MUT_NO_NAVIGATE
 *     FlairSF_navigate does NOT re-enumerate (it returns OK without pushing the
 *     path / rebuilding the list), leaving the PARENT list in place. Oracle
 *     step 3 navigates into "DATA" and expects count==2 / row 0 == "Q1.WKS";
 *     the mutant leaves count==3 / row 0 == "REPORT.DBF" -> RED.
 *
 *   SF_MUT_CANCEL_GOOD
 *     FlairSF_doCancel sets reply.good = 1 instead of 0. Oracle step 6 expects
 *     good == 0 after Cancel -> RED.
 *
 * ARTIFACT code: freestanding, no libc, no malloc. <stdint.h>/<stddef.h> only
 * (via stdfile.h -> list.h). Dual-compiles for the host oracle and the
 * -m32 -ffreestanding -nostdlib kernel type-check.
 *
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#include <stdint.h>
#include <stddef.h>

#include "stdfile.h"

/* --------------------------------------------------------------------------
 * Freestanding byte helpers -- no libc, no string.h (Law 3).
 * -------------------------------------------------------------------------- */

/* sf_strlen: length of a NUL-terminated string, capped at `cap` (never reads
 * past the buffer; returns `cap` if no NUL is found within it). */
static int16_t sf_strlen(const char *s, int16_t cap)
{
    int16_t n = 0;
    while (n < cap && s[n] != '\0') {
        n++;
    }
    return n;
}

/* sf_bytecopy: copy n bytes src->dst (non-overlapping). */
static void sf_bytecopy(void *dst, const void *src, int16_t n)
{
    uint8_t       *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    int16_t        i;
    for (i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

/* sf_byteset: set n bytes at dst to val. */
static void sf_byteset(void *dst, uint8_t val, int16_t n)
{
    uint8_t *d = (uint8_t *)dst;
    int16_t  i;
    for (i = 0; i < n; i++) {
        d[i] = val;
    }
}

/* sf_name_copy: copy a NUL-terminated 8.3 name into a FLAIR_SF_NAME_MAX buffer,
 * always NUL-terminating (truncation-safe; the source is always <= 12 chars by
 * the 8.3 contract, but we cap defensively -- Rule 2 never overruns). */
static void sf_name_copy(char *dst, const char *src)
{
    int16_t i = 0;
    while (i < (int16_t)(FLAIR_SF_NAME_MAX - 1) && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

/* --------------------------------------------------------------------------
 * sf_rebuild_list -- rebuild the FlairList from sf->entries[0..count).
 *
 * THIS is where the file list is built ON the real List Manager: FlairList_init
 * binds the list to the stored cellSize/rView, FlairLAddRow appends one row per
 * entry (auto single-column), and FlairLSetCell writes each 8.3 name into
 * column 0. selFlags is set to lOnlyOne so FlairLClick single-selects.
 *
 * Fail loud (Rule 2): an underlying FlairList op failing -> FLAIR_SF_ERR_LIST.
 * Returns FLAIR_SF_OK on success.
 * -------------------------------------------------------------------------- */
static int sf_rebuild_list(FlairStandardFile *sf)
{
    int16_t i;
    int     r;

    FlairList_init(&sf->list, sf->cellSize, sf->rView);

    /* Single-selection policy (mirror IM-I Standard File: one highlighted
     * item). FlairLClick consults selFlags & lOnlyOne to deselect others. */
    sf->list.selFlags = FLAIR_LSEL_ONLY_ONE;

    if (sf->count <= 0) {
        return FLAIR_SF_OK;          /* empty directory: a 0-row list           */
    }

    r = FlairLAddRow(&sf->list, sf->count, -1);
    if (r != FLAIR_LIST_OK) {
        return FLAIR_SF_ERR_LIST;
    }

    for (i = 0; i < sf->count; i++) {
        Cell    c;
        int16_t len;
        c.h = 0;                     /* column 0 (single-column file list)       */
        c.v = i;                     /* row i                                    */
        len = sf_strlen(sf->entries[i].name, (int16_t)FLAIR_SF_NAME_MAX);
        r = FlairLSetCell(&sf->list, sf->entries[i].name, len, c);
        if (r != FLAIR_LIST_OK) {
            return FLAIR_SF_ERR_LIST;
        }
    }
    return FLAIR_SF_OK;
}

/* --------------------------------------------------------------------------
 * sf_enumerate -- call the provider for `path`, validate, and store into
 * sf->entries[]/sf->count. Fail loud (Rule 2): provider error ->
 * FLAIR_SF_ERR_PROVIDER; count > MAX -> FLAIR_SF_ERR_OVERFLOW.
 * Does NOT touch sf->path (the caller owns the path transition).
 * -------------------------------------------------------------------------- */
static int sf_enumerate(FlairStandardFile *sf, const char *path)
{
    int n = sf->provider(path, sf->entries, (int16_t)FLAIR_SF_MAX_ENTRIES,
                         sf->user);
    if (n < 0) {
        return FLAIR_SF_ERR_PROVIDER;
    }
    if (n > (int)FLAIR_SF_MAX_ENTRIES) {
        return FLAIR_SF_ERR_OVERFLOW;
    }
    sf->count = (int16_t)n;
    return FLAIR_SF_OK;
}

/* --------------------------------------------------------------------------
 * sf_set_path / sf_child_path / sf_parent_path -- the "/"-rooted component
 * stack. Root is "/"; a child of "/" is "/NAME"; a child of "/DATA" is
 * "/DATA/NAME"; the parent of "/DATA/SUB" is "/DATA"; the parent of "/DATA" is
 * "/". Pure string math; fail loud on overflow (Rule 2).
 * -------------------------------------------------------------------------- */

/* sf_set_path: store `path` (NUL-terminated, length `len`) into sf->path. */
static void sf_set_path(FlairStandardFile *sf, const char *path, int16_t len)
{
    sf_bytecopy(sf->path, path, len);
    sf->path[len] = '\0';
    sf->path_len = len;
}

/* sf_child_path: write the child path (sf->path descended into `name`) into
 * `out` (capacity FLAIR_SF_PATH_MAX) and set *out_len. Returns FLAIR_SF_OK or
 * FLAIR_SF_ERR_PATH if it would overflow. */
static int sf_child_path(const FlairStandardFile *sf, const char *name,
                         char *out, int16_t *out_len)
{
    int16_t plen = sf->path_len;
    int16_t nlen = sf_strlen(name, (int16_t)FLAIR_SF_NAME_MAX);
    int16_t need;
    int16_t pos;

    /* "/" already ends in '/': child = path + name. Otherwise insert a '/'. */
    int needs_slash = (plen > 0 && sf->path[plen - 1] != '/');

    need = (int16_t)(plen + (needs_slash ? 1 : 0) + nlen); /* excludes NUL */
    if (need >= (int16_t)FLAIR_SF_PATH_MAX) {
        return FLAIR_SF_ERR_PATH;
    }

    pos = 0;
    sf_bytecopy(out + pos, sf->path, plen);
    pos = plen;
    if (needs_slash) {
        out[pos] = '/';
        pos = (int16_t)(pos + 1);
    }
    sf_bytecopy(out + pos, name, nlen);
    pos = (int16_t)(pos + nlen);
    out[pos] = '\0';
    *out_len = pos;
    return FLAIR_SF_OK;
}

/* sf_parent_path: write the parent of sf->path into `out` and set *out_len.
 * The root ("/") has no parent (returns the root unchanged). */
static void sf_parent_path(const FlairStandardFile *sf, char *out,
                           int16_t *out_len)
{
    int16_t plen = sf->path_len;
    int16_t last = -1;
    int16_t i;

    if (plen <= 1) {                 /* "/" or empty: already at the root        */
        out[0] = '/';
        out[1] = '\0';
        *out_len = 1;
        return;
    }

    for (i = 0; i < plen; i++) {
        if (sf->path[i] == '/') {
            last = i;
        }
    }
    if (last <= 0) {                 /* parent is the root                       */
        out[0] = '/';
        out[1] = '\0';
        *out_len = 1;
        return;
    }
    sf_bytecopy(out, sf->path, last);
    out[last] = '\0';
    *out_len = last;
}

/* sf_current_sel: the FIRST selected row in [0, count), read FROM the FlairList
 * (FlairLGetSelect -- the List Manager is the source of truth), or -1 if none. */
static int16_t sf_current_sel(const FlairStandardFile *sf)
{
    int16_t i;
    for (i = 0; i < sf->count; i++) {
        Cell c;
        c.h = 0;
        c.v = i;
        if (FlairLGetSelect(&sf->list, c) == 1) {
            return i;
        }
    }
    return -1;
}

/* ==========================================================================
 * Public API.
 * ========================================================================== */

int FlairSF_open(FlairStandardFile *sf, FlairSFEnumProc provider, void *user,
                 Cell cellSize, rgn_rect_t rView)
{
    int rc;

    if (!sf || !provider) {
        return FLAIR_SF_ERR_NULL;
    }

    /* Zero the record for a deterministic initial state (Rule 11). */
    sf_byteset(sf, 0, (int16_t)sizeof(*sf));

    sf->provider = provider;
    sf->user     = user;
    sf->cellSize = cellSize;
    sf->rView    = rView;

    /* Start at the root. */
    sf_set_path(sf, "/", 1);

    rc = sf_enumerate(sf, sf->path);
    if (rc != FLAIR_SF_OK) {
        return rc;
    }
    return sf_rebuild_list(sf);
}

int16_t FlairSF_count(const FlairStandardFile *sf)
{
    if (!sf) {
        return 0;                    /* defensive (mirrors FlairLClick NULL guard)*/
    }
    return sf->count;
}

void FlairSF_select(FlairStandardFile *sf, int16_t index)
{
    int16_t i;
    Cell    c;

    if (!sf) {
        return;                      /* defensive NULL guard                     */
    }
    if (index < 0 || index >= sf->count) {
        return;                      /* out of range: select nothing             */
    }

    /* Mirror lOnlyOne: clear every row, then select `index`. */
    c.h = 0;
    for (i = 0; i < sf->count; i++) {
        c.v = i;
        FlairLSetSelect(&sf->list, 0, c);
    }
    c.v = index;
    FlairLSetSelect(&sf->list, 1, c);
}

int FlairSF_click(FlairStandardFile *sf, flair_point_t localPt)
{
    Cell hit;
    int  r;

    if (!sf) {
        return -1;                   /* defensive NULL guard                     */
    }

    /* FlairLClick applies lOnlyOne single-selection and reports the hit cell. */
    r = FlairLClick(&sf->list, localPt, &hit);
    if (r != 1) {
        return -1;                   /* miss                                     */
    }
    return (int)hit.v;               /* hit row index                            */
}

int FlairSF_navigate(FlairStandardFile *sf, int16_t index)
{
    char    child[FLAIR_SF_PATH_MAX];
    int16_t child_len;
    int     rc;

    if (!sf) {
        return FLAIR_SF_ERR_NULL;
    }
    if (index < 0 || index >= sf->count) {
        return FLAIR_SF_ERR_RANGE;   /* fail loud (Rule 2)                       */
    }
    if (!sf->entries[index].is_dir) {
        return FLAIR_SF_ERR_NOTDIR;  /* navigate only descends into folders      */
    }

    /* Build the child path (used by both the canonical and mutant builds, so
     * the helper stays referenced -- the mutant must still COMPILE, Rule 6). */
    rc = sf_child_path(sf, sf->entries[index].name, child, &child_len);
    if (rc != FLAIR_SF_OK) {
        return rc;
    }

#ifdef SF_MUT_NO_NAVIGATE
    /*
     * MUTANT SF_MUT_NO_NAVIGATE: do NOT re-enumerate. Return OK while leaving
     * the PARENT directory's entries/list/path in place. Oracle step 3 expects
     * count==2 / row 0 == "Q1.WKS" after navigating into "DATA"; the parent
     * still has count==3 / row 0 == "REPORT.DBF" -> RED.
     * Ref: CLAUDE.md Rule 6 (mutation-proven golden).
     */
    return FLAIR_SF_OK;
#else
    /* Re-enumerate the child, then COMMIT (path + list) only on success so a
     * provider error leaves the parent view intact (Rule 2). */
    rc = sf_enumerate(sf, child);
    if (rc != FLAIR_SF_OK) {
        return rc;
    }
    sf_set_path(sf, child, child_len);
    return sf_rebuild_list(sf);
#endif
}

int FlairSF_up(FlairStandardFile *sf)
{
    char    parent[FLAIR_SF_PATH_MAX];
    int16_t parent_len;
    int     rc;

    if (!sf) {
        return FLAIR_SF_ERR_NULL;
    }
    if (sf->path_len <= 1) {
        return FLAIR_SF_OK;          /* already at the root: no-op               */
    }

    sf_parent_path(sf, parent, &parent_len);
    rc = sf_enumerate(sf, parent);
    if (rc != FLAIR_SF_OK) {
        return rc;
    }
    sf_set_path(sf, parent, parent_len);
    return sf_rebuild_list(sf);
}

void FlairSF_doOpen(FlairStandardFile *sf, FlairSFReply *reply)
{
    int16_t sel;

    if (!sf || !reply) {
        return;                      /* defensive NULL guard                     */
    }

    sel = sf_current_sel(sf);        /* canonical: read the List Manager state   */

#ifdef SF_MUT_RETURN_WRONG_CELL
    /*
     * MUTANT SF_MUT_RETURN_WRONG_CELL: ignore the real selection and use the
     * FIXED index 0. Oracle step 5 selects row 2 ("TPS.TXT") and expects doOpen
     * to return "TPS.TXT"; the mutant returns entries[0] = "REPORT.DBF" -> RED.
     * Ref: CLAUDE.md Rule 6 (mutation-proven golden).
     */
    sel = 0;
#endif

    if (sel < 0 || sel >= sf->count) {
        /* Nothing selected: Open confirms nothing (IM-I: Open is disabled). */
        reply->good = 0;
        reply->fName[0] = '\0';
        reply->is_dir = 0;
        return;
    }

    if (sf->entries[sel].is_dir) {
        /* FOLDER-ON-OPEN (IM-I): navigate into it; do NOT confirm. Copy the
         * folder name FIRST (navigate overwrites entries[]). */
        sf_name_copy(reply->fName, sf->entries[sel].name);
        reply->is_dir = 1;
        reply->good   = 0;           /* not-yet-confirmed (we descended)         */
        (void)FlairSF_navigate(sf, sel);
        return;
    }

    /* FILE-ON-OPEN (IM-I): confirm with the chosen file's name. */
    sf_name_copy(reply->fName, sf->entries[sel].name);
    reply->is_dir = 0;
    reply->good   = 1;               /* SFReply.good = TRUE                      */
}

void FlairSF_doCancel(FlairStandardFile *sf, FlairSFReply *reply)
{
    (void)sf;
    if (!reply) {
        return;                      /* defensive NULL guard                     */
    }

#ifdef SF_MUT_CANCEL_GOOD
    /*
     * MUTANT SF_MUT_CANCEL_GOOD: set good = 1 on Cancel. Oracle step 6 expects
     * good == 0 after Cancel -> RED.
     * Ref: CLAUDE.md Rule 6 (mutation-proven golden).
     */
    reply->good = 1;
#else
    reply->good = 0;                 /* SFReply.good = FALSE on Cancel (IM-I)    */
#endif
    reply->fName[0] = '\0';
    reply->is_dir = 0;
}
