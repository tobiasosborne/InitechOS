/*
 * palette_extract.c -- desktop palette extraction tool (factory, C11-only).
 *
 * beads: initech-vcq ("asset-extraction v0"). Ref: PRD Sec 10 (asset
 *        pipeline: palette quantization + named-anchor sampling from the
 *        reference frame), PRD Sec 6.4 (resources/assets), PRD Sec 12 (the
 *        frame is REFERENCE ONLY -- we MEASURE it, we never embed it).
 *        CLAUDE.md Law 1 (cite sources), Law 3 (factory is C), Rule 8
 *        (specs are locked JSON/headers), Rule 11 (reproducible).
 *
 * Given a binary PPM (P6) decoded from the frame webp into build/ (the PPM
 * derives from the film and is NOT committed; palette.json is the committed
 * artifact), this tool:
 *   --dominant N     quantize and print the N most common colors (5-bit/ch
 *                    buckets), so the palette is regenerable, not hand-typed.
 *   --anchors FILE   parse the named anchors out of palette.json and print
 *                    the 3x3-averaged RGB the frame yields at each (x,y).
 *   --header FILE    emit a C header of named RGB constants to stdout (used
 *                    by the Makefile to regenerate spec/assets/palette.h).
 *
 * JSON parsing here is deliberately tiny and tolerant of OUR palette.json
 * shape only (a flat "colors" object of {x,y,rgb,role}); it is not a general
 * JSON parser. ASCII-only source (Rule 12). No timestamps emitted (Rule 11).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---- PPM (P6) loader ---------------------------------------------------- */

typedef struct { long w, h; unsigned char *px; } ppm_t; /* px = w*h*3, RGB */

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
    if (!f) { fprintf(stderr, "palette_extract: cannot open %s\n", path); return -1; }
    char magic[3] = {0};
    if (fread(magic, 1, 2, f) != 2 || strcmp(magic, "P6") != 0) {
        fprintf(stderr, "palette_extract: %s is not a P6 PPM\n", path);
        fclose(f); return -1;
    }
    long w = 0, h = 0, maxv = 0;
    if (ppm_read_uint(f, &w) || ppm_read_uint(f, &h) || ppm_read_uint(f, &maxv)) {
        fprintf(stderr, "palette_extract: malformed PPM header in %s\n", path);
        fclose(f); return -1;
    }
    if (w <= 0 || h <= 0 || maxv != 255) {
        fprintf(stderr, "palette_extract: bad dims/maxval w=%ld h=%ld max=%ld\n", w, h, maxv);
        fclose(f); return -1;
    }
    long n = w * h;
    unsigned char *buf = malloc((size_t)n * 3);
    if (!buf) { fprintf(stderr, "palette_extract: OOM\n"); fclose(f); return -1; }
    if (fread(buf, 3, (size_t)n, f) != (size_t)n) {
        fprintf(stderr, "palette_extract: short raster in %s\n", path);
        free(buf); fclose(f); return -1;
    }
    fclose(f);
    out->w = w; out->h = h; out->px = buf;
    return 0;
}

/* 3x3-averaged RGB at (cx,cy), clamped to image bounds. */
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

/* ---- dominant-color quantization (5-bit/channel buckets) ---------------- */

static void cmd_dominant(const ppm_t *p, int topn)
{
    /* 32 levels per channel -> 32768 buckets. */
    static unsigned long cnt[32768];
    static unsigned long rsum[32768], gsum[32768], bsum[32768];
    memset(cnt, 0, sizeof cnt);
    memset(rsum, 0, sizeof rsum);
    memset(gsum, 0, sizeof gsum);
    memset(bsum, 0, sizeof bsum);
    long n = p->w * p->h;
    for (long i = 0; i < n; i++) {
        int r = p->px[i*3+0], g = p->px[i*3+1], b = p->px[i*3+2];
        int idx = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
        cnt[idx]++; rsum[idx] += r; gsum[idx] += g; bsum[idx] += b;
    }
    printf("# dominant colors (5-bit/ch buckets, %ld px sampled), top %d:\n",
           n, topn);
    for (int k = 0; k < topn; k++) {
        int best = -1; unsigned long bestc = 0;
        for (int i = 0; i < 32768; i++) if (cnt[i] > bestc) { bestc = cnt[i]; best = i; }
        if (best < 0 || bestc == 0) break;
        int r = (int)(rsum[best] / cnt[best]);
        int g = (int)(gsum[best] / cnt[best]);
        int b = (int)(bsum[best] / cnt[best]);
        double pct = 100.0 * (double)bestc / (double)n;
        printf("  #%2d  RGB(%3d,%3d,%3d)  %6.2f%%  (%lu px)\n",
               k + 1, r, g, b, pct, bestc);
        cnt[best] = 0;
    }
}

/* ---- minimal palette.json reader (OUR shape only) ----------------------- */

#define MAX_ANCHORS 64
typedef struct { char name[64]; long x, y; int rgb[3]; } anchor_t;

/* Read whole file into a NUL-terminated buffer. */
static char *slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "palette_extract: cannot open %s\n", path); return NULL; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return NULL; }
    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); return NULL; }
    buf[sz] = '\0'; fclose(f);
    return buf;
}

/* Scan forward from *pp for the next quoted string into out (cap). */
static int next_string(const char **pp, const char *end, char *out, size_t cap)
{
    const char *p = *pp;
    while (p < end && *p != '"') p++;
    if (p >= end) return -1;
    p++; /* opening quote */
    size_t i = 0;
    while (p < end && *p != '"') {
        if (i + 1 < cap) out[i++] = *p;
        p++;
    }
    if (p >= end) return -1;
    out[i] = '\0';
    *pp = p + 1;
    return 0;
}

/* Find the next occurrence of key "<key>" after p, return ptr just past the
 * closing quote of the key, or NULL. */
static const char *find_key(const char *p, const char *end, const char *key)
{
    size_t kl = strlen(key);
    for (; p + kl + 2 <= end; p++) {
        if (*p == '"' && strncmp(p + 1, key, kl) == 0 && p[1 + kl] == '"')
            return p + 1 + kl + 1;
    }
    return NULL;
}

/* Read a long integer that appears after the next ':' following p. */
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

/*
 * Parse the "colors" object. Each entry is  "name": { "x":N, "y":N,
 * "rgb":[r,g,b], ... }. We locate the "colors" key, then for each color we
 * read its name, then x, y, and the three rgb ints (in that source order).
 */
static int parse_anchors(const char *json, anchor_t *out, int max_out,
                         int *tol_out, char *fixture, size_t fixcap)
{
    const char *end = json + strlen(json);

    /* fixture + tolerance (top-level, optional). */
    const char *fp = find_key(json, end, "source_fixture");
    if (fp) { const char *q = fp; next_string(&q, end, fixture, fixcap); }
    const char *tp = find_key(json, end, "tolerance");
    if (tp) { long t; if (!read_int_after_colon(tp, end, &t)) *tol_out = (int)t; }

    const char *colors = find_key(json, end, "colors");
    if (!colors) { fprintf(stderr, "palette_extract: no \"colors\" in JSON\n"); return -1; }
    /* advance past the opening brace of the colors object */
    const char *p = colors;
    while (p < end && *p != '{') p++;
    if (p >= end) return -1;
    p++;

    int n = 0;
    while (n < max_out) {
        /* The color name is the next string before the value brace. */
        char name[64];
        const char *save = p;
        if (next_string(&p, end, name, sizeof name)) break;
        /* The value must be an object: find its '{'. If we hit a closing '}'
         * of the colors object first, we are done. */
        const char *brace = p;
        while (brace < end && *brace != '{' && *brace != '}') brace++;
        if (brace >= end || *brace == '}') break;
        (void)save;
        /* x */
        const char *kx = find_key(brace, end, "x");
        const char *ky = find_key(brace, end, "y");
        const char *kr = find_key(brace, end, "rgb");
        if (!kx || !ky || !kr) break;
        long x, y;
        if (read_int_after_colon(kx, end, &x)) break;
        if (read_int_after_colon(ky, end, &y)) break;
        /* rgb: three ints inside [ ... ] */
        const char *q = kr;
        while (q < end && *q != '[') q++;
        if (q >= end) break;
        q++;
        long r, g, b;
        char *e;
        r = strtol(q, &e, 10); if (e == q) break; q = e; while (q<end && (*q==','||*q==' ')) q++;
        g = strtol(q, &e, 10); if (e == q) break; q = e; while (q<end && (*q==','||*q==' ')) q++;
        b = strtol(q, &e, 10); if (e == q) break;

        anchor_t *a = &out[n++];
        strncpy(a->name, name, sizeof a->name - 1);
        a->name[sizeof a->name - 1] = '\0';
        a->x = x; a->y = y; a->rgb[0] = (int)r; a->rgb[1] = (int)g; a->rgb[2] = (int)b;

        /* advance p past this color object's matching close brace (shallow:
         * our objects contain no nested braces). */
        p = brace + 1;
        while (p < end && *p != '}') p++;
        if (p < end) p++;
    }
    return n;
}

/*
 * Parse the OPTIONAL "canonical" object: entries are "name": { "rgb":[r,g,b], ... }
 * with NO x/y -- these are OS-CANON values (set by ADR/oracle, e.g. the SEAFOAM
 * desktop_bg, ADR-0004 OD-4), NOT frame-sampled, so test-assets does not re-check
 * them against the frame. Non-object entries (e.g. "_comment": [ ... ]) are
 * skipped. Returns the count (0 if the block is absent -- it is optional). Shallow
 * parser like parse_anchors (entry objects contain arrays but no nested braces).
 */
static int parse_canonical(const char *json, anchor_t *out, int max_out)
{
    const char *end = json + strlen(json);
    const char *can = find_key(json, end, "canonical");
    if (!can) return 0;  /* optional block */
    const char *p = can;
    while (p < end && *p != '{') p++;
    if (p >= end) return 0;
    p++;  /* past the canonical object's opening brace */

    int n = 0;
    while (n < max_out) {
        char name[64];
        if (next_string(&p, end, name, sizeof name)) break;
        /* Locate the value opener: '{' (object), '[' (array), or '}' (end). */
        const char *v = p;
        while (v < end && *v != '{' && *v != '[' && *v != '}') v++;
        if (v >= end || *v == '}') break;            /* end of canonical object */
        if (*v == '[') {                              /* array (e.g. _comment): skip */
            const char *q = v;
            while (q < end && *q != ']') q++;
            p = (q < end) ? q + 1 : end;
            continue;
        }
        /* *v == '{' : an entry object -- parse its rgb (shallow). */
        const char *kr = find_key(v, end, "rgb");
        const char *obj_end = v + 1;
        while (obj_end < end && *obj_end != '}') obj_end++;
        if (!kr || kr > obj_end) {                    /* no rgb in this object: skip */
            p = (obj_end < end) ? obj_end + 1 : end;
            continue;
        }
        const char *q = kr;
        while (q < end && *q != '[') q++;
        if (q >= end) break;
        q++;
        long r, g, b; char *e;
        r = strtol(q, &e, 10); if (e == q) break; q = e; while (q<end && (*q==','||*q==' ')) q++;
        g = strtol(q, &e, 10); if (e == q) break; q = e; while (q<end && (*q==','||*q==' ')) q++;
        b = strtol(q, &e, 10); if (e == q) break;

        anchor_t *a = &out[n++];
        strncpy(a->name, name, sizeof a->name - 1);
        a->name[sizeof a->name - 1] = '\0';
        a->x = -1; a->y = -1;  /* not frame-sampled */
        a->rgb[0] = (int)r; a->rgb[1] = (int)g; a->rgb[2] = (int)b;

        p = (obj_end < end) ? obj_end + 1 : end;
    }
    return n;
}

static void cmd_anchors(const ppm_t *p, const char *jsonpath)
{
    char *json = slurp(jsonpath);
    if (!json) exit(2);
    anchor_t a[MAX_ANCHORS]; int tol = 10; char fixture[256] = "";
    int na = parse_anchors(json, a, MAX_ANCHORS, &tol, fixture, sizeof fixture);
    free(json);
    if (na <= 0) { fprintf(stderr, "palette_extract: parsed 0 anchors\n"); exit(2); }
    printf("# anchors re-sampled from frame (3x3 avg), tolerance +/-%d:\n", tol);
    printf("# %-20s  (x,y)        recorded        sampled\n", "name");
    for (int i = 0; i < na; i++) {
        int s[3]; ppm_avg3x3(p, a[i].x, a[i].y, s);
        printf("  %-20s  (%4ld,%4ld)  (%3d,%3d,%3d)  (%3d,%3d,%3d)\n",
               a[i].name, a[i].x, a[i].y,
               a[i].rgb[0], a[i].rgb[1], a[i].rgb[2], s[0], s[1], s[2]);
    }
}

/* Emit a C header of named RGB constants from palette.json (no PPM needed). */
static void cmd_header(const char *jsonpath)
{
    char *json = slurp(jsonpath);
    if (!json) exit(2);
    anchor_t a[MAX_ANCHORS]; int tol = 10; char fixture[256] = "(unknown)";
    int na = parse_anchors(json, a, MAX_ANCHORS, &tol, fixture, sizeof fixture);
    anchor_t canon[MAX_ANCHORS];
    int nc = parse_canonical(json, canon, MAX_ANCHORS);
    free(json);
    if (na <= 0) { fprintf(stderr, "palette_extract: parsed 0 anchors\n"); exit(2); }

    printf("/*\n");
    printf(" * palette.h -- GENERATED from %s by tools/palette_extract.c.\n", jsonpath);
    printf(" * DO NOT EDIT BY HAND. Regenerate: make %s\n", "spec/assets/palette.h");
    printf(" *\n");
    printf(" * InitechOS desktop palette v0. Ref: PRD Sec 10 (palette extracted\n");
    printf(" * from the reference frame), Sec 6.4 (assets), Sec 12 (frame is\n");
    printf(" * REFERENCE ONLY -- these are MEASURED samples, not embedded image).\n");
    printf(" * Source fixture: %s. Each color was 3x3-sampled at the (x,y)\n", fixture);
    printf(" * recorded in palette.json (auditable; test-assets re-checks it).\n");
    printf(" */\n");
    printf("#ifndef INITECH_PALETTE_H\n#define INITECH_PALETTE_H\n\n");
    for (int i = 0; i < na; i++) {
        char up[64];
        size_t j;
        for (j = 0; a[i].name[j] && j < sizeof up - 1; j++)
            up[j] = (char)toupper((unsigned char)a[i].name[j]);
        up[j] = '\0';
        printf("/* sampled @ (%ld,%ld) */\n", a[i].x, a[i].y);
        printf("#define INITECH_%s_R 0x%02X\n", up, a[i].rgb[0]);
        printf("#define INITECH_%s_G 0x%02X\n", up, a[i].rgb[1]);
        printf("#define INITECH_%s_B 0x%02X\n", up, a[i].rgb[2]);
        printf("#define INITECH_%s_RGB 0x%02X%02X%02Xu\n\n",
               up, a[i].rgb[0], a[i].rgb[1], a[i].rgb[2]);
    }
    /* Canonical (OS-canon) values -- set by ADR/oracle, NOT frame-sampled (e.g.
     * the SEAFOAM desktop_bg, ADR-0004 OD-4 / AM-9). Emitted with the same
     * INITECH_<NAME>_* macro form so render/chrome code consumes one source. */
    for (int i = 0; i < nc; i++) {
        char up[64];
        size_t j;
        for (j = 0; canon[i].name[j] && j < sizeof up - 1; j++)
            up[j] = (char)toupper((unsigned char)canon[i].name[j]);
        up[j] = '\0';
        printf("/* canonical (OS canon -- ADR/oracle, not frame-sampled) */\n");
        printf("#define INITECH_%s_R 0x%02X\n", up, canon[i].rgb[0]);
        printf("#define INITECH_%s_G 0x%02X\n", up, canon[i].rgb[1]);
        printf("#define INITECH_%s_B 0x%02X\n", up, canon[i].rgb[2]);
        printf("#define INITECH_%s_RGB 0x%02X%02X%02Xu\n\n",
               up, canon[i].rgb[0], canon[i].rgb[1], canon[i].rgb[2]);
    }
    printf("#endif /* INITECH_PALETTE_H */\n");
}

static void usage(const char *me)
{
    fprintf(stderr,
        "usage:\n"
        "  %s --dominant N <frame.ppm>     print N dominant colors\n"
        "  %s --anchors PALETTE.json <frame.ppm>  re-sample named anchors\n"
        "  %s --header  PALETTE.json       emit palette.h to stdout\n", me, me, me);
}

int main(int argc, char **argv)
{
    if (argc < 2) { usage(argv[0]); return 2; }

    if (strcmp(argv[1], "--header") == 0) {
        if (argc != 3) { usage(argv[0]); return 2; }
        cmd_header(argv[2]);
        return 0;
    }
    if (strcmp(argv[1], "--dominant") == 0) {
        if (argc != 4) { usage(argv[0]); return 2; }
        int n = atoi(argv[2]);
        if (n <= 0 || n > 256) n = 16;
        ppm_t p; if (ppm_load(argv[3], &p)) return 2;
        cmd_dominant(&p, n); free(p.px);
        return 0;
    }
    if (strcmp(argv[1], "--anchors") == 0) {
        if (argc != 4) { usage(argv[0]); return 2; }
        ppm_t p; if (ppm_load(argv[3], &p)) return 2;
        cmd_anchors(&p, argv[2]); free(p.px);
        return 0;
    }
    usage(argv[0]);
    return 2;
}
