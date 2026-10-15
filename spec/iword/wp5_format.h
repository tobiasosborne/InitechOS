/*
 * spec/iword/wp5_format.h -- LOCKED spec-data: the WordPerfect 5.1 document
 * container (.WP5) as far as InitechWord P1 uses it (CLAUDE.md Rule 8).
 *
 * bead: initech-l4tj (IWORD P1). Epic initech-fdxa. Plan:
 *       docs/plans/INITECH-WORD-plan.md Sec 6(a) / Sec 7 P1.
 *
 * EVERY value below is cited to the sister corpus ../wordperfect51-decomp
 * (WP 5.1 03-09-92, the five documents minted by driving the real WP.EXE
 * under DOSBox-X):
 *   [H]  specs/file-formats/wp5-header-and-prefix.md (Sec 1 header, Sec 2
 *        prefix geometry, Sec 4 "writing a conforming file");
 *   [B]  specs/file-formats/wp5-body-codes.md (Sec 1 byte classes, Sec 2
 *        single-byte codes, Sec 3 fixed groups, Sec 4 variable groups);
 *   [G]  goldens/minted/MINT01..05.WP5 (hex-checked by test-iword-wp5-
 *        roundtrip, which also proves IW_WP5_DEFAULT_PREFIX against them).
 * INDEPENDENCE (plan Sec 6(a)): nothing here is copied from tools/wp5_ref.py,
 * the corpus' independent reader; that reader grades what this header lets
 * the C codec write. Only VERIFIED rows are admitted: a code the corpus
 * marks [oracle-resolves] (C5, C7..CF, the unseen single bytes) is NOT in
 * these tables, so a file that uses one is REFUSED (Rule 2), never guessed.
 *
 * ASCII-clean (Rule 12). No timestamps (Rule 11).
 */
#ifndef INITECH_SPEC_IWORD_WP5_FORMAT_H
#define INITECH_SPEC_IWORD_WP5_FORMAT_H

/* ---- the 16-byte header [H Sec 1] ------------------------------------- */
#define IW_WP5_MAGIC0          0xFFu   /* offset 0: 0xFF then "WPC"        */
#define IW_WP5_MAGIC1          0x57u   /* 'W'                              */
#define IW_WP5_MAGIC2          0x50u   /* 'P'                              */
#define IW_WP5_MAGIC3          0x43u   /* 'C'                              */
#define IW_WP5_OFF_DOCSTART    4u      /* u32 LE: where the body starts    */
#define IW_WP5_OFF_PRODUCT     8u      /* u8 = 1                           */
#define IW_WP5_OFF_FILETYPE    9u      /* u8 = 0x0A (document)             */
#define IW_WP5_OFF_MAJOR       10u     /* u8 = 0                           */
#define IW_WP5_OFF_MINOR       11u     /* u8 = 1                           */
#define IW_WP5_OFF_ENCRYPT     12u     /* u16 = 0 (not encrypted)          */
#define IW_WP5_HEADER_BYTES    16u
#define IW_WP5_PRODUCT         0x01u
#define IW_WP5_FILETYPE_DOC    0x0Au
#define IW_WP5_MAJOR           0x00u
#define IW_WP5_MINOR           0x01u

/* ---- body byte classes [B Sec 1, Sec 2] -------------------------------- */
#define IW_WP5_TEXT_LO         0x20u   /* 0x20..0x7F: one ASCII character  */
#define IW_WP5_TEXT_HI         0x7Fu   /* (0x7F unseen; classed with text) */
#define IW_WP5_HRT             0x0Au   /* hard return  [HRt]  (MINT02 @0x147) */
#define IW_WP5_SRT             0x0Du   /* soft return  (MINT01 @0x188: the typed
                                        * space BECOMES the 0x0D; 65 chars
                                        * precede it on the line)             */
#define IW_WP5_HPG             0x0Cu   /* hard page [HPg] (MINT03 @0x1AF)     */
#define IW_WP5_HSPACE          0xA0u   /* hard space [ ] (MINT05 @0x18C)      */
#define IW_WP5_END_CENTER      0x83u   /* end of center / flush right (MINT01 @0x1D8) */
#define IW_WP5_FIXED_LO        0xC0u   /* fixed-length group, ends with its own code */
#define IW_WP5_FIXED_HI        0xCFu
#define IW_WP5_VAR_LO          0xD0u   /* variable group [code][sub][u16 size]
                                        * payload [u16 size][sub][code];
                                        * size counts the bytes AFTER the
                                        * 4-byte header, trailer included    */
#define IW_WP5_VAR_HDR         4u
#define IW_WP5_VAR_TRAILER     4u

/* Fixed-group TOTAL lengths, VERIFIED rows only [B Sec 3]; 0 = not verified
 * (refused). Indexed by code - 0xC0. C0 extended char, C1 center/flush/tab,
 * C2 indent, C3 attribute on, C4 attribute off, C6 end of indent. */
#define IW_WP5_FIXED_LEN_INIT { 4u, 9u, 11u, 3u, 3u, 0u, 6u, 0u, \
                                0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u }

/* ---- attribute on / off codes (bead initech-jojk, IWORD P2 slice 1) ------ *
 * [B Sec 3]: `C3 attr C3` turns an attribute ON, `C4 attr C4` turns the same
 * attribute OFF; the ON and OFF codes carry the same number. VERIFIED numbers
 * only (each in a document the real WP.EXE wrote):
 *   0x02 Large      MINT04.WP5 @0x186 / @0x193 (Ctrl-F8 1 5, Ctrl-F8 3)
 *   0x08 Italic     MINT06 @0x14F / @0x15F (Ctrl-F8 2 4 ... Ctrl-F8 2 4)
 *   0x0C Bold       MINT01.WP5 @0x19B / @0x1A2 (F6 ... F6); MINT07 @0x148 /
 *                   @0x154 (Ctrl-F8 2 1 ... Ctrl-F8 2 1: the same bytes)
 *   0x0E Underline  MINT01.WP5 @0x1AA / @0x1B7 (F8 ... F8)
 * MINT06..MINT08 were minted for this bead on 2026-10-16 by driving the real
 * WP.EXE under DOSBox-X with ../wordperfect51-decomp/tools/mint_drive.sh and
 * the step files mint/wpMINT06.steps .. wpMINT08.steps (the harness wrote the
 * documents to mint/work/WP51/MINT06..08 and the screens to
 * mint/shots_wpMINT06..08/; filing them under goldens/minted is the corpus
 * owner's act). Every other attribute number is [oracle-resolves]. */
#define IW_WP5_ATTR_ON         0xC3u
#define IW_WP5_ATTR_OFF        0xC4u
#define IW_WP5_ATTR_LEN        3u
#define IW_WP5_ATTR_LARGE      0x02u
#define IW_WP5_ATTR_ITALIC     0x08u
#define IW_WP5_ATTR_BOLD       0x0Cu
#define IW_WP5_ATTR_UNDERLINE  0x0Eu
#define IW_WP5_ATTR_NUMBERS    16u     /* attr is one byte; WP 5.1's table runs 0..15 */

/* The prefix records an italic body. MINT06 (a body with an italic pair) has
 * 0x08 at file offset 0x103 -- the first data byte of prefix packet 0x06
 * (index entry at 0x10: type 0x06, len 16, offset 0x103) -- where every other
 * mint has 0x00: MINT01 (bold + underline), MINT04 (large), MINT07 (bold
 * through the Font menu) and MINT08 (an italic pair inserted, then deleted in
 * Reveal Codes before the save). So the byte follows the SAVED body, not the
 * keys pressed: 0x08 when the body holds an italic code, else 0x00
 * [inferred: 1 positive, 6 negative documents; the packet itself is
 * undecoded, corpus GAPS F-1/W-3]. The writer applies it only to a prefix
 * that is the default one in every other byte. */
#define IW_WP5_PFX_ITALIC_OFF  0x103u
#define IW_WP5_PFX_ITALIC      0x08u

/* ---- the prefix area [H Sec 2, Sec 4] ---------------------------------- *
 * A fresh-install WP 5.1 writes the SAME 327 bytes (0x147) before the body
 * of every document in the corpus but one byte: 0x103 marks an italic body
 * (IW_WP5_PFX_ITALIC_OFF above; MINT06) -- the body length is recorded
 * nowhere in it; [H Sec 4]: "emit the 327-byte prefix of section 2 verbatim, then the
 * body." These are those bytes, transcribed from MINT02.WP5 0x000..0x146
 * (cmp -n 327 against MINT01/03/04/05: identical). The packets inside
 * (0x0F, 0x07 "Courier 10 Pitch", 0x0C, 0x06, 0x08) are undecoded
 * ([oracle-resolves], corpus GAPS F-1/W-3) and are carried opaque. */
#define IW_WP5_DEFAULT_DOCSTART 0x147u
#define IW_WP5_DEFAULT_PREFIX_INIT { \
  0xff, 0x57, 0x50, 0x43, 0x47, 0x01, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x01, \
  0x00, 0x00, 0x00, 0x00, 0xfb, 0xff, 0x05, 0x00, 0x32, 0x00, 0x13, 0x01, \
  0x00, 0x00, 0x0f, 0x00, 0x56, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, \
  0x07, 0x00, 0x11, 0x00, 0x00, 0x00, 0x98, 0x00, 0x00, 0x00, 0x0c, 0x00, \
  0x5a, 0x00, 0x00, 0x00, 0xa9, 0x00, 0x00, 0x00, 0x06, 0x00, 0x10, 0x00, \
  0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x7a, 0x00, \
  0x4e, 0x00, 0x78, 0x00, 0x78, 0x00, 0x78, 0x00, 0x0a, 0x00, 0x01, 0x00, \
  0x00, 0x00, 0x00, 0x55, 0x5e, 0xdb, 0x01, 0x78, 0x00, 0x14, 0x1e, 0x0c, \
  0x17, 0x8c, 0x0a, 0x00, 0x00, 0x00, 0x04, 0x11, 0x40, 0xc9, 0x00, 0x87, \
  0xcf, 0x01, 0x00, 0x01, 0x00, 0x58, 0x02, 0x40, 0xff, 0xff, 0xff, 0xff, \
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x43, 0x6f, 0x75, 0x72, \
  0x69, 0x65, 0x72, 0x20, 0x31, 0x30, 0x20, 0x50, 0x69, 0x74, 0x63, 0x68, \
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, \
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x7c, 0x00, 0x78, \
  0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfb, \
  0xff, 0x05, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x02, \
  0x00, 0x00, 0x00, 0x45, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
  0x00, 0x00, 0x00 \
}

/* ---- the default page as the minted documents carry it ------------------ *
 * Units: 1200 WordPerfect units per inch [B Sec 5]. The default left/right
 * margins are 1" (MINT04's D0/01 group: old pair 1200,1200) and the initial
 * font is "Courier 10 Pitch" (prefix packet 0x07) = 10 characters per inch,
 * so a default line holds (8.5 - 1 - 1) * 10 = 65 characters -- exactly the
 * 65 characters that precede the SRt in MINT01 [B Sec 2, MINT01 @0x188].
 * Manual (initech-manuals-ref/wordperfect51/WPDOS51Reference.pdf): "The
 * default setting is for a 1" margin on each side of the page" (Margins,
 * Left and Right, p. 394, PDF 411); the default paper is "Standard 8 1/2" x
 * 11"" (Conversion, PDF 154). */
#define IW_WP5_WRAP_COLS       65u
/* Vertical: the status line reads Ln 1" for the first line and 1.17" for the
 * second (MINT02.shots/02_typed.png): a 1" top margin and 6 lines per inch.
 * An 11" page with 1" top and bottom margins holds 9 * 6 = 54 lines
 * [inferred: top/bottom margin defaults (Margins, Top and Bottom, p. 396)
 * are not shown by any mint; 54 is unexercised by P1's gates]. */
#define IW_WP5_LINES_PER_INCH  6u
#define IW_WP5_LINES_PER_PAGE  54u

#endif /* INITECH_SPEC_IWORD_WP5_FORMAT_H */
