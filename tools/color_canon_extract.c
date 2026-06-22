/*
 * color_canon_extract.c -- FLAIR Color Canon generator (factory, C11-only).
 *
 * beads: initech-h714 (step 2 of epic initech-qipc).
 * Ref: ADR-0004-AMENDMENT-DEC-09 Sec 3.2 (ARB-1: color_canon.h contract),
 *      Sec 3.9 (canon index table), spec/assets/color_canon.json (LOCKED).
 *      CLAUDE.md Law 1 (cite sources), Law 3 (factory is C, no external libs),
 *      Rule 8 (locked spec-data), Rule 11 (deterministic/reproducible),
 *      Rule 12 (ASCII-only source and output).
 *
 * Reads spec/assets/color_canon.json (path given as argv[1]) and writes
 * spec/assets/color_canon.h to stdout.  The Makefile redirects with >.
 * Invocation: color_canon_extract spec/assets/color_canon.json
 *             > spec/assets/color_canon.h
 *
 * Emits deterministically: entries in idx order 0..8, no timestamps, no
 * map-ordering dependence.  Fails loud (nonzero exit + stderr) on malformed
 * input, missing fields, or wrong entry count.  Never emits a partial header.
 *
 * JSON parsing reuses the idiom from tools/palette_extract.c: slurp the
 * whole file, then scan with find_key / next_string / strtol.  Hand-rolled
 * parser for OUR schema only; not a general JSON parser.
 *
 * ASCII-only output (Rule 12).  No libc beyond stdio/stdlib/string (Law 3).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- JSON helpers (idiom from tools/palette_extract.c) ------------------- */

/* Read whole file into a NUL-terminated buffer. Caller frees. */
static char *slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "color_canon_extract: cannot open %s\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return NULL; }
    char *buf = malloc((size_t)sz + 1);
    if (!buf) {
        fprintf(stderr, "color_canon_extract: OOM\n");
        fclose(f);
        return NULL;
    }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        fprintf(stderr, "color_canon_extract: short read %s\n", path);
        free(buf); fclose(f); return NULL;
    }
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

/*
 * Find the next occurrence of the quoted key "key" at or after p.
 * Returns a pointer just past the closing quote of the key, or NULL.
 */
static const char *find_key(const char *p, const char *end, const char *key)
{
    size_t kl = strlen(key);
    for (; p + kl + 2 <= end; p++) {
        if (*p == '"' && strncmp(p + 1, key, kl) == 0 && p[1 + kl] == '"')
            return p + 1 + kl + 1;
    }
    return NULL;
}

/* Read next quoted string from *pp into out[cap]; advances *pp past the
 * closing quote.  Returns 0 ok, -1 on error. */
static int next_string(const char **pp, const char *end, char *out, size_t cap)
{
    const char *p = *pp;
    while (p < end && *p != '"') p++;
    if (p >= end) return -1;
    p++; /* past opening quote */
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

/* Read a long integer after the next ':' at or after p.  Returns 0 ok. */
static int read_int_after_colon(const char *p, const char *end, long *out)
{
    while (p < end && *p != ':') p++;
    if (p >= end) return -1;
    p++;
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
    char *e;
    long v = strtol(p, &e, 10);
    if (e == p) return -1;
    *out = v;
    return 0;
}

/*
 * Parse a three-element byte array [ r, g, b ] from the JSON.
 * p should be positioned at or before the '['.  Returns 0 ok.
 */
static int read_rgb_bytes(const char *p, const char *end,
                          int *r, int *g, int *b)
{
    while (p < end && *p != '[') p++;
    if (p >= end) return -1;
    p++; /* past '[' */
    char *e;
    long rv = strtol(p, &e, 10); if (e == p) return -1; p = e;
    while (p < end && (*p == ',' || *p == ' ')) p++;
    long gv = strtol(p, &e, 10); if (e == p) return -1; p = e;
    while (p < end && (*p == ',' || *p == ' ')) p++;
    long bv = strtol(p, &e, 10); if (e == p) return -1;
    *r = (int)rv; *g = (int)gv; *b = (int)bv;
    return 0;
}

/* ---- Canon entry store ---------------------------------------------------- */

#define CANON_N   9   /* exactly 9 indexed entries, idx 0..8 */
#define DERIVED_N 2   /* exactly 2 derived rows               */

typedef struct {
    int  idx;
    char name[64];
    int  r, g, b;
    int  seen;
} entry_t;

typedef struct {
    char name[64];
    int  r, g, b;
} derived_t;

/* ---- Parse the "entries" array ------------------------------------------- */

/*
 * Scan the "entries" array; populate tbl[0..CANON_N-1] keyed by idx field.
 * Returns 0 ok, -1 on any error.
 */
static int parse_entries(const char *json, entry_t tbl[CANON_N])
{
    const char *end = json + strlen(json);
    int i;
    for (i = 0; i < CANON_N; i++) { tbl[i].seen = 0; tbl[i].idx = -1; }

    const char *ep = find_key(json, end, "entries");
    if (!ep) {
        fprintf(stderr, "color_canon_extract: no \"entries\" key in JSON\n");
        return -1;
    }
    const char *p = ep;
    while (p < end && *p != '[') p++;
    if (p >= end) {
        fprintf(stderr, "color_canon_extract: \"entries\" has no array\n");
        return -1;
    }
    p++; /* past '[' */

    int parsed = 0;
    while (parsed < CANON_N) {
        /* Find the next '{' of an entry object; stop at ']'. */
        while (p < end && *p != '{' && *p != ']') p++;
        if (p >= end || *p == ']') break;
        const char *obj_start = p;
        p++;

        /*
         * Find the matching '}'.  Our entry objects contain arrays (rgb_bytes)
         * but no nested braces, so a shallow scan is sufficient.
         */
        const char *obj_end = p;
        while (obj_end < end && *obj_end != '}') obj_end++;
        if (obj_end >= end) {
            fprintf(stderr, "color_canon_extract: unterminated entry object\n");
            return -1;
        }

        /* --- idx --- */
        const char *kidx = find_key(obj_start, obj_end + 1, "idx");
        if (!kidx) {
            fprintf(stderr, "color_canon_extract: entry missing \"idx\"\n");
            return -1;
        }
        long idx;
        if (read_int_after_colon(kidx, obj_end + 1, &idx)) {
            fprintf(stderr, "color_canon_extract: cannot parse \"idx\"\n");
            return -1;
        }
        if (idx < 0 || idx >= CANON_N) {
            fprintf(stderr,
                    "color_canon_extract: idx %ld out of range 0..%d\n",
                    idx, CANON_N - 1);
            return -1;
        }
        if (tbl[idx].seen) {
            fprintf(stderr, "color_canon_extract: duplicate idx %ld\n", idx);
            return -1;
        }

        /* --- name --- */
        const char *kname = find_key(obj_start, obj_end + 1, "name");
        if (!kname) {
            fprintf(stderr,
                    "color_canon_extract: entry idx=%ld missing \"name\"\n",
                    idx);
            return -1;
        }
        if (next_string(&kname, obj_end + 1,
                        tbl[idx].name, sizeof tbl[idx].name)) {
            fprintf(stderr,
                    "color_canon_extract: cannot read name for idx=%ld\n",
                    idx);
            return -1;
        }

        /* --- rgb_bytes --- */
        const char *krb = find_key(obj_start, obj_end + 1, "rgb_bytes");
        if (!krb) {
            fprintf(stderr,
                    "color_canon_extract: entry idx=%ld missing \"rgb_bytes\"\n",
                    idx);
            return -1;
        }
        if (read_rgb_bytes(krb, obj_end + 1,
                           &tbl[idx].r, &tbl[idx].g, &tbl[idx].b)) {
            fprintf(stderr,
                    "color_canon_extract: bad rgb_bytes for idx=%ld\n",
                    idx);
            return -1;
        }

        tbl[idx].idx  = (int)idx;
        tbl[idx].seen = 1;
        parsed++;
        p = obj_end + 1;
    }

    if (parsed != CANON_N) {
        fprintf(stderr,
                "color_canon_extract: expected %d entries, parsed %d\n",
                CANON_N, parsed);
        return -1;
    }
    for (i = 0; i < CANON_N; i++) {
        if (!tbl[i].seen) {
            fprintf(stderr,
                    "color_canon_extract: idx=%d not present in entries\n",
                    i);
            return -1;
        }
    }
    return 0;
}

/* ---- Parse the "derived_rows" array -------------------------------------- */

static int parse_derived(const char *json, derived_t drv[DERIVED_N])
{
    const char *end = json + strlen(json);

    const char *dp = find_key(json, end, "derived_rows");
    if (!dp) {
        fprintf(stderr, "color_canon_extract: no \"derived_rows\" key in JSON\n");
        return -1;
    }
    const char *p = dp;
    while (p < end && *p != '[') p++;
    if (p >= end) {
        fprintf(stderr, "color_canon_extract: \"derived_rows\" has no array\n");
        return -1;
    }
    p++;

    int parsed = 0;
    while (parsed < DERIVED_N) {
        while (p < end && *p != '{' && *p != ']') p++;
        if (p >= end || *p == ']') break;
        const char *obj_start = p;
        p++;
        const char *obj_end = p;
        while (obj_end < end && *obj_end != '}') obj_end++;
        if (obj_end >= end) {
            fprintf(stderr,
                    "color_canon_extract: unterminated derived_row object\n");
            return -1;
        }

        /* name */
        const char *kn = find_key(obj_start, obj_end + 1, "name");
        if (!kn) {
            fprintf(stderr,
                    "color_canon_extract: derived_row missing \"name\"\n");
            return -1;
        }
        if (next_string(&kn, obj_end + 1,
                        drv[parsed].name, sizeof drv[parsed].name)) {
            fprintf(stderr,
                    "color_canon_extract: cannot read derived_row name\n");
            return -1;
        }

        /* rgb_bytes */
        const char *krb = find_key(obj_start, obj_end + 1, "rgb_bytes");
        if (!krb) {
            fprintf(stderr,
                    "color_canon_extract: derived_row \"%s\" missing "
                    "\"rgb_bytes\"\n",
                    drv[parsed].name);
            return -1;
        }
        if (read_rgb_bytes(krb, obj_end + 1,
                           &drv[parsed].r,
                           &drv[parsed].g,
                           &drv[parsed].b)) {
            fprintf(stderr,
                    "color_canon_extract: bad rgb_bytes for derived_row \"%s\"\n",
                    drv[parsed].name);
            return -1;
        }

        parsed++;
        p = obj_end + 1;
    }

    if (parsed != DERIVED_N) {
        fprintf(stderr,
                "color_canon_extract: expected %d derived_rows, parsed %d\n",
                DERIVED_N, parsed);
        return -1;
    }
    return 0;
}

/* ---- Parse a top-level string field -------------------------------------- */

static int parse_top_string(const char *json, const char *key,
                             char *out, size_t cap)
{
    const char *end = json + strlen(json);
    const char *kp = find_key(json, end, key);
    if (!kp) return -1;
    return next_string(&kp, end, out, cap);
}

/* ---- String helpers ------------------------------------------------------- */

/*
 * Strip the "CIDX_" prefix from an entry name.
 * "CIDX_BLACK" -> "BLACK".  If prefix absent, return name as-is.
 */
static const char *strip_cidx(const char *name)
{
    if (strncmp(name, "CIDX_", 5) == 0) return name + 5;
    return name;
}

/* ASCII upper-case a string in-place into dst[cap]. */
static void upcase(const char *src, char *dst, size_t cap)
{
    size_t i;
    for (i = 0; src[i] && i + 1 < cap; i++) {
        char c = src[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - ('a' - 'A'));
        dst[i] = c;
    }
    dst[i] = '\0';
}

/* ---- Emit color_canon.h to stdout ---------------------------------------- */

static void emit_header(const entry_t tbl[CANON_N],
                        const derived_t drv[DERIVED_N],
                        const char *canon_version)
{
    int i;

    /* ---- banner ---- */
    printf("/*\n");
    printf(" * color_canon.h -- GENERATED from spec/assets/color_canon.json\n");
    printf(" *                   by tools/color_canon_extract.c\n");
    printf(" *\n");
    printf(" * DO NOT EDIT BY HAND.\n");
    printf(" * Regenerate: tools/color_canon_extract spec/assets/color_canon.json"
           " > spec/assets/color_canon.h\n");
    printf(" *\n");
    printf(" * Canon version: %s\n", canon_version);
    printf(" *\n");
    printf(" * THE single color-policy authority for FLAIR.  Exposes flair_canon_rgb(idx):\n");
    printf(" * idx 0..8  -> the 9-entry color_canon table;\n");
    printf(" * idx >= 9  -> deterministic gray ramp (v<<16)|(v<<8)|v  (v = idx),\n");
    printf(" *              identical to palette.h flair_palette_rgb default branch\n");
    printf(" *              (ADR-0004-AMENDMENT-DEC-09 ARB-1).\n");
    printf(" * Freestanding-safe: <stdint.h> only, no libc.\n");
    printf(" *\n");
    printf(" * HONESTY (ADR-0004-AMENDMENT-DEC-09 FO-9 / P4):\n");
    printf(" * idx2 (CIDX_DESKTOP teal) and both bevel derived rows are\n");
    printf(" * graded_by:authored (Initech-identity injections; no upstream decomp\n");
    printf(" * golden exists).  Gated against drift by VALUE mutants (seafoam/lavender\n");
    printf(" * relapse MUST go RED).  Never claimed decomp-sourced.\n");
    printf(" *\n");
    printf(" * Ref: ADR-0004-AMENDMENT-DEC-09 Sec 3.2 (ARB-1) + Sec 3.9 (index table);\n");
    printf(" *      spec/assets/color_canon.json (LOCKED, CLAUDE.md Rule 8);\n");
    printf(" *      CLAUDE.md Rule 11 (deterministic), Rule 12 (ASCII-clean).\n");
    printf(" */\n");
    printf("#ifndef INITECH_COLOR_CANON_H\n");
    printf("#define INITECH_COLOR_CANON_H\n");
    printf("\n");
    printf("#include <stdint.h>\n");
    printf("\n");

    /* ---- color_canon table + _Static_assert ---- */
    printf("/* ---------------------------------------------------------------------------\n");
    printf(" * color_canon[idx][0..2] = {R, G, B}  --  9 entries, idx 0..8, index order.\n");
    printf(" * Ref: ADR-0004-AMENDMENT-DEC-09 Sec 3.9; color_canon.json \"entries\".\n");
    printf(" */\n");
    printf("static const unsigned char color_canon[9][3] = {\n");
    for (i = 0; i < CANON_N; i++) {
        const entry_t *e = &tbl[i];
        if (i < CANON_N - 1)
            printf("    /* [%d] %-16s */ { %3d, %3d, %3d },\n",
                   e->idx, e->name, e->r, e->g, e->b);
        else
            printf("    /* [%d] %-16s */ { %3d, %3d, %3d }\n",
                   e->idx, e->name, e->r, e->g, e->b);
    }
    printf("};\n");
    printf("_Static_assert(sizeof(color_canon) == 27, \"9 x 3 canon bytes\");\n");
    printf("\n");

    /* ---- CIDX_* index constants ---- */
    printf("/* ---------------------------------------------------------------------------\n");
    printf(" * CIDX_* index constants -- name each slot so consumers can write\n");
    printf(" * flair_canon_rgb(CIDX_BLACK) rather than flair_canon_rgb(0).\n");
    printf(" */\n");
    for (i = 0; i < CANON_N; i++) {
        printf("#define %-20s %d\n", tbl[i].name, tbl[i].idx);
    }
    printf("\n");

    /* ---- INITECH_CANON_<SUFFIX>_RGB macros ---- */
    printf("/* ---------------------------------------------------------------------------\n");
    printf(" * INITECH_CANON_<SUFFIX>_RGB -- compile-time 0x00RRGGBB packed constant.\n");
    printf(" * Convention: entry->name with leading \"CIDX_\" stripped becomes the SUFFIX\n");
    printf(" * (e.g. CIDX_BLACK -> INITECH_CANON_BLACK_RGB).\n");
    printf(" * Prefer flair_canon_rgb(CIDX_*) in runtime paths; use these macros only\n");
    printf(" * where a compile-time constant is required.\n");
    printf(" */\n");
    for (i = 0; i < CANON_N; i++) {
        const entry_t *e = &tbl[i];
        const char *suf = strip_cidx(e->name); /* e.g. "BLACK" */
        char mac[96]; /* 14 + 64 + 4 + NUL = 83; 96 is safe */
        snprintf(mac, sizeof mac, "INITECH_CANON_%s_RGB", suf);
        printf("#define %-36s 0x%02X%02X%02Xu\n", mac, e->r, e->g, e->b);
    }
    printf("\n");

    /* ---- Derived bevel macros ---- */
    printf("/* ---------------------------------------------------------------------------\n");
    printf(" * Derived bevel rows  (WL-0053 lavender->teal swap; graded_by:authored).\n");
    printf(" * bevel_light  == idx2 teal #8DDCDC;  bevel_shadow == darkened teal #4E9BA3.\n");
    printf(" * Neither is wired to a wctb decomp golden; both gated by VALUE mutants.\n");
    printf(" * Ref: ADR-0004-AMENDMENT-DEC-09 Sec 3.4 (ARB-5); color_canon.json derived_rows.\n");
    printf(" */\n");
    for (i = 0; i < DERIVED_N; i++) {
        const derived_t *d = &drv[i];
        /* "bevel_light" -> "BEVEL_LIGHT" -> macro "INITECH_CANON_BEVEL_LIGHT_RGB" */
        char up[64];
        upcase(d->name, up, sizeof up);
        char mac[96]; /* 14 + 64 + 4 + NUL = 83; 96 is safe */
        snprintf(mac, sizeof mac, "INITECH_CANON_%s_RGB", up);
        printf("#define %-36s 0x%02X%02X%02Xu\n", mac, d->r, d->g, d->b);
    }
    printf("\n");

    /* ---- wctb part<->index crosswalk comment ---- */
    printf("/* ---------------------------------------------------------------------------\n");
    printf(" * wctb part<->index crosswalk (comments only; for consumers and the oracle).\n");
    printf(" *\n");
    printf(" * idx0  CIDX_BLACK      part1  wFrameColor         system7-quickdraw\n");
    printf(" * idx1  CIDX_WHITE      part0  wContentColor       system7-quickdraw\n");
    printf(" * idx2  CIDX_DESKTOP    none   no wctb part; Initech teal, graded:authored\n");
    printf(" * idx3  CIDX_MENUBAR    part0  wContentColor (bar shares content white)\n");
    printf(" * idx4  CIDX_TITLE_INK  part2  wTextColor          system7-quickdraw\n");
    printf(" * idx5  CIDX_ACCENT     none   Win-3.1/GDI accent navy (graded LEG B)\n");
    printf(" * idx6  CIDX_CONTROL    none   Win-3.1/GDI BTNFACE (graded LEG B)\n");
    printf(" * idx7  CIDX_PIN_LIGHT  none   rendered dither shade -- NOT wctb part7\n");
    printf(" *                              (#FFFFFF WDEF endpoint); graded vs pinstripe.md\n");
    printf(" * idx8  CIDX_PIN_DARK   none   rendered dither shade -- NOT wctb part8\n");
    printf(" *                              (#000000 WDEF endpoint); graded vs pinstripe.md\n");
    printf(" *\n");
    printf(" * Bevel derived rows substitute teal for System-7 dialog-lavender:\n");
    printf(" * bevel_light  part9/part11  (#CCCCFF/#DADAFF teal-SUBSTITUTED) authored\n");
    printf(" * bevel_shadow part9/part11  (#B3B3DA teal-SUBSTITUTED shadow)  authored\n");
    printf(" */\n");
    printf("\n");

    /* ---- flair_canon_rgb inline accessor ---- */
    printf("/* ---------------------------------------------------------------------------\n");
    printf(" * flair_canon_rgb(idx) -- single runtime index->0x00RRGGBB accessor.\n");
    printf(" *\n");
    printf(" * idx 0..8  -> color_canon table (the 9 indexed canon entries above).\n");
    printf(" * idx >= 9  -> deterministic gray ramp: (v<<16)|(v<<8)|v  where v = idx.\n");
    printf(" *              This is the IDENTICAL default branch from palette.h\n");
    printf(" *              flair_palette_rgb, so idx>=9 behavior is byte-identical.\n");
    printf(" *\n");
    printf(" * Freestanding-safe (no libc).  Callable from the kernel (kmain.c live-DAC\n");
    printf(" * path) and the host harness.  Ref: ADR-0004-AMENDMENT-DEC-09 ARB-1;\n");
    printf(" * palette.h flair_palette_rgb default branch (gray ramp shared verbatim).\n");
    printf(" */\n");
    printf("static inline uint32_t flair_canon_rgb(uint8_t idx)\n");
    printf("{\n");
    printf("    if (idx < 9u) {\n");
    printf("        return ((uint32_t)color_canon[idx][0] << 16)\n");
    printf("             | ((uint32_t)color_canon[idx][1] <<  8)\n");
    printf("             |  (uint32_t)color_canon[idx][2];\n");
    printf("    }\n");
    printf("    /* idx >= 9: deterministic gray ramp (identical to palette.h default). */\n");
    printf("    {\n");
    printf("        uint32_t v = (uint32_t)idx;\n");
    printf("        return (v << 16) | (v << 8) | v;\n");
    printf("    }\n");
    printf("}\n");
    printf("\n");
    printf("#endif /* INITECH_COLOR_CANON_H */\n");
}

/* ---- main ----------------------------------------------------------------- */

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr,
                "usage: color_canon_extract <color_canon.json>\n"
                "  Reads the LOCKED color_canon.json and writes color_canon.h"
                " to stdout.\n"
                "  Example: color_canon_extract spec/assets/color_canon.json"
                " > spec/assets/color_canon.h\n");
        return 2;
    }

    char *json = slurp(argv[1]);
    if (!json) return 2;

    /* Parse canon_version for the banner. */
    char canon_version[256] = "(unknown)";
    if (parse_top_string(json, "canon_version",
                         canon_version, sizeof canon_version)) {
        fprintf(stderr,
                "color_canon_extract: warning: \"canon_version\" not found\n");
    }

    /* Parse the 9 indexed entries. */
    entry_t tbl[CANON_N];
    if (parse_entries(json, tbl)) { free(json); return 1; }

    /* Parse the 2 derived rows. */
    derived_t drv[DERIVED_N];
    if (parse_derived(json, drv)) { free(json); return 1; }

    free(json);

    /* All parsing succeeded -- emit the header to stdout. */
    emit_header(tbl, drv, canon_version);
    return 0;
}
