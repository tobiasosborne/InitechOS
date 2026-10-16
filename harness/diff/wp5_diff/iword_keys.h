/*
 * harness/diff/wp5_diff/iword_keys.h -- drive the InitechWord core with the
 * keys of a ../wordperfect51-decomp mint step file (factory C; bead
 * initech-jojk, IWORD P2 slice 2). Shared by test_iword_wp5_roundtrip.c
 * (the typed documents == the files WP.EXE saved) and
 * test_iword_reveal_codes.c (the Reveal Codes screens taken part-way).
 *
 * A script is a NULL-terminated array of strings: plain text is typed; a
 * string starting with '%' is a key, optionally "*N" repeated:
 *   %RET   Return (Enter at "Move cursor; press Enter to retrieve." retrieves)
 *   %TAB %F4 %CRET          Tab, F4 ->Indent, Ctrl-Enter hard page
 *   %BS %DEL                Backspace, Delete (Block on: "Delete Block?")
 *   %Y %N                   the answer to the pending question
 *   %L %R %U %D             the arrows
 *   %HOME                   WP's Home, Left (the steps' "key Home" "key Left")
 *   %END                    End
 *   %TOP                    Home, Home, Up: the top of the document
 *   %BOT                    Home, Home, Down: the end of the document
 *   %F6 %F8 %ITAL %REV      Bold, Underline, Ctrl-F8 2 4, Alt-F3
 *   %BLK                    Alt-F4
 *   %MOVE %COPY %CUT        Ctrl-F4 1 then 1 Move / 2 Copy / 3 Delete
 *   %MAR=L,R                Shift-F8 1 7 L Enter R Enter (WPU: 1.3" = 1560)
 *   %JL %JC %JR %JF         Shift-F8 1 3 1 / 2 / 3 / 4 (Left Center Right Full)
 *                           (slice 3, mint/wpMINT21..24.steps)
 *   (slice 3, mint/wpMINT25..31.steps:)
 *   %SP=H                   Shift-F8 1 6, the spacing typed (hundredths: 1.33 =
 *                           133), Enter, F7
 *   %HHHL %HR               Home, Home, Home, Left; Home, Right
 *   %TOP3                   Home, Home, Home, Up (%TOP is Home, Home, Up)
 *   %HUP %HDN               Home, Up / Home, Down on a document that fits one
 *                           screen: its first / last line
 *   %SZ=n %AP=n             Ctrl-F8 1 n (Size) / Ctrl-F8 2 n (Appearance); F6,
 *                           F8, %ITAL and these give the attribute to the
 *                           block when Block is on
 *   %SENT %PARA %PAGE       Ctrl-F4 1 / 2 / 3 with Block off (then %MOVE /
 *                           %COPY / %CUT act on what they highlight)
 *   %F1                     F1 Cancel at that menu (Block off, the cursor kept)
 *   %RETB                   Ctrl-F4 4 1: the last moved / copied text again
 * iw_ed_fixup runs after every key, as WORD.EXE runs it. Returns 1 when every
 * key was accepted the way the mint saw it (a question asked where WP asked).
 *
 * ASCII-clean (Rule 12). Deterministic (Rule 11).
 */
#ifndef IWORD_KEYS_H
#define IWORD_KEYS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iword.h"

#define IWK_BUF 8192u
static uint8_t  iwk_buf[IWK_BUF];
static uint32_t iwk_n;
static int      iwk_pending, iwk_ask, iwk_blkdel;

static int iwk_key(iw_ed_t *e, const char *k)
{
    int rc = IW_OK;
    if (!strcmp(k, "RET")) {
        if (iwk_pending) { rc = iw_ed_retrieve(e, iwk_buf, iwk_n); iwk_pending = 0; }
        else rc = iw_ed_enter(e);
    } else if (!strcmp(k, "TAB")) rc = iw_ed_tab(e);
    else if (!strcmp(k, "F4")) rc = iw_ed_indent(e);
    else if (!strcmp(k, "CRET")) rc = iw_ed_hpage(e);
    else if (!strcmp(k, "BS") || !strcmp(k, "DEL")) {
        if (e->blk != IW_NO_BLOCK) { iwk_blkdel = 1; return 1; }
        rc = !strcmp(k, "BS") ? iw_ed_backspace(e) : iw_ed_delete(e);
        if (rc == IW_E_ASK) { iwk_ask = 1; rc = IW_OK; }
        else if (rc == IW_E_PROTECTED) rc = IW_OK;
    } else if (!strcmp(k, "Y") || !strcmp(k, "N")) {
        int yes = k[0] == 'Y';
        if (iwk_blkdel) { iwk_blkdel = 0; rc = yes ? iw_ed_block_cut(e, 0, 0, 0) : IW_OK; }
        else if (iwk_ask) { iwk_ask = 0; rc = iw_ed_answer(e, yes); }
        else return 0;                       /* no question was asked */
    } else if (!strcmp(k, "L")) iw_ed_left(e);
    else if (!strcmp(k, "R")) iw_ed_right(e);
    else if (!strcmp(k, "U")) iw_ed_up(e);
    else if (!strcmp(k, "D")) iw_ed_down(e);
    else if (!strcmp(k, "HOME")) iw_ed_line_home(e);
    else if (!strcmp(k, "END") || !strcmp(k, "HR")) iw_ed_line_end(e);
    else if (!strcmp(k, "HHHL")) iw_ed_line_start(e);
    else if (!strcmp(k, "TOP")) iw_ed_doc_top(e, 0);
    else if (!strcmp(k, "TOP3")) iw_ed_doc_top(e, 1);
    else if (!strcmp(k, "BOT")) iw_ed_doc_end(e);
    else if (!strcmp(k, "HUP")) iw_ed_to_line(e, 0);
    else if (!strcmp(k, "HDN")) iw_ed_to_line(e, 0xFFFFFFFFu);
    else if (!strcmp(k, "F6")) rc = iw_ed_block_attr(e, 0x0C);
    else if (!strcmp(k, "F8")) rc = iw_ed_block_attr(e, 0x0E);
    else if (!strcmp(k, "ITAL")) rc = iw_ed_block_attr(e, 0x08);
    else if (!strncmp(k, "SZ=", 3) || !strncmp(k, "AP=", 3)) {
        static const uint8_t SZ[7] = { 0x05, 0x06, 0x04, 0x03, 0x02, 0x01, 0x00 };
        static const uint8_t AP[9] = { 0x0C, 0x0E, 0x0B, 0x08, 0x07, 0x09, 0x0F, 0x0A, 0x0D };
        int n = atoi(k + 3);
        if (k[0] == 'S' ? (n < 1 || n > 7) : (n < 1 || n > 9)) return 0;
        rc = iw_ed_block_attr(e, k[0] == 'S' ? SZ[n - 1] : AP[n - 1]);
    }
    else if (!strncmp(k, "SP=", 3)) rc = iw_ed_spacing(e, iw_spacing_from_hundredths((uint32_t)atoi(k + 3)));
    else if (!strcmp(k, "SENT")) rc = iw_ed_select(e, 1);
    else if (!strcmp(k, "PARA")) rc = iw_ed_select(e, 2);
    else if (!strcmp(k, "PAGE")) rc = iw_ed_select(e, 3);
    else if (!strcmp(k, "F1")) e->blk = IW_NO_BLOCK;
    else if (!strcmp(k, "RETB")) rc = iw_ed_retrieve(e, iwk_buf, iwk_n);
    else if (!strcmp(k, "REV")) e->reveal = (uint8_t)!e->reveal;
    else if (!strcmp(k, "BLK")) iw_ed_block(e);
    else if (!strcmp(k, "MOVE")) { rc = iw_ed_block_cut(e, iwk_buf, IWK_BUF, &iwk_n); iwk_pending = 1; }
    else if (!strcmp(k, "COPY")) { rc = iw_ed_block_copy(e, iwk_buf, IWK_BUF, &iwk_n); iwk_pending = 1; }
    else if (!strcmp(k, "CUT")) rc = iw_ed_block_cut(e, 0, 0, 0);
    else if (!strncmp(k, "MAR=", 4)) {
        unsigned l = 0, r = 0;
        if (sscanf(k + 4, "%u,%u", &l, &r) != 2) return 0;
        rc = iw_ed_margins(e, l, r);
    }
    else if (!strcmp(k, "JL")) rc = iw_ed_just(e, 0);
    else if (!strcmp(k, "JF")) rc = iw_ed_just(e, 1);
    else if (!strcmp(k, "JC")) rc = iw_ed_just(e, 2);
    else if (!strcmp(k, "JR")) rc = iw_ed_just(e, 3);
    else return 0;
    if (rc != IW_OK) return 0;
    return iw_ed_fixup(e) == IW_OK;
}

static int iwk_run(iw_ed_t *e, const char *const *script)
{
    iwk_pending = iwk_ask = iwk_blkdel = 0;
    iwk_n = 0;
    for (; *script; script++) {
        const char *s = *script;
        if (s[0] != '%') {
            for (; *s; s++) if (iw_ed_type(e, (uint8_t)*s) != IW_OK || iw_ed_fixup(e) != IW_OK) return 0;
            continue;
        }
        {
            char k[16];
            int times = 1;
            const char *star = strchr(s + 1, '*');
            size_t len = star ? (size_t)(star - (s + 1)) : strlen(s + 1);
            if (len >= sizeof k) return 0;
            memcpy(k, s + 1, len);
            k[len] = 0;
            if (star) times = atoi(star + 1);
            while (times-- > 0) if (!iwk_key(e, k)) return 0;
        }
    }
    return 1;
}

#endif /* IWORD_KEYS_H */
