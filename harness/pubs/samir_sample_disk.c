/* The InitechBase "Sample Programs and Utilities" diskette (PUB-REF-0001
 * AN-16 / CP-12; bead initech-ipu5.8).  Mints the files every InitechBase
 * manual example uses, from the plain-text sources in
 * pubs/examples/samir/disk, through the SHIPPED SAMIR writers -- the same
 * one-writer route as harness/diff/dbf_diff/mint_invoice_dbf.c, so the files
 * are read back by the shipped reader with no second encoder to drift:
 *   .DBF  dbf_create / dbf_append_rec (os/samir/fs/dbf.c)
 *   .DBT  dbt_create / dbt_append     (os/samir/fs/dbt.c), the memo text of
 *         each M field, the block number stored in the record
 *   .NDX  ndx_build                   (os/samir/fs/ndx.c) over one field
 *   .FRM  the 1990-byte report form of dbase3-decomp
 *         specs/file-formats/frm.md Sec 2-5 (header tables, heap, 12-byte
 *         columns at 0x68C, footer at 0x7B6); the shipped reader is
 *         os/samir/cmd/report.c rp_load, which refuses anything else (#50)
 *   other files are copied byte for byte (the .PRG programs).
 *
 * Usage: samir_sample_disk <disk dir> <out dir>
 * <disk dir>/MANIFEST lists the diskette, one file per line ('#' comments):
 *   NAME|source
 * source *.dat is a table or report description (format below); anything
 * else is copied.  A table .dat:
 *   TABLE <name.DBF>
 *   FIELD <name> <C|N|D|L|M> <len> <dec>
 *   REC <v>|<v>|...      one value per field; D as YYYYMMDD or empty for a
 *   DEL <v>|<v>|...      blank date, L as T or F, M the memo text (\n is a
 *                        line break) or empty; DEL is a record marked deleted
 *   INDEX <name.NDX> <field>   key = the field (C: its width; N, D: numeric)
 * A report .dat:
 *   REPORT <name.FRM>
 *   PAGE <width> <lines per page> <left margin> <right margin> <option byte>
 *   TITLE <title>             (';' separates title lines)
 *   GROUP <expr>|<heading>    (0, 1 or 2 lines: group, subgroup)
 *   COLUMN <width> <dec> <Y|N total> <expr>|<heading>
 * Deterministic (Rule 11): the PAL date is the edition date 03/01/92, and no
 * byte depends on the host.  Fails loud on any malformed line (Rule 2).
 * Host C (Law 3). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "samir/dbf.h"
#include "samir/dbt.h"
#include "samir/ndx.h"
#include "samir/value.h"
#include "samir/rt.h"
struct pal_host_cfg { uint8_t yy,mm,dd; uint32_t heap_size; };
samir_pal_t *pal_host_make(struct pal_host_cfg);
void pal_host_free(samir_pal_t *);

#define MAXF 32
#define MAXR 64
#define MAXV 512
static const char *g_file; static int g_line;
static void die(const char *why)
{ fprintf(stderr, "!!! samir_sample_disk FAIL: %s:%d: %s\n", g_file, g_line, why); exit(1); }

typedef struct { char name[12]; char t; int len, dec; } fld;
typedef struct { char v[MAXF][MAXV]; int del; } rec;
static fld F[MAXF]; static int NF;
static rec R[MAXR]; static int NR;
static char IX[8][16], IXF[8][12]; static int NI;

static int split(char *s, char out[][MAXV], int max)
{
    int n = 0;
    for (;;) {
        char *bar = strchr(s, '|');
        if (n == max) die("too many values");
        size_t k = bar ? (size_t)(bar - s) : strlen(s);
        if (k >= MAXV) die("value too long");
        memcpy(out[n], s, k); out[n][k] = 0; n++;
        if (!bar) return n;
        s = bar + 1;
    }
}
static double jdn(const char *s)
{
    int y, m, d;
    if (strlen(s) != 8 || sscanf(s, "%4d%2d%2d", &y, &m, &d) != 3) die("date is not YYYYMMDD");
    return (double)jdn_from_ymd(y, m, d);
}
/* Unescape \n in memo text in place; returns the length. */
static uint32_t memo_text(char *s)
{
    char *o = s;
    for (char *p = s; *p; p++) {
        if (p[0] == '\\' && p[1] == 'n') { *o++ = '\r'; *o++ = '\n'; p++; }
        else *o++ = *p;
    }
    *o = 0; return (uint32_t)(o - s);
}

static samir_pal_t *g_pal;
static int g_keyfield;
static int keyof(void *user, uint32_t recno, uint8_t *out, uint16_t len)
{
    (void)user;
    const fld *f = &F[g_keyfield]; const char *v = R[recno - 1].v[g_keyfield];
    if (f->t == 'C') {
        memset(out, ' ', len);
        memcpy(out, v, strlen(v) < len ? strlen(v) : len);
    } else {
        double x = f->t == 'D' ? (*v ? jdn(v) : 0.0) : strtod(v, NULL);
        memcpy(out, &x, 8);
    }
    return 0;
}

static void table(const char *outdir, const char *name)
{
    char path[512], dbt[512];
    dbf_field_spec spec[MAXF];
    dbf_table *tbl = NULL; dbt_file *memo = NULL;
    int has_memo = 0;
    for (int i = 0; i < NF; i++) {
        spec[i].name = F[i].name;
        spec[i].type = F[i].t; spec[i].field_len = (uint8_t)F[i].len; spec[i].dec = (uint8_t)F[i].dec;
        if (F[i].t == 'M') has_memo = 1;
    }
    snprintf(path, sizeof path, "%s/%s", outdir, name);
    if (dbf_create(g_pal, path, spec, NF, &tbl) != DBF_OK) die("dbf_create refused the structure");
    if (has_memo) {
        snprintf(dbt, sizeof dbt, "%s", path);
        dbt[strlen(dbt) - 1] = 'T';
        if (dbt_create(g_pal, dbt, &memo) != DBT_OK) die("dbt_create failed");
    }
    for (int r = 0; r < NR; r++) {
        xb_val v[MAXF];
        for (int i = 0; i < NF; i++) {
            char *s = R[r].v[i];
            switch (F[i].t) {
            case 'C': v[i] = xb_c(s, (uint16_t)strlen(s)); break;
            case 'N': v[i] = xb_n(strtod(s, NULL)); break;
            case 'D': v[i] = *s ? xb_d(jdn(s)) : xb_u(); break;
            case 'L': if (strcmp(s, "T") && strcmp(s, "F")) die("logical is not T or F");
                      v[i] = xb_l(*s == 'T'); break;
            case 'M': {
                uint32_t n = memo_text(s), block = 0;
                if (n && dbt_append(memo, (const uint8_t *)s, n, &block) != DBT_OK) die("dbt_append failed");
                v[i] = n ? xb_n((double)block) : xb_u(); break; }
            default: die("field type");
            }
        }
        if (dbf_append_rec(tbl, v, R[r].del) != DBF_OK) die("dbf_append_rec failed");
    }
    if (dbf_flush(tbl) != DBF_OK) die("dbf_flush failed");
    dbf_close(tbl);
    if (memo) { if (dbt_flush(memo) != DBT_OK) die("dbt_flush failed"); dbt_close(memo); }
    for (int k = 0; k < NI; k++) {
        int f = -1;
        for (int i = 0; i < NF; i++) if (!strcmp(F[i].name, IXF[k])) f = i;
        if (f < 0) die("INDEX names no field of the table");
        g_keyfield = f;
        uint16_t type = F[f].t == 'C' ? 0 : 1, len = F[f].t == 'C' ? (uint16_t)F[f].len : 8;
        if (F[f].t != 'C' && F[f].t != 'N' && F[f].t != 'D') die("INDEX field must be C, N or D");
        snprintf(path, sizeof path, "%s/%s", outdir, IX[k]);
        if (ndx_build(g_pal, path, type, len, F[f].name, (uint32_t)NR, keyof, NULL) != NDX_OK) die("ndx_build failed");
    }
}

/* ---- the report form (frm.md) ---- */
static uint8_t FRM[1990]; static int heap, nstr, ncol, ngroup;
static void put16(int o, int v) { FRM[o] = (uint8_t)(v & 255); FRM[o + 1] = (uint8_t)(v >> 8); }
static int string(const char *s)        /* returns the 1-based pointer index */
{
    int len = (int)strlen(s) + 1;
    if (0xE0 + heap + len > 0x68C) die("report strings overflow the heap");
    memcpy(FRM + 0xE0 + heap, s, (size_t)len);
    put16(0x04 + 2 * (nstr + 1), len);
    put16(0x74 + 2 * nstr, heap);
    heap += len; nstr++;
    if (nstr > 54) die("too many report strings");
    return nstr;
}

static void manifest_entry(const char *dir, const char *outdir, const char *name, const char *src)
{
    char path[512], line[1024], vals[MAXF][MAXV];
    snprintf(path, sizeof path, "%s/%s", dir, src);
    FILE *in = fopen(path, "rb");
    if (!in) die("source file missing");
    size_t sl = strlen(src);
    if (sl < 4 || strcmp(src + sl - 4, ".dat")) {
        snprintf(path, sizeof path, "%s/%s", outdir, name);
        FILE *out = fopen(path, "wb"); int c;
        if (!out) die("cannot write output");
        while ((c = fgetc(in)) != EOF) fputc(c, out);
        fclose(in); if (fclose(out)) die("write failed");
        return;
    }
    const char *saved = g_file; int savedl = g_line;
    g_file = path; g_line = 0;
    NF = NR = NI = 0; int report = 0; char made[64] = "";
    memset(FRM, ' ', sizeof FRM); memset(FRM, 0, 0xE0); heap = nstr = ncol = ngroup = 0;
    while (fgets(line, sizeof line, in)) {
        g_line++;
        line[strcspn(line, "\r\n")] = 0;
        if (!line[0] || line[0] == '#') continue;
        char *arg = strchr(line, ' ');
        if (!arg) die("directive without argument");
        *arg++ = 0;
        if (!strcmp(line, "TABLE")) snprintf(made, sizeof made, "%s", arg);
        else if (!strcmp(line, "REPORT")) { snprintf(made, sizeof made, "%s", arg); report = 1; }
        else if (!strcmp(line, "FIELD")) {
            if (NF == MAXF) die("too many fields");
            fld *f = &F[NF++]; memset(f, 0, sizeof *f);
            if (sscanf(arg, "%10s %c %d %d", f->name, &f->t, &f->len, &f->dec) != 4) die("FIELD <name> <type> <len> <dec>");
        } else if (!strcmp(line, "REC") || !strcmp(line, "DEL")) {
            if (NR == MAXR) die("too many records");
            if (split(arg, vals, MAXF) != NF) die("value count differs from the field count");
            memcpy(R[NR].v, vals, sizeof vals); R[NR].del = line[0] == 'D'; NR++;
        } else if (!strcmp(line, "INDEX")) {
            if (NI == 8 || sscanf(arg, "%15s %11s", IX[NI], IXF[NI]) != 2) die("INDEX <name.NDX> <field>");
            NI++;
        } else if (!strcmp(line, "PAGE")) {
            int w, lpp, lm, rm, opt;
            if (sscanf(arg, "%d %d %d %d %d", &w, &lpp, &lm, &rm, &opt) != 5) die("PAGE <w> <lpp> <lm> <rm> <opt>");
            put16(0x7B6, w); put16(0x7B8, lpp); put16(0x7BA, lm); put16(0x7BC, rm); FRM[0x7C3] = (uint8_t)opt;
        } else if (!strcmp(line, "TITLE")) {
            if (heap) die("TITLE must come first");
            int len = (int)strlen(arg) + 1;
            memcpy(FRM + 0xE0, arg, (size_t)len); put16(0x04, len); heap = len;
        } else if (!strcmp(line, "GROUP")) {
            if (ncol || ngroup == 2 || split(arg, vals, 2) != 2) die("GROUP <expr>|<heading>, before the columns, at most two");
            string(vals[0]); string(vals[1]); ngroup++;
        } else if (!strcmp(line, "COLUMN")) {
            int w, dec; char tot; char *rest;
            if (sscanf(arg, "%d %d %c", &w, &dec, &tot) != 3 || !(rest = strchr(arg, ' ')) ||
                !(rest = strchr(rest + 1, ' ')) || !(rest = strchr(rest + 1, ' ')) || split(rest + 1, vals, 2) != 2)
                die("COLUMN <w> <dec> <Y|N> <expr>|<heading>");
            if (ncol == 24) die("too many columns");
            int e = string(vals[0]), h = string(vals[1]), o = 0x68C + 12 * ncol++;
            put16(o, w); put16(o + 2, 1); FRM[o + 4] = ' '; FRM[o + 5] = (uint8_t)tot;
            put16(o + 6, dec); put16(o + 8, e); put16(o + 10, h);
        } else die("unknown directive");
    }
    fclose(in);
    if (!made[0] || strcmp(made, name)) die("the .dat names a different file than the MANIFEST");
    if (report) {
        if (!heap || !ncol) die("a report needs a TITLE and a COLUMN");
        put16(0x00, 2); put16(0x02, heap); put16(0x7BE, ncol);
        FRM[0x7C0] = FRM[0x7C1] = FRM[0x7C2] = 'N'; put16(0x7C4, 2);
        /* Unused heap and gap bytes: zero up to the column table. */
        memset(FRM + 0xE0 + heap, 0, (size_t)(0x68C - 0xE0 - heap));
        snprintf(path, sizeof path, "%s/%s", outdir, name);
        FILE *out = fopen(path, "wb");
        if (!out || fwrite(FRM, 1, sizeof FRM, out) != sizeof FRM || fclose(out)) die("cannot write the report form");
    } else {
        if (!NF) die("a table needs FIELD lines");
        table(outdir, name);
    }
    g_file = saved; g_line = savedl;
}

int main(int argc, char **argv)
{
    char path[512], line[512];
    if (argc != 3) { fprintf(stderr, "usage: samir_sample_disk <disk dir> <out dir>\n"); return 2; }
    snprintf(path, sizeof path, "%s/MANIFEST", argv[1]);
    FILE *m = fopen(path, "rb");
    g_file = path;
    if (!m) die("MANIFEST missing");
    g_pal = pal_host_make((struct pal_host_cfg){92, 3, 1, 4u << 20});
    if (!g_pal) die("pal_host_make");
    int n = 0;
    while (fgets(line, sizeof line, m)) {
        g_line++;
        line[strcspn(line, "\r\n")] = 0;
        if (!line[0] || line[0] == '#') continue;
        char *bar = strchr(line, '|');
        if (!bar || !bar[1]) die("MANIFEST line is not NAME|source");
        *bar = 0;
        manifest_entry(argv[1], argv[2], line, bar + 1);
        n++;
    }
    fclose(m); pal_host_free(g_pal);
    printf(">>> samir_sample_disk: %d files minted into %s\n", n, argv[2]);
    return 0;
}
