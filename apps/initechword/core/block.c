/*
 * os/iword/core/block.c -- InitechWord's Block (Alt-F4) and Move (Ctrl-F4):
 * block copy, cut and retrieve over the piece table (THE ARTIFACT; bead
 * initech-jojk, IWORD P2 slice 2).
 *
 * THE RULES are the real WP.EXE's (5.1, 03-09-92), minted for this slice
 * under DOSBox-X (../wordperfect51-decomp tools/mint_drive.sh, mint/
 * wpMINT12..15.steps; the documents mint/work/WP51/MINT12..15, the screens
 * mint/shots_wpMINT12..15; ledger re/mint-results-003.md):
 *   THE BLOCK runs from the anchor (where Alt-F4 was pressed) to the cursor,
 *     [min, max): MINT12 "Alpha Beta Gamma." Left x11 Alt-F4 Right x5 blocks
 *     "Beta " (02_block_hl). Codes are cursor stops like characters (MINT13:
 *     End, Left x9 from "...words[bold] after." lands before "s").
 *   COPY (iw_ed_block_copy) is SELF-CONTAINED: an on code for every
 *     attribute on where the block starts, the block's bytes, an off code for
 *     every attribute on where it ends. MINT14 "RS" blocked inside
 *     "[BOLD]RS[bold]" and copied came back "[BOLD]RS[bold]"; MINT13
 *     "ep[UND]u" came back "ep[UND]u[und]"; MINT13 "s[bold] a" moved came
 *     back "[BOLD]s[bold] a". Several attributes: ascending attribute number
 *     [authored order; every mint has one].
 *   CUT (Move, Delete Block) leaves every other character in its
 *     attributes: the block goes, and where it was go the codes that change
 *     between its two ends, the cursor after them (MINT13 03c_deleted_reveal
 *     "Dr[BOLD]|old here[bold]", 04b_deleted_reveal "her[und]|nd.", 01c
 *     "word[bold]fter.").
 *   RETRIEVE (Enter at "Move cursor; press Enter to retrieve.") inserts the
 *     copy at the cursor and the cursor STAYS in front of it (MINT12 06_moved
 *     Pos 2.2" -- "Beta " went in after "Gamma." and the next Return split
 *     before it; 08_copied Pos 2.3"); then every code that changes nothing
 *     from there on is dropped -- an on code of an attribute already on, an
 *     off code of one already off. MINT14: plain "xy" retrieved into bold
 *     "C|DEF" is bold ("CxyDEF", no code added); "[BOLD]x[bold]" into
 *     "[BOLD]vwxyz|[bold]" -> "[BOLD]vwxyzx[bold]"; MINT15: "[BOLD]w[bold]"
 *     into "[BOLD]vwx|yz[bold]" -> "[BOLD]vwxw[bold]yz" (the inserted
 *     [bold] ends the bold, the old one is dropped).
 * Every result keeps the attribute codes alternating on, off per attribute
 * (test-iword-codes property C3).
 *
 * Freestanding C (no libc). Deterministic (Rule 11). ASCII-clean (Rule 12).
 */
#include "iword.h"
#include "iword/wp5_format.h"

#define NONE 0xFFFFFFFFu

static uint32_t abit(uint32_t n) { return n < 32u ? 1u << n : 0u; }

void iw_ed_block(iw_ed_t *e)
{
    e->blk = e->blk == IW_NO_BLOCK ? e->cur : IW_NO_BLOCK;
}

void iw_ed_block_span(const iw_ed_t *e, uint32_t *s, uint32_t *t)
{
    uint32_t a = e->blk == IW_NO_BLOCK ? e->cur : e->blk;
    *s = a < e->cur ? a : e->cur;
    *t = a < e->cur ? e->cur : a;
}

/* Append the attribute codes of `bits` (C3 for on, C4 for off), ascending. */
static uint32_t codes(uint8_t *out, uint32_t cap, uint32_t n, uint32_t bits, uint8_t c)
{
    for (uint32_t a = 0; a < IW_WP5_ATTR_NUMBERS; a++) {
        if (!(bits & abit(a))) continue;
        if (out && n + IW_WP5_ATTR_LEN <= cap) {
            out[n] = c;
            out[n + 1u] = (uint8_t)a;
            out[n + 2u] = c;
        }
        n += IW_WP5_ATTR_LEN;
    }
    return n;
}

int iw_ed_block_copy(iw_ed_t *e, uint8_t *buf, uint32_t cap, uint32_t *n)
{
    uint32_t s, t, k, on_s, on_t;
    iw_ed_block_span(e, &s, &t);
    on_s = iw_attr_state(&e->doc, s);
    on_t = iw_attr_state(&e->doc, t);
#ifdef IWORD_MUT_COPY_BARE
    /* MUTANT (Rule 6; test-iword-codes-mutant): the copy is the block's
     * bytes alone, not self-contained. NEVER real. */
    on_s = on_t = 0;
#endif
    k = codes(0, 0, 0, on_s, IW_WP5_ATTR_ON) + (t - s) + codes(0, 0, 0, on_t, IW_WP5_ATTR_OFF);
    *n = 0;
    if (k > cap) return IW_E_FULL;
    k = codes(buf, cap, 0, on_s, IW_WP5_ATTR_ON);
    k += iw_doc_read(&e->doc, s, buf + k, t - s);
    k = codes(buf, cap, k, on_t, IW_WP5_ATTR_OFF);
    *n = k;
    e->blk = IW_NO_BLOCK;
    return IW_OK;
}

int iw_ed_block_cut(iw_ed_t *e, uint8_t *buf, uint32_t cap, uint32_t *n)
{
    iw_doc_t *d = &e->doc;
    uint32_t s, t, on_s, on_t, diff, k = 0;
    uint8_t mend[3u * IW_WP5_ATTR_NUMBERS];
    int rc;
    iw_ed_block_span(e, &s, &t);
    if (n) *n = 0;
    if (buf) {
        uint32_t blk = e->blk;
        rc = iw_ed_block_copy(e, buf, cap, n);
        e->blk = blk;
        if (rc != IW_OK) return rc;
    }
    on_s = iw_attr_state(d, s);
    on_t = iw_attr_state(d, t);
    diff = on_s ^ on_t;
#ifdef IWORD_MUT_CUT_NO_MEND
    /* MUTANT (Rule 6; test-iword-codes-mutant): the cut leaves no codes where
     * the block was -- the text after it changes its attributes. NEVER real. */
    diff = 0;
#endif
    k = codes(mend, sizeof mend, 0, diff & on_t, IW_WP5_ATTR_ON);
    k = codes(mend, sizeof mend, k, diff & ~on_t, IW_WP5_ATTR_OFF);
    /* all or nothing: a delete (one split) and an insert (two) */
    if (d->npc + 3u > d->pc_cap || d->add_len + k > d->add_cap) return IW_E_FULL;
    rc = iw_doc_delete(d, s, t - s);
    if (rc == IW_OK && k) rc = iw_doc_insert(d, s, mend, k);
    if (rc != IW_OK) return rc;
    e->cur = s + k;
    e->blk = IW_NO_BLOCK;
    e->goal = NONE;
    e->dirty = 1;
    return iw_reflow_para(d, s, e->width);
}

int iw_ed_retrieve(iw_ed_t *e, const uint8_t *buf, uint32_t n)
{
    iw_doc_t *d = &e->doc;
    uint32_t at = e->cur, p, end, st;
    int rc;
    e->goal = NONE;
    e->blk = IW_NO_BLOCK;
    if (n == 0u) return IW_OK;
    if (d->npc + 2u > d->pc_cap) return IW_E_FULL;
    rc = iw_doc_insert(d, at, buf, n);
    if (rc != IW_OK) return rc;
    e->dirty = 1;
    end = at + n;
    /* drop every code that changes nothing from the insertion on */
    st = iw_attr_state(d, at);
    p = at;
    while (p < d->len) {
        int on, a = iw_code_attr(d, p, &on);
        uint32_t q = iw_unit_next(d, p);
#ifndef IWORD_MUT_RETRIEVE_DUP
        /* MUTANT knob (Rule 6; test-iword-codes-mutant): IWORD_MUT_RETRIEVE_DUP
         * keeps the redundant codes -- a doubled [BOLD], a stray [bold]. */
        if (a >= 0 && (on ? (st & abit((uint32_t)a)) != 0u : (st & abit((uint32_t)a)) == 0u)) {
            if (d->npc + 1u > d->pc_cap) return IW_E_FULL;
            rc = iw_doc_delete(d, p, q - p);
            if (rc != IW_OK) return rc;
            if (end > p) end = end >= q ? end - (q - p) : p;
            continue;
        }
#endif
        if (a >= 0) st = on ? (st | abit((uint32_t)a)) : (st & ~abit((uint32_t)a));
        p = q;
    }
    /* re-wrap every paragraph the copy touches; the cursor stays in front */
    p = at;
    for (;;) {
        rc = iw_reflow_para(d, p, e->width);
        if (rc != IW_OK) return rc;
        while (p < d->len && iw_unit_kind(d, p) != IW_U_HRT && iw_unit_kind(d, p) != IW_U_HPG) p = iw_unit_next(d, p);
        if (p >= d->len || p >= end) break;
        p = iw_unit_next(d, p);
    }
    e->cur = at;
    return IW_OK;
}

/* ---- an attribute given to the block (bead initech-jojk, P2 slice 3) ---------- *
 * F6 / F8 / Ctrl-F8 1 n (Size) / Ctrl-F8 2 n (Appearance) with Block on: the
 * on code at the block's start, the off code at its end, Block off
 * (Reference Manual, Attributes p. 37 "Existing Text": "block the text ...
 * then select the attribute"; minted mint/wpMINT28.steps). The cursor keeps
 * its end of the block: at the end it rests after the off code (the manual,
 * "the cursor is placed after the attribute off code"; MINT28 03_bolded_reveal
 * "[BOLD]ghi[bold]|"), at the start after the on code (05/06 the block
 * "kl" made with Left from its end: "j[UND]|kl[und]"). The codes go in even
 * when other codes lie inside the block (MINT28 16_overlap_reveal
 * "ab[BOLD]c[large] [ITALC]def[bold]|[italc]") [an attribute already on at
 * either end is not minted: the pair goes in all the same -- inferred]. */
int iw_ed_block_attr(iw_ed_t *e, uint8_t attr)
{
    iw_doc_t *d = &e->doc;
    uint8_t on[IW_WP5_ATTR_LEN] = { IW_WP5_ATTR_ON, 0, IW_WP5_ATTR_ON };
    uint8_t off[IW_WP5_ATTR_LEN] = { IW_WP5_ATTR_OFF, 0, IW_WP5_ATTR_OFF };
    uint32_t s, t;
    int at_end, rc;
    if (e->blk == IW_NO_BLOCK) return iw_ed_attr(e, attr);
    if (attr >= IW_WP5_ATTR_NUMBERS) return IW_E_RANGE;
    on[1] = off[1] = attr;
    iw_ed_block_span(e, &s, &t);
    at_end = e->cur == t && t != s;
    /* all or nothing: two inserts, two splits each at most */
    if (d->npc + 4u > d->pc_cap || d->add_len + 2u * IW_WP5_ATTR_LEN > d->add_cap ||
        d->len + 2u * IW_WP5_ATTR_LEN > d->max_len) return IW_E_FULL;
    rc = iw_doc_insert(d, t, off, sizeof off);
    if (rc == IW_OK) rc = iw_doc_insert(d, s, on, sizeof on);
    if (rc != IW_OK) return rc;
#ifdef IWORD_MUT_BLK_CUR_START
    /* MUTANT (Rule 6; test-iword-codes-mutant): the cursor always goes
     * after the off code, whichever end of the block it was at. NEVER real. */
    at_end = 1;
#endif
    e->cur = at_end ? t + 2u * IW_WP5_ATTR_LEN : s + IW_WP5_ATTR_LEN;
    e->blk = IW_NO_BLOCK;
    e->goal = NONE;
    e->dirty = 1;
    return IW_OK;
}
