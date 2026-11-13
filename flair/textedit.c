/*
 * os/flair/textedit.c -- FLAIR TextEdit (TERec) implementation -- THE ARTIFACT.
 *
 * beads: initech-77dj (FLAIR Phase 4.5 -- the TextEdit half). REDUCED first cut
 *        (committee ruling wf_00931e9e); see textedit.h for the full contract +
 *        the deferred-full-TERec note.
 * ERA AXIS: Mac OS 8 Platinum (DEC-10) is the FLAIR BASE; this unchanged
 * TextEdit semantic core remains retained Toolbox heritage (BC-10.10).
 * Ref:   ../system7-decomp/specs/toolbox/textedit.md -- Sec 2 (half-open
 *        [selStart,selEnd) selection), Sec 4 (CR-delimited + crOnly<0 + word-wrap
 *        line layout; lineStarts/nLines), Sec 6 (TEKey/TECut/TECopy/TEPaste/
 *        TESetSelect semantics). os/flair/scrap.h (FlairPutScrap/FlairGetScrap --
 *        cut/copy/paste route through the shell-owned Scrap; FLAIR_SCRAP_TYPE_TEXT).
 *        CLAUDE.md Law 1 (ground truth), Law 2 (oracle is truth, not the agent),
 *        Law 3 (freestanding artifact; no libc, no malloc; dual-compile), Rule 2
 *        (fail loud), Rule 6 (mutants: each #ifdef MUST compile + go RED), Rule 11
 *        (deterministic), Rule 12 (ASCII-clean).
 *
 * MUTANT GUARDS (Rule 6 -- mutation-proven oracle; NEVER define in a real build):
 *   TE_MUT_SEL_INCLUSIVE   -- treat the selection as inclusive [selStart,selEnd]
 *       (delete selEnd-selStart+1 chars). The half-open delete in
 *       te_delete_range() over-deletes by one -> step 5 / step 6 RED.
 *   TE_MUT_PASTE_NO_DELETE -- TEKey / TEInsert / TEPaste insert WITHOUT first
 *       deleting the selected range -> the range survives the insert -> step 5 RED.
 *   TE_MUT_LINEBREAK_OFF   -- the CR line break records the CR offset instead of
 *       the offset PAST it (off-by-one lineStarts) -> step 9 (CR test) RED.
 *   TE_MUT_WRAP_CHARS      -- the retired monospaced rule: wrap when a line
 *       reaches wrapWidth / 8 CHARACTERS instead of measuring proportional
 *       pixels -> the pixel-wrap steps (10-12) RED (bead initech-tdnl.68).
 *   The editing surface (bead initech-tdnl.16; harness/proptest/
 *   test_te_surface.c, each RED for its named reason):
 *   TE_MUT_HIT_FLOOR / TE_MUT_HIT_PAST_CR / TE_MUT_ARROW_NO_COLLAPSE /
 *   TE_MUT_EXTEND_ANCHOR / TE_MUT_PROBE_NO_LINES (see each site).
 *
 * ERA CEILING (Law 3): monostyled, Mac TextEdit model only -- no Win32 edit
 *   control names, no styled (txSize==-1) runs (textedit.md Sec 5, out of scope).
 *
 * FREESTANDING (Law 3): <stdint.h> (via textedit.h) + <stddef.h> (NULL) + the
 *   Font Manager's header-only metrics (text.h: text_face_advance) only.
 *   No string.h / libc / malloc. The in-place insert/delete shifts use the
 *   overlap-safe te_memmove helper below (forward when dst<src, backward else).
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#include <stddef.h>   /* NULL -- freestanding-safe (C11 S4p6 required header) */
#include "textedit.h"
#include "text.h"       /* text_face_advance -- the styled proportional advance */

/* ---------------------------------------------------------------------------
 * Fail-loud dual-compile pattern (mirrors scrap.c SCRAP_PANIC / process.c
 * PROC_PANIC). Hosted: abort() crashes cleanly so the oracle traces the cause.
 * Freestanding kernel: a deliberate infinite loop is the loud deterministic
 * halt (Rule 2 / Rule 11). The (msg) is documentation; it is intentionally not
 * emitted (no serial here -- the in-kernel caller surfaces PC LOAD LETTER).
 * ------------------------------------------------------------------------- */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1 && !defined(__KERNEL_FREESTANDING__)
#  include <stdlib.h>   /* abort() -- hosted only */
#  define TE_PANIC(msg)  abort()
#else
#  define TE_PANIC(msg)  do { for (;;) { } } while (0)
#endif

/* ASCII control codes handled by the editor (named, not magic). */
#define TE_CH_BS  0x08   /* Backspace */
#define TE_CH_CR  0x0D   /* Return / carriage return -- the line delimiter      */
#define TE_CH_SP  0x20   /* Space -- the word-wrap break opportunity            */

/* ---------------------------------------------------------------------------
 * te_memmove -- freestanding overlap-safe byte move (no string.h / libc).
 * Copies `n` bytes from src to dst, correct even when the regions overlap:
 * forward when dst < src (a left shift, e.g. delete), backward when dst > src
 * (a right shift, e.g. insert). This is THE reason a naive forward-only copy is
 * not enough -- insert shifts the tail RIGHT into itself.
 * ------------------------------------------------------------------------- */
static void te_memmove(char *dst, const char *src, int n)
{
    int i;
    if (dst == src || n <= 0)
        return;
    if (dst < src) {
        for (i = 0; i < n; i++)
            dst[i] = src[i];
    } else {
        for (i = n - 1; i >= 0; i--)
            dst[i] = src[i];
    }
}

/* ---------------------------------------------------------------------------
 * te_recompute_lines -- rebuild nLines + lineStarts after a mutation
 * (textedit.md Sec 4). Line 0 always starts at offset 0. A CR (0x0D) terminates
 * the current line; the next line begins PAST the CR. crOnly < 0 == Return-only,
 * no word wrap (the line may run past the width) -- textedit.md Sec 4 step 1.
 *
 * WORD WRAP IN PIXELS (bead initech-tdnl.68; textedit.md Sec 4 step 1: "lines
 * whose width is bounded by destRect's width ... Breaking is by WORD", the
 * widths "from the NFNT owTable"): wrapWidth is the destRect width in PIXELS
 * and every char costs its styled proportional advance in (txFont, txFace)
 * (text_face_advance: the real NFNT 5478 advances for Chicago, +1 when bold).
 * A non-space char whose advance would carry the line past wrapWidth ends the
 * line at the start of its word (just past the line's last space); a word
 * wider than the whole width hard-breaks before the char that does not fit
 * (every line keeps at least one char). Spaces never start a wrap: they stay
 * on the line they follow, hanging past the edge [InitechOS reading of the
 * default wordBreak; consistent with the golden test-textedit step 10 grades:
 * System 7.5.3's alert text in s7_dialog_alert.png breaks exactly where this
 * rule breaks it for any width in 299..303 px].
 * lineStarts holds N == nLines entries (each line's START), not the IM
 * nLines+1 sentinel form -- the capped first cut.
 * ------------------------------------------------------------------------- */
/* The record's strike: (txFont, txSize) through the ONE text.h mapping
 * (bead initech-uelk: applFont at 12 = Geneva 12). */
static text_font_t te_font(const FlairTE *te)
{
    return text_font_from_txfont_size((int)te->txFont, (int)te->txSize);
}

static int te_char_w(const FlairTE *te, char ch)
{
    text_font_t f = te_font(te);
    return text_face_advance(f, (unsigned)te->txFace & TEXT_FACE_SYNTH,
                             (int)(unsigned char)ch);
}

/* THE TEXT A LAYOUT READS (bead initech-tdnl.16). The live buffer, or the
 * buffer AS IT WOULD BE after replacing [selA, selB) with an inserted run: a
 * three-piece view (head | run | tail) read char by char, so the capacity
 * probe FlairTECanReplace lays out the hypothetical text WITHOUT writing it
 * anywhere (no second 4 KiB buffer). One layout routine serves both. */
typedef struct te_view {
    const char *a; int na;   /* head: text[0, na)              */
    const char *b; int nb;   /* the inserted run               */
    const char *c; int nc;   /* tail: the text after the range */
} te_view_t;

static char te_view_at(const te_view_t *v, int i)
{
    if (i < v->na) return v->a[i];
    i -= v->na;
    if (i < v->nb) return v->b[i];
    return v->c[i - v->nb];
}

/* te_layout -- the line breakdown of the view (rules above). Writes up to `cap`
 * line starts into `starts` (NULL: count only) and returns the line count, or
 * -1 as soon as a line beyond `cap` would be needed (nothing past `cap` is
 * ever written). */
static int te_layout(const FlairTE *te, const te_view_t *v, int16_t *starts,
                     int cap)
{
    int     n = 0;        /* lines emitted                                     */
    int     len = v->na + v->nb + v->nc;
    int     s;            /* start offset of the current line                  */
    int     last_break;   /* offset just past the line's last space (> s)      */
    int     w;            /* pixel width of text[s, i)                         */
    int     wrap_on;
    int     i;

#define TE_EMIT(off) do { if (n >= cap) return -1;                            \
                          if (starts) { starts[n] = (int16_t)(off); }           \
                          n++; } while (0)
    wrap_on = (te->crOnly >= 0) && (te->wrapWidth > 0);

    TE_EMIT(0);              /* line 0 always starts at offset 0 */
    s = 0;
    last_break = -1;
    w = 0;
    i = 0;

    while (i < len) {
        char ch = te_view_at(v, i);

        if (ch == TE_CH_CR) {
#ifdef TE_MUT_LINEBREAK_OFF
            /* MUTANT (Rule 6): record the CR offset instead of the offset PAST
             * it -- an off-by-one lineStarts. Step 9 (CR) RED. */
            TE_EMIT(i);
#else
            TE_EMIT(i + 1);   /* next line starts past the CR */
#endif
            s = i + 1;
            last_break = -1;
            w = 0;
            i++;
            continue;
        }

#ifdef TE_MUT_WRAP_CHARS
        /* MUTANT (Rule 6): the retired monospaced rule, wrapWidth/8 chars. */
        if (wrap_on && ch != TE_CH_SP && (i - s) >= te->wrapWidth / 8 && i > s) {
#else
        if (wrap_on && ch != TE_CH_SP && w + te_char_w(te, ch) > te->wrapWidth &&
            i > s) {
#endif
            int brk = (last_break > s) ? last_break : i;
            TE_EMIT(brk);
            s = brk;
            last_break = -1;
            w = 0;
            for (int k = s; k < i; k++)        /* the carried partial word */
                w += te_char_w(te, te_view_at(v, k));
            continue;   /* re-examine text[i] on the new line (no advance) */
        }

        w += te_char_w(te, ch);
        if (ch == TE_CH_SP)
            last_break = i + 1;
        i++;
    }
#undef TE_EMIT
    return n;
}

static void te_recompute_lines(FlairTE *te)
{
    te_view_t v = { te->text, te->teLength, NULL, 0, NULL, 0 };
    int n = te_layout(te, &v, te->lineStarts, FLAIR_TE_MAX_LINES);
    if (n < 0)
        TE_PANIC("FlairTE: line count exceeds FLAIR_TE_MAX_LINES (reduced cut)");
    te->nLines = (int16_t)n;
}

/* ---------------------------------------------------------------------------
 * te_delete_span -- delete `count` chars starting at offset `a` ([a, a+count)),
 * shifting the tail left, and collapse the selection to the caret at `a`.
 * ------------------------------------------------------------------------- */
static void te_delete_span(FlairTE *te, int16_t a, int16_t count)
{
    int16_t end  = (int16_t)(a + count);
    int16_t tail = (int16_t)(te->teLength - end);   /* chars after the span */
    if (count <= 0)
        return;
    te_memmove(te->text + a, te->text + end, (int)tail);   /* dst<src: left shift */
    te->teLength = (int16_t)(te->teLength - count);
    te->selStart = a;
    te->selEnd   = a;
}

/* ---------------------------------------------------------------------------
 * te_delete_range -- delete the half-open range [a, b) (a <= b, both in
 * [0, teLength]). This is the ONE place the selection span is measured, so
 * TE_MUT_SEL_INCLUSIVE perturbs it in exactly one spot. The count is clamped to
 * stay in-bounds so the mutant produces a WRONG ANSWER, not a memory fault.
 * ------------------------------------------------------------------------- */
static void te_delete_range(FlairTE *te, int16_t a, int16_t b)
{
    int16_t count = (int16_t)(b - a);
#ifdef TE_MUT_SEL_INCLUSIVE
    /* MUTANT (Rule 6): inclusive [a,b] -- delete one extra char. Step 5/6 RED. */
    count = (int16_t)(count + 1);
#endif
    if (count < 0)
        count = 0;
    if ((int)a + (int)count > (int)te->teLength)
        count = (int16_t)(te->teLength - a);   /* clamp: stay memory-safe */
    te_delete_span(te, a, count);
}

/* ---------------------------------------------------------------------------
 * te_insert_at -- insert `len` bytes from `s` at offset `pos`, shifting the tail
 * right. Fail loud (Rule 2) if the result would exceed FLAIR_TE_TEXT_MAX. The
 * caret advances past the inserted run (selStart=selEnd=pos+len).
 * ------------------------------------------------------------------------- */
static void te_insert_at(FlairTE *te, int16_t pos, const char *s, int16_t len)
{
    int16_t tail;
    int     i;

    if (len <= 0) {
        te->selStart = pos;
        te->selEnd   = pos;
        return;
    }
    if ((int)te->teLength + (int)len > FLAIR_TE_TEXT_MAX)
        TE_PANIC("FlairTE: insert would overflow text buffer (fail loud)");

    tail = (int16_t)(te->teLength - pos);
    /* shift the tail right by len (dst = pos+len > src = pos): backward copy. */
    te_memmove(te->text + pos + len, te->text + pos, (int)tail);
    /* drop the new chars in (no overlap with the now-shifted tail). */
    for (i = 0; i < len; i++)
        te->text[pos + i] = s[i];

    te->teLength = (int16_t)(te->teLength + len);
    te->selStart = (int16_t)(pos + len);
    te->selEnd   = (int16_t)(pos + len);
}

/* ===========================================================================
 * THE EDITING SURFACE (bead initech-tdnl.16) -- see textedit.h for the rules.
 * ===========================================================================*/
static int te_line_h(const FlairTE *te)
{
    return text_cell_height(te_font(te));
}

/* The line holding boundary `off`: the LAST line whose start is <= off (a
 * wrap boundary belongs to the line it starts). */
static int te_line_of(const FlairTE *te, int off)
{
    int L = 0;
    while (L + 1 < te->nLines && te->lineStarts[L + 1] <= off)
        L++;
    return L;
}

/* The last boundary the caret can take on line L: before its CR, or the next
 * line's start for a wrapped line, or teLength on the last line. */
static int te_line_reach(const FlairTE *te, int L)
{
    int e;
    if (L + 1 >= te->nLines)
        return te->teLength;
    e = te->lineStarts[L + 1];
#ifndef TE_MUT_HIT_PAST_CR
    if (e > 0 && te->text[e - 1] == TE_CH_CR)
        e--;                               /* the caret stops before the CR */
#endif
    return e;
}

static int te_clamp_off(const FlairTE *te, int off)
{
    if (off < 0) return 0;
    if (off > te->teLength) return te->teLength;
    return off;
}

static int te_offset_at(const FlairTE *te, int h, int v)
{
    int lh = te_line_h(te);
    int L = (v < 0) ? 0 : v / lh;
    int i, e, x = 0;
    if (L >= te->nLines) L = te->nLines - 1;
    e = te_line_reach(te, L);
    for (i = te->lineStarts[L]; i < e; i++) {
        int w = te_char_w(te, te->text[i]);
#ifdef TE_MUT_HIT_FLOOR
        /* MUTANT (Rule 6): the char under the point, never the NEAREST
         * boundary -- a click on a char's right half lands before it. */
        if (h < x + w)
#else
        if (2 * h < 2 * x + w)             /* nearer the left boundary */
#endif
            return i;
        x += w;
    }
    return e;
}

static void te_arrow(FlairTE *te, char ch)
{
    int a = te->selStart, b = te->selEnd, off, L;
    int16_t h, v;

    if (ch == FLAIR_TE_CH_LEFT) {
#ifdef TE_MUT_ARROW_NO_COLLAPSE
        /* MUTANT (Rule 6): a range is not collapsed -- the caret steps left of
         * its start as if it were an insertion point. */
        off = a - 1;
#else
        off = (a < b) ? a : a - 1;
#endif
    } else if (ch == FLAIR_TE_CH_RIGHT) {
        off = (a < b) ? b : b + 1;
    } else {
        int from = (ch == FLAIR_TE_CH_UP) ? a : b;
        FlairTEGetPoint(te, (int16_t)from, &h, &v);
        L = te_line_of(te, from);
        if (ch == FLAIR_TE_CH_UP)
            off = (L == 0) ? 0 : te_offset_at(te, h, v - te_line_h(te));
        else
            off = (L + 1 >= te->nLines) ? te->teLength
                                        : te_offset_at(te, h, v + te_line_h(te));
    }
    off = te_clamp_off(te, off);
    te->selStart = (int16_t)off;
    te->selEnd   = (int16_t)off;
}

int16_t FlairTELineHeight(const FlairTE *te)
{
    if (te == NULL)
        TE_PANIC("FlairTELineHeight: NULL te (contract error)");
    return (int16_t)te_line_h(te);
}

void FlairTEGetPoint(const FlairTE *te, int16_t offset, int16_t *h, int16_t *v)
{
    int off, L, i, x = 0;
    if (te == NULL || h == NULL || v == NULL)
        TE_PANIC("FlairTEGetPoint: NULL argument (contract error)");
    off = te_clamp_off(te, offset);
    L = te_line_of(te, off);
    for (i = te->lineStarts[L]; i < off; i++)
        x += te_char_w(te, te->text[i]);
    *h = (int16_t)x;
    *v = (int16_t)(L * te_line_h(te));
}

int16_t FlairTEGetOffset(const FlairTE *te, int16_t h, int16_t v)
{
    if (te == NULL)
        TE_PANIC("FlairTEGetOffset: NULL te (contract error)");
    return (int16_t)te_offset_at(te, h, v);
}

int16_t FlairTEClick(FlairTE *te, int16_t h, int16_t v, int extend)
{
    int off, anchor;
    if (te == NULL)
        TE_PANIC("FlairTEClick: NULL te (contract error)");
    off = te_offset_at(te, h, v);
    if (!extend) {
        anchor = off;
    } else {
#ifdef TE_MUT_EXTEND_ANCHOR
        /* MUTANT (Rule 6): the anchor is always selStart -- a shift-click
         * before the selection throws its end away. */
        anchor = te->selStart;
#else
        /* keep the end FARTHER from the point */
        int da = off - te->selStart, db = te->selEnd - off;
        if (da < 0) da = -da;
        if (db < 0) db = -db;
        anchor = (da >= db) ? te->selStart : te->selEnd;
#endif
    }
    FlairTESetSelect(te, (int16_t)anchor, (int16_t)off);
    return (int16_t)anchor;
}

int FlairTECanReplace(const FlairTE *te, int16_t a, int16_t b, const char *s,
                      int16_t len)
{
    int x = a, y = b, n;
    te_view_t view;
    if (te == NULL || (s == NULL && len > 0) || len < 0)
        TE_PANIC("FlairTECanReplace: bad argument (contract error)");
    x = te_clamp_off(te, x);
    y = te_clamp_off(te, y);
    if (x > y) { int t = x; x = y; y = t; }
    if ((int)te->teLength - (y - x) + (int)len > FLAIR_TE_TEXT_MAX)
        return 0;
    view.a = te->text;      view.na = x;
    view.b = s;             view.nb = len;
    view.c = te->text + y;  view.nc = te->teLength - y;
#ifdef TE_MUT_PROBE_NO_LINES
    /* MUTANT (Rule 6): the probe checks the length only -- an edit that needs
     * a line past FLAIR_TE_MAX_LINES is let through (and the edit panics). */
    (void)view; (void)n;
    return 1;
#else
    n = te_layout(te, &view, NULL, FLAIR_TE_MAX_LINES);
    return n >= 0;
#endif
}

/* ===========================================================================
 * Public API
 * ===========================================================================*/

void FlairTE_init(FlairTE *te, int16_t wrapWidth, int16_t just)
{
    int i;
    if (te == NULL)
        TE_PANIC("FlairTE_init: NULL te (contract error)");
    for (i = 0; i < FLAIR_TE_TEXT_MAX; i++)
        te->text[i] = 0;                 /* deterministic empty buffer (Rule 11) */
    te->selStart  = 0;
    te->selEnd    = 0;
    te->teLength  = 0;
    te->just      = just;
    te->crOnly    = 0;                    /* word-wrap enabled by default          */
    te->wrapWidth = wrapWidth;
    te->txFont    = 0;                    /* systemFont: Chicago 12 (grafport.h)    */
    te->txFace    = 0;                    /* plain                                  */
    te->txSize    = 0;                    /* the family's default strike            */
    te->nLines    = 0;
    for (i = 0; i < FLAIR_TE_MAX_LINES; i++)
        te->lineStarts[i] = 0;
    te_recompute_lines(te);              /* empty -> nLines=1, lineStarts[0]=0     */
}

void FlairTESetText(FlairTE *te, const char *s, int16_t len)
{
    int i;
    if (te == NULL || (s == NULL && len > 0))
        TE_PANIC("FlairTESetText: NULL argument (contract error)");
    /* Rule 2: fail loud on a bad length -- NEVER silently truncate. */
    if (len < 0 || (int)len > FLAIR_TE_TEXT_MAX)
        TE_PANIC("FlairTESetText: len out of [0, FLAIR_TE_TEXT_MAX]");

    for (i = 0; i < len; i++)
        te->text[i] = s[i];
    te->teLength = len;

    /* Clamp the selection into the new [0, teLength]. */
    if (te->selStart > len) te->selStart = len;
    if (te->selEnd   > len) te->selEnd   = len;
    if (te->selStart < 0)   te->selStart = 0;
    if (te->selEnd   < 0)   te->selEnd   = 0;
    if (te->selStart > te->selEnd) te->selStart = te->selEnd;

    te_recompute_lines(te);
}

int16_t FlairTEGetText(const FlairTE *te, char *buf, int16_t bufsz)
{
    int i;
    if (te == NULL || buf == NULL)
        TE_PANIC("FlairTEGetText: NULL argument (contract error)");
    /* Rule 2: fail loud if the caller's buffer is too small -- NO partial copy. */
    if (bufsz < te->teLength)
        TE_PANIC("FlairTEGetText: bufsz < teLength (fail loud, no partial copy)");
    for (i = 0; i < te->teLength; i++)
        buf[i] = te->text[i];
    return te->teLength;
}

void FlairTESetSelect(FlairTE *te, int16_t selStart, int16_t selEnd)
{
    int16_t a = selStart;
    int16_t b = selEnd;
    if (te == NULL)
        TE_PANIC("FlairTESetSelect: NULL te (contract error)");

    /* Clamp both ends to [0, teLength] (textedit.md Sec 2). */
    if (a < 0) a = 0;
    if (a > te->teLength) a = te->teLength;
    if (b < 0) b = 0;
    if (b > te->teLength) b = te->teLength;
    /* Keep selStart <= selEnd. */
    if (a > b) {
        int16_t t = a; a = b; b = t;
    }
    te->selStart = a;
    te->selEnd   = b;
}

void FlairTEKey(FlairTE *te, char ch)
{
    if (te == NULL)
        TE_PANIC("FlairTEKey: NULL te (contract error)");

    if (ch >= FLAIR_TE_CH_LEFT && ch <= FLAIR_TE_CH_DOWN) {
        te_arrow(te, ch);                  /* moves the selection, no edit */
        return;
    }
    if (ch == TE_CH_BS) {
        if (te->selStart < te->selEnd) {
            te_delete_range(te, te->selStart, te->selEnd);     /* delete range  */
        } else if (te->selStart > 0) {
            te_delete_range(te, (int16_t)(te->selStart - 1), te->selStart); /* del prev char */
        }
        /* else: caret at 0, nothing to delete */
        te_recompute_lines(te);
        return;
    }

    /* Normal char: delete any range first, then insert at the insertion point. */
#ifndef TE_MUT_PASTE_NO_DELETE
    if (te->selStart < te->selEnd)
        te_delete_range(te, te->selStart, te->selEnd);
#endif
    te_insert_at(te, te->selStart, &ch, 1);
    te_recompute_lines(te);
}

void FlairTEInsert(FlairTE *te, const char *s, int16_t len)
{
    if (te == NULL || (s == NULL && len > 0))
        TE_PANIC("FlairTEInsert: NULL argument (contract error)");
#ifndef TE_MUT_PASTE_NO_DELETE
    if (te->selStart < te->selEnd)
        te_delete_range(te, te->selStart, te->selEnd);
#endif
    te_insert_at(te, te->selStart, s, len);
    te_recompute_lines(te);
}

void FlairTEDelete(FlairTE *te)
{
    if (te == NULL)
        TE_PANIC("FlairTEDelete: NULL te (contract error)");
    if (te->selStart < te->selEnd) {
        te_delete_range(te, te->selStart, te->selEnd);
        te_recompute_lines(te);
    }
    /* empty insertion point -> no-op (textedit.md Sec 6) */
}

int FlairTECopy(const FlairTE *te, FlairScrap *scrap)
{
    int16_t n;
    int32_t r;
    if (te == NULL || scrap == NULL)
        TE_PANIC("FlairTECopy: NULL argument (contract error)");

    /* The selStart <= selEnd invariant holds; the selected run is half-open. */
    n = (int16_t)(te->selEnd - te->selStart);
    if (n < 0)
        n = 0;

    /* Route through the shell-owned Scrap -- this is the shared clipboard seam. */
    r = FlairPutScrap(scrap, FLAIR_SCRAP_TYPE_TEXT,
                      te->text + te->selStart, (uint32_t)n);
    return (r == 0) ? 0 : (int)r;   /* propagate a fail-loud Scrap error */
}

int FlairTECut(FlairTE *te, FlairScrap *scrap)
{
    int r;
    if (te == NULL || scrap == NULL)
        TE_PANIC("FlairTECut: NULL argument (contract error)");
    r = FlairTECopy(te, scrap);
    if (r != 0)
        return r;                /* copy failed -> do NOT delete */
    FlairTEDelete(te);
    return 0;
}

int FlairTEPaste(FlairTE *te, const FlairScrap *scrap)
{
    /* The paste staging buffer is the text-buffer size: a TEXT flavor larger
     * than the FlairTE can ever hold is a fail-loud condition (Rule 2). */
    char     tmp[FLAIR_TE_TEXT_MAX];
    uint32_t got = 0;
    int32_t  r;

    if (te == NULL || scrap == NULL)
        TE_PANIC("FlairTEPaste: NULL argument (contract error)");

    r = FlairGetScrap(scrap, FLAIR_SCRAP_TYPE_TEXT, tmp, (uint32_t)sizeof(tmp), &got);
    if (r == -1)
        return -1;               /* no TEXT flavor in the scrap */
    if (r < 0)
        TE_PANIC("FlairTEPaste: TEXT flavor exceeds the FlairTE text buffer");

    /* Insert the pasted bytes at the selection (replacing any range). Reusing
     * FlairTEInsert keeps the TE_MUT_PASTE_NO_DELETE guard in one place. */
    FlairTEInsert(te, tmp, (int16_t)got);
    return 0;
}
