/*
 * asset_check.c -- asset-extraction v0 gate checker (factory, C11-only).
 *
 * beads: initech-vcq ("asset-extraction v0"). Ref: CLAUDE.md Law 2 (the
 *        oracle is the truth -- exit NON-ZERO on any mismatch), Law 1 (cite
 *        sources), Rule 8 (locked spec-data), Rule 6 (golden must bite).
 *        PRD Sec 10 (palette/chrome extracted from the frame), Sec 6.4
 *        (Chicago strike), Sec 12 (frame is REFERENCE ONLY).
 *
 * Two checks, both must pass or the process exits non-zero:
 *  (a) PALETTE HONESTY: parse spec/assets/palette.json, re-sample the frame
 *      PPM (build/preview.ppm, derived from spec/assets/preview.webp) with a
 *      3x3 average at every recorded (x,y), and assert each named color
 *      still matches its recorded RGB within the JSON's tolerance. This is
 *      what keeps the committed palette HONEST against the fixture: corrupt
 *      a recorded RGB and the gate goes red.
 *  (b) STRIKE WELL-FORMEDNESS: include the generated chicago8x16.h and
 *      assert the table is well-formed: exact glyph count for the declared
 *      range, no row byte out of [0,255] (compile-time guaranteed), the
 *      REQUIRED coverage (A-Z a-z 0-9 space . , : - ' ( )) is all non-blank
 *      ink, and the space cell is blank. A malformed/short table fails.
 *
 * Usage: asset_check <palette.json> <frame.ppm>
 * ASCII-only source (Rule 12). No timestamps emitted (Rule 11).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../spec/assets/chicago8x16.h"

/* ---- PPM (P6) loader (same logic as palette_extract) -------------------- */

typedef struct { long w, h; unsigned char *px; } ppm_t;

static int ppm_read_uint(FILE *f, long *out)
{
    int c;
    for (;;) {
        c = fgetc(f);
        if (c == EOF) return -1;
        if (c == '#') { while (c != '\n' && c != EOF) c = fgetc(f); continue; }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
        break;
    }
    if (c < '0' || c > '9') return -1;
    long v = 0;
    while (c >= '0' && c <= '9') { v = v * 10 + (c - '0'); c = fgetc(f); }
    *out = v;
    return 0;
}

static int ppm_load(const char *path, ppm_t *out)
{
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "asset_check: cannot open PPM %s\n", path); return -1; }
    char magic[3] = {0};
    if (fread(magic, 1, 2, f) != 2 || strcmp(magic, "P6") != 0) {
        fprintf(stderr, "asset_check: %s is not a P6 PPM\n", path);
        fclose(f); return -1;
    }
    long w = 0, h = 0, maxv = 0;
    if (ppm_read_uint(f, &w) || ppm_read_uint(f, &h) || ppm_read_uint(f, &maxv) ||
        w <= 0 || h <= 0 || maxv != 255) {
        fprintf(stderr, "asset_check: bad PPM header in %s\n", path);
        fclose(f); return -1;
    }
    long n = w * h;
    unsigned char *buf = malloc((size_t)n * 3);
    if (!buf) { fclose(f); return -1; }
    if (fread(buf, 3, (size_t)n, f) != (size_t)n) {
        fprintf(stderr, "asset_check: short raster in %s\n", path);
        free(buf); fclose(f); return -1;
    }
    fclose(f);
    out->w = w; out->h = h; out->px = buf;
    return 0;
}

static void ppm_avg3x3(const ppm_t *p, long cx, long cy, int rgb[3])
{
    long rs = 0, gs = 0, bs = 0, n = 0;
    for (long y = cy - 1; y <= cy + 1; y++) {
        for (long x = cx - 1; x <= cx + 1; x++) {
            long xx = x, yy = y;
            if (xx < 0) xx = 0;
            if (xx >= p->w) xx = p->w - 1;
            if (yy < 0) yy = 0;
            if (yy >= p->h) yy = p->h - 1;
            unsigned char *q = p->px + (yy * p->w + xx) * 3;
            rs += q[0]; gs += q[1]; bs += q[2]; n++;
        }
    }
    rgb[0] = (int)(rs / n); rgb[1] = (int)(gs / n); rgb[2] = (int)(bs / n);
}

/* ---- minimal palette.json reader (OUR shape only) ----------------------- */

#define MAX_ANCHORS 64
typedef struct { char name[64]; long x, y; int rgb[3]; } anchor_t;

static char *slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "asset_check: cannot open %s\n", path); return NULL; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return NULL; }
    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); return NULL; }
    buf[sz] = '\0'; fclose(f);
    return buf;
}

static int next_string(const char **pp, const char *end, char *out, size_t cap)
{
    const char *p = *pp;
    while (p < end && *p != '"') p++;
    if (p >= end) return -1;
    p++;
    size_t i = 0;
    while (p < end && *p != '"') { if (i + 1 < cap) out[i++] = *p; p++; }
    if (p >= end) return -1;
    out[i] = '\0';
    *pp = p + 1;
    return 0;
}

static const char *find_key(const char *p, const char *end, const char *key)
{
    size_t kl = strlen(key);
    for (; p + kl + 2 <= end; p++)
        if (*p == '"' && strncmp(p + 1, key, kl) == 0 && p[1 + kl] == '"')
            return p + 1 + kl + 1;
    return NULL;
}

static int read_int_after_colon(const char *p, const char *end, long *out)
{
    while (p < end && *p != ':') p++;
    if (p >= end) return -1;
    p++;
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
    char *e; long v = strtol(p, &e, 10);
    if (e == p) return -1;
    *out = v;
    return 0;
}

static int parse_anchors(const char *json, anchor_t *out, int max_out, int *tol_out)
{
    const char *end = json + strlen(json);
    const char *tp = find_key(json, end, "tolerance");
    if (tp) { long t; if (!read_int_after_colon(tp, end, &t)) *tol_out = (int)t; }
    const char *colors = find_key(json, end, "colors");
    if (!colors) { fprintf(stderr, "asset_check: no \"colors\" in JSON\n"); return -1; }
    const char *p = colors;
    while (p < end && *p != '{') p++;
    if (p >= end) return -1;
    p++;
    int n = 0;
    while (n < max_out) {
        char name[64];
        if (next_string(&p, end, name, sizeof name)) break;
        const char *brace = p;
        while (brace < end && *brace != '{' && *brace != '}') brace++;
        if (brace >= end || *brace == '}') break;
        const char *kx = find_key(brace, end, "x");
        const char *ky = find_key(brace, end, "y");
        const char *kr = find_key(brace, end, "rgb");
        if (!kx || !ky || !kr) break;
        long x, y;
        if (read_int_after_colon(kx, end, &x)) break;
        if (read_int_after_colon(ky, end, &y)) break;
        const char *q = kr;
        while (q < end && *q != '[') q++;
        if (q >= end) break;
        q++;
        char *e; long r, g, b;
        r = strtol(q, &e, 10); if (e == q) break; q = e; while (q<end && (*q==','||*q==' ')) q++;
        g = strtol(q, &e, 10); if (e == q) break; q = e; while (q<end && (*q==','||*q==' ')) q++;
        b = strtol(q, &e, 10); if (e == q) break;
        anchor_t *a = &out[n++];
        strncpy(a->name, name, sizeof a->name - 1);
        a->name[sizeof a->name - 1] = '\0';
        a->x = x; a->y = y; a->rgb[0] = (int)r; a->rgb[1] = (int)g; a->rgb[2] = (int)b;
        p = brace + 1;
        while (p < end && *p != '}') p++;
        if (p < end) p++;
    }
    return n;
}

/* ---- check (a): palette honesty ----------------------------------------- */

static int check_palette(const char *jsonpath, const char *ppmpath)
{
    char *json = slurp(jsonpath);
    if (!json) return 1;
    anchor_t a[MAX_ANCHORS]; int tol = 10;
    int na = parse_anchors(json, a, MAX_ANCHORS, &tol);
    free(json);
    if (na <= 0) { fprintf(stderr, "asset_check: parsed 0 anchors from %s\n", jsonpath); return 1; }

    ppm_t p;
    if (ppm_load(ppmpath, &p)) return 1;

    printf("    [palette] %d anchors, tolerance +/-%d, fixture %ldx%ld\n",
           na, tol, p.w, p.h);
    int bad = 0;
    for (int i = 0; i < na; i++) {
        int s[3];
        ppm_avg3x3(&p, a[i].x, a[i].y, s);
        int dr = abs(s[0] - a[i].rgb[0]);
        int dg = abs(s[1] - a[i].rgb[1]);
        int db = abs(s[2] - a[i].rgb[2]);
        int ok = (dr <= tol && dg <= tol && db <= tol);
        printf("      %-20s (%4ld,%4ld) recorded(%3d,%3d,%3d) sampled(%3d,%3d,%3d) d(%d,%d,%d) %s\n",
               a[i].name, a[i].x, a[i].y,
               a[i].rgb[0], a[i].rgb[1], a[i].rgb[2], s[0], s[1], s[2],
               dr, dg, db, ok ? "ok" : "MISMATCH");
        if (!ok) bad++;
    }
    free(p.px);
    if (bad) {
        fprintf(stderr, "    [palette] FAIL -- %d/%d anchors drifted beyond tolerance\n", bad, na);
        return 1;
    }
    printf("    [palette] PASS -- all %d anchors match within tolerance\n", na);
    return 0;
}

/* ---- check (b): Chicago strike well-formedness -------------------------- */

static int cell_is_blank(const unsigned char *cell)
{
    for (int r = 0; r < CHICAGO_CELL_H; r++) if (cell[r]) return 0;
    return 1;
}

static int check_strike(void)
{
    int fail = 0;

    /* Glyph count matches the declared contiguous range. */
    size_t got = sizeof(chicago8x16) / sizeof(chicago8x16[0]);
    if (got != (size_t)CHICAGO_COUNT) {
        fprintf(stderr, "    [strike] FAIL -- glyph count %zu != declared %d\n",
                got, CHICAGO_COUNT);
        fail++;
    }
    /* Each glyph row is exactly CHICAGO_CELL_H bytes (compile-time array). */
    if (sizeof(chicago8x16[0]) != (size_t)CHICAGO_CELL_H) {
        fprintf(stderr, "    [strike] FAIL -- cell height %zu != %d\n",
                sizeof(chicago8x16[0]), CHICAGO_CELL_H);
        fail++;
    }
    /* Space cell (0x20) must be blank. */
    if (!cell_is_blank(chicago8x16_glyph(' '))) {
        fprintf(stderr, "    [strike] FAIL -- space cell is not blank\n");
        fail++;
    }
    /* Out-of-range codes return the blank space cell. */
    if (!cell_is_blank(chicago8x16_glyph(0x10)) ||
        !cell_is_blank(chicago8x16_glyph(0x7F))) {
        fprintf(stderr, "    [strike] FAIL -- out-of-range code did not map to blank cell\n");
        fail++;
    }
    /* REQUIRED coverage must all be NON-blank ink. */
    const char *required =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        ".,:-'()";
    int missing = 0;
    for (const char *c = required; *c; c++) {
        if (cell_is_blank(chicago8x16_glyph((unsigned char)*c))) {
            fprintf(stderr, "    [strike] FAIL -- required glyph '%c' (0x%02X) is blank\n",
                    *c, (unsigned char)*c);
            missing++;
        }
    }
    if (missing) fail++;

    if (fail) return 1;
    printf("    [strike] PASS -- %dx%d cell, %d glyphs (0x%02X..0x%02X), "
           "required A-Z a-z 0-9 space . , : - ' ( ) all inked\n",
           CHICAGO_CELL_W, CHICAGO_CELL_H, CHICAGO_COUNT, CHICAGO_FIRST, CHICAGO_LAST);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <palette.json> <frame.ppm>\n", argv[0]);
        return 2;
    }
    printf(">>> asset_check: (a) palette honesty vs fixture\n");
    int rc_pal = check_palette(argv[1], argv[2]);
    printf(">>> asset_check: (b) Chicago strike well-formedness\n");
    int rc_str = check_strike();

    if (rc_pal || rc_str) {
        fprintf(stderr, ">>> asset_check: FAIL (palette=%d strike=%d)\n", rc_pal, rc_str);
        return 1;
    }
    printf(">>> asset_check: PASS -- palette honest + strike well-formed\n");
    return 0;
}
