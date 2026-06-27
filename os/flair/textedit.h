/*
 * os/flair/textedit.h -- FLAIR TextEdit (TERec): the shared editable-text
 * engine -- THE ARTIFACT.
 *
 * beads: initech-77dj (FLAIR Phase 4.5 -- the TextEdit half of the shared
 *        editable-text engine). Committee ruling wf_00931e9e: a REDUCED first
 *        cut of the Inside Macintosh TERec (a flat fixed text buffer + the
 *        half-open selection + CR/word-wrap line breaking + cut/copy/paste
 *        against the shell-owned Scrap), explicitly capped and labelled.
 *
 * GROUND TRUTH (Law 1):
 *   ../system7-decomp/specs/toolbox/textedit.md -- the VERBATIM TERec layout
 *   and semantics. This header mirrors the IM field NAMES (Sec 1), the
 *   half-open [selStart,selEnd) selection model (Sec 2), the just constants
 *   teJustLeft=0/teJustCenter=1/teJustRight=-1 (Sec 3), and the CR-delimited /
 *   crOnly<0 / lineStarts+nLines line-layout rule (Sec 4). It implements only
 *   the MONOSTYLED model (Sec 5); styled runs are out of scope.
 *
 * THE REDUCTION (committee ruling wf_00931e9e; Rule 8 -- locked, deliberate):
 *   The full IM TERec carries `lineStarts: ARRAY[0..16000] OF INTEGER` (~32 KB)
 *   plus the QuickDraw rects/hooks/blink machinery (textedit.md Sec 1). This
 *   FIRST CUT keeps a flat fixed `char text[FLAIR_TE_TEXT_MAX]` buffer, the
 *   teLength/selStart/selEnd/just/crOnly/nLines fields, and a CAPPED
 *   lineStarts[FLAIR_TE_MAX_LINES] array. It drops destRect/viewRect/selRect,
 *   the proc hooks, caret-blink timing, and the styled (txSize==-1) path.
 *   FOLLOW-UP (deferred full TERec): restore the QuickDraw rects, the heap-handle
 *   hText, the proportional (owTable) line layout, the caret/highlight hooks and
 *   TEClick/TEIdle/TEActivate -- a Rule-8 deliberate act with its own bead +
 *   worklog note, NOT a silent edit. The IM field NAMES + the load-bearing `just`
 *   triple are preserved here so the M5+ record is a superset (textedit.md Sec 7).
 *
 * FREESTANDING (Law 3): <stdint.h> + the Scrap header only; no libc, no malloc.
 *   Compiles BOTH under the kernel flags (gcc -m32 -ffreestanding -nostdlib
 *   -std=c11 -Wall -Wextra -Werror) AND hosted for the property suite
 *   (harness/proptest/test_textedit.c). The dual-compile pattern of scrap.c /
 *   heap.c / process.c.
 *
 * CAPACITY CONSTANTS (Rule 8: raising either is a DELIBERATE ACT with an issue +
 *   worklog note -- never a silent edit. The `_Static_assert` ceilings below pin
 *   the reduction):
 *   FLAIR_TE_TEXT_MAX  = 4096  (flat text buffer; teLength <= this)
 *   FLAIR_TE_MAX_LINES = 128   (capped lineStarts; << the full IM 16001)
 *
 * Ref: os/flair/scrap.h (FlairScrap, FlairPutScrap/FlairGetScrap,
 *      FLAIR_SCRAP_TYPE_TEXT -- the shared clipboard cut/copy/paste route
 *      through THIS API, proving the Scrap is the shared seam); os/flair/ostype.h
 *      (flair_ostype_t). CLAUDE.md Law 1 (ground truth before code), Law 2 (the
 *      oracle vs an INDEPENDENT golden -- never by-construction), Law 3
 *      (freestanding artifact, no 2026-ism), Rule 2 (fail loud: over-MAX,
 *      bufsz-too-small, NULL arg -> panic; never silent truncation), Rule 8
 *      (spec as locked data), Rule 11 (deterministic), Rule 12 (ASCII-clean).
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_TEXTEDIT_H
#define INITECH_OS_FLAIR_TEXTEDIT_H

#include <stdint.h>
#include "scrap.h"   /* FlairScrap, FlairPutScrap/FlairGetScrap, FLAIR_SCRAP_TYPE_TEXT */

/* --------------------------------------------------------------------------
 * Capacity constants (Rule 8: locked; raise only with issue + worklog note).
 * -------------------------------------------------------------------------- */
#define FLAIR_TE_TEXT_MAX   4096   /* flat text buffer length ceiling           */
#define FLAIR_TE_MAX_LINES  128    /* capped lineStarts entries (reduced cut)   */

/* --------------------------------------------------------------------------
 * Justification constants (textedit.md Sec 3 -- the System-7 triple FLAIR pins;
 * the later script-aware aliases teFlushDefault/teCenter/... are out of scope).
 * -------------------------------------------------------------------------- */
#define FLAIR_TE_JUST_LEFT     0    /* teJustLeft   -- flush left (default)      */
#define FLAIR_TE_JUST_CENTER   1    /* teJustCenter -- centered                  */
#define FLAIR_TE_JUST_RIGHT  (-1)   /* teJustRight  -- flush right               */

/* --------------------------------------------------------------------------
 * REDUCTION CEILINGS (Rule 8 -- pin the first cut at build time).
 *
 * The line cap is the headline reduction: the full IM TERec's lineStarts is
 * ARRAY[0..16000] (16001 entries). FLAIR_TE_MAX_LINES must stay at or below that
 * full ceiling -- raising it toward the full TERec is a deliberate Rule-8 act
 * with a follow-up bead, not a silent edit. The int16_t offsets/indices require
 * both caps to fit a signed 16-bit value.
 * -------------------------------------------------------------------------- */
_Static_assert(FLAIR_TE_MAX_LINES <= 16001,
               "reduced TERec line cap must stay <= the full IM lineStarts[0..16000] "
               "ceiling; raising toward the full TERec is a Rule-8 deliberate act "
               "(deferred-full-TERec follow-up bead)");
_Static_assert(FLAIR_TE_MAX_LINES >= 2,
               "need at least a couple of lines for CR-delimited text");
_Static_assert(FLAIR_TE_TEXT_MAX >= 1 && FLAIR_TE_TEXT_MAX <= 32767,
               "teLength/selStart/selEnd are int16_t offsets -- text max must fit");
_Static_assert(FLAIR_TE_MAX_LINES <= 32767,
               "lineStarts holds int16_t offsets and is indexed by an int16_t count");

/* Pin the load-bearing just triple (textedit.md Sec 3 / Sec 7: the M5+ record
 * MUST keep these values). If any drifts, justification is silently wrong. */
_Static_assert(FLAIR_TE_JUST_LEFT == 0,    "teJustLeft must be 0 (textedit.md Sec 3)");
_Static_assert(FLAIR_TE_JUST_CENTER == 1,  "teJustCenter must be 1 (textedit.md Sec 3)");
_Static_assert(FLAIR_TE_JUST_RIGHT == -1,  "teJustRight must be -1 (textedit.md Sec 3)");

/* --------------------------------------------------------------------------
 * FlairTE -- the REDUCED edit record (first cut of IM TERec).
 *
 * Field NAMES mirror the verbatim IM TERec (textedit.md Sec 1); INTEGER is
 * re-typed to int16_t (the FLAIR re-type convention, Sec 6/Sec 7). The selection
 * is the HALF-OPEN range [selStart, selEnd) (Sec 2): 0 == before the first char,
 * teLength == after the last; selStart == selEnd is an insertion point (caret).
 * `text` is a flat buffer with NO terminator reliance -- `teLength` is the length.
 * -------------------------------------------------------------------------- */
typedef struct {
    char    text[FLAIR_TE_TEXT_MAX];     /* IM hText: flat chars; teLength bytes valid */
    int16_t selStart;                    /* IM selStart: selection start (Sec 2)       */
    int16_t selEnd;                      /* IM selEnd: selection end; [selStart,selEnd) */
    int16_t teLength;                    /* IM teLength: valid char count in text[]     */
    int16_t just;                        /* IM just: FLAIR_TE_JUST_* (Sec 3)            */
    int16_t crOnly;                      /* IM crOnly: <0 == Return-only (no word wrap) */
    int16_t nLines;                      /* IM nLines: number of laid-out lines         */
    int16_t wrapWidth;                   /* FLAIR: word-wrap width in chars (IM destRect */
                                         /*   width; monospaced -- textedit.md Sec 4)   */
    int16_t lineStarts[FLAIR_TE_MAX_LINES]; /* IM lineStarts: char offset of each line  */
                                            /*   start; N == nLines (Sec 4; CAPPED cut) */
} FlairTE;

/* --------------------------------------------------------------------------
 * API -- the IM TextEdit subset (textedit.md Sec 6 semantics). Every routine
 * takes an explicit FlairTE * (no implicit global in the artifact); a NULL
 * pointer argument is a contract violation and fails loud (Rule 2).
 * -------------------------------------------------------------------------- */

/* FlairTE_init -- empty record: teLength=0, caret at 0, the given just +
 * wrapWidth, word-wrap enabled (crOnly=0). nLines=1 (one empty line). */
void FlairTE_init(FlairTE *te, int16_t wrapWidth, int16_t just);

/* FlairTESetText -- replace the buffer with `len` bytes from `s`. Fail loud
 * (Rule 2) if len < 0 or len > FLAIR_TE_TEXT_MAX -- never silent truncation.
 * Recomputes the line breakdown and clamps the selection into [0, len]. */
void FlairTESetText(FlairTE *te, const char *s, int16_t len);

/* FlairTEGetText -- copy the buffer out into `buf` (bufsz bytes available).
 * Fail loud (Rule 2) if bufsz < teLength -- NO partial copy. Returns teLength.
 * Does NOT null-terminate (teLength is the length; no terminator reliance). */
int16_t FlairTEGetText(const FlairTE *te, char *buf, int16_t bufsz);

/* FlairTESetSelect -- set the selection. Both ends are CLAMPED to [0, teLength]
 * and ordered so selStart <= selEnd (textedit.md Sec 2). */
void FlairTESetSelect(FlairTE *te, int16_t selStart, int16_t selEnd);

/* FlairTEKey -- type one char at the insertion point (textedit.md Sec 6).
 * A range selection is deleted first; then `ch` is inserted at selStart and the
 * caret advances past it (selStart=selEnd=insertion+1). Backspace (0x08) deletes
 * the selected range if any, else the char before the caret. Recomputes lines. */
void FlairTEKey(FlairTE *te, char ch);

/* FlairTEInsert -- insert `len` bytes from `s` at the insertion point (deleting
 * any range first); the caret advances past the inserted run. Recomputes lines. */
void FlairTEInsert(FlairTE *te, const char *s, int16_t len);

/* FlairTEDelete -- delete the current range selection. An empty insertion point
 * (selStart == selEnd) is a no-op. Recomputes lines. */
void FlairTEDelete(FlairTE *te);

/* FlairTECopy -- put the selected chars [selStart,selEnd) into `scrap` as the
 * FLAIR_SCRAP_TYPE_TEXT flavor via FlairPutScrap (the shared clipboard route).
 * Returns 0 on success; a negative FlairPutScrap error code otherwise. */
int FlairTECopy(const FlairTE *te, FlairScrap *scrap);

/* FlairTECut -- FlairTECopy then FlairTEDelete. Returns 0 on success (or the
 * copy's negative error code, in which case nothing is deleted). */
int FlairTECut(FlairTE *te, FlairScrap *scrap);

/* FlairTEPaste -- GetScrap the FLAIR_SCRAP_TYPE_TEXT flavor and INSERT it at the
 * selection (replacing any range), like FlairTEInsert. Returns 0 on success;
 * -1 if the scrap has no TEXT flavor. Fails loud (Rule 2) if the TEXT flavor is
 * larger than the FlairTE text buffer can hold. */
int FlairTEPaste(FlairTE *te, const FlairScrap *scrap);

#endif /* INITECH_OS_FLAIR_TEXTEDIT_H */
