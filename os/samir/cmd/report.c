/*
 * os/samir/cmd/report.c -- SAMIR REPORT FORM renderer (bead initech-airx, B78).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): freestanding C, <stdint.h> + samir/ headers
 * only, all OS contact through the PAL, static storage only, ASCII (Rule 12).
 *
 *   REPORT FORM <file> [<scope>] [FOR <c>] [WHILE <c>] [PLAIN]
 *               [HEADING <expC>] [NOEJECT] [TO FILE <file>] [SUMMARY]
 *
 * Ref (Law 1):
 *   - Using dBase III Plus.pdf U5-177/U5-178 (REPORT syntax and options);
 *     U7-8 group-expression types; U7-16 #126; U7-17 Report file invalid.
 *   - ../dbase3-decomp/specs/file-formats/frm.md Sec 2-5, 7 (the 1990-byte
 *     .frm: header tables, heap, 12-byte columns at 0x68C, footer at 0x7B6)
 *     and specs/reports-labels/report-form-generator.md Sec 3.5 (option byte).
 *   - ../dbase3-decomp/re/mint-results-004.md: real DBASE.EXE 1.1 REPORT FORM
 *     text minted under dosbox-x. Every layout and paging rule below is read
 *     off those bytes [verified: mint 004] unless tagged [inferred]:
 *       stream   : each line is CR LF + text; an eject is CR FF; the report
 *                  ends CR LF; TO FILE closes with ^Z. The console shows the
 *                  same stream without the FF (screenshots R4S1..R4S3).
 *       page hdr : lm + "Page No." + STR(page,6) [+ HEADING]; lm + date;
 *                  ';'-split title lines at lm+(U-len)/2, U = pw-lm-rm;
 *                  blank; column-heading band; blank; blank. PLAIN prints
 *                  title + band on page 1 only, no page/date lines.
 *       HEADING  : split on ';' and wrapped like a C cell to U-32 columns
 *                  [inferred: the one wrap golden allows U-32..U-27], each
 *                  piece at lm+(U-len)/2-1; an empty HEADING prints nothing.
 *       columns  : start lm, one blank between columns; C/D/L text left, N
 *                  via dec_format(width,dec); numeric headings right-aligned;
 *                  C values right-trimmed, split on ';', word-wrapped (break
 *                  after the last blank inside the width, else hard cut).
 *                  A line pads only up to each column that has text on it;
 *                  heading lines are top-aligned [inferred].
 *       groups   : lm-1 + "** " heading " " value; subgroup "* "; no blank
 *                  before the report's first group header.
 *       totals   : lm-1 + "* Subsubtotal *" / "** Subtotal **" /
 *                  "*** Total ***", then the sums under the totalled columns;
 *                  the end bands print even when no record was selected.
 *       scope    : default ALL (GO TOP); WHILE alone implies REST; NEXT n
 *                  leaves the pointer ON record n; counts are literals, as
 *                  in LIST.
 *       paging   : before each processed record eject when L >= lpp; a unit
 *                  of n lines (subtotal, total, group header) ejects when
 *                  L + n + 1 > lpp, a two-level group header counting n = 5;
 *                  a header placed after its own eject drops its blank; a
 *                  record's wrapped lines are never split. Fitted exactly on
 *                  83 lpp-sweep goldens; 111 goldens in all (mint 004).
 *       carry    : real III+ continues the SET ALTERNATE line count into a
 *                  report without an initial eject; SAMIR has no ALTERNATE,
 *                  so a report starts at line 0 (the alternate-free goldens).
 *   Refused LOUD, never guessed: TO PRINT (#126, InitechOS has no printer),
 *   the ? picklist and unknown clauses (#36), .frm double-space / summary /
 *   eject flags and option bits 1-2 that no golden shows (#50), memo values
 *   (#34), N/L/M group expressions and non-numeric totals (#9).
 */

#include <stdint.h>

#include "samir/interp.h"
#include "samir/workarea.h"
#include "samir/nav.h"
#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"
#include "samir/pal.h"

/* query.c engine helpers (expression evaluation + value display text). */
extern void q_render_val(char *buf, uint32_t *pos, uint32_t cap,
                         const xb_val *v, const xb_ctx *ctx, int dotted);

#define RP_FRM   1990    /* frm.md Sec 1: fixed record size */
#define RP_COLS  24      /* (0x7B6 - 0x68C) / 12 */
#define RP_LINE  512
#define RP_POOL  1024    /* one record's cell text */
#define RP_TXT   256

typedef struct {
    const char *expr, *head;
    int w, dec, start, off, len, tot, num;
} rp_col;

static struct {
    xb_interp *ip;
    samir_pal_t *pal;
    pal_fd fd;
    int ncol, nlev, plain, summary, nto, err;
    int lpp, lm, U, L, page, pos, gvn[2];
    const char *title, *gx[2], *gh[2];
    rp_col col[RP_COLS];
    double acc[3][RP_COLS];
    uint8_t frm[RP_FRM];
    char line[RP_LINE], pool[RP_POOL], gv[2][RP_TXT], hd[RP_TXT], fn[RP_TXT];
} R;

static int rp_u16(int o) { return R.frm[o] | (R.frm[o + 1] << 8); }
/* Shared III+ keyword policy; length is the typed spelling, not the canon. */
static int rp_word(const char *s, const char *w)
{
    return rt_kw_match(s, w);
}

/* ---- sinks: the TO FILE handle gets every byte, the console no FF ---- */
static void rp_raw(const char *s, int n)
{
    int i;
    if (R.fd >= 0 && R.pal->write(R.pal, R.fd, s, (uint32_t)n) != n) R.err = 29;
    for (i = 0; i < n && s[i] != '\f'; i++) ;
    xb_interp_output(R.ip, s, (uint32_t)i);
    if (i + 1 < n) xb_interp_output(R.ip, s + i + 1, (uint32_t)(n - i - 1));
}
static void rp_str(const char *s, int n) { while (n-- > 0 && R.pos < RP_LINE) R.line[R.pos++] = *s++; }
static void rp_to(int c) { while (R.pos < c && R.pos < RP_LINE) R.line[R.pos++] = ' '; }
static void rp_out(void)
{
    rp_raw("\r\n", 2);
    rp_raw(R.line, R.pos);
    R.pos = 0;
    R.L++;
}

/* Next wrapped piece of s[0..n) for width w: returns its print length; *adv
 * = bytes consumed (a ';' is consumed, not printed). [verified: R4DT] */
static int rp_seg(const char *s, int n, int w, int *adv)
{
    int i = 0, k;
    while (i < n && s[i] != ';') i++;
    if (i <= w) { *adv = i + (i < n); return i; }
    for (k = w; k && s[k - 1] != ' '; k--) ;
    *adv = k ? k : w;
    return *adv;
}
static int rp_center(int len) { return R.lm + (len < R.U ? (R.U - len) / 2 : 0); }
/* i-th ';' piece of s (NULL when absent); *n its length. */
static const char *rp_piece(const char *s, int i, int *n)
{
    while (i--) { while (*s && *s != ';') s++; if (!*s) return 0; s++; }
    for (*n = 0; s[*n] && s[*n] != ';'; ++*n) ;
    return s;
}

static void rp_header(void)
{
    const char *s;
    int i, n, a, k, c;
    if (!R.plain) {
        char t[16];
        uint32_t p = 0;
        xb_val d = xb_d(xb_interp_ctx(R.ip)->ctx_today);
        rp_to(R.lm); rp_str("Page No.     ", 13);
        for (k = R.page, i = 0; k; k /= 10) t[i++] = (char)('0' + k % 10);
        R.pos -= i - 1;                       /* STR(page,6) right-aligned */
        while (i) R.line[R.pos++] = t[--i];
        for (s = R.hd, n = rt_strlen(s), i = 0; n; s += a, n -= a) {
            k = rp_seg(s, n, R.U > 33 ? R.U - 32 : 1, &a);
            if (i++) rp_out();
            rp_to(rp_center(k) - 1);
            rp_str(s, k);
        }
        rp_out();
        rp_to(R.lm);
        q_render_val(t, &p, sizeof t, &d, xb_interp_ctx(R.ip), 0);
        rp_str(t, (int)p);
        rp_out();
    }
    if (R.plain && R.page > 1) return;
    for (i = 0; (s = rp_piece(R.title, i, &n)) != 0; i++) { rp_to(rp_center(n)); rp_str(s, n); rp_out(); }
    rp_out();
    for (i = 0;; i++) {                       /* the column-heading band */
        a = 0;
        for (c = 0; c < R.ncol; c++) {
            rp_col *q = &R.col[c];
            if (!(s = rp_piece(q->head, i, &n))) continue;
            a = 1;
            rp_to(q->start + ((q->num && n < q->w) ? q->w - n : 0));
            rp_str(s, n);
        }
        if (!a) break;
        rp_out();
    }
    rp_out(); rp_out();
}

static void rp_eject(void)
{
    rp_raw("\r\f", 2);
    R.L = 0; R.page++;
    rp_header();
}
/* A unit of n lines needs n+1 lines left. [verified: 83 lpp sweeps] */
static int rp_need(int n)
{
#ifdef SAMIR_REPORT_MUTATE_PAGE
    n--;   /* MUTANT: off-by-one room check */
#endif
    if (R.L + n + 1 > R.lpp) { rp_eject(); return 1; }
    return 0;
}
static void rp_label(const char *s)
{
    rp_to(R.lm - 1);
    rp_str(s, rt_strlen(s));
}
/* Subtotal / total band for level k (0 group, 1 subgroup, 2 grand). */
static void rp_total(int k)
{
    static const char *const lab[3] = { "** Subtotal **", "* Subsubtotal *", "*** Total ***" };
    int c;
    char t[RP_TXT];
    if (!R.nto) return;
#ifdef SAMIR_REPORT_MUTATE_NO_SUBTOTAL
    if (k < 2) return;   /* MUTANT: subtotal bands dropped */
#endif
    rp_need(2);
    rp_label(lab[k]); rp_out();
    for (c = 0; c < R.ncol; c++) {
        rp_col *q = &R.col[c];
        if (!q->tot) continue;
        rp_to(q->start);
        dec_format(R.acc[k][c], q->w, q->dec, t);
        rp_str(t, q->w);
        if (k < 2) R.acc[k][c] = 0.0;
    }
    rp_out();
}

/* Display text of v at dst[*p]; C right-trimmed when trim. 0 or catalog code. */
static int rp_text(const xb_val *v, char *dst, uint32_t *p, uint32_t cap, int trim)
{
    uint32_t s = *p;
    if (v->t == XB_M) return 34;
    q_render_val(dst, p, cap, v, xb_interp_ctx(R.ip), 1);
    if (*p >= cap) return 43;
    while (trim && v->t == XB_C && *p > s && dst[*p - 1] == ' ') (*p)--;
    return 0;
}

static int rp_record(int first, int *ec)
{
    char tmp[2][RP_TXT];
    uint32_t tn[2], p = 0;
    int i, c, k, more, ch[2] = { 0, 0 };
    xb_val v;
    if (R.L >= R.lpp) rp_eject();
    for (k = 0; k < R.nlev; k++) {
        tn[k] = 0;
        if (xb_interp_eval(R.ip, R.gx[k], &v, ec)) return -1;
        if (v.t != XB_C && v.t != XB_D) { *ec = 9; return -1; }   /* U7-8 */
        if ((*ec = rp_text(&v, tmp[k], &tn[k], RP_TXT, 0)) != 0) return -1;
        ch[k] = first || (k && ch[0]) || (int)tn[k] != R.gvn[k] || rt_memcmp(tmp[k], R.gv[k], tn[k]);
    }
    if (!first) {
        if (R.nlev > 1 && ch[1]) rp_total(1);
        if (R.nlev && ch[0]) rp_total(0);
    }
    for (k = 0; k < R.nlev; k++) {
        if (!ch[k]) continue;
        rt_memcpy(R.gv[k], tmp[k], tn[k]); R.gvn[k] = (int)tn[k];
        /* A subgroup header right under its group header keeps its blank. */
        if ((k && ch[0]) || !(rp_need(k || R.nlev < 2 ? 2 : 5) || first)) rp_out();
        rp_label(k ? "* " : "** ");
        rp_str(R.gh[k], rt_strlen(R.gh[k])); rp_str(" ", 1);
        rp_str(tmp[k], (int)tn[k]); rp_out();
    }
    for (c = 0; c < R.ncol; c++) {            /* evaluate every cell */
        rp_col *q = &R.col[c];
        if (xb_interp_eval(R.ip, q->expr, &v, ec)) return -1;
        q->off = (int)p;
        if (v.t == XB_N) {
            if (p + (uint32_t)q->w >= RP_POOL) { *ec = 43; return -1; }
            dec_format(v.u.n, q->w, q->dec, R.pool + p);
            p += (uint32_t)q->w;
            for (k = 0; q->tot && k < 3; k++) R.acc[k][c] += v.u.n;
        } else {
            if (q->tot) { *ec = 9; return -1; }
            if ((*ec = rp_text(&v, R.pool, &p, RP_POOL, 1)) != 0) return -1;
        }
        q->len = (int)p - q->off;
    }
    for (i = 0; !R.summary; i++) {            /* the record's row lines */
        more = 0;
        for (c = 0; c < R.ncol; c++) {
            rp_col *q = &R.col[c];
            int a, n;
            if (i && !q->len) continue;
            n = rp_seg(R.pool + q->off, q->len, q->w, &a);
            rp_to(q->start
#ifdef SAMIR_REPORT_MUTATE_COLUMN
                  + (c == 1)   /* MUTANT: second column shifted right */
#endif
                  );
            rp_str(R.pool + q->off, n);
            q->off += a; q->len -= a;
            more |= q->len;
        }
        rp_out();
        if (!more) break;
    }
    return 0;
}

/* name (+ ext when the last path part has none) into R.fn; 0 or 10. */
static int rp_name(const char *name, const char *ext)
{
    int n = rt_strlen(name), i, dot = 0;
    if (!n || n + 5 > RP_TXT) return 10;
    rt_memcpy(R.fn, name, (uint32_t)n + 1u);
    for (i = 0; i < n; i++) dot = name[i] == '.' ? 1 : (name[i] == '\\' || name[i] == ':') ? 0 : dot;
    if (!dot) rt_memcpy(R.fn + n, ext, 5u);
    return 0;
}

/* Load + validate the .frm (frm.md Sec 7). 0 or a catalog code. */
static int rp_load(void)
{
    const char *str[54];
    int i, x, tot, nz = 0, sum = 0, nstr;
    pal_fd fd = R.pal->open(R.pal, R.fn, PAL_RD);
    if (fd < 0) return fd == -PAL_ENOENT ? 1 : 29;
    i = R.pal->read(R.pal, fd, R.frm, RP_FRM);
    if (i == RP_FRM && R.pal->read(R.pal, fd, R.fn, 1u) != 0) i = -1;   /* longer */
    R.pal->close(R.pal, fd);
    tot = rp_u16(2);
    if (i != RP_FRM || rp_u16(0) != 2 || rp_u16(0x7C4) != 2 || tot > 0x68C - 0xE0) return 50;
    for (i = 0; i < 55; i++) { x = rp_u16(4 + 2 * i); sum += x; nz += x != 0; }
    nstr = nz - 1;
    R.ncol = rp_u16(0x7BE);
    R.nlev = nstr / 2 - R.ncol;
    if (sum != tot || !rp_u16(4) || (nstr & 1) || R.ncol < 1 || R.ncol > RP_COLS || R.nlev < 0 || R.nlev > 2) return 50;
    for (i = 0; i <= nstr; i++) {             /* entry 0 = title at heap 0 */
        int off = i ? rp_u16(0x72 + 2 * i) : 0, len = rp_u16(4 + 2 * i);
        if (!len || off + len > tot || R.frm[0xE0 + off + len - 1]) return 50;
        str[i ? i - 1 : 0] = (const char *)R.frm + 0xE0 + off;
        if (!i) R.title = str[0];
    }
    for (i = 0; i < R.nlev; i++) { R.gx[i] = str[2 * i]; R.gh[i] = str[2 * i + 1]; }
    R.lpp = rp_u16(0x7B8); R.lm = rp_u16(0x7BA);
    R.U = rp_u16(0x7B6) - R.lm - rp_u16(0x7BC);
    /* .frm options no golden shows are refused, never guessed. */
    if (R.frm[0x7C0] != 'N' || R.frm[0x7C1] != 'N' || R.frm[0x7C2] != 'N' || (R.frm[0x7C3] & 6) || R.U < 0) return 50;
    x = R.lm;
    R.nto = 0;
    for (i = 0; i < R.ncol; i++) {
        int o = 0x68C + 12 * i, e = rp_u16(o + 8), h = rp_u16(o + 10);
        rp_col *q = &R.col[i];
        q->w = rp_u16(o); q->dec = rp_u16(o + 6);
        q->tot = R.frm[o + 5] == 'Y';
        if (e < 1 || h < 1 || e > nstr || h > nstr || q->w < 1 || q->w > 250 || q->dec > 15 || (!q->tot && R.frm[o + 5] != 'N')) return 50;
        R.nto += q->tot;
        q->expr = str[e - 1]; q->head = str[h - 1];
        q->start = x; x += q->w + 1;
        if (x >= RP_LINE) return 50;
    }
    return 0;
}

/* ---- clause parsing (U5-177 syntax) ---- */
enum { K_FOR, K_WHILE, K_HEADING, K_TO, K_NEXT, K_RECORD, K_PLAIN, K_NOEJECT, K_SUMMARY, K_ALL, K_REST, K_N };
static const char *const g_kw[K_N] = { "FOR", "WHILE", "HEADING", "TO", "NEXT", "RECORD", "PLAIN", "NOEJECT", "SUMMARY", "ALL", "REST" };

static int rp_cond(const char *c, int *ec)    /* 1 true, 0 false, -1 fault */
{
    xb_val v;
    if (!c) return 1;
    if (xb_interp_eval(R.ip, c, &v, ec)) return -1;
    if (v.t != XB_L) { *ec = 37; return -1; }
    return v.u.l != 0;
}

/* Position for the scope (default ALL; WHILE implies REST), set the column
 * types from the starting record, print the first page header, then walk. */
static int rp_walk(char *cl[K_N], int *ec)
{
    wa_env *env = xb_interp_env(R.ip);
    int area = wa_selected(env), first = 1, t, i;
    uint32_t lim = 0, seen = 0;
    const char *n = cl[K_NEXT] ? cl[K_NEXT] : cl[K_RECORD];
    xb_val v;
    if (n) {                                  /* literal count, as LIST */
        while (*n >= '0' && *n <= '9') lim = lim * 10u + (uint32_t)(*n++ - '0');
        if (*n || !lim) { *ec = 10; return -1; }
    }
    if (cl[K_RECORD]) {
        if (wa_nav_goto(env, area, lim) != NAV_OK) { *ec = 5; return -1; }
        lim = 1;
    } else if (!n && !cl[K_REST] && !cl[K_WHILE] && wa_nav_go_top(env, area) != NAV_OK) {
        *ec = wa_view_error(env); return -1;
    }
    /* Numeric columns right-align their headings. At EOF no record value
     * exists; the heading then stays left-aligned [inferred, no golden]. */
    for (i = 0; i < R.ncol; i++)
        R.col[i].num = !wa_eof(env, area) && !xb_interp_eval(R.ip, R.col[i].expr, &v, ec) && v.t == XB_N;
    *ec = 0;
    /* NOEJECT or option bit 0 suppress the initial eject [verified: FLT_INFO]. */
    if (!cl[K_NOEJECT] && !(R.frm[0x7C3] & 1)) rp_raw("\r\f", 2);
    R.L = 0; R.page = 1;
    rp_header();
    while (!wa_eof(env, area)) {
        t = cl[K_RECORD] ? 1 : wa_visible(env, area);
        if (t < 0) { *ec = wa_view_error(env); return -1; }
        if (t) {
            if (lim && seen >= lim) break;
            seen++;
            if ((t = rp_cond(cl[K_WHILE], ec)) <= 0) { if (t) return -1; break; }
            if ((t = rp_cond(cl[K_FOR], ec)) < 0) return -1;
            if (t) { if (rp_record(first, ec)) return -1; first = 0; }
            if (lim && seen >= lim) break;    /* NEXT n stays on record n */
        }
        if (wa_nav_skip(env, area, 1)) { *ec = wa_view_error(env); return -1; }
    }
    /* End bands print even for an empty selection [verified: R4HT]. */
    if (R.nlev > 1) rp_total(1);
    if (R.nlev) rp_total(0);
    rp_total(2);
    return 0;
}

static int rp_run(const char *args, int *ec)
{
    char a[RP_TXT], *cl[K_N], *s, *e, *last = 0;
    int k = -1, q = 0, depth = 0, rc = 0, i;
    wa_env *env = xb_interp_env(R.ip);
    xb_val v;
    R.pal = xb_interp_pal(R.ip);
    R.fd = -1; R.err = 0; R.pos = 0;
    i = rt_strlen(args);
    if (i >= RP_TXT) { *ec = 18; return -1; }
    rt_memcpy(a, args, (uint32_t)i + 1u);
    for (s = a; *s == ' '; s++) ;
    if (!rp_word(s, "FORM")) { *ec = 36; return -1; }
    for (s += 4; *s == ' '; s++) ;
    for (e = s; *e && *e != ' '; e++) ;
    if (*e) *e++ = 0;
    if (*s == '?' || (rc = rp_name(s, ".FRM")) != 0) { *ec = rc ? rc : 36; return -1; }
    for (i = 0; i < K_N; i++) cl[i] = 0;
    /* Split the rest at top-level clause keywords. */
    for (s = e;; e++) {
        int kw = -1;
        if (!q && !depth && (e == s || e[-1] == ' '))
            for (i = 0; i < K_N && kw < 0; i++) if (rp_word(e, g_kw[i])) kw = i;
        if (kw >= 0 || !*e) {
            int kwlen = kw >= 0 ? rp_word(e, g_kw[kw]) : 0;
            char *t = last ? last : s, *z = e;
            while (*t == ' ') t++;
            while (z > t && z[-1] == ' ') z--;
            if (k < 0 ? (z > t) : (k >= K_PLAIN && z > t)) { *ec = 36; return -1; }
            if (!*e) { if (k >= 0) { *z = 0; cl[k] = t; } break; }
            if (k >= 0) { *z = 0; cl[k] = t; }
            k = kw; e += kwlen; last = e; e--;
            continue;
        }
        if (q) { if (*e == q) q = 0; }
        else if (*e == '"' || *e == '\'') q = *e;
        else if (*e == '[') q = ']';
        else if (*e == '(') depth++;
        else if (*e == ')' && depth) depth--;
    }
    if ((s = cl[K_TO]) != 0) {
        int print_len = rp_word(s, "PRINT");
        if (print_len && !s[print_len]) { *ec = 126; return -1; }   /* U7-16 */
        if (!rp_word(s, "FILE")) { *ec = 36; return -1; }
        for (s += 4; *s == ' '; s++) ;
        cl[K_TO] = s;
    }
    if (!wa_is_open(env, wa_selected(env))) { *ec = 52; return -1; }
    if ((rc = rp_load()) != 0) { *ec = rc; return -1; }
    R.plain = cl[K_PLAIN] != 0;
    R.summary = cl[K_SUMMARY] != 0;
    R.hd[0] = 0;
    if (cl[K_HEADING] && !R.plain) {          /* U5-177: PLAIN takes precedence */
        uint32_t p = 0;
        if (xb_interp_eval(R.ip, cl[K_HEADING], &v, ec)) return -1;
        if (v.t != XB_C) { *ec = 9; return -1; }
        if ((*ec = rp_text(&v, R.hd, &p, RP_TXT - 1u, 1)) != 0) return -1;
        R.hd[p] = 0;
    }
    rt_memset(R.acc, 0, sizeof R.acc);
    if (cl[K_TO]) {
        if ((rc = rp_name(cl[K_TO], ".TXT")) != 0) { *ec = rc; return -1; }   /* U5-178 */
        R.fd = R.pal->open(R.pal, R.fn, PAL_WR | PAL_CREATE | PAL_TRUNC);
        if (R.fd < 0) { *ec = 29; return -1; }
    }
    i = wa_include_deleted(env, cl[K_RECORD] || cl[K_NEXT]);
    rc = rp_walk(cl, ec);
    wa_include_deleted(env, i);
    if (!rc) rp_raw("\r\n", 2);
    if (R.fd >= 0) {
        if (R.pal->write(R.pal, R.fd, "\x1a", 1u) != 1 || R.pal->close(R.pal, R.fd)) R.err = 29;
        R.fd = -1;
    }
    if (!rc && R.err) { *ec = R.err; rc = -1; }
    return rc;
}

/* LABEL FORM: Using III+ U5-143; DML-15 ADDRESS.LBL and EXTRA-01..05
 * independently pin comma trimming, across columns and CR band terminators. */
static int lbl_run(const char *args,int *ec)
{
    char a[256],*p,*s,*e,expr[61],band[16][512];int len[16];
    int h,w,lm,gap,across,spacing,area,col=0,rc=0;
    wa_env *env=xb_interp_env(R.ip);pal_fd form;
    R.pal=xb_interp_pal(R.ip);R.fd=-1;R.err=0;
    if(rt_strlen(args)>=sizeof a){*ec=18;return -1;}
    rt_memcpy(a,args,rt_strlen(args)+1);p=a;
    while(*p==' ')p++;
    if(!rp_word(p,"FORM")){*ec=36;return -1;}
    for(p+=4;*p==' ';p++);
    for(e=p;*e && *e!=' ';e++);
    if(*e)*e++=0;
    if((*ec=rp_name(p,".LBL")))return -1;
    form=R.pal->open(R.pal,R.fn,PAL_RD);if(form<0){*ec=form==-PAL_ENOENT?1:29;return -1;}
    int n=R.pal->read(R.pal,form,R.frm,sizeof R.frm);R.pal->close(R.pal,form);
    if(n!=1034 || R.frm[0]!=2 || R.frm[1033]!=2){*ec=54;return -1;}
    h=rp_u16(61);w=rp_u16(63);lm=rp_u16(65);spacing=rp_u16(67);gap=rp_u16(69);across=rp_u16(71);
    if(h<1 || h>16 || !w || across<1 || across>16 || lm+(w+gap)*across>512 || spacing>100){*ec=54;return -1;}
    for(p=e;*p==' ';p++);
    if(*p) {
        if(!rp_word(p,"TO")){*ec=36;return -1;}
        for(p+=2;*p==' ';p++);
        if(!rp_word(p,"FILE")){*ec=36;return -1;}
        for(p+=4;*p==' ';p++);
        if((*ec=rp_name(p,".TXT")))return -1;
        R.fd=R.pal->open(R.pal,R.fn,PAL_WR|PAL_CREATE|PAL_TRUNC);
        if(R.fd<0){*ec=29;return -1;}
    }
    area=wa_selected(env);
    if(!wa_is_open(env,area)){*ec=52;rc=-1;goto done;}
    rt_memset(band,' ',sizeof band);rt_memset(len,0,sizeof len);
    if(wa_nav_index_top(env,area)!=NAV_OK){*ec=29;rc=-1;goto done;}
    while(!wa_eof(env,area)) {
        for(int line=0;line<h;line++) {
            unsigned pos=0;int first=1;char cell[512];
            rt_memcpy(expr,R.frm+73+line*60,60);expr[60]=0;
            for(p=expr;*p;) {
                int q=0,depth=0;xb_val v;
                for(e=p;*e;e++) {
                    if(q){if(*e==q)q=0;}
                    else if(*e=='"' || *e=='\'' || *e=='[')q=*e=='['?']':*e;
                    else if(*e=='(')depth++;
                    else if(*e==')')depth--;
                    else if(*e==',' && !depth)break;
                }
                s=*e?e+1:e;if(*e)*e=0;
                while(*p==' ')p++;
                if(*p) {
                    if(xb_interp_eval(R.ip,p,&v,ec)){rc=-1;goto done;}
                    if(v.t!=XB_C){*ec=9;rc=-1;goto done;}
                    unsigned z=v.u.c.len;
                    while(z && v.u.c.p[z-1]==' ')z--;
                    if(z) {
                        if(!first && pos<sizeof cell)cell[pos++]=' ';
                        first=0;
                        for(unsigned j=0;j<z && pos<sizeof cell;j++)cell[pos++]=v.u.c.p[j];
                    }
                }
                p=s;
            }
            unsigned start=(unsigned)(lm+col*(w+gap)),take=pos>(unsigned)w?(unsigned)w:pos;
#ifdef SAMIR_DML_MUTANT_label
            take=0; /* suppress label text while retaining the band geometry */
#endif
            rt_memcpy(band[line]+start,cell,take);
            if(take)len[line]=(int)(start+take);
        }
        col++;
        if(wa_nav_skip(env,area,1)!=NAV_OK){*ec=29;rc=-1;goto done;}
        if(col==across || wa_eof(env,area)) {
            for(int i=0;i<h;i++){rp_raw("\r\n",2);rp_raw(band[i],len[i]);}
            for(int i=0;i<spacing;i++)rp_raw("\r\n",2);
            rp_raw("\r",1);col=0;
            rt_memset(band,' ',sizeof band);rt_memset(len,0,sizeof len);
        }
    }
 done:
    if(R.fd>=0) {
        if(R.pal->write(R.pal,R.fd,"\x1a",1)!=1 || R.pal->close(R.pal,R.fd))R.err=29;
        R.fd=-1;
    }
    if(!rc && R.err){*ec=R.err;rc=-1;}
    return rc;
}

static int report_cmd_hook(void *user, xb_interp *ip, const char *verb,
                           const char *args, int *err_code)
{
    (void)user;
    if(rp_word(verb,"LABEL")) {R.ip=ip;*err_code=0;return lbl_run(args,err_code)?-INTERP_ERR_EVAL:CMD_OK;}
    if (!rp_word(verb, "REPORT")) return CMD_UNKNOWN;
    R.ip = ip;
    *err_code = 0;
    return rp_run(args, err_code) ? -INTERP_ERR_EVAL : CMD_OK;
}

int report_register(xb_interp *ip);
int report_register(xb_interp *ip)
{
    return xb_interp_add_cmd_hook(ip, report_cmd_hook, (void *)0);
}
