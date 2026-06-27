/*
 * test_textedit.c -- the FLAIR TextEdit (TERec) host oracle (initech-77dj).
 *
 * beads: initech-77dj (FLAIR Phase 4.5 -- the TextEdit half of the shared
 *        editable-text engine; REDUCED first cut, committee ruling wf_00931e9e).
 * Ref:   textedit.h / textedit.c (the API under test); scrap.h / scrap.c (the
 *        shared clipboard the cut/copy/paste route through);
 *        ../system7-decomp/specs/toolbox/textedit.md (Sec 2 half-open selection,
 *        Sec 4 line layout, Sec 6 routine semantics).
 *        CLAUDE.md Law 2 (the oracle is the truth -- graded against an INDEPENDENT
 *        golden, NEVER by-construction), Rule 1 (Red->Green->Refactor), Rule 6
 *        (mutation-proven), Rule 12 (ASCII-clean).
 *
 * ANTI-BY-CONSTRUCTION (Law 2, the central discipline):
 *   Every EXPECTED value is a LITERAL spelled out in THIS file -- it is NEVER
 *   read back out of the FlairTE internals to form the expectation. Reading
 *   te.text via FlairTEGetText and then comparing to te.text would agree BY
 *   CONSTRUCTION and could not catch a wrong value (the FLAIR palette-grading
 *   heresy; ADR-0010 / Revocation Record HER-02). Instead the expected bytes are:
 *     "hello"        == { 0x68,0x65,0x6C,0x6C,0x6F }
 *     '!'            == 0x21      'X' == 0x58      'A','B','C' == 0x41,0x42,0x43
 *     "X! world"     == { 0x58,0x21,0x20,0x77,0x6F,0x72,0x6C,0x64 }
 *     "! worldX"     == { 0x21,0x20,0x77,0x6F,0x72,0x6C,0x64,0x58 }
 *     "ABC! worldX"  == { 0x41,0x42,0x43,0x21,... }
 *     "AB! worldX"   == { 0x41,0x42,0x21,... }
 *     "a\rb\rc" lineStarts == { 0,2,4 }   "the cat sat" wrap@4 == { 0,4,8 }
 *   These are the INDEPENDENT golden; the implementation is proved against them.
 *   (Reading te.nLines / te.lineStarts and comparing to the LITERALS above is a
 *   normal assertion -- the heresy is deriving the EXPECTED from the artifact's
 *   own computation, which this file never does.)
 *
 * MUTANT COVERAGE (Rule 6 -- each mutant must COMPILE and go RED):
 *   TE_MUT_SEL_INCLUSIVE   -- inclusive delete (one char too many) -> step 5 RED
 *       (delete-then-insert over the half-open selection mis-sizes the deletion).
 *   TE_MUT_PASTE_NO_DELETE -- insert without deleting the selected range first
 *       -> step 5 RED (the range "hello" survives the 'X' insert).
 *   TE_MUT_LINEBREAK_OFF   -- CR line break off-by-one -> step 9 RED (lineStarts
 *       for "a\rb\rc" come out {0,1,3} instead of {0,2,4}).
 *
 * Compile (host, green):
 *   gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *       -Ispec -Ios/flair -Ios/flair/atkinson -Iseed \
 *       -o /tmp/laneD/test_textedit \
 *       harness/proptest/test_textedit.c \
 *       os/flair/textedit.c os/flair/scrap.c os/flair/heap.c
 *
 * Compile (kernel type-check):
 *   gcc -m32 -ffreestanding -nostdlib -std=c11 -Wall -Wextra -Werror \
 *       -Ispec -Ios/milton -Ios/flair -c os/flair/textedit.c
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#include <stdint.h>
#include <stdio.h>

#include "ostype.h"       /* flair_ostype_t (-Ios/flair)                       */
#include "scrap.h"        /* FlairScrap + Scrap API (-Ios/flair)               */
#include "textedit.h"     /* FlairTE, the API under test (-Ios/flair)          */
#include "test_assert.h"  /* TEST_HARNESS / CHECK / TEST_SUMMARY (-Iseed)      */

TEST_HARNESS();

/* Fetch the buffer out (independent of internals) and NUL-terminate for clarity.
 * FlairTEGetText returns teLength (the API output), which the steps compare to a
 * LITERAL expected length. */
static int16_t te_fetch(const FlairTE *te, char *out)
{
    int16_t n = FlairTEGetText(te, out, 255);
    out[n] = '\0';
    return n;
}

#define UB(c) ((unsigned char)(c))   /* unsigned view of a fetched byte */

int main(void)
{
    /* FlairTE is ~4.3 KB and FlairScrap ~32 KB -- static to avoid a deep stack. */
    static FlairTE   te;        /* the main record, mutated across steps 1..8 */
    static FlairTE   te2;       /* step 9: CR-delimited line breaking         */
    static FlairTE   te3;       /* step 9: word-wrap line breaking            */
    static FlairScrap scrap;    /* the shared clipboard (cut/copy/paste seam)  */
    static FlairScrap scrapABC; /* a scrap pre-loaded with "ABC" for step 7    */

    char    out[256];
    uint8_t sbuf[64];
    uint32_t sn;
    int32_t  sr;
    int16_t  n;

    FlairTE_init(&te, /*wrapWidth*/ 200, FLAIR_TE_JUST_LEFT);
    FlairScrap_init(&scrap);

    /* -----------------------------------------------------------------------
     * STEP 1: SetText "hello world" (len 11) -> teLength==11, nLines==1.
     * "hello world" has no CR and fits under wrapWidth=200, so it is one line.
     * ----------------------------------------------------------------------- */
    FlairTESetText(&te, "hello world", 11);
    n = te_fetch(&te, out);
    CHECK(n == 11,
          "step-1: FlairTEGetText returns teLength == 11");
    CHECK(te.nLines == 1,
          "step-1: nLines == 1 (single line; no CR, fits wrapWidth)");
    CHECK(UB(out[0]) == 0x68u && UB(out[4]) == 0x6Fu && UB(out[10]) == 0x64u,
          "step-1: buffer is 'hello world' (h..o..d literal bytes)");

    /* -----------------------------------------------------------------------
     * STEP 2: SetSelect clamping. SetSelect(-5, 99) on teLength==11 clamps to
     * selStart==0, selEnd==11 (textedit.md Sec 2).
     * ----------------------------------------------------------------------- */
    FlairTESetSelect(&te, -5, 99);
    CHECK(te.selStart == 0,
          "step-2: SetSelect(-5,99) clamps selStart to 0");
    CHECK(te.selEnd == 11,
          "step-2: SetSelect(-5,99) clamps selEnd to teLength==11");

    /* -----------------------------------------------------------------------
     * STEP 3: SetSelect(0,5); TECopy -> the SELECTED chars route through the
     * Scrap. FlairGetScrap(TEXT) returns 5 bytes == {0x68,0x65,0x6C,0x6C,0x6F}
     * ("hello"). This proves cut/copy go through FlairPutScrap/FlairGetScrap.
     * ----------------------------------------------------------------------- */
    FlairTESetSelect(&te, 0, 5);
    CHECK(FlairTECopy(&te, &scrap) == 0,
          "step-3: FlairTECopy(sel [0,5)) == 0 (routed through FlairPutScrap)");

    for (n = 0; n < 64; n++) sbuf[n] = 0;
    sn = 0;
    sr = FlairGetScrap(&scrap, FLAIR_SCRAP_TYPE_TEXT, sbuf, sizeof(sbuf), &sn);
    CHECK(sr == 5,
          "step-3: FlairGetScrap(TEXT) returns 5 (the copied char count)");
    CHECK(sn == 5u,
          "step-3: *len_out == 5");
    CHECK(sbuf[0] == 0x68u, "step-3: scrap[0] == 0x68 ('h')");
    CHECK(sbuf[1] == 0x65u, "step-3: scrap[1] == 0x65 ('e')");
    CHECK(sbuf[2] == 0x6Cu, "step-3: scrap[2] == 0x6C ('l')");
    CHECK(sbuf[3] == 0x6Cu, "step-3: scrap[3] == 0x6C ('l')");
    CHECK(sbuf[4] == 0x6Fu, "step-3: scrap[4] == 0x6F ('o')");

    /* -----------------------------------------------------------------------
     * STEP 4: TEKey insert at an insertion point. SetSelect(5,5); TEKey('!')
     * -> "hello! world", teLength==12, caret advances to selStart==selEnd==6.
     * Independent golden: 'h''e''l''l''o''!'' ''w''o''r''l''d'.
     * ----------------------------------------------------------------------- */
    FlairTESetSelect(&te, 5, 5);
    FlairTEKey(&te, '!');
    n = te_fetch(&te, out);
    CHECK(n == 12,
          "step-4: teLength == 12 after inserting '!'");
    CHECK(te.selStart == 6 && te.selEnd == 6,
          "step-4: caret advanced to 6 (insertion+1)");
    CHECK(UB(out[0]) == 0x68u, "step-4: out[0] == 0x68 ('h')");
    CHECK(UB(out[5]) == 0x21u, "step-4: out[5] == 0x21 ('!')");
    CHECK(UB(out[6]) == 0x20u, "step-4: out[6] == 0x20 (' ')");
    CHECK(UB(out[7]) == 0x77u, "step-4: out[7] == 0x77 ('w')");
    CHECK(UB(out[11]) == 0x64u, "step-4: out[11] == 0x64 ('d') -- 'hello! world'");

    /* -----------------------------------------------------------------------
     * STEP 5: Range replace proves HALF-OPEN delete-then-insert. The buffer is
     * "hello! world"; SetSelect(0,5) selects "hello"; TEKey('X') deletes the
     * range [0,5) and inserts 'X' at 0 -> "X! world", teLength==8.
     *
     * TE_MUT_SEL_INCLUSIVE deletes "hello!" (6 chars) -> "X world" (7) -> RED.
     * TE_MUT_PASTE_NO_DELETE inserts 'X' without deleting -> "Xhello! world"
     *   (13) -> RED. (Either way the literal checks below fail.)
     * ----------------------------------------------------------------------- */
    FlairTESetSelect(&te, 0, 5);
    FlairTEKey(&te, 'X');
    n = te_fetch(&te, out);
    CHECK(n == 8,
          "step-5: teLength == 8 ('X! world' -- half-open [0,5) deleted)");
    CHECK(UB(out[0]) == 0x58u, "step-5: out[0] == 0x58 ('X')");
    CHECK(UB(out[1]) == 0x21u, "step-5: out[1] == 0x21 ('!') -- exactly 'hello' removed");
    CHECK(UB(out[2]) == 0x20u, "step-5: out[2] == 0x20 (' ')");
    CHECK(UB(out[3]) == 0x77u, "step-5: out[3] == 0x77 ('w')");
    CHECK(UB(out[7]) == 0x64u, "step-5: out[7] == 0x64 ('d') -- 'X! world'");

    /* -----------------------------------------------------------------------
     * STEP 6: TECut then TEPaste at the end -- the cut char MOVES. Buffer is
     * "X! world". SetSelect(0,1); TECut(scrap) copies "X" to the scrap and
     * deletes it -> "! world" (7). The scrap holds {0x58}. SetSelect(7,7);
     * TEPaste(scrap) inserts "X" at the end -> "! worldX" (8).
     * ----------------------------------------------------------------------- */
    FlairTESetSelect(&te, 0, 1);
    CHECK(FlairTECut(&te, &scrap) == 0,
          "step-6: FlairTECut(sel [0,1)) == 0");
    n = te_fetch(&te, out);
    CHECK(n == 7,
          "step-6: teLength == 7 after cutting 'X' ('! world')");

    /* The scrap held the cut text ("X" == 0x58) -- proves Cut routed through it. */
    for (n = 0; n < 64; n++) sbuf[n] = 0;
    sn = 0;
    sr = FlairGetScrap(&scrap, FLAIR_SCRAP_TYPE_TEXT, sbuf, sizeof(sbuf), &sn);
    CHECK(sr == 1 && sn == 1u && sbuf[0] == 0x58u,
          "step-6: scrap holds the cut 'X' (1 byte == 0x58)");

    FlairTESetSelect(&te, 7, 7);   /* caret at the end */
    CHECK(FlairTEPaste(&te, &scrap) == 0,
          "step-6: FlairTEPaste(scrap) at end == 0");
    n = te_fetch(&te, out);
    CHECK(n == 8,
          "step-6: teLength == 8 after pasting 'X' at the end ('! worldX')");
    CHECK(UB(out[0]) == 0x21u, "step-6: out[0] == 0x21 ('!')");
    CHECK(UB(out[6]) == 0x64u, "step-6: out[6] == 0x64 ('d')");
    CHECK(UB(out[7]) == 0x58u, "step-6: out[7] == 0x58 ('X') -- the cut char moved to the end");

    /* -----------------------------------------------------------------------
     * STEP 7: TEPaste from a scrap holding "ABC" at an insertion point ->
     * inserted literally. Buffer is "! worldX". SetSelect(0,0); paste "ABC"
     * -> "ABC! worldX" (11); the caret advances past the insert (to 3).
     * Independent golden: 'A''B''C' == 0x41,0x42,0x43.
     * ----------------------------------------------------------------------- */
    FlairScrap_init(&scrapABC);
    {
        static const uint8_t abc[3] = { 0x41, 0x42, 0x43 };   /* 'A','B','C' */
        CHECK(FlairPutScrap(&scrapABC, FLAIR_SCRAP_TYPE_TEXT, abc, 3u) == 0,
              "step-7: seed scrapABC with 'ABC' == 0");
    }
    FlairTESetSelect(&te, 0, 0);
    CHECK(FlairTEPaste(&te, &scrapABC) == 0,
          "step-7: FlairTEPaste('ABC') at [0,0) == 0");
    n = te_fetch(&te, out);
    CHECK(n == 11,
          "step-7: teLength == 11 after pasting 'ABC' ('ABC! worldX')");
    CHECK(te.selStart == 3 && te.selEnd == 3,
          "step-7: caret advanced past the 3-char paste (to 3)");
    CHECK(UB(out[0]) == 0x41u, "step-7: out[0] == 0x41 ('A')");
    CHECK(UB(out[1]) == 0x42u, "step-7: out[1] == 0x42 ('B')");
    CHECK(UB(out[2]) == 0x43u, "step-7: out[2] == 0x43 ('C')");
    CHECK(UB(out[3]) == 0x21u, "step-7: out[3] == 0x21 ('!') -- 'ABC' inserted before '!'");

    /* -----------------------------------------------------------------------
     * STEP 8: Backspace deletes the preceding char. Buffer is "ABC! worldX",
     * caret at 3. TEKey(0x08) deletes text[2] ('C') -> "AB! worldX" (10).
     * ----------------------------------------------------------------------- */
    FlairTEKey(&te, 0x08);   /* backspace at caret 3 */
    n = te_fetch(&te, out);
    CHECK(n == 10,
          "step-8: teLength == 10 after backspace ('AB! worldX')");
    CHECK(te.selStart == 2 && te.selEnd == 2,
          "step-8: caret moved back to 2");
    CHECK(UB(out[0]) == 0x41u, "step-8: out[0] == 0x41 ('A')");
    CHECK(UB(out[1]) == 0x42u, "step-8: out[1] == 0x42 ('B')");
    CHECK(UB(out[2]) == 0x21u, "step-8: out[2] == 0x21 ('!') -- 'C' deleted before the caret");

    /* -----------------------------------------------------------------------
     * STEP 9a: Multi-line CR-delimited. "a\rb\rc" (crOnly < 0, Return-only) ->
     * nLines==3, lineStarts == {0,2,4} (literal). The CR at index 1 starts line
     * 1 at 2; the CR at index 3 starts line 2 at 4.
     *
     * TE_MUT_LINEBREAK_OFF records the CR offset (1, 3) instead of past it
     * (2, 4) -> lineStarts == {0,1,3} -> RED here.
     * ----------------------------------------------------------------------- */
    FlairTE_init(&te2, /*wrapWidth*/ 80, FLAIR_TE_JUST_LEFT);
    te2.crOnly = -1;                 /* Return-only: no word wrap (Sec 4 step 1) */
    FlairTESetText(&te2, "a\rb\rc", 5);
    CHECK(te2.nLines == 3,
          "step-9a: 'a\\rb\\rc' -> nLines == 3 (two CRs -> three lines)");
    CHECK(te2.lineStarts[0] == 0,
          "step-9a: lineStarts[0] == 0 (line 0 always starts at 0)");
    CHECK(te2.lineStarts[1] == 2,
          "step-9a: lineStarts[1] == 2 (past the CR at index 1)");
    CHECK(te2.lineStarts[2] == 4,
          "step-9a: lineStarts[2] == 4 (past the CR at index 3)");

    /* -----------------------------------------------------------------------
     * STEP 9b: Word-wrap. "the cat sat" (11 chars) with wrapWidth==4, crOnly==0
     * wraps at the last space before the 4-char boundary:
     *   line 0 = "the " [0,4), line 1 = "cat " [4,8), line 2 = "sat" [8,11)
     * -> nLines==3, lineStarts == {0,4,8} (literal, hand-computed).
     * ----------------------------------------------------------------------- */
    FlairTE_init(&te3, /*wrapWidth*/ 4, FLAIR_TE_JUST_LEFT);  /* crOnly == 0 */
    FlairTESetText(&te3, "the cat sat", 11);
    CHECK(te3.nLines == 3,
          "step-9b: 'the cat sat' wrap@4 -> nLines == 3");
    CHECK(te3.lineStarts[0] == 0,
          "step-9b: lineStarts[0] == 0 ('the ')");
    CHECK(te3.lineStarts[1] == 4,
          "step-9b: lineStarts[1] == 4 ('cat ' -- broke after the space at 3)");
    CHECK(te3.lineStarts[2] == 8,
          "step-9b: lineStarts[2] == 8 ('sat' -- broke after the space at 7)");

    return TEST_SUMMARY("test-textedit");
}
