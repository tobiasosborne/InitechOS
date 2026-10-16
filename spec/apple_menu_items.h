/*
 * spec/apple_menu_items.h -- the Apple menu's rows (LOCKED spec-data; beads
 * initech-tdnl.47 / initech-tdnl.17; CLAUDE.md Rule 8).
 *
 * GROUND TRUTH (Law 1): the real Mac OS 8.1 Apple menu,
 * ../system7-decomp/goldens/captures/s8_apple_menu.png (mint-007): row 1
 * "About This Computer..." (the MacRoman ellipsis 0xC9), a divider, then the
 * installed items of System Folder:Apple Menu Items in ALPHABETICAL order
 * (Apple System Profiler, AppleCD Audio Player, Automated Tasks, Calculator,
 * Chooser, ...). InitechOS keeps the period rule F4-3 of
 * docs/design/GUI-remediation-R3-finder-design.md: an accessory is LISTED
 * ONLY IF ITS PROGRAM IS INSTALLED on the mounted volume (omit, never gray).
 *
 * THE TABLE IS THE DATA-DRIVEN SEAM: Note Pad, Scrapbook, Key Caps join by
 * adding a row here (title, its directory, its 8.3 program) and shipping the
 * program -- no code changes. Rows stay sorted by title (the oracle
 * harness/proptest/test_apple_menu.c holds the order).
 *
 * ASCII-clean (Rule 12): the ellipsis is the octal escape \311 (0xC9).
 */
#ifndef INITECH_SPEC_APPLE_MENU_ITEMS_H
#define INITECH_SPEC_APPLE_MENU_ITEMS_H

#define APPLE_MENU_ABOUT_TEXT  "About This Computer\311"

/* { title, directory (8.3, from the volume root), program (8.3) } */
#define APPLE_MENU_DA_TABLE {                         \
    { "Calculator", "APPS", "CALC.EXE" },             \
}
#define APPLE_MENU_DA_COUNT 1

#endif /* INITECH_SPEC_APPLE_MENU_ITEMS_H */
