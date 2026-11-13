/* Recording Toolbox, extracted from the legacy face oracle.
 * Included after face_a.c; application drivers remain component-local. */
static volatile int *g_ctr;            /* [0] checks, [1] failures */
#define CHECK(cond, id, msg) do { g_ctr[0]++; if (!(cond)) { g_ctr[1]++; \
    fprintf(stderr, "  FAIL %s %s (line %d)\n", (id), (msg), __LINE__); } } while (0)

/* ---- the recording mock ------------------------------------------------- */
/* RE-KEYED 2026-10-16 (bead initech-tdnl.51, stated): the window content is
 * 613 px since it starts inside the left body rail (frame.left + 6), so the
 * face is 76 cells wide (was 77). */
#define SCR_W 76
#define SCR_H 24
static char m_scr[SCR_H][SCR_W + 1];
static char m_inv[SCR_H][SCR_W];        /* drawn NAVY on WHITE (the cursor) */
static unsigned char m_face[SCR_H][SCR_W];   /* the txFace of the last plain-
                                         * colour draw of the cell (fg >> 8) */
static unsigned char m_col[SCR_H][SCR_W];    /* (fg token << 4) | bg token  */
static int  m_exit_calls, m_exit_rc;
static const char *m_dir;
static FILE *m_fh[8];
static uint32_t m_read_limit, m_write_limit; /* DOS short-transfer oracle */

int32_t tbx_register(void) { return TBX_OK; }
int32_t tbx_new_window(int32_t l, int32_t t, int32_t r, int32_t b, int32_t g)
{ (void)l; (void)t; (void)r; (void)b; return g == (int32_t)TBX_GOAWAY_ASK ? 1 : TBX_ERR_BADARG; }
int32_t tbx_set_wtitle(int32_t w, const char *s) { (void)w; (void)s; return TBX_OK; }
int32_t tbx_draw_text(int32_t w, int32_t x, int32_t y, const char *s, uint32_t f, uint32_t b)
{ (void)w; (void)x; (void)y; (void)s; (void)f; (void)b; return TBX_OK; }
int32_t tbx_fill_rect(int32_t w, int32_t l, int32_t t, int32_t r, int32_t b, uint32_t c)
{
    (void)w; (void)l; (void)t; (void)r; (void)b; (void)c;
    for (int i = 0; i < SCR_H; i++) { memset(m_scr[i], '?', SCR_W); memset(m_inv[i], 0, SCR_W); }
    return TBX_OK;
}
int32_t tbx_set_mbar(const tbx_menubar_t *b) { (void)b; return TBX_OK; }
int32_t tbx_draw_menubar(void) { return TBX_OK; }
int32_t tbx_alert(const char *t, const char *d, const char *c, const char *o)
{ (void)t; (void)d; (void)c; (void)o; return TBX_ERR_BADCODE; }
/* TBX_CLOCK (spec/cp_dialog.h) answers m_clock; the List Files directory is
 * the table a test installs (bead initech-qvgd; test_iword_listfiles.c). */
static cp_time_t m_clock = { 1992, 3, 1, 9, 0, 0, 0 };
typedef struct m_dirent { const char *dir; const char *name; uint8_t attr; uint16_t time, date; uint32_t size; } m_dirent_t;
static const m_dirent_t *m_dirents;
static int m_ndirents;
static int32_t m_free = TBX_ERR_DOS(0x0F);
static int m_find_calls;
int32_t tbx_trap(uint32_t c, uint32_t n, const uint32_t *a) { (void)c; (void)n; (void)a; return TBX_ERR_BADCODE; }
int32_t tbx_get_clock(void *t) { memcpy(t, &m_clock, sizeof m_clock); return TBX_OK; }
#ifndef IWORD_LF_OPS_MOCK
int32_t tbx_file_find(const char *pat, uint32_t attr, void *recs, uint32_t max)
{
    size_t L = strlen(pat);
    uint32_t k = 0;
    m_find_calls++;
    if (L < 3 || strcmp(pat + L - 3, "*.*") || attr != 0x16u) return TBX_ERR_BADARG;
    for (int i = 0; i < m_ndirents; i++) {
        const m_dirent_t *e = &m_dirents[i];
        uint8_t *r = (uint8_t *)recs + k * TBX_FIND_REC_BYTES;
        if (strlen(e->dir) != L - 3 || strncmp(e->dir, pat, L - 3)) continue;
        if (k == max) break;
        memset(r, 0, TBX_FIND_REC_BYTES);
        r[0] = e->attr; r[1] = (uint8_t)e->time; r[2] = (uint8_t)(e->time >> 8);
        r[3] = (uint8_t)e->date; r[4] = (uint8_t)(e->date >> 8);
        r[5] = (uint8_t)e->size; r[6] = (uint8_t)(e->size >> 8); r[7] = (uint8_t)(e->size >> 16); r[8] = (uint8_t)(e->size >> 24);
        strncpy((char *)r + 9, e->name, 12);
        k++;
    }
    return k ? (int32_t)k : (L == 3 ? 0 : TBX_ERR_DOS(3));
}
#endif
int32_t tbx_disk_free(void) { return m_free; }

int32_t tbx_draw_cells(int32_t w, int32_t x, int32_t y, const char *s, uint32_t f, uint32_t b)
{
    int col = (x - 1) / 8, line = y / 16;
    (void)w;
    if (line < 0 || line >= SCR_H || col < 0 || (x - 1) % 8 || y % 16) return TBX_ERR_BADARG;
    if ((f & 0xFFu) >= TBX_COLOR_COUNT || (f >> 16) || ((f >> 8) & ~0x07u) || b >= TBX_COLOR_COUNT)
        return TBX_ERR_BADARG;                 /* toolbox_gate.h Sec 7a STYLE */
    for (int i = 0; s[i] && col + i < SCR_W; i++) {
        int inv = (f == TBX_COLOR_NAVY && b == TBX_COLOR_WHITE);
        m_scr[line][col + i] = s[i];
        m_inv[line][col + i] = (char)inv;
        if (!inv) { m_face[line][col + i] = (unsigned char)(f >> 8); m_col[line][col + i] = (unsigned char)(((f & 0xFFu) << 4) | b); }
    }
    return TBX_OK;
}

int32_t tbx_exit(int32_t rc) { m_exit_calls++; m_exit_rc = rc; return TBX_OK; }

static void host_name(const char *path, char *out)
{
    char *o;
    snprintf(out, 300, "%s/", m_dir);
    o = out + strlen(out);
    for (const char *p = path; *p; p++) *o++ = *p == '\\' ? '/' : *p;   /* UPPER names; DIR\NAME */
    *o = 0;
}
static int32_t new_handle(FILE *f)
{
    for (int i = 0; i < 8; i++) if (!m_fh[i]) { m_fh[i] = f; return 5 + i; }
    fclose(f); return TBX_ERR_DOS(4);
}
static FILE *fh(int32_t h) { return (h >= 5 && h < 13) ? m_fh[h - 5] : 0; }
#ifndef IWORD_LF_OPS_MOCK
int32_t tbx_file_create(const char *path)
{ char hp[300]; FILE *f; host_name(path, hp); f = fopen(hp, "w+b"); return f ? new_handle(f) : TBX_ERR_DOS(5); }
int32_t tbx_file_open(const char *path, uint32_t mode)
{ char hp[300]; FILE *f; (void)mode; if (!m_dir) return TBX_ERR_DOS(2); host_name(path, hp); f = fopen(hp, "r+b"); return f ? new_handle(f) : TBX_ERR_DOS(2); }
int32_t tbx_file_read(int32_t h, void *b, uint32_t n)
{ FILE *f = fh(h); if (m_read_limit && n > m_read_limit) n = m_read_limit; return f ? (int32_t)fread(b, 1, n, f) : TBX_ERR_BADARG; }
int32_t tbx_file_write(int32_t h, const void *b, uint32_t n)
{ FILE *f = fh(h); if (m_write_limit && n > m_write_limit) n = m_write_limit; return f ? (int32_t)fwrite(b, 1, n, f) : TBX_ERR_BADARG; }
int32_t tbx_file_seek(int32_t h, int32_t d, uint32_t o)
{
    FILE *f = fh(h);
    if (!f || fseek(f, d, o == TBX_FILE_FROM_END ? SEEK_END : o == TBX_FILE_FROM_HERE ? SEEK_CUR : SEEK_SET)) return TBX_ERR_BADARG;
    return (int32_t)ftell(f);
}
int32_t tbx_file_close(int32_t h)
{ FILE *f = fh(h); if (!f) return TBX_ERR_BADARG; fclose(f); m_fh[h - 5] = 0; return TBX_OK; }
int32_t tbx_file_delete(const char *path)
{ char hp[300]; host_name(path, hp); return remove(hp) == 0 ? TBX_OK : TBX_ERR_DOS(2); }
/* Sec 10b CREATE_NEW (initech-mcep): DOS 3.3 AH=5Bh -- never truncates. */
int32_t tbx_file_create_new(const char *path)
{
    char hp[300]; FILE *f;
    host_name(path, hp);
    if ((f = fopen(hp, "rb")) != NULL) { fclose(f); return TBX_ERR_DOS(0x50); }
    return tbx_file_create(path);
}
/* Sec 10b RENAME (initech-mcep): DOS 3.3 AH=56h -- an existing target is
 * access denied, a missing source is file not found. */
int32_t tbx_file_rename(const char *o, const char *n)
{
    char ho[300], hn[300]; FILE *f;
    host_name(o, ho); host_name(n, hn);
    if ((f = fopen(hn, "rb")) != NULL) { fclose(f); return TBX_ERR_DOS(5); }
    return rename(ho, hn) == 0 ? TBX_OK : TBX_ERR_DOS(2);
}
#endif


/* Component input contract must match the actual Toolbox ABI. */
_Static_assert(IW_LF_REC_BYTES == TBX_FIND_REC_BYTES, "List Files record BYTES");
_Static_assert(IW_LF_REC_ATTR_OFF == TBX_FIND_REC_ATTR_OFF, "List Files record ATTR_OFF");
_Static_assert(IW_LF_REC_TIME_OFF == TBX_FIND_REC_TIME_OFF, "List Files record TIME_OFF");
_Static_assert(IW_LF_REC_DATE_OFF == TBX_FIND_REC_DATE_OFF, "List Files record DATE_OFF");
_Static_assert(IW_LF_REC_SIZE_OFF == TBX_FIND_REC_SIZE_OFF, "List Files record SIZE_OFF");
_Static_assert(IW_LF_REC_NAME_OFF == TBX_FIND_REC_NAME_OFF, "List Files record NAME_OFF");
