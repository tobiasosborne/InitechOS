/*
 * harness/proptest/test_calc.c -- the Calculator desk accessory's HOST ORACLE
 * (factory C; bead initech-tdnl.17, GUI remediation R4.3).
 *
 * Grades the artifact's engine (os/apps/calc/calc_core.c) and face
 * (os/apps/calc/calc_view.c) against goldens that are NOT the artifact
 * (Law 2 / ADR-0010):
 *
 *   C1 VECTORS   87 key sequences typed into the REAL Mac OS 8.1 Calculator
 *                (spec/calc/calc_vectors.tsv, hand-transcribed from the
 *                mint-007 captures): the engine's readout must equal each.
 *   C2 KEYMAP    the PC keyDown messages (os/flair/event.c cooking) map to
 *                the Calculator's commands: digits . + - * / = c C, Enter,
 *                the NumLock "Clear" key, and Escape/Backspace ignored.
 *   C3 PROPERTY  20000 seeded random key sequences: after EVERY key the
 *                engine's readout equals an INDEPENDENT reference written
 *                here from the manual + the captures' rules (calc_ref_*:
 *                strtold for entry, glibc printf %Lf/%Le for the readout --
 *                not the artifact's big-integer formatter), shrunk to the
 *                shortest failing prefix on a mismatch.
 *   C4 PARITY    every key of the window: a click on it (calc_view_hit +
 *                calc_view_click_cmd -- the tenant's click path) and the
 *                keyboard character printed on it (calc_cmd_from_key -- the
 *                tenant's key path) send the SAME command, and a 200-key
 *                random session driven by clicks and the same session driven
 *                by keys end on the same readout at every step.
 *   C5 CAPTURE   when ../system7-decomp is present: (a) every C1 row's
 *                readout, painted by calc_view through the locked strike,
 *                equals the capture's readout interior pixel for pixel
 *                (black vs not-black); (b) the whole content painted idle
 *                equals s8_calc_idle's content (black / white / other) pixel
 *                for pixel. LOUD-SKIP (ADR-0010 CD-8) when absent.
 *
 * Mutants (Rule 6; each -D on the artifact TUs; the Makefile proves each RED
 * on its leg): CALC_MUT_PRECEDENCE (C1/C3), CALC_MUT_DIV0_ZERO (C1),
 * CALC_MUT_ENTRY_CAP14 (C1/C3), CALC_MUT_FIXED_E3 (C1),
 * CALC_MUT_KEY_CLICK_DIVERGE (C4), CALC_MUT_READOUT_SHIFT (C5).
 *
 * Usage: test_calc <calc_vectors.tsv> <system7-decomp root>
 * ASCII-clean (Rule 12). Deterministic (Rule 11: a fixed seed).
 */
#define _POSIX_C_SOURCE 200809L
#include <math.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "calc_core.h"
#include "calc_view.h"
#include "calc/calc_layout.h"

static int g_fail = 0;
static int g_checks = 0;
#define CHECK(leg, cond, ...) do { g_checks++; if (!(cond)) { g_fail++; \
    printf("FAIL %s: ", leg); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* ===========================================================================
 * THE TOKEN LANGUAGE of calc_vectors.tsv -> commands (command level).
 * ===========================================================================*/
static calc_cmd_t tok_cmd(char t)
{
    if (t >= '0' && t <= '9') return (calc_cmd_t)(t - '0');
    switch (t) {
    case '.': return CALC_DOT;
    case '+': return CALC_ADD;
    case '-': return CALC_SUB;
    case '*': return CALC_MUL;
    case '/': return CALC_DIV;
    case '=': return CALC_EQ;
    case 'E': return CALC_EQ;              /* keypad Enter (q17)           */
    case 'c': case 'C': case 'K': return CALC_CLEAR;
    default:  return CALC_NONE;            /* R X B: ignored (q16 s09 q18) */
    }
}

/* ===========================================================================
 * THE INDEPENDENT REFERENCE (calc_ref_*). Written from the rules the minted
 * probes establish (calc_core.h lists them with their probe ids), in a
 * different shape: an explicit mode, strtold for the entry, glibc printf for
 * the readout. Shares NOTHING with calc_core.c but the command enum.
 * ===========================================================================*/
enum { M_FRESH, M_TYPING, M_OPERATED };
typedef struct {
    long double acc;
    int  op;                 /* 0 or '+', '-', '*', '/' */
    int  mode;
    char ent[32];
    char shown[32];
} ref_t;

static int ref_explen(int x) { char b[16]; return snprintf(b, sizeof b, "E%d", x); }

static void ref_fmt(long double v, char *out)
{
    char b[96];
    if (isnan(v)) { strcpy(out, "not a number"); return; }
    if (isinf(v)) { strcpy(out, v < 0 ? "-infinity" : "infinity"); return; }
    if (v == 0.0L) { strcpy(out, "0"); return; }
    int s = signbit(v) ? 1 : 0;
    long double a = fabsl(v);
    /* the decision record: 12 significant digits, glibc's correctly rounded
     * conversion of the exact binary value (probes u01/u02/r04) */
    snprintf(b, sizeof b, "%.11Le", a);
    int e = atoi(strchr(b, 'e') + 1);
    char dig[16]; int nd = 0;
    for (const char *p = b; *p && *p != 'e'; p++) if (*p >= '0' && *p <= '9') dig[nd++] = *p;
    int integral = e >= 0;
    for (int i = e + 1; i < nd; i++) if (i >= 0 && dig[i] != '0') integral = 0;
    if (e >= -2) {
        if (integral) {
            if (s + e + 1 <= 12) { snprintf(out, 32, "%s%.0Lf", s ? "-" : "", a); return; }
        } else {
            int intd = e >= 0 ? e + 1 : 0;
            int f = 12 - s - intd - 1;
            if (f >= 1) {
                snprintf(b, sizeof b, "%.*Lf", f, a);
                char *p = b + strlen(b) - 1;
                while (*p == '0') *p-- = '\0';
                if (*p == '.') *p = '\0';
                const char *q = b;
                if (q[0] == '0' && q[1] == '.') q++;          /* ".333" */
                snprintf(out, 32, "%s%.20s", s ? "-" : "", q);
                return;
            }
        }
    }
    int n = 12 - s - ref_explen(e) - 1;
    snprintf(b, sizeof b, "%.*Le", n - 1, a);
    int x = atoi(strchr(b, 'e') + 1);
    if (ref_explen(x) != ref_explen(e)) {
        n = 12 - s - ref_explen(x) - 1;
        snprintf(b, sizeof b, "%.*Le", n - 1, a);
        x = atoi(strchr(b, 'e') + 1);
    }
    *strchr(b, 'e') = '\0';
    if (strchr(b, '.')) {
        char *p = b + strlen(b) - 1;
        while (*p == '0') *p-- = '\0';
        if (*p == '.') *p = '\0';
    }
    snprintf(out, 32, "%s%.16sE%d", s ? "-" : "", b, x);
}

static void ref_init(ref_t *r)
{
    memset(r, 0, sizeof *r);
    r->mode = M_FRESH;
    strcpy(r->shown, "0");
}

static long double ref_apply(long double a, int op, long double b)
{
    switch (op) {
    case '+': return a + b;
    case '-': return a - b;
    case '*': return a * b;
    case '/': return a / b;
    default:  return b;
    }
}

static void ref_key(ref_t *r, calc_cmd_t k)
{
    static const char opch[] = { '+', '-', '*', '/' };
    if (k == CALC_NONE) return;
    if (k == CALC_CLEAR) { ref_init(r); return; }
    if (k <= CALC_DOT) {
        char ch = (k == CALC_DOT) ? '.' : (char)('0' + k);
        if (r->mode != M_TYPING) { r->ent[0] = '\0'; r->mode = M_TYPING; }
        size_t n = strlen(r->ent);
        if (ch == '.') {
            if (strchr(r->ent, '.')) return;
            if (n == 0) { strcpy(r->ent, "0"); n = 1; }
        } else if (strcmp(r->ent, "0") == 0) {
            r->ent[0] = '\0'; n = 0;
        }
        if (n >= 13) return;
        r->ent[n] = ch; r->ent[n + 1] = '\0';
        strcpy(r->shown, r->ent);
        return;
    }
    /* an operator or '=' */
    if (r->mode == M_TYPING) {
        long double x = strtold(r->ent, NULL);
        r->acc = r->op ? ref_apply(r->acc, r->op, x) : x;
    }
    r->op = (k == CALC_EQ) ? 0 : opch[k - CALC_ADD];
    r->mode = M_OPERATED;
    ref_fmt(r->acc, r->shown);
}

/* ===========================================================================
 * C1 VECTORS + C5a readout crops
 * ===========================================================================*/
typedef struct { char id[16], cap[48], keys[64], want[32]; } vec_t;
static vec_t g_vec[128];
static int   g_nvec = 0;

static void load_vectors(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[256];
    if (!f) { printf("FAIL C1: cannot open %s\n", path); exit(1); }
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        line[strcspn(line, "\r\n")] = '\0';
        vec_t *v = &g_vec[g_nvec];
        char *p = line, *t[4];
        for (int i = 0; i < 4; i++) {
            t[i] = p;
            char *tab = strchr(p, '\t');
            if (i < 3) { if (!tab) { printf("FAIL C1: bad row %s\n", line); exit(1); } *tab = '\0'; p = tab + 1; }
        }
        snprintf(v->id, sizeof v->id, "%s", t[0]);
        snprintf(v->cap, sizeof v->cap, "%s", t[1]);
        snprintf(v->keys, sizeof v->keys, "%s", t[2]);
        snprintf(v->want, sizeof v->want, "%s", t[3]);
        g_nvec++;
    }
    fclose(f);
}

static void leg_vectors(void)
{
    for (int i = 0; i < g_nvec; i++) {
        calc_t c;
        calc_init(&c);
        for (const char *k = g_vec[i].keys; *k; k++) calc_press(&c, tok_cmd(*k));
        CHECK("C1 VECTORS", strcmp(calc_display(&c), g_vec[i].want) == 0,
              "%s keys \"%s\": readout \"%s\", the Mac shows \"%s\"",
              g_vec[i].id, g_vec[i].keys, calc_display(&c), g_vec[i].want);
    }
}

/* ===========================================================================
 * C2 KEYMAP (PC message = vkey << 8 | ascii; os/flair/event.c)
 * ===========================================================================*/
static void leg_keymap(void)
{
    static const struct { uint32_t msg; calc_cmd_t want; const char *what; } m[] = {
        { 0x0B30u, CALC_0, "'0'" }, { 0x0231u, CALC_1, "'1'" },
        { 0x0A39u, CALC_9, "'9'" }, { 0x342Eu, CALC_DOT, "'.'" },
        { 0x0D2Bu, CALC_ADD, "shift-= '+'" }, { 0x4E2Bu, CALC_ADD, "keypad +" },
        { 0x0C2Du, CALC_SUB, "'-'" }, { 0x092Au, CALC_MUL, "shift-8 '*'" },
        { 0x352Fu, CALC_DIV, "'/'" }, { 0x0D3Du, CALC_EQ, "'='" },
        { 0x1C0Du, CALC_EQ, "Enter" }, { 0x2E63u, CALC_CLEAR, "'c'" },
        { 0x2E43u, CALC_CLEAR, "'C'" }, { 0x4500u, CALC_CLEAR, "NumLock (Clear)" },
        { 0x011Bu, CALC_NONE, "Escape" }, { 0x0E08u, CALC_NONE, "Backspace" },
        { 0x1E61u, CALC_NONE, "'a'" },
    };
    for (size_t i = 0; i < sizeof m / sizeof m[0]; i++)
        CHECK("C2 KEYMAP", calc_cmd_from_key(m[i].msg) == m[i].want,
              "%s (0x%04X) -> %d, want %d", m[i].what, m[i].msg,
              (int)calc_cmd_from_key(m[i].msg), (int)m[i].want);
}

/* ===========================================================================
 * C3 PROPERTY
 * ===========================================================================*/
static uint32_t g_rng = 0x1BADB002u;
static uint32_t rnd(void) { g_rng = g_rng * 1103515245u + 12345u; return g_rng >> 8; }

static calc_cmd_t rnd_cmd(void)
{
    uint32_t r = rnd() % 100u;
    if (r < 55u) return (calc_cmd_t)(rnd() % 10u);
    if (r < 62u) return CALC_DOT;
    if (r < 90u) return (calc_cmd_t)(CALC_ADD + rnd() % 4u);
    if (r < 97u) return CALC_EQ;
    return CALC_CLEAR;
}

static const char *cmd_name(calc_cmd_t k)
{
    static const char *n[] = { "0","1","2","3","4","5","6","7","8","9",".","+","-","*","/","=","C","none" };
    return n[k];
}

static void leg_property(void)
{
    calc_cmd_t seq[48];
    int bad = 0;
    for (int run = 0; run < 20000 && bad < 3; run++) {
        int n = 1 + (int)(rnd() % 40u);
        for (int i = 0; i < n; i++) seq[i] = rnd_cmd();
        if (run % 7 == 0) {                   /* stress the formatter */
            for (int i = 0; i < n; i++)
                if (seq[i] == CALC_DOT && rnd() % 2u) seq[i] = CALC_9;
        }
        calc_t c; ref_t r;
        calc_init(&c); ref_init(&r);
        for (int i = 0; i < n; i++) {
            calc_press(&c, seq[i]); ref_key(&r, seq[i]);
            if (strcmp(calc_display(&c), r.shown) != 0) {
                char keys[160] = "";
                for (int j = 0; j <= i; j++) strcat(keys, cmd_name(seq[j]));
                CHECK("C3 PROPERTY", 0, "seed run %d, shortest failing prefix \"%s\": "
                      "readout \"%s\", reference \"%s\"", run, keys,
                      calc_display(&c), r.shown);
                bad++;
                break;
            }
        }
        g_checks++;
    }
}

/* ===========================================================================
 * C4 PARITY
 * ===========================================================================*/
static uint32_t key_msg_of_label(char label)
{
    switch (label) {               /* the PC key that types the label */
    case 'C': return 0x2E63u;
    case '=': return 0x0D3Du;
    case '/': return 0x352Fu;
    case '*': return 0x092Au;
    case '-': return 0x0C2Du;
    case '+': return 0x0D2Bu;
    case '.': return 0x342Eu;
    default:  return (uint32_t)(unsigned char)label;
    }
}

static void leg_parity(void)
{
    static const calc_key_rect_t keys[CALC_KEY_COUNT] = CALC_KEY_TABLE;
    calc_cmd_t click[CALC_KEY_COUNT], key[CALC_KEY_COUNT];
    for (int i = 0; i < CALC_KEY_COUNT; i++) {
        int l, t, r, b;
        calc_view_key_rect(i, &l, &t, &r, &b);
        int hit = calc_view_hit((l + r) / 2, (t + b) / 2);
        click[i] = calc_view_click_cmd(hit);
        key[i] = calc_cmd_from_key(key_msg_of_label(keys[i].label));
        CHECK("C4 PARITY", hit == i, "the centre of key '%c' hits key %d", keys[i].label, hit);
        CHECK("C4 PARITY", click[i] == key[i],
              "key '%c': a click commands %s, the keyboard commands %s",
              keys[i].label, cmd_name(click[i]), cmd_name(key[i]));
        CHECK("C4 PARITY", calc_view_hit(l - 1, t) != i && calc_view_hit(r, b - 1) != i,
              "key '%c': a click outside its frame still hits it", keys[i].label);
    }
    /* one session, two drivers */
    calc_t a, b;
    calc_init(&a); calc_init(&b);
    for (int s = 0; s < 200; s++) {
        int i = (int)(rnd() % CALC_KEY_COUNT);
        calc_press(&a, click[i]);
        calc_press(&b, key[i]);
        if (strcmp(calc_display(&a), calc_display(&b)) != 0) {
            CHECK("C4 PARITY", 0, "step %d (key '%c'): clicks show \"%s\", keys show \"%s\"",
                  s, keys[i].label, calc_display(&a), calc_display(&b));
            break;
        }
    }
}

/* ===========================================================================
 * C5 CAPTURE (libpng; loud-skip when the corpus is absent)
 * ===========================================================================*/
typedef struct { int w, h; unsigned char *rgb; } img_t;

static int png_load(const char *path, img_t *im)
{
    png_image p;
    memset(&p, 0, sizeof p);
    p.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&p, path)) return 0;
    p.format = PNG_FORMAT_RGB;
    im->w = (int)p.width; im->h = (int)p.height;
    im->rgb = malloc(PNG_IMAGE_SIZE(p));
    if (!png_image_finish_read(&p, NULL, im->rgb, 0, NULL)) { free(im->rgb); return 0; }
    return 1;
}

/* 0 white, 1 black, 2 anything else */
static int cls_rgb(const unsigned char *px)
{
    if (px[0] == 0 && px[1] == 0 && px[2] == 0) return 1;
    if (px[0] == 255 && px[1] == 255 && px[2] == 255) return 0;
    return 2;
}

static unsigned char g_canvas[CALC_CONTENT_H][CALC_CONTENT_W];
static void canvas_rect(void *u, int l, int t, int r, int b, int tok)
{
    (void)u;
    for (int y = t; y < b; y++) for (int x = l; x < r; x++)
        if (x >= 0 && y >= 0 && x < CALC_CONTENT_W && y < CALC_CONTENT_H)
            g_canvas[y][x] = (unsigned char)tok;
}

#define CAP_CX (CALC_WIN_L + CALC_CONTENT_DX)   /* 268: content origin */
#define CAP_CY (CALC_WIN_T + CALC_CONTENT_DY)   /* 113 */

static int leg_capture(const char *root)
{
    char path[512];
    img_t im;
    int skipped = 0, graded = 0;

    for (int i = 0; i < g_nvec; i++) {
        snprintf(path, sizeof path, "%s/goldens/captures/%s.png", root, g_vec[i].cap);
        if (!png_load(path, &im)) { skipped++; continue; }
        memset(g_canvas, 9, sizeof g_canvas);
        calc_view_paint_readout(g_vec[i].want, canvas_rect, NULL);
        int bad = 0, fx = -1, fy = -1;
        for (int y = CALC_DISP_T + 1; y < CALC_DISP_B; y++)
            for (int x = CALC_DISP_L + 1; x < CALC_DISP_R; x++) {
                int cap = cls_rgb(&im.rgb[3 * ((CAP_CY + y) * im.w + CAP_CX + x)]) == 1;
                int our = g_canvas[y][x] == CALC_TOK_BLACK;
                if (cap != our) { if (!bad) { fx = x; fy = y; } bad++; }
            }
        CHECK("C5 CAPTURE readout", bad == 0,
              "%s \"%s\": %d pixels differ from %s.png (first at content (%d,%d))",
              g_vec[i].id, g_vec[i].want, bad, g_vec[i].cap, fx, fy);
        graded++;
        free(im.rgb);
    }

    snprintf(path, sizeof path, "%s/goldens/captures/s8_calc_idle.png", root);
    if (png_load(path, &im)) {
        memset(g_canvas, 9, sizeof g_canvas);
        calc_view_paint("0", canvas_rect, NULL);
        int bad = 0, fx = -1, fy = -1;
        for (int y = 0; y < CALC_CONTENT_H; y++)
            for (int x = 0; x < CALC_CONTENT_W; x++) {
                /* the rDocProc inner corner notches are the frame's (the
                 * kernel WDEF draws them; graded by the emu gate) */
                if ((y < 2 && (x < 2 || x >= CALC_CONTENT_W - 2)) ||
                    (y >= CALC_CONTENT_H - 3 && (x < 2 || x >= CALC_CONTENT_W - 3)))
                    continue;
                int cap = cls_rgb(&im.rgb[3 * ((CAP_CY + y) * im.w + CAP_CX + x)]);
                if (cap != g_canvas[y][x]) { if (!bad) { fx = x; fy = y; } bad++; }
            }
        CHECK("C5 CAPTURE face", bad == 0,
              "the idle face: %d pixels differ from s8_calc_idle.png (first at content (%d,%d))",
              bad, fx, fy);
        graded++;
        free(im.rgb);
    } else {
        skipped++;
    }
    if (skipped)
        printf("LOUD-SKIP C5 CAPTURE: %d capture(s) absent under %s/goldens/captures "
               "(override SYSTEM7_DECOMP=...); %d rows NOT graded against the Mac pixels\n",
               skipped, root, skipped);
    return graded;
}

int main(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "usage: %s <calc_vectors.tsv> <system7-decomp>\n", argv[0]); return 2; }
    load_vectors(argv[1]);
    leg_vectors();
    leg_keymap();
    leg_property();
    leg_parity();
    int graded = leg_capture(argv[2]);
    printf("test_calc: %d checks, %d failures; %d vectors; %d capture crops graded\n",
           g_checks, g_fail, g_nvec, graded);
    return g_fail ? 1 : 0;
}
