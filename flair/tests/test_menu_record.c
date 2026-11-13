/* test_menu_record.c -- oracle for spec/menu_record.h (FLAIR P1-2).
 *
 * beads: initech-dh5k.2 (P1-2: spec/menu_record.h + host oracle).
 *
 * Ref:   spec/menu_record.h (the locked spec-data under test; all constants
 *          and invariants asserted here are grounded in menu-manager.md).
 *        ../system7-decomp/specs/toolbox/menu-manager.md (PRIMARY source;
 *          verbatim MenuInfo record, mark/style/enableFlags/result-word).
 *        spec/assets/menu_canon.h (FROZEN Photoshop bar -- CANON WARNING:
 *          do NOT "correct" to any real Photoshop version; Law 4).
 *        os/flair/menu.h (the FLAIR impl; READ-ONLY cross-check).
 *        CLAUDE.md Law 1 (ground truth before code), Law 2 (oracle is truth),
 *          Law 4 (fidelity), Rule 1 (red->green), Rule 4 (skepticism),
 *          Rule 6 (mutation-proven), Rule 11 (deterministic), Rule 12 (ASCII).
 *
 * PROPERTIES CHECKED (ordered by decisiveness):
 *
 *  P1. MARK CONSTANTS.   noMark=0, commandMark=17, checkMark=18,
 *      diamondMark=19, appleMark=20.  The mark char fits in uint8_t.
 *
 *  P2. STYLE BITS.       bold=1..extend=64; all disjoint powers of 2;
 *      full union <= 0xFF (fits in one byte).
 *
 *  P3. ENABLEFLAGS BIT CONVENTION.
 *      Bit 0 = title mask; bit i = item i (1..31); bit 31 = 0x80000000.
 *      FLAIR_MENU_ENABLE_MAX_ITEM == 31.
 *
 *  P4. MENUSELECT RESULT-WORD PACKING.
 *      pack(menuID=128, item=1) -> high word == 128, low word == 1.
 *      pack(id, 0) -> nothing-chosen for items side (menuID still present).
 *      FLAIR_MENU_RESULT_NONE == 0.
 *      pack -> unpack round-trip is lossless for diverse IDs and items.
 *
 *  P5. MDEF MESSAGES.    mDrawMsg=0..mPopUpMsg=3; sequential; standard
 *      pull-down MDEF resID == 0.
 *
 *  P6. FIELD SIZES AND OFFSETS (68K layout; verbatim order is authority).
 *      menuID=2, menuWidth=2, menuHeight=2, menuProc=4, enableFlags=4.
 *      Offsets: 0,2,4,6,10,14 in that order.
 *
 *  P7. DIVIDER PREFIX.   "(-" is the two-character divider marker.
 *
 *  P8. CANON CROSS-CHECK.  FLAIR_CANON_PHOTOSHOP_MENU_COUNT == 8;
 *      FLAIR_CANON_PHOTOSHOP_MENUBAR_LEN == 45; the flat string matches
 *      the expected literal.  (This fires the canonical canon oracle; Law 4.)
 *
 * MUTANT SMOKE TEST (Rule 6 -- included at the bottom):
 *  If MENU_REC_MUTATE_CHECKCONSTANT is defined, checkMark is forced to a
 *  wrong value (17) so the P1 check on checkMark goes RED.  The Makefile
 *  gate drives this mutant and confirms the oracle bites.
 *
 * OUTPUT FORMAT:
 *   "test-menu-record: N checks, 0 failures, green"  on success.
 *   "test-menu-record: N checks, F failures"          on failure.
 *   Exits 0 on all-green, 1 on any failure.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* The spec under test.  -Ispec needed for this include. */
#include "menu_record.h"    /* also pulls in assets/menu_canon.h */

/* The test harness.  -Iseed needed. */
#include "test_assert.h"

TEST_HARNESS();

/* ===========================================================================
 * Internal round-trip helpers (mirror the spec's inline helpers without
 * calling them so a bug in the helper does not mask itself).
 * ===========================================================================*/

/* Pack (menuID, 1-based item) the same way the spec formula says. */
static uint32_t local_pack(int16_t menuID, uint16_t item1)
{
    return ((uint32_t)(uint16_t)menuID << 16) | (uint32_t)item1;
}

/* Extract high word (menuID). */
static int16_t local_hiword(uint32_t r)
{
    return (int16_t)(r >> 16);
}

/* Extract low word (1-based item). */
static uint16_t local_loword(uint32_t r)
{
    return (uint16_t)(r & 0xFFFFu);
}

/* ===========================================================================
 * main
 * ===========================================================================*/
int main(int argc, char **argv)
{
    (void)argc; (void)argv;

#ifdef MENU_REC_MUTATE_CHECKCONSTANT
    /* Rule 6 mutant smoke: assert a value we KNOW is false (checkMark is 18,
     * not 17) so the oracle MUST report a failure and exit non-zero. If this
     * passes, the gate is decoration. (Wired by the orchestrator during Wave-1
     * grading -- the lane left this hook as a comment only.) */
    CHECK(checkMark == 17, "MUTANT: checkMark forced-wrong (must FAIL -- Rule 6)");
#endif

    /* ======================================================================
     * P1. MARK-CHARACTER CONSTANTS
     * Ref: menu-manager.md Sec 2.1; menu_record.h Sec 2.
     * ====================================================================== */

    CHECK(noMark == 0,
          "noMark == 0 (menu-manager.md Sec 2.1; FLAIR_MENU_NO_MARK)");
    CHECK(commandMark == 17,
          "commandMark == 17 = 0x11 (menu-manager.md Sec 2.1)");
    CHECK(checkMark == 18,
          "checkMark == 18 = 0x12 (menu-manager.md Sec 2.1)");
    CHECK(diamondMark == 19,
          "diamondMark == 19 = 0x13 (menu-manager.md Sec 2.1)");
    CHECK(appleMark == 20,
          "appleMark == 20 = 0x14 (menu-manager.md Sec 2.1)");

    /* Mark constants are hex-equivalent. */
    CHECK(commandMark == 0x11, "commandMark == 0x11 (hex check)");
    CHECK(checkMark   == 0x12, "checkMark   == 0x12 (hex check)");
    CHECK(diamondMark == 0x13, "diamondMark == 0x13 (hex check)");
    CHECK(appleMark   == 0x14, "appleMark   == 0x14 (hex check)");

    /* All mark constants fit in uint8_t. */
    CHECK(noMark      <= 255, "noMark fits in uint8_t");
    CHECK(commandMark <= 255, "commandMark fits in uint8_t");
    CHECK(checkMark   <= 255, "checkMark fits in uint8_t");
    CHECK(diamondMark <= 255, "diamondMark fits in uint8_t");
    CHECK(appleMark   <= 255, "appleMark fits in uint8_t");

    /* Mark constants are strictly increasing (noMark < commandMark < ... < appleMark). */
    CHECK(noMark < commandMark,  "noMark < commandMark");
    CHECK(commandMark < checkMark,  "commandMark < checkMark");
    CHECK(checkMark < diamondMark, "checkMark < diamondMark");
    CHECK(diamondMark < appleMark, "diamondMark < appleMark");

    /* ======================================================================
     * P2. ITEM STYLE BITS  (QuickDraw Style byte)
     * Ref: menu-manager.md Sec 2.2; menu_record.h Sec 3.
     * ====================================================================== */

    CHECK(FLAIR_MENU_STYLE_PLAIN     == 0x00, "STYLE_PLAIN == 0x00 (Sec 2.2)");
    CHECK(FLAIR_MENU_STYLE_BOLD      == 0x01, "STYLE_BOLD  == 0x01 (bit 0; Sec 2.2)");
    CHECK(FLAIR_MENU_STYLE_ITALIC    == 0x02, "STYLE_ITALIC == 0x02 (bit 1; Sec 2.2)");
    CHECK(FLAIR_MENU_STYLE_UNDERLINE == 0x04, "STYLE_UNDERLINE == 0x04 (bit 2; Sec 2.2)");
    CHECK(FLAIR_MENU_STYLE_OUTLINE   == 0x08, "STYLE_OUTLINE == 0x08 (bit 3; Sec 2.2)");
    CHECK(FLAIR_MENU_STYLE_SHADOW    == 0x10, "STYLE_SHADOW == 0x10 (bit 4; Sec 2.2)");
    CHECK(FLAIR_MENU_STYLE_CONDENSE  == 0x20, "STYLE_CONDENSE == 0x20 (bit 5; Sec 2.2)");
    CHECK(FLAIR_MENU_STYLE_EXTEND    == 0x40, "STYLE_EXTEND == 0x40 (bit 6; Sec 2.2)");

    /* Each style bit is a distinct power of 2. */
    CHECK((FLAIR_MENU_STYLE_BOLD & (FLAIR_MENU_STYLE_BOLD - 1)) == 0,
          "STYLE_BOLD is a power of 2");
    CHECK((FLAIR_MENU_STYLE_EXTEND & (FLAIR_MENU_STYLE_EXTEND - 1)) == 0,
          "STYLE_EXTEND is a power of 2");

    /* Style bits are disjoint. */
    CHECK((FLAIR_MENU_STYLE_BOLD & FLAIR_MENU_STYLE_ITALIC) == 0,
          "BOLD and ITALIC bits are disjoint");
    CHECK((FLAIR_MENU_STYLE_SHADOW & FLAIR_MENU_STYLE_CONDENSE) == 0,
          "SHADOW and CONDENSE bits are disjoint");

    /* Union of all style bits fits in one byte. */
    {
        uint32_t all = FLAIR_MENU_STYLE_BOLD | FLAIR_MENU_STYLE_ITALIC |
                       FLAIR_MENU_STYLE_UNDERLINE | FLAIR_MENU_STYLE_OUTLINE |
                       FLAIR_MENU_STYLE_SHADOW | FLAIR_MENU_STYLE_CONDENSE |
                       FLAIR_MENU_STYLE_EXTEND;
        CHECK(all == 0x7F, "all style bits ORed == 0x7F (fits in uint8_t; Sec 2.2)");
        CHECK(all <= 0xFF, "all style bits ORed <= 0xFF (byte-sized; Sec 2.2)");
    }

    /* ======================================================================
     * P3. ENABLEFLAGS BIT CONVENTION
     * Ref: menu-manager.md Sec 3; menu_record.h Sec 4.
     * ====================================================================== */

    /* Bit 0 = title enable. */
    CHECK(FLAIR_MENU_ENABLE_TITLE_MASK == 1u,
          "ENABLE_TITLE_MASK == 1 (bit 0; Sec 3)");

    /* Items 1..31 map to bits 1..31. */
    CHECK(FLAIR_MENU_ENABLE_ITEM_MASK(1)  == 0x00000002u,
          "item 1 -> bit 1 = 0x2 (Sec 3)");
    CHECK(FLAIR_MENU_ENABLE_ITEM_MASK(2)  == 0x00000004u,
          "item 2 -> bit 2 = 0x4 (Sec 3)");
    CHECK(FLAIR_MENU_ENABLE_ITEM_MASK(3)  == 0x00000008u,
          "item 3 -> bit 3 = 0x8 (Sec 3)");
    CHECK(FLAIR_MENU_ENABLE_ITEM_MASK(31) == 0x80000000u,
          "item 31 -> bit 31 = 0x80000000 (Sec 3)");

    /* ENABLE_MAX_ITEM is 31. */
    CHECK(FLAIR_MENU_ENABLE_MAX_ITEM == 31,
          "ENABLE_MAX_ITEM == 31 (only 31 maskable item bits; Sec 3)");

    /* ENABLE_ALL has all 32 bits set. */
    CHECK(FLAIR_MENU_ENABLE_ALL == 0xFFFFFFFFu,
          "ENABLE_ALL == 0xFFFFFFFF (Sec 4)");

    /* Title bit is NOT the same as item-1 bit. */
    CHECK((FLAIR_MENU_ENABLE_TITLE_MASK & FLAIR_MENU_ENABLE_ITEM_MASK(1)) == 0,
          "title bit and item-1 bit are disjoint (Sec 3)");

    /* enableFlags field is exactly 4 bytes (LONGINT). */
    CHECK(FLAIR_MENUREC_SIZEOF_ENABLEFLAGS == 4,
          "enableFlags sizeof == 4 (LONGINT = 4 bytes; Sec 1)");

    /* ======================================================================
     * P4. MENUSELECT / MENUKEY RESULT-WORD PACKING
     * Ref: menu-manager.md Sec 4; menu_record.h Sec 5.
     * ====================================================================== */

    /* Pack/unpack round-trip: menuID=128, item=1 */
    {
        int16_t  mid  = 128;
        uint16_t item = 1;
        uint32_t r    = local_pack(mid, item);
        CHECK(local_hiword(r) == mid,  "pack(128,1): hiword == 128 (Sec 4)");
        CHECK(local_loword(r) == item, "pack(128,1): loword == 1 (Sec 4)");
    }

    /* Pack/unpack round-trip: menuID=255, item=31 */
    {
        int16_t  mid  = 255;
        uint16_t item = 31;
        uint32_t r    = local_pack(mid, item);
        CHECK(local_hiword(r) == 255, "pack(255,31): hiword == 255 (Sec 4)");
        CHECK(local_loword(r) == 31,  "pack(255,31): loword == 31 (Sec 4)");
    }

    /* Pack/unpack round-trip: menuID=1, item=1 (minimum valid) */
    {
        uint32_t r = local_pack(1, 1);
        CHECK(local_hiword(r) == 1, "pack(1,1): hiword == 1 (Sec 4)");
        CHECK(local_loword(r) == 1, "pack(1,1): loword == 1 (Sec 4)");
    }

    /* Nothing-chosen sentinel is 0. */
    CHECK(FLAIR_MENU_RESULT_NONE == 0u,
          "MENU_RESULT_NONE == 0 (nothing chosen; Sec 4)");

    /* A packed result with item=0 has a zero low word. */
    {
        uint32_t r = local_pack(128, 0);
        CHECK(local_loword(r) == 0,
              "pack(128,0): loword==0 means no item chosen (Sec 4)");
    }

    /* High and low words are independent (the packing formula is additive). */
    {
        uint32_t r = local_pack(0x5A5A, 0xA5A5);
        CHECK(local_hiword(r) == (int16_t)0x5A5A,
              "pack: high word is preserved across arbitrary low word (Sec 4)");
        CHECK(local_loword(r) == 0xA5A5u,
              "pack: low word is preserved across arbitrary high word (Sec 4)");
    }

    /* The spec inline helpers agree with the local mirror.
     * (If menu_record.h's inlines are wrong, this fires.) */
    {
        int16_t  mid  = 200;
        uint16_t item = 7;
        uint32_t r_spec  = flair_menu_result_pack(mid, item);
        uint32_t r_local = local_pack(mid, item);
        CHECK(r_spec == r_local,
              "spec flair_menu_result_pack agrees with local formula (Sec 5)");
        CHECK(flair_menu_result_id(r_spec) == local_hiword(r_spec),
              "flair_menu_result_id matches local hiword extraction (Sec 5)");
        CHECK(flair_menu_result_item(r_spec) == local_loword(r_spec),
              "flair_menu_result_item matches local loword extraction (Sec 5)");
    }

    /* ======================================================================
     * P5. MDEF MESSAGES
     * Ref: menu-manager.md Sec 5; menu_record.h Sec 6.
     * ====================================================================== */

    CHECK(mDrawMsg   == 0, "mDrawMsg == 0 (Sec 5)");
    CHECK(mChooseMsg == 1, "mChooseMsg == 1 (Sec 5)");
    CHECK(mSizeMsg   == 2, "mSizeMsg == 2 (Sec 5)");
    CHECK(mPopUpMsg  == 3, "mPopUpMsg == 3 (Sec 5)");

    /* Sequential. */
    CHECK(mChooseMsg == mDrawMsg   + 1, "mChooseMsg sequential (Sec 5)");
    CHECK(mSizeMsg   == mChooseMsg + 1, "mSizeMsg sequential (Sec 5)");
    CHECK(mPopUpMsg  == mSizeMsg   + 1, "mPopUpMsg sequential (Sec 5)");

    /* Standard pull-down MDEF resource ID is 0. */
    CHECK(FLAIR_MDEF_STANDARD_RESID == 0,
          "standard pull-down MDEF resID == 0 (Sec 5)");

    /* ======================================================================
     * P6. FIELD SIZES AND IM 68K OFFSETS (verbatim; [inferred] arithmetic)
     * Ref: menu-manager.md Sec 1 (field table); menu_record.h Sec 1.
     * ====================================================================== */

    CHECK(FLAIR_MENUREC_SIZEOF_MENUID      == 2,
          "menuID sizeof == 2 (INTEGER; Sec 1)");
    CHECK(FLAIR_MENUREC_SIZEOF_MENUWIDTH   == 2,
          "menuWidth sizeof == 2 (INTEGER; Sec 1)");
    CHECK(FLAIR_MENUREC_SIZEOF_MENUHEIGHT  == 2,
          "menuHeight sizeof == 2 (INTEGER; Sec 1; DROPPED in FLAIR)");
    CHECK(FLAIR_MENUREC_SIZEOF_MENUPROC    == 4,
          "menuProc sizeof == 4 (Handle; Sec 1; DROPPED in FLAIR)");
    CHECK(FLAIR_MENUREC_SIZEOF_ENABLEFLAGS == 4,
          "enableFlags sizeof == 4 (LONGINT; Sec 1)");

    CHECK(FLAIR_MENUREC_OFF_MENUID      ==  0, "menuID offset == 0 (Sec 1)");
    CHECK(FLAIR_MENUREC_OFF_MENUWIDTH   ==  2, "menuWidth offset == 2 (Sec 1)");
    CHECK(FLAIR_MENUREC_OFF_MENUHEIGHT  ==  4, "menuHeight offset == 4 (Sec 1)");
    CHECK(FLAIR_MENUREC_OFF_MENUPROC    ==  6, "menuProc offset == 6 (Sec 1)");
    CHECK(FLAIR_MENUREC_OFF_ENABLEFLAGS == 10, "enableFlags offset == 10 (Sec 1)");
    CHECK(FLAIR_MENUREC_OFF_MENUDATA    == 14, "menuData offset == 14 (Sec 1)");

    /* Offsets are strictly increasing (verbatim order is the authority). */
    CHECK(FLAIR_MENUREC_OFF_MENUWIDTH   > FLAIR_MENUREC_OFF_MENUID,
          "menuWidth after menuID (verbatim order; Sec 1)");
    CHECK(FLAIR_MENUREC_OFF_MENUHEIGHT  > FLAIR_MENUREC_OFF_MENUWIDTH,
          "menuHeight after menuWidth (Sec 1)");
    CHECK(FLAIR_MENUREC_OFF_MENUPROC    > FLAIR_MENUREC_OFF_MENUHEIGHT,
          "menuProc after menuHeight (Sec 1)");
    CHECK(FLAIR_MENUREC_OFF_ENABLEFLAGS > FLAIR_MENUREC_OFF_MENUPROC,
          "enableFlags after menuProc (Sec 1)");
    CHECK(FLAIR_MENUREC_OFF_MENUDATA    > FLAIR_MENUREC_OFF_ENABLEFLAGS,
          "menuData after enableFlags (Sec 1)");

    /* Offset deltas match the field sizes (arithmetic consistency). */
    CHECK(FLAIR_MENUREC_OFF_MENUWIDTH   - FLAIR_MENUREC_OFF_MENUID
          == FLAIR_MENUREC_SIZEOF_MENUID,
          "offset gap menuID->menuWidth == sizeof(menuID) (Sec 1)");
    CHECK(FLAIR_MENUREC_OFF_MENUHEIGHT  - FLAIR_MENUREC_OFF_MENUWIDTH
          == FLAIR_MENUREC_SIZEOF_MENUWIDTH,
          "offset gap menuWidth->menuHeight == sizeof(menuWidth) (Sec 1)");
    CHECK(FLAIR_MENUREC_OFF_MENUPROC    - FLAIR_MENUREC_OFF_MENUHEIGHT
          == FLAIR_MENUREC_SIZEOF_MENUHEIGHT,
          "offset gap menuHeight->menuProc == sizeof(menuHeight) (Sec 1)");
    CHECK(FLAIR_MENUREC_OFF_ENABLEFLAGS - FLAIR_MENUREC_OFF_MENUPROC
          == FLAIR_MENUREC_SIZEOF_MENUPROC,
          "offset gap menuProc->enableFlags == sizeof(menuProc) (Sec 1)");
    CHECK(FLAIR_MENUREC_OFF_MENUDATA    - FLAIR_MENUREC_OFF_ENABLEFLAGS
          == FLAIR_MENUREC_SIZEOF_ENABLEFLAGS,
          "offset gap enableFlags->menuData == sizeof(enableFlags) (Sec 1)");

    /* ======================================================================
     * P7. DIVIDER PREFIX
     * Ref: menu-manager.md Sec 2; menu_record.h Sec 7.
     * ====================================================================== */

    CHECK(FLAIR_MENU_DIVIDER_PREFIX[0] == '(',
          "divider prefix[0] == '(' (menu-manager.md Sec 2)");
    CHECK(FLAIR_MENU_DIVIDER_PREFIX[1] == '-',
          "divider prefix[1] == '-' (menu-manager.md Sec 2)");
    CHECK(FLAIR_MENU_DIVIDER_PREFIX[2] == '\0',
          "divider prefix is exactly 2 chars + NUL (Sec 2)");
    CHECK(strlen(FLAIR_MENU_DIVIDER_PREFIX) == 2,
          "divider prefix strlen == 2 (Sec 2)");

    /* ======================================================================
     * P8. CANON CROSS-CHECK (Law 4; ADR-0004 D-3 / AM-4)
     * Ref: spec/assets/menu_canon.h (the FROZEN InitechPaint bar string);
     *      menu-manager.md Sec 7 (CANON WARNING).
     *
     * CANON WARNING: do NOT "correct" the string. The combination of Layer +
     * View is historically impossible in any single real Photoshop version.
     * THAT IS THE POINT.  It is the Office Space prop chimera.  The canon
     * oracle asserts it byte-for-byte.
     * ====================================================================== */

    CHECK(FLAIR_CANON_PHOTOSHOP_MENU_COUNT == 8,
          "canon menu count == 8 (menu_canon.h; Law 4)");

    CHECK(FLAIR_CANON_PHOTOSHOP_MENUBAR_LEN == 45,
          "canon flat string length == 45 (menu_canon.h; Law 4)");

    /* Flat string byte-exact check. */
    {
        const char *expected = "File Edit Image Layer Select View Window Help";
        CHECK_STR_EQ(FLAIR_CANON_PHOTOSHOP_MENUBAR, expected,
                     "canon flat string matches exactly (Law 4 / ADR-0004 D-3)");
    }

    /* Length macro matches actual string length. */
    {
        size_t actual_len = strlen(FLAIR_CANON_PHOTOSHOP_MENUBAR);
        CHECK((int)actual_len == FLAIR_CANON_PHOTOSHOP_MENUBAR_LEN,
              "FLAIR_CANON_PHOTOSHOP_MENUBAR_LEN matches strlen (menu_canon.h)");
    }

    /* Item-by-item canon checks (the chimera signal: Layer AND View). */
    CHECK_STR_EQ(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[0], "File",
                 "canon item 0 == \"File\" (menu_canon.h; Law 4)");
    CHECK_STR_EQ(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[1], "Edit",
                 "canon item 1 == \"Edit\" (menu_canon.h; Law 4)");
    CHECK_STR_EQ(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[2], "Image",
                 "canon item 2 == \"Image\" (menu_canon.h; Law 4)");
    CHECK_STR_EQ(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[3], "Layer",
                 "canon item 3 == \"Layer\" (MUST be Layer, NOT Mode; Law 4)");
    CHECK_STR_EQ(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[4], "Select",
                 "canon item 4 == \"Select\" (menu_canon.h; Law 4)");
    CHECK_STR_EQ(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[5], "View",
                 "canon item 5 == \"View\" (MUST be View; chimera signal; Law 4)");
    CHECK_STR_EQ(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[6], "Window",
                 "canon item 6 == \"Window\" (menu_canon.h; Law 4)");
    CHECK_STR_EQ(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[7], "Help",
                 "canon item 7 == \"Help\" (menu_canon.h; Law 4)");

    /* NULL sentinel at index 8. */
    CHECK(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[FLAIR_CANON_PHOTOSHOP_MENU_COUNT]
          == (const char *)0,
          "canon items array NULL-terminated at index 8 (menu_canon.h)");

    /* Chimera signal: Layer AND View are BOTH present simultaneously.
     * This is the key canon invariant -- it is historically impossible in
     * any real Photoshop version, and that is exactly the point (Law 4). */
    CHECK(strcmp(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[3], "Layer") == 0 &&
          strcmp(FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[5], "View")  == 0,
          "chimera signal: Layer AND View both present simultaneously (canon)");

    /* ======================================================================
     * PRINT SUMMARY
     * ====================================================================== */

    if (g_fails == 0)
        printf("test-menu-record: %d checks, 0 failures, green\n", g_checks);
    else
        printf("test-menu-record: %d checks, %d failures\n",
               g_checks, g_fails);

    return (g_fails == 0) ? 0 : 1;
}
