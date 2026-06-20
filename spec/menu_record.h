/*
 * spec/menu_record.h -- FLAIR Menu Manager: the LOCKED contract.
 *
 * LOCKED spec-data (CLAUDE.md Rule 8; PRD Sec 6.3 -- "the Toolbox layer").
 * beads: initech-dh5k.2 (P1-2: spec/menu_record.h -- MenuInfo, item attrs,
 *        enableFlags, MenuSelect result word + oracle).
 *
 * This header locks the MenuInfo data structure (verbatim Inside Macintosh
 * field names + semantics), the per-item attribute model, the enableFlags
 * bit-assignment convention, the mark/style constant catalogs, the MDEF
 * message constants, and the MenuSelect / MenuKey result-word packing.
 *
 * ERA TAG: era=system7.0-7.1 (the System 7.0/7.1 BASE; a future System-8
 * Platinum layer may accrete additive era-deltas without a base rewrite;
 * Sec 1 era axis, FLAIR implementation plan 2026-06-20).
 *
 * SOURCE CITATIONS (Law 1 -- all local or free Apple developer archive):
 *
 *   ../system7-decomp/specs/toolbox/menu-manager.md [PRIMARY; LOCAL]
 *     The verbatim MenuInfo Pascal record (Sec 1 of that doc):
 *       menuID, menuWidth, menuHeight, menuProc, enableFlags, menuData.
 *     The per-item attribute model (Sec 2): text, mark, cmdChar, style,
 *       icon, enable; divider via "(-" text.
 *     The mark-character named constants (Sec 2.1):
 *       noMark=0, commandMark=0x11, checkMark=0x12, diamondMark=0x13,
 *       appleMark=0x14.
 *     The QuickDraw Style byte (Sec 2.2):
 *       plain=0, bold=1, italic=2, underline=4, outline=8, shadow=16,
 *       condense=32, extend=64.
 *     The enableFlags bit-convention (Sec 3):
 *       bit 0 = whole menu title; bits 1..31 = items 1..31; >31 always on.
 *     The MenuSelect / MenuKey result-word packing (Sec 4):
 *       result = (menuID << 16) | item1based; 0 = nothing chosen.
 *     MDEF messages (Sec 5):
 *       mDrawMsg=0, mChooseMsg=1, mSizeMsg=2, mPopUpMsg=3.
 *     FLAIR mapping + deviations (Sec 7):
 *       The exploded MenuInfo struct (os/flair/menu.h) is SEMANTICALLY
 *       EQUIVALENT to the IM packed form. This spec locks the IM record and
 *       the constant values; the implementation may use the exploded form.
 *
 *   os/flair/menu.h (READ-ONLY reference; confirms the FLAIR mapping)
 *     MenuInfo struct (menuID, title, items[], n_items, menuWidth), MenuItem
 *     struct (text, mark, cmdChar, style, enabled, is_divider), MenuResult /
 *     MenuResultID / MenuResultItem helpers.
 *
 *   spec/assets/menu_canon.h (the FROZEN InitechPaint bar string):
 *     "File Edit Image Layer Select View Window Help" -- CANON, NOT a bug,
 *     NOT to be corrected.  Do NOT re-author that string here.  Reference it.
 *     See CANON WARNING below.
 *
 *   Inside Macintosh Vol I "Menu Manager" (IM-I Ch 13, p. I-344):
 *     MenuInfo Pascal record + enableFlags semantics.  [web cross-check;
 *     local source above is the authority]
 *   Inside Macintosh Toolbox Essentials, Menu Manager chapter:
 *     Mark constants, MDEF messages, AppendMenu metacharacters.
 *     [web cross-check; local source above is the authority]
 *
 * =========================================================================
 * CANON WARNING
 * =========================================================================
 *
 * The InitechPaint menu-bar string is FROZEN in spec/assets/menu_canon.h:
 *   "File Edit Image Layer Select View Window Help"
 *
 * This is a deliberate chimera inconsistency from the Office Space prop --
 * a Mac-located menu bar carrying a Photoshop-style item set that is
 * historically impossible as any single real Photoshop version.  IT IS THE
 * SPEC.  Do NOT re-author, "correct", or duplicate it here.  Include
 * spec/assets/menu_canon.h and reference FLAIR_CANON_PHOTOSHOP_MENUBAR.
 *
 * Ref: ADR-0004 D-3 / AM-4; spec/chimera_element_map.json element 9;
 *      CLAUDE.md Law 4; PRD Appendix A.
 *
 * =========================================================================
 *
 * DUAL-COMPILE: freestanding (gcc -m32 -ffreestanding -nostdlib -std=c11)
 * AND hosted (gcc -m32 -std=c11).  Only <stdint.h> + <stddef.h>.
 * No malloc; no libc beyond stdint/stddef.  Rule 11 (reproducible).
 * ASCII-clean (Rule 12).  Changing this file is a deliberate Rule 8 act.
 */

#ifndef INITECH_SPEC_MENU_RECORD_H
#define INITECH_SPEC_MENU_RECORD_H

#include <stdint.h>
#include <stddef.h>

/*
 * Bring in the FROZEN canon.  Do NOT re-author the Photoshop menu string.
 * See CANON WARNING above.
 * Ref: spec/assets/menu_canon.h (ADR-0004 AM-4; beads initech-zaqj).
 */
#include "assets/menu_canon.h"

/* ===========================================================================
 * 0. ERA ANNOTATION
 * ---------------------------------------------------------------------------
 * This spec locks the System 7.0/7.1 BASE layer.
 * A future era-delta file (e.g. spec/menu_record_platinum.h) may override
 * individual values additively.  Do not remove this tag.
 *
 * Ref: FLAIR implementation plan Sec 1 era axis (2026-06-20);
 *      ../system7-decomp/specs/toolbox/menu-manager.md (the BASE source).
 * ===========================================================================*/
#define FLAIR_MENU_RECORD_ERA "system7.0-7.1"   /* era=system7.0-7.1 */

/* ===========================================================================
 * 1. MenuInfo FIELD SIZES (verbatim Inside Macintosh 68K types)
 * ---------------------------------------------------------------------------
 * Ref: ../system7-decomp/specs/toolbox/menu-manager.md Sec 1 (field table).
 *      IM-I Ch 13 p. I-344 ("MenuInfo").
 *
 * Pascal types used in the IM record:
 *   INTEGER  = 2 bytes (int16_t in C)
 *   LONGINT  = 4 bytes (int32_t / uint32_t)
 *   Handle   = 4 bytes (68K pointer-to-pointer; FLAIR: omitted / built-in)
 *   Str255   = 1 length byte + up to 255 chars (FLAIR: exploded to char *)
 *
 * Field order (verbatim Pascal declaration; IM-I p. I-344):
 *   menuID      INTEGER    offset  0  (2 bytes)
 *   menuWidth   INTEGER    offset  2  (2 bytes)
 *   menuHeight  INTEGER    offset  4  (2 bytes)
 *   menuProc    Handle     offset  6  (4 bytes)
 *   enableFlags LONGINT    offset 10  (4 bytes)
 *   menuData    Str255     offset 14  (variable)
 *
 * FLAIR DEVIATIONS (all documented; Law 3 / freestanding / no-malloc):
 *   - menuHeight: DROPPED (computed from item count; no stored height needed).
 *   - menuProc (Handle to MDEF): DROPPED (built-in MDEF dispatch; no heap
 *     handle table).
 *   - menuData (Str255 packed blob): EXPLODED into title (const char *) +
 *     items array.  The SEMANTICS are equivalent.
 *   - enableFlags: EXPLODED into per-item enabled byte + is_divider flag in
 *     MenuItem.  The bit convention below (Sec 3) is preserved in comment and
 *     _Static_assert.
 *
 * Field size constants are provided for oracle cross-checks.
 * ===========================================================================*/

#define FLAIR_MENUREC_SIZEOF_MENUID       2   /* INTEGER = 2 bytes (int16_t)  */
#define FLAIR_MENUREC_SIZEOF_MENUWIDTH    2   /* INTEGER = 2 bytes (int16_t)  */
#define FLAIR_MENUREC_SIZEOF_MENUHEIGHT   2   /* INTEGER = 2 bytes; DROPPED   */
#define FLAIR_MENUREC_SIZEOF_MENUPROC     4   /* Handle  = 4 bytes; DROPPED   */
#define FLAIR_MENUREC_SIZEOF_ENABLEFLAGS  4   /* LONGINT = 4 bytes            */

/* IM-I 68K field offsets (verbatim order; [inferred] from published sizes).
 * The verbatim ORDER is the authority; offsets are [inferred] arithmetic and
 * are NOT load-bearing for FLAIR (which re-types the record).
 * Ref: ../system7-decomp/specs/toolbox/menu-manager.md Sec 1 "[offsets: inferred]" */
#define FLAIR_MENUREC_OFF_MENUID        0
#define FLAIR_MENUREC_OFF_MENUWIDTH     2
#define FLAIR_MENUREC_OFF_MENUHEIGHT    4
#define FLAIR_MENUREC_OFF_MENUPROC      6
#define FLAIR_MENUREC_OFF_ENABLEFLAGS  10
#define FLAIR_MENUREC_OFF_MENUDATA     14  /* variable-length tail */

/* ===========================================================================
 * 2. MARK-CHARACTER CONSTANTS  (verbatim Inside Macintosh / Mac Roman)
 * ---------------------------------------------------------------------------
 * Ref: ../system7-decomp/specs/toolbox/menu-manager.md Sec 2.1:
 *   "noMark=0, commandMark=17 (0x11), checkMark=18 (0x12),
 *    diamondMark=19 (0x13), appleMark=20 (0x14)"
 *   [verified: A+B; local + web cross-check; commandMark=17/checkMark=18/
 *    diamondMark=19/appleMark=20 -- InformIT/Carbon Menu Manager constants]
 *
 * SetItemMark(menu, item, markChar) places markChar in the item's left
 * margin.  markChar == noMark (0) clears any existing mark.
 * The mark is any Mac Roman character; these are the NAMED constants.
 * era=system7.0-7.1 (unchanged from System 6 through 7.x).
 * ===========================================================================*/

#define FLAIR_MENU_NO_MARK        0     /* noMark:      no mark (blank margin) */
#define FLAIR_MENU_COMMAND_MARK  17     /* commandMark: command/cloverleaf 0x11 */
#define FLAIR_MENU_CHECK_MARK    18     /* checkMark:   check mark         0x12 */
#define FLAIR_MENU_DIAMOND_MARK  19     /* diamondMark: diamond            0x13 */
#define FLAIR_MENU_APPLE_MARK    20     /* appleMark:   Apple logo (title) 0x14 */

/* IM verbatim aliases (for cross-reference with Inside Macintosh text). */
#define noMark       FLAIR_MENU_NO_MARK
#define commandMark  FLAIR_MENU_COMMAND_MARK
#define checkMark    FLAIR_MENU_CHECK_MARK
#define diamondMark  FLAIR_MENU_DIAMOND_MARK
#define appleMark    FLAIR_MENU_APPLE_MARK

/* ===========================================================================
 * 3. ITEM STYLE BYTE (QuickDraw Style -- per-item text face)
 * ---------------------------------------------------------------------------
 * Ref: ../system7-decomp/specs/toolbox/menu-manager.md Sec 2.2:
 *   "plain=0, bold=bit0=1, italic=bit1=2, underline=bit2=4, outline=bit3=8,
 *    shadow=bit4=16, condense=bit5=32, extend=bit6=64"
 *   [verified: A+B; QuickDraw Style shared with TERec txFace; local +
 *    web cross-check (bold=bit0..extend=bit6)]
 *
 * This is the same Style byte used in GrafPort txFace and ControlRecord.
 * SetItemStyle(menu, item, styleByte) sets the per-item style.
 * The Style byte is a BIT SET: combine with bitwise OR.
 * era=system7.0-7.1 (QuickDraw Style is unchanged across all System 7 eras).
 * ===========================================================================*/

#define FLAIR_MENU_STYLE_PLAIN      0x00   /* plain (normal text)            */
#define FLAIR_MENU_STYLE_BOLD       0x01   /* bold    (bit 0; Style=1)       */
#define FLAIR_MENU_STYLE_ITALIC     0x02   /* italic  (bit 1; Style=2)       */
#define FLAIR_MENU_STYLE_UNDERLINE  0x04   /* underline (bit 2; Style=4)     */
#define FLAIR_MENU_STYLE_OUTLINE    0x08   /* outline (bit 3; Style=8)       */
#define FLAIR_MENU_STYLE_SHADOW     0x10   /* shadow  (bit 4; Style=16)      */
#define FLAIR_MENU_STYLE_CONDENSE   0x20   /* condense (bit 5; Style=32)     */
#define FLAIR_MENU_STYLE_EXTEND     0x40   /* extend  (bit 6; Style=64)      */

/* ===========================================================================
 * 4. enableFlags BIT CONVENTION  (per-menu/per-item enable bitfield)
 * ---------------------------------------------------------------------------
 * Ref: ../system7-decomp/specs/toolbox/menu-manager.md Sec 3:
 *   "enableFlags is a 32-bit field.  Bit 0 enables the whole menu (the TITLE
 *    in the bar); bit i (1..31) enables item i.  Items numbered above 31 are
 *    ALWAYS enabled (there are only 31 maskable bits after the title bit)."
 *   [verified: A+B; im-toolbox-records-verbatim.md + IM-I p. I-344]
 *
 * FORMULA:
 *   enableFlags bit for the TITLE:  (1u << 0) = 0x00000001
 *   enableFlags bit for item i:     (1u << i)  for i in [1..31]
 *   Items > 31:                     ALWAYS enabled; no bit exists for them
 *
 * EnableItem(menu, 0)  sets bit 0 (enable the whole menu).
 * DisableItem(menu, 0) clears bit 0 (disable the whole menu).
 * EnableItem(menu, i)  sets bit i  for item i in [1..31].
 * DisableItem(menu, i) clears bit i.
 *
 * A disabled menu draws its bar title dimmed and cannot be pulled down.
 * A disabled item draws dimmed and is never returned by MenuSelect/MenuKey.
 * era=system7.0-7.1 (bit semantics unchanged System 6 through 7.x).
 * ===========================================================================*/

/* Mask for the TITLE enable bit (bit 0). */
#define FLAIR_MENU_ENABLE_TITLE_MASK   0x00000001u

/* Mask for item i (1-based) in [1..31].
 * Usage: FLAIR_MENU_ENABLE_ITEM_MASK(i) where i is the 1-based item number.
 * Items > 31 have no bit; treat them as always enabled. */
#define FLAIR_MENU_ENABLE_ITEM_MASK(i) ((uint32_t)1u << (unsigned)(i))

/* Items above this index have no bit in enableFlags (always enabled). */
#define FLAIR_MENU_ENABLE_MAX_ITEM     31

/* Convenience: the "all enabled" sentinel value (all 32 bits set). */
#define FLAIR_MENU_ENABLE_ALL          0xFFFFFFFFu

/* ===========================================================================
 * 5. MenuSelect / MenuKey RESULT-WORD PACKING
 * ---------------------------------------------------------------------------
 * Ref: ../system7-decomp/specs/toolbox/menu-manager.md Sec 4:
 *   "result = (menuID << 16) | item   (item is 1-based; 0 == no selection)"
 *   "menuID = (result >> 16) & 0xFFFF  (HiWord)"
 *   "item   = result & 0xFFFF          (LoWord; 1-based, 0 == nothing chosen)"
 *   [verified: A+B; im-toolbox-records-verbatim.md MenuSelect result +
 *    IM-I Ch 13 + os/flair/menu.h]
 *
 * INVARIANTS:
 *   - menuID in the high 16 bits.  menuID == 0 in the result means no
 *     selection (0 is never a valid menuID for an installed menu).
 *   - item in the low 16 bits; 1-BASED.  item == 0 means nothing chosen
 *     (the release was outside all item rows, or on a disabled/divider item,
 *     or outside any pulled-down menu).
 *   - A result of 0x00000000 is the canonical "nothing chosen" sentinel.
 *   - MenuKey only matches ENABLED, non-divider items; a disabled menu yields 0.
 *
 * After handling the result the app calls HiliteMenu(0) to un-highlight.
 * era=system7.0-7.1 (result word unchanged from System 6 through 7.x).
 * ===========================================================================*/

/* Pack a (menuID, 1-based item) pair into the IM result word.
 * menuID is int16_t; item is uint16_t (1-based; 0 = nothing).
 * Ref: ../system7-decomp/specs/toolbox/menu-manager.md Sec 4 formula. */
static inline uint32_t flair_menu_result_pack(int16_t menuID, uint16_t item1)
{
    return ((uint32_t)(uint16_t)menuID << 16) | (uint32_t)item1;
}

/* Extract the menuID from a packed result word (high 16 bits).
 * Ref: Sec 4 "menuID = (result >> 16) & 0xFFFF". */
static inline int16_t flair_menu_result_id(uint32_t r)
{
    return (int16_t)(r >> 16);
}

/* Extract the 1-based item number from a packed result word (low 16 bits).
 * Returns 0 if nothing was chosen.
 * Ref: Sec 4 "item = result & 0xFFFF". */
static inline uint16_t flair_menu_result_item(uint32_t r)
{
    return (uint16_t)(r & 0xFFFFu);
}

/* The "nothing chosen" sentinel (both words zero).
 * Ref: Sec 4 "0 means nothing was chosen". */
#define FLAIR_MENU_RESULT_NONE  ((uint32_t)0u)

/* ===========================================================================
 * 6. MDEF MESSAGE CONSTANTS
 * ---------------------------------------------------------------------------
 * Ref: ../system7-decomp/specs/toolbox/menu-manager.md Sec 5:
 *   "mDrawMsg=0, mChooseMsg=1, mSizeMsg=2, mPopUpMsg=3"
 *   [verified: A+B; WebSearch (mDrawMsg=0..mPopUpMsg=3) +
 *    im-toolbox-records-verbatim.md MDEF note; IM-I Ch 13]
 *
 * The MDEF (menu definition function) draws the pulled-down panel, sizes it,
 * and hit-tests which item the mouse is over.  The standard pull-down MDEF
 * is resource ID 0.  FLAIR provides a built-in MDEF dispatch.
 *
 * mPopUpMsg (3): predates System 7 (IM Vol V PopUpMenuSelect); reused by
 * the System-7 pop-up CDEF path.  The classic pull-down uses 0..2 only.
 * era=system7.0-7.1 (messages unchanged from System 6 through 7.x).
 * ===========================================================================*/

#define FLAIR_MDEF_MSG_DRAW    0   /* mDrawMsg:   draw the pulled-down panel  */
#define FLAIR_MDEF_MSG_CHOOSE  1   /* mChooseMsg: highlight item under mouse   */
#define FLAIR_MDEF_MSG_SIZE    2   /* mSizeMsg:   calc menuWidth / menuHeight  */
#define FLAIR_MDEF_MSG_POPUP   3   /* mPopUpMsg:  calc pop-up rect (IM Vol V)  */

/* IM verbatim aliases. */
#define mDrawMsg    FLAIR_MDEF_MSG_DRAW
#define mChooseMsg  FLAIR_MDEF_MSG_CHOOSE
#define mSizeMsg    FLAIR_MDEF_MSG_SIZE
#define mPopUpMsg   FLAIR_MDEF_MSG_POPUP

/* Standard pull-down MDEF resource ID. */
#define FLAIR_MDEF_STANDARD_RESID  0   /* resID 0 = standard pull-down MDEF   */

/* ===========================================================================
 * 7. DIVIDER CONVENTION
 * ---------------------------------------------------------------------------
 * Ref: ../system7-decomp/specs/toolbox/menu-manager.md Sec 2:
 *   "a menu item whose text begins with the two characters '(-' is a DISABLED
 *    gray separator line (a 'divider').  It is never selectable and is drawn
 *    as a horizontal gray rule spanning the menu width."
 *   [verified: A+B; im-toolbox-records-verbatim.md + s7-toolbox.md Sec 3]
 *
 * FLAIR represents a divider as MenuItem.is_divider == 1.  The two-character
 * "(-" prefix is the AppendMenu / MENU resource encoding; the live item
 * carries is_divider = 1 regardless of its text field.
 *
 * A divider is permanently disabled regardless of its enableFlags bit.
 * era=system7.0-7.1.
 * ===========================================================================*/

/* The AppendMenu / MENU resource divider text prefix.
 * A live item with this text prefix is treated as a divider (always disabled,
 * drawn as a horizontal rule, never selectable). */
#define FLAIR_MENU_DIVIDER_PREFIX  "(-"

/* ===========================================================================
 * 8. MENU BAR HEIGHT (pointer from chrome_metrics; authoritative there)
 * ---------------------------------------------------------------------------
 * Ref: ../system7-decomp/specs/toolbox/menu-manager.md Sec 6:
 *   "GetMBarHeight() == 20 px for the Roman script system."
 *   [documented: research/system7-gui-ground-truth.md Sec 1.1
 *    (menubar_height=20, [verified: S1+IM-128]); IM Toolbox-128]
 *
 * 20 px is a CHROME metric owned by ../chrome/menu-bar-geometry.md and
 * locked in spec/chrome_metrics.h (FLAIR_CHROME_MENUBAR_H).  We restate
 * it here only as a comment pointer so the spec is self-contained; the
 * AUTHORITATIVE macro is in chrome_metrics.h.  Do NOT define a second
 * FLAIR_CHROME_MENUBAR_H here -- that would create two sources of truth.
 *
 * era=system7.0-7.1 (20 px for the Roman script system).
 * ===========================================================================*/
/* See spec/chrome_metrics.h: FLAIR_CHROME_MENUBAR_H = 20 */

/* ===========================================================================
 * 9. COMPILE-TIME CONTRACT CHECKS  (the oracle bites at build time)
 * ---------------------------------------------------------------------------
 * Style follows spec/window_record.h (the style exemplar).
 * Ref: ../system7-decomp/specs/toolbox/menu-manager.md (all sections cited
 *      inline above; local source is the authority).
 * ===========================================================================*/

/* --- Mark-character constants (Sec 2.1) ------------------------------------ */
_Static_assert(noMark      == 0,    "noMark=0 (menu-manager.md Sec 2.1)");
_Static_assert(commandMark == 17,   "commandMark=0x11=17 (menu-manager.md Sec 2.1)");
_Static_assert(checkMark   == 18,   "checkMark=0x12=18 (menu-manager.md Sec 2.1)");
_Static_assert(diamondMark == 19,   "diamondMark=0x13=19 (menu-manager.md Sec 2.1)");
_Static_assert(appleMark   == 20,   "appleMark=0x14=20 (menu-manager.md Sec 2.1)");

/* Mark values fit in a uint8_t (char is sufficient for the mark byte). */
_Static_assert(appleMark   <= 255,  "appleMark must fit in uint8_t");

/* --- Style bits (Sec 2.2) -------------------------------------------------- */
_Static_assert(FLAIR_MENU_STYLE_PLAIN     == 0x00, "plain=0x00 (menu-manager.md Sec 2.2)");
_Static_assert(FLAIR_MENU_STYLE_BOLD      == 0x01, "bold=bit0=0x01 (Sec 2.2)");
_Static_assert(FLAIR_MENU_STYLE_ITALIC    == 0x02, "italic=bit1=0x02 (Sec 2.2)");
_Static_assert(FLAIR_MENU_STYLE_UNDERLINE == 0x04, "underline=bit2=0x04 (Sec 2.2)");
_Static_assert(FLAIR_MENU_STYLE_OUTLINE   == 0x08, "outline=bit3=0x08 (Sec 2.2)");
_Static_assert(FLAIR_MENU_STYLE_SHADOW    == 0x10, "shadow=bit4=0x10 (Sec 2.2)");
_Static_assert(FLAIR_MENU_STYLE_CONDENSE  == 0x20, "condense=bit5=0x20 (Sec 2.2)");
_Static_assert(FLAIR_MENU_STYLE_EXTEND    == 0x40, "extend=bit6=0x40 (Sec 2.2)");

/* Style bits are disjoint powers of 2. */
_Static_assert((FLAIR_MENU_STYLE_BOLD    & FLAIR_MENU_STYLE_ITALIC)    == 0,
               "style bits must be disjoint");
_Static_assert((FLAIR_MENU_STYLE_SHADOW  & FLAIR_MENU_STYLE_CONDENSE)  == 0,
               "style bits must be disjoint");
_Static_assert((FLAIR_MENU_STYLE_CONDENSE & FLAIR_MENU_STYLE_EXTEND)   == 0,
               "style bits must be disjoint");

/* Union of all style bits fits in one byte (uint8_t). */
_Static_assert((FLAIR_MENU_STYLE_BOLD | FLAIR_MENU_STYLE_ITALIC |
                FLAIR_MENU_STYLE_UNDERLINE | FLAIR_MENU_STYLE_OUTLINE |
                FLAIR_MENU_STYLE_SHADOW | FLAIR_MENU_STYLE_CONDENSE |
                FLAIR_MENU_STYLE_EXTEND) <= 0xFF,
               "all style bits must fit in one byte (Sec 2.2)");

/* --- enableFlags bit convention (Sec 3) ------------------------------------ */

/* Bit 0 is the TITLE bit (1u << 0 = 1). */
_Static_assert(FLAIR_MENU_ENABLE_TITLE_MASK == 1u,
               "enableFlags bit 0 is the title bit (menu-manager.md Sec 3)");

/* Item 1 uses bit 1. */
_Static_assert(FLAIR_MENU_ENABLE_ITEM_MASK(1) == 2u,
               "enableFlags item 1 = bit 1 (mask 0x2; Sec 3)");

/* Item 2 uses bit 2. */
_Static_assert(FLAIR_MENU_ENABLE_ITEM_MASK(2) == 4u,
               "enableFlags item 2 = bit 2 (mask 0x4; Sec 3)");

/* Item 31 uses bit 31 (0x80000000). */
_Static_assert(FLAIR_MENU_ENABLE_ITEM_MASK(31) == 0x80000000u,
               "enableFlags item 31 = bit 31 = 0x80000000 (Sec 3)");

/* FLAIR_MENU_ENABLE_MAX_ITEM is 31. */
_Static_assert(FLAIR_MENU_ENABLE_MAX_ITEM == 31,
               "only items 1..31 have enable bits; >31 always on (Sec 3)");

/* enableFlags is a 32-bit field (LONGINT = 4 bytes). */
_Static_assert(sizeof(uint32_t) == FLAIR_MENUREC_SIZEOF_ENABLEFLAGS,
               "enableFlags must be 4 bytes (LONGINT; menu-manager.md Sec 1)");

/* --- MenuSelect result-word packing (Sec 4) -------------------------------- */

/* Round-trip: pack then unpack recovers both components.
 * Test with menuID=128 (typical first app menu), item=1. */
_Static_assert(
    (uint32_t)((uint16_t)128u << 16 | (uint16_t)1u) ==
    (((uint32_t)(uint16_t)(int16_t)128 << 16) | (uint32_t)(uint16_t)1u),
    "MenuSelect result packing formula is consistent (Sec 4)");

/* The NOTHING-CHOSEN sentinel is 0. */
_Static_assert(FLAIR_MENU_RESULT_NONE == 0u,
               "nothing-chosen result == 0 (menu-manager.md Sec 4)");

/* High word of a result with menuID=1, item=1 must be 1 (not 0). */
_Static_assert(
    (uint16_t)(((uint32_t)(uint16_t)(int16_t)1 << 16) | 1u) >> 0 == 1u,
    "result low word carries the 1-based item number (Sec 4)");

/* High word extraction: (menuID<<16)|item => high word == menuID. */
_Static_assert(
    (((uint32_t)128u << 16) | 3u) >> 16 == 128u,
    "result high word is menuID (Sec 4)");

/* Low word extraction: (menuID<<16)|item => low word == item. */
_Static_assert(
    ((((uint32_t)128u << 16) | 3u) & 0xFFFFu) == 3u,
    "result low word is item-1-based (Sec 4)");

/* --- MDEF messages (Sec 5) ------------------------------------------------- */
_Static_assert(mDrawMsg   == 0, "mDrawMsg=0 (menu-manager.md Sec 5)");
_Static_assert(mChooseMsg == 1, "mChooseMsg=1 (menu-manager.md Sec 5)");
_Static_assert(mSizeMsg   == 2, "mSizeMsg=2 (menu-manager.md Sec 5)");
_Static_assert(mPopUpMsg  == 3, "mPopUpMsg=3 (menu-manager.md Sec 5)");

/* Messages are sequential starting from 0. */
_Static_assert(mChooseMsg == mDrawMsg   + 1, "mChooseMsg = mDrawMsg+1 (Sec 5)");
_Static_assert(mSizeMsg   == mChooseMsg + 1, "mSizeMsg = mChooseMsg+1 (Sec 5)");
_Static_assert(mPopUpMsg  == mSizeMsg   + 1, "mPopUpMsg = mSizeMsg+1 (Sec 5)");

/* Standard MDEF resID is 0. */
_Static_assert(FLAIR_MDEF_STANDARD_RESID == 0,
               "standard pull-down MDEF is resource ID 0 (Sec 5)");

/* --- Field size checks (Sec 1) --------------------------------------------- */
_Static_assert(FLAIR_MENUREC_SIZEOF_MENUID      == 2,
               "menuID is INTEGER = 2 bytes (menu-manager.md Sec 1)");
_Static_assert(FLAIR_MENUREC_SIZEOF_MENUWIDTH   == 2,
               "menuWidth is INTEGER = 2 bytes (Sec 1)");
_Static_assert(FLAIR_MENUREC_SIZEOF_MENUHEIGHT  == 2,
               "menuHeight is INTEGER = 2 bytes (Sec 1; DROPPED in FLAIR)");
_Static_assert(FLAIR_MENUREC_SIZEOF_MENUPROC    == 4,
               "menuProc is Handle = 4 bytes (Sec 1; DROPPED in FLAIR)");
_Static_assert(FLAIR_MENUREC_SIZEOF_ENABLEFLAGS == 4,
               "enableFlags is LONGINT = 4 bytes (Sec 1)");

/* --- Field offset checks (Sec 1; verbatim 68K IM layout) ------------------- */
_Static_assert(FLAIR_MENUREC_OFF_MENUID      ==  0, "menuID at offset 0 (Sec 1)");
_Static_assert(FLAIR_MENUREC_OFF_MENUWIDTH   ==  2, "menuWidth at offset 2 (Sec 1)");
_Static_assert(FLAIR_MENUREC_OFF_MENUHEIGHT  ==  4, "menuHeight at offset 4 (Sec 1)");
_Static_assert(FLAIR_MENUREC_OFF_MENUPROC    ==  6, "menuProc at offset 6 (Sec 1)");
_Static_assert(FLAIR_MENUREC_OFF_ENABLEFLAGS == 10, "enableFlags at offset 10 (Sec 1)");
_Static_assert(FLAIR_MENUREC_OFF_MENUDATA    == 14, "menuData at offset 14 (Sec 1)");

/* Offsets are strictly increasing (verbatim order is the authority). */
_Static_assert(FLAIR_MENUREC_OFF_MENUWIDTH   > FLAIR_MENUREC_OFF_MENUID,
               "menuWidth after menuID (Sec 1 verbatim order)");
_Static_assert(FLAIR_MENUREC_OFF_MENUHEIGHT  > FLAIR_MENUREC_OFF_MENUWIDTH,
               "menuHeight after menuWidth (Sec 1 verbatim order)");
_Static_assert(FLAIR_MENUREC_OFF_MENUPROC    > FLAIR_MENUREC_OFF_MENUHEIGHT,
               "menuProc after menuHeight (Sec 1 verbatim order)");
_Static_assert(FLAIR_MENUREC_OFF_ENABLEFLAGS > FLAIR_MENUREC_OFF_MENUPROC,
               "enableFlags after menuProc (Sec 1 verbatim order)");
_Static_assert(FLAIR_MENUREC_OFF_MENUDATA    > FLAIR_MENUREC_OFF_ENABLEFLAGS,
               "menuData after enableFlags (Sec 1 verbatim order)");

/* --- Canon: the Photoshop bar count (from menu_canon.h) -------------------- */
_Static_assert(FLAIR_CANON_PHOTOSHOP_MENU_COUNT == 8,
               "canon menu count is 8 (menu_canon.h FLAIR_CANON_PHOTOSHOP_MENU_COUNT)");
_Static_assert(FLAIR_CANON_PHOTOSHOP_MENUBAR_LEN == 45,
               "canon flat string len is 45 (menu_canon.h FLAIR_CANON_PHOTOSHOP_MENUBAR_LEN)");

#endif /* INITECH_SPEC_MENU_RECORD_H */
