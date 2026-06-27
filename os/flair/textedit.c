/*
 * os/flair/textedit.c -- FLAIR TextEdit (TERec) implementation -- THE ARTIFACT.
 *
 * beads: initech-77dj (FLAIR Phase 4.5 -- the TextEdit half). REDUCED first cut
 *        (committee ruling wf_00931e9e); see textedit.h for the full contract +
 *        the deferred-full-TERec note.
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
 *
 * ERA CEILING (Law 3): monostyled, Mac TextEdit model only -- no Win32 edit
 *   control names, no styled (txSize==-1) runs (textedit.md Sec 5, out of scope).
 *
 * FREESTANDING (Law 3): <stdint.h> (via textedit.h) + <stddef.h> (NULL) only.
 *   No string.h / libc / malloc. The in-place insert/delete shifts use the
 *   overlap-safe te_memmove helper below (forward when dst<src, backward else).
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#include <stddef.h>   /* NULL -- freestanding-safe (C11 S4p6 required header) */
#include "textedit.h"

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
 * te_emit_line -- record a line start at char offset `start`. Fail loud (Rule 2)
 * if the line count would exceed the capped lineStarts array.
 * ------------------------------------------------------------------------- */
static void te_emit_line(FlairTE *te, int16_t start)
{
    if (te->nLines >= FLAIR_TE_MAX_LINES)
        TE_PANIC("FlairTE: line count exceeds FLAIR_TE_MAX_LINES (reduced cut)");
    te->lineStarts[te->nLines] = start;
    te->nLines++;
}

/* ---------------------------------------------------------------------------
 * te_recompute_lines -- rebuild nLines + lineStarts after a mutation
 * (textedit.md Sec 4). Line 0 always starts at offset 0. A CR (0x0D) terminates
 * the current line; the next line begins PAST the CR. When word-wrap is active
 * (crOnly >= 0 AND wrapWidth > 0) a line that would exceed wrapWidth chars breaks
 * at the last space at/before the boundary (the space stays on the line); a word
 * longer than wrapWidth hard-breaks at the boundary. crOnly < 0 == Return-only,
 * no word wrap (the line may run past the width) -- textedit.md Sec 4 step 1.
 *
 * REDUCED MODEL (Law 1 honesty): FLAIR's Chicago is a fixed cell, so wrapWidth is
 * counted in CHARS (monospaced) where real TextEdit measures proportional pixels
 * from the owTable -- a documented FLAIR deviation, NOT a TextEdit semantic
 * (textedit.md Sec 4 final paragraph). lineStarts holds N == nLines entries (each
 * line's START), not the IM nLines+1 sentinel form -- the capped first cut.
 * ------------------------------------------------------------------------- */
static void te_recompute_lines(FlairTE *te)
{
    int16_t s;            /* start offset of the current line                  */
    int16_t last_space;   /* last space offset seen on the current line (>= s) */
    int     wrap_on;
    int     i;

    wrap_on = (te->crOnly >= 0) && (te->wrapWidth > 0);

    te->nLines = 0;
    te_emit_line(te, 0);     /* line 0 always starts at offset 0 */
    s = 0;
    last_space = -1;
    i = 0;

    while (i < te->teLength) {
        char ch = te->text[i];

        if (ch == TE_CH_CR) {
#ifdef TE_MUT_LINEBREAK_OFF
            /* MUTANT (Rule 6): record the CR offset instead of the offset PAST
             * it -- an off-by-one lineStarts. Step 9 (CR test) -> RED. */
            te_emit_line(te, (int16_t)i);
#else
            te_emit_line(te, (int16_t)(i + 1));   /* next line starts past the CR */
#endif
            s = (int16_t)(i + 1);
            last_space = -1;
            i++;
            continue;
        }

        if (wrap_on && (i - (int)s) >= (int)te->wrapWidth) {
            int16_t brk;
            if (last_space >= s)
                brk = (int16_t)(last_space + 1);  /* soft break: space ends line */
            else
                brk = (int16_t)i;                 /* hard break: word > wrapWidth */
            te_emit_line(te, brk);
            s = brk;
            last_space = -1;
            continue;   /* re-examine text[i] on the new line (no advance) */
        }

        if (ch == TE_CH_SP)
            last_space = (int16_t)i;
        i++;
    }
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
