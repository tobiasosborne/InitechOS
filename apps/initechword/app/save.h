/* Original Document Backup: goldens/dataloss/wordperfect/{collision,full,
 * readonly}, WP5.1 Reference pp.41-43,746. The phase-2 ruling strengthens
 * WP's full-disk behavior: restore the original name before reporting failure.
 *
 * Keep a separate original while calling either today's delete/rename helper
 * or a safer replacement helper. Never reuse another caller's scratch file.
 * All gate paths are module/BSS storage (Toolbox Files contract).
 * This is reported-error recovery, not power-loss atomicity. */
static char g_word_original[TBX_FILE_PATH_MAX];
static char g_word_backup[TBX_FILE_PATH_MAX];
static char g_word_stage[TBX_FILE_PATH_MAX];
static char g_word_recovery[TBX_FILE_PATH_MAX + 40];
static int g_word_blocked;

static int word_same(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static int word_extension(char *dst, const char *src, const char *ext)
{
    uint32_t i = 0, leaf = 0, dot = 0;
    for (; src[i]; i++) {
        if (src[i] == '\\') { leaf = i + 1u; dot = 0; }
        else if (src[i] == '.') dot = i;
    }
    if (dot < leaf || !dot) dot = i;
    if (dot + 5u > TBX_FILE_PATH_MAX) return 0;
    for (i = 0; i < dot; i++) dst[i] = src[i];
    dst[dot++] = '.';
    for (i = 0; i < 3; i++) dst[dot++] = ext[i];
    dst[dot] = 0;
    return 1;
}

/* Reserve an unused recovery name. The cooperative scheduler cannot interleave
 * another tenant between deleting our reservation and renaming the original. */
static int32_t word_reserve_path(char *path, const char *base, const char *ext,
                                 const char *avoid)
{
    int32_t h, cl;
    if (!word_extension(path, base, ext)) return TBX_ERR_BADARG;
    uint32_t end = slen(path);
    for (uint32_t i = 0; i < 100; i++) {
        path[end-2] = (char)('0' + i / 10u);
        path[end-1] = (char)('0' + i % 10u);
        if (word_same(path, base) || (avoid && word_same(path, avoid))) continue;
        h = tbx_file_create_new(path);
        if (h == TBX_ERR_DOS(80)) continue;
        if (h < 0) return h;
        cl = tbx_file_close(h);
        int32_t rc = tbx_file_delete(path);
        return cl < 0 ? cl : rc;
    }
    return TBX_ERR_DOS(80);
}

static int32_t word_reserve(char *path, const char *ext)
{
    return word_reserve_path(path, g_path, ext, 0);
}

static int32_t word_save(uint32_t n)
{
    int32_t h = tbx_file_open(g_path, TBX_FILE_READ_ONLY), rc;
    if (h == TBX_ERR_DOS(2)) return tbx_file_save(g_path, g_bank[1u-g_live], n);
    if (h < 0) return h;
    rc = tbx_file_close(h);
    if (rc < 0) return rc;
    rc = word_reserve(g_word_original, "W00");
    if (rc < 0) return rc;
    rc = word_reserve(g_word_stage, "N00");
    if (rc < 0) return rc;
    if (g_settings[6]) {
        if (!word_extension(g_word_backup, g_path, "BK!") || word_same(g_word_backup, g_path))
            return TBX_ERR_BADARG; /* never copy a backup onto itself */
        /* WP's captured full floppy reclaims the older backup's cluster.
         * A read-only backup refuses here without changing either file. */
        rc = tbx_file_delete(g_word_backup);
        if (rc < 0 && rc != TBX_ERR_DOS(2)) return rc;
        rc = tbx_file_copy_save(g_path, g_word_backup);
        if (rc < 0) return rc;
    }
    /* Finish the new stream before moving the old name. This also keeps the
     * complete candidate under OUR name if publication and restoration fail,
     * independently of either generation of the shared helper's cleanup. */
    rc = tbx_file_save(g_word_stage, g_bank[1u-g_live], n);
    if (rc != (int32_t)n) return rc;
    rc = tbx_file_rename(g_path, g_word_original);
    if (rc < 0) { (void)tbx_file_delete(g_word_stage); return rc; }
    rc = tbx_file_rename(g_word_stage, g_path);
    if (rc == TBX_OK) {
        /* The new version has committed. Cleanup cannot turn that success
         * into a reported failure; a refused delete retains an extra original. */
        (void)tbx_file_delete(g_word_original);
        return (int32_t)n;
    }
    /* The helper has no old target to destroy. Restore by metadata only:
     * even a full disk or persistent write error cannot prevent this rename. */
    if (tbx_file_rename(g_word_original, g_path) < 0) {
        /* Known residual until the kernel replace primitive (initech-7mb66).
         * Delete neither complete stream. Name the original and stop further
         * saves in this tenant; the original-path ruling remains unmet here. */
        g_word_blocked = 1;
        scopy(g_word_recovery, "ERROR: Original retained in ");
        scopy(g_word_recovery + slen(g_word_recovery), g_word_original);
        return TBX_ERR_BADARG;
    }
    (void)tbx_file_delete(g_word_stage);
    return rc;
}

/* The ordinary error row cannot hold a 127-character path. Keep the working
 * text in memory and show the complete recovery name over two full rows. */
static int word_recovery_paint(void)
{
    if (!g_word_blocked || g_err != g_word_recovery) return 0;
    iw_where_t w;
    iw_ed_where(&g_ed, &w); paint_doc(&w, EDIT_ROWS);
    setup_row(SCR_LINES-3, "ERROR: Original file retained as:", 0);
    uint32_t n = slen(g_word_original), first = n < SCR_COLS ? n : SCR_COLS;
    row_t r;
    row_clear(&r);
    for (uint32_t i=0; i<first; i++) r.t[i]=g_word_original[i];
    put_row(SCR_LINES-2, &r);
    row_clear(&r);
    if (n > first) {
        for (uint32_t i=first; i<n; i++) r.t[i-first]=g_word_original[i];
        put_row(SCR_LINES-1, &r);
    } else setup_row(SCR_LINES-1, "Press any key to continue", 0);
    return 1;
}
