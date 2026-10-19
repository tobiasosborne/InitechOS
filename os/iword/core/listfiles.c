/*
 * os/iword/core/listfiles.c -- InitechWord's List Files (F5) rows, composed
 * exactly as the real WordPerfect 5.1 lays them out, at WP's own 80 columns,
 * then fitted to the face's cells by the locked column deletes (THE ARTIFACT;
 * bead initech-qvgd, IWORD P3 remainder).
 *
 * Ref: spec/iword/listfiles.h -- every column, label and order from the real
 *      WP 5.1 mint ../wordperfect51-decomp mint/work/LISTF/mint-results-005.md
 *      (grids LF11_06 list, LF11_09 Look, LF11_01 prompt);
 *      spec/toolbox_gate.h Sec 10a (the 22-byte find record);
 *      DOS 3.3 PRM AH=4Eh (the packed date: bits 15-9 year - 1980, 8-5 month,
 *      4-0 day; time: 15-11 hour, 10-5 minute).
 * The two-digit year is WordPerfect's own arithmetic -- '0' + (year - 1900) / 10,
 * '0' + (year - 1900) % 10 -- so 1992 is "92" and a 2026 clock prints "<6"
 * as the real WP 5.1 does (mint 005 ledger, row 0). Canon, not fixed.
 * Pure: no Toolbox call, no I/O; the face draws what these compose.
 * Freestanding C, ASCII-clean (Rule 12), deterministic (Rule 11).
 */
#include "iword.h"
#include "iword/listfiles.h"
#include "toolbox_gate.h"     /* Sec 10a: the 22-byte find record */

static void put(char *row, uint32_t col, const char *s)
{
    for (uint32_t i = 0; s[i] && col + i < IW_LF_WP_COLS; i++) row[col + i] = s[i];
}

void iw_lf_blank(char *row)
{
    for (uint32_t i = 0; i < IW_LF_WP_COLS; i++) row[i] = ' ';
    row[IW_LF_WP_COLS] = 0;
}

uint32_t iw_lf_group(uint32_t v, char *out)
{
    char t[16];
    uint32_t n = 0, k = 0, i;
    do {
        if (k == 3u) { t[n++] = ','; k = 0; }
        t[n++] = (char)('0' + v % 10u); v /= 10u; k++;
    } while (v);
    for (i = 0; i < n; i++) out[i] = t[n - 1u - i];
    out[n] = 0;
    return n;
}

/* the value comma-grouped, right-aligned to end at column `end` */
static void put_right(char *row, uint32_t end, uint32_t v)
{
    char num[16];
    uint32_t n = iw_lf_group(v, num);
#ifdef IWORD_MUT_LF_COLUMN
    /* MUTANT (Rule 6; test-iword-listfiles-mutant, test-flair-iword-listfiles-
     * mutant): every right-aligned figure one column right. NEVER real. */
    end++;
#endif
    if (end + 1u >= n && end < IW_LF_WP_COLS) put(row, end + 1u - n, num);
}

static void two(char *at, uint32_t v) { at[0] = (char)('0' + v / 10u % 10u); at[1] = (char)('0' + v % 10u); }

void iw_lf_put_date(char *at, uint32_t year, uint32_t month, uint32_t day)
{
    uint32_t yy = year >= 1900u ? year - 1900u : 0u;   /* WP's arithmetic */
    two(at, month); at[2] = '-';
    two(at + 3, day); at[5] = '-';
    at[6] = (char)('0' + yy / 10u); at[7] = (char)('0' + yy % 10u);
}

void iw_lf_put_time(char *at, uint32_t hour, uint32_t minute)
{
    uint32_t h = hour % 12u;
    two(at, h ? h : 12u); at[2] = ':';
    two(at + 3, minute);
    at[5] = hour >= 12u ? 'p' : 'a';
}

/* A zero date word shows no stamp at all (mint 005 LF14_02: the kernel-made
 * DESKTOP.DB and TRASH, date and time 0, list with blank date and time). */
static void put_dos_stamp(char *row, uint32_t col, uint16_t date, uint16_t time)
{
#ifndef IWORD_MUT_LF_ZERO_DATE
    if (date == 0u) return;
#endif
    iw_lf_put_date(row + col, 1980u + (date >> 9), (date >> 5) & 15u, date & 31u);
    iw_lf_put_time(row + col + IW_LF_E_TIME - IW_LF_E_DATE, time >> 11, (time >> 5) & 63u);
}

void iw_lf_rec(const uint8_t *rec, iw_lf_ent_t *e)
{
    uint32_t i;
    e->attr = rec[TBX_FIND_REC_ATTR_OFF];
    e->time = (uint16_t)(rec[TBX_FIND_REC_TIME_OFF] | rec[TBX_FIND_REC_TIME_OFF + 1u] << 8);
    e->date = (uint16_t)(rec[TBX_FIND_REC_DATE_OFF] | rec[TBX_FIND_REC_DATE_OFF + 1u] << 8);
    e->size = (uint32_t)rec[TBX_FIND_REC_SIZE_OFF] | (uint32_t)rec[TBX_FIND_REC_SIZE_OFF + 1u] << 8 |
              (uint32_t)rec[TBX_FIND_REC_SIZE_OFF + 2u] << 16 | (uint32_t)rec[TBX_FIND_REC_SIZE_OFF + 3u] << 24;
    for (i = 0; i < 12u && rec[TBX_FIND_REC_NAME_OFF + i]; i++) e->name[i] = (char)rec[TBX_FIND_REC_NAME_OFF + i];
    e->name[i] = 0;
}

/* the 8-cell name and 3-cell extension of "NAME.EXT" */
static void split(const char *nm, char base[9], char ext[4])
{
    uint32_t i = 0, k = 0;
    for (; nm[i] && nm[i] != '.' && k < 8u; i++) base[k++] = nm[i];
    base[k] = 0;
    while (nm[i] && nm[i] != '.') i++;
    k = 0;
    if (nm[i] == '.') for (i++; nm[i] && k < 3u; i++) ext[k++] = nm[i];
    ext[k] = 0;
}

/* WP's order (spec "the list"): directories before files, each by the
 * space-padded 8-cell name, then the space-padded 3-cell extension. */
static void sort_key(const char *nm, char key[11])
{
    char base[9], ext[4];
    uint32_t i;
    split(nm, base, ext);
    for (i = 0; i < 11u; i++) key[i] = ' ';
    for (i = 0; base[i]; i++) key[i] = base[i];
    for (i = 0; ext[i]; i++) key[8u + i] = ext[i];
}
static int before(const iw_lf_ent_t *a, const iw_lf_ent_t *b)
{
    char ka[11], kb[11];
    int ad = (a->attr & IW_LF_ATTR_DIR) != 0, bd = (b->attr & IW_LF_ATTR_DIR) != 0;
    if (ad != bd) return ad;
    sort_key(a->name, ka); sort_key(b->name, kb);
    for (uint32_t i = 0; i < 11u; i++)
        if (ka[i] != kb[i]) return (unsigned char)ka[i] < (unsigned char)kb[i];
    return 0;
}

uint32_t iw_lf_load(const uint8_t *recs, uint32_t nrec, iw_lf_ent_t *out, uint32_t cap,
                    uint32_t *used, uint32_t *files)
{
    uint32_t n = 0, i, j;
    *used = 0; *files = 0;
    for (i = 0; i < nrec && n < cap; i++) {
        iw_lf_ent_t e;
        iw_lf_rec(recs + i * TBX_FIND_REC_BYTES, &e);
        if (e.name[0] == '.') continue;          /* "." and "..": slots 0 and 1 */
        if (!(e.attr & IW_LF_ATTR_DIR)) { *used += e.size; (*files)++; }
        /* insertion into the sorted run (stable; n is small) */
        for (j = n; j > 0 && before(&e, &out[j - 1u]); j--) out[j] = out[j - 1u];
        out[j] = e;
        n++;
    }
    return n;
}

void iw_lf_row_header0(char *row, const iw_lf_clock_t *c, const char *pattern)
{
    iw_lf_blank(row);
    iw_lf_put_date(row + IW_LF_DATE_COL, c->year, c->month, c->day);
    iw_lf_put_time(row + IW_LF_TIME_COL, c->hour, c->minute);
    put(row, IW_LF_DIR_COL, IW_LF_DIR_LABEL);
    put(row, IW_LF_DIR_COL + sizeof IW_LF_DIR_LABEL - 1u, pattern);
}

void iw_lf_row_header1(char *row, uint32_t docsize, uint32_t free_bytes, uint32_t used, uint32_t files)
{
    iw_lf_blank(row);
    put(row, IW_LF_DOCSIZE_COL, IW_LF_DOCSIZE_LABEL); put_right(row, IW_LF_DOCSIZE_END, docsize);
    put(row, IW_LF_FREE_COL, IW_LF_FREE_LABEL);       put_right(row, IW_LF_FREE_END, free_bytes);
    put(row, IW_LF_USED_COL, IW_LF_USED_LABEL);       put_right(row, IW_LF_USED_END, used);
    put(row, IW_LF_FILES_COL, IW_LF_FILES_LABEL);     put_right(row, IW_LF_FILES_END, files);
}

/* slot 0 ".  Current", slot 1 "..  Parent", slot k >= 2 the entry k - 2 */
static void put_slot(char *row, uint32_t lead, const iw_lf_ent_t *ents, uint32_t slot)
{
    if (slot < 2u) {
        put(row, lead, slot ? ".." : ".");
        put(row, lead + IW_LF_E_WORD, slot ? IW_LF_PARENT : IW_LF_CURRENT);
        put(row, lead + IW_LF_E_DIR, IW_LF_DIR_TAG);
        return;
    }
    {
        const iw_lf_ent_t *e = &ents[slot - 2u];
        char base[9], ext[4];
        split(e->name, base, ext);
        put(row, lead, base);
        row[lead + IW_LF_E_DOT] = '.';
        put(row, lead + IW_LF_E_EXT, ext);
        if (e->attr & IW_LF_ATTR_DIR) put(row, lead + IW_LF_E_DIR, IW_LF_DIR_TAG);
        else put_right(row, lead + IW_LF_E_SIZE_END, e->size);
        put_dos_stamp(row, lead + IW_LF_E_DATE, e->date, e->time);
    }
}

void iw_lf_row_list(char *row, const iw_lf_ent_t *ents, uint32_t nents, uint32_t list_row)
{
    uint32_t slots = nents + 2u, s = list_row * 2u;
    iw_lf_blank(row);
    if (s < slots) put_slot(row, IW_LF_LEAD_LEFT, ents, s);
    if (s + 1u < slots) {
        row[IW_LF_BAR_COL] = IW_LF_BAR_CHAR;
        put_slot(row, IW_LF_LEAD_RIGHT, ents, s + 1u);
    }
}

void iw_lf_row_prompt(char *row, const char *path, int hint)
{
    iw_lf_blank(row);
    put(row, 0, IW_LF_DIR_PROMPT);
    put(row, IW_LF_PATH_COL, path);
    if (hint) put(row, IW_LF_HINT_COL, IW_LF_HINT);
}

void iw_lf_row_look(char *row, const char *path, int wp51, uint16_t date, uint16_t time)
{
    uint32_t col = IW_LF_WP_COLS - (sizeof IW_LF_LOOK_REVISED - 1u) - 15u;
    iw_lf_blank(row);
    put(row, 0, IW_LF_LOOK_FILE);
    put(row, sizeof IW_LF_LOOK_FILE - 1u, path);
    if (wp51) put(row, IW_LF_LOOK_WP51_COL, IW_LF_LOOK_WP51);
    put(row, col, IW_LF_LOOK_REVISED);
    col += sizeof IW_LF_LOOK_REVISED - 1u;
    iw_lf_put_date(row + col, 1980u + (date >> 9), (date >> 5) & 15u, date & 31u);
    row[col + 8u] = ' ';
    iw_lf_put_time(row + col + 9u, time >> 11, (time >> 5) & 63u);
}

uint32_t iw_lf_fit(const char *row, const uint8_t *del, uint32_t ndel, char *out, uint32_t width)
{
    uint32_t c, n = 0, k;
    for (c = 0; c < IW_LF_WP_COLS && n < width; c++) {
        for (k = 0; k < ndel && del[k] != c; k++) {}
        if (k < ndel) continue;
        out[n++] = row[c];
    }
    for (k = n; k < width; k++) out[k] = ' ';
    out[width] = 0;
    return n;
}

uint32_t iw_lf_face_col(uint32_t wp_col, const uint8_t *del, uint32_t ndel)
{
    uint32_t k, n = wp_col;
    for (k = 0; k < ndel; k++) if (del[k] < wp_col) n--;
    return n;
}
