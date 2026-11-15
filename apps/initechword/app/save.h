/* Original Document Backup: goldens/dataloss/wordperfect/{collision,full,
 * readonly}, WP5.1 Reference pp.41-43,746. The phase-2 ruling strengthens
 * WP's full-disk behavior: a reported failure leaves the previous document
 * under its own name (initech-go4vt).
 *
 * The document itself is published by the shared helper tbx_file_save
 * (sdk/include/tbx_save.h): a verified same-directory stage committed by the
 * kernel's FILE_REPLACE, whose reported errors preserve the destination.
 * All gate paths are module/BSS storage (Toolbox Files contract).
 * This is reported-error recovery, not power-loss atomicity.
 * No application-wide blocked/recovery state remains: Move also uses the
 * published replacement commit point. */
static char g_word_backup[TBX_FILE_PATH_MAX];

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

static int32_t word_save(uint32_t n)
{
    int32_t h = tbx_file_open(g_path, TBX_FILE_READ_ONLY), rc;
    if (h == TBX_ERR_DOS(2)) return tbx_file_save(g_path, g_bank[1u-g_live], n);
    if (h < 0) return h;
    rc = tbx_file_close(h);
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
    return tbx_file_save(g_path, g_bank[1u-g_live], n);
}
