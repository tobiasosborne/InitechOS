/* Locked R4.4 persistence contract, initech-tdnl.18 (Rule 8).
 * Ref: store.json; period-gui-suite.md Sec 4 (eight Note Pad pages, paged
 * Scrapbook); toolbox_gate.h Sec 10/12 (DOS files, desk TEXT Scrap).
 * This is an Initech format, not Apple's resource/data fork format. */
#ifndef INITECH_DA_STORE_H
#define INITECH_DA_STORE_H
#define DA_TEXT_MAX 1024
#define DA_NOTE_PAGES 8
#define DA_SCRAP_PAGES 32
#define DA_HEADER_BYTES 16
#define DA_RECORD_BYTES 1026
#ifdef SCRAPBOOK
#define DA_SLOTS DA_SCRAP_PAGES
#define DA_PATH "SCRAPBK.DAT"
#define DA_MAGIC "SBK1"
#else
#define DA_SLOTS DA_NOTE_PAGES
#define DA_PATH "NOTEPAD.DAT"
#define DA_MAGIC "NPAD"
#endif
#define DA_FILE_BYTES (DA_HEADER_BYTES + DA_SLOTS * DA_RECORD_BYTES)
/* Composed documentProc frame. Native Note Pad/Scrapbook captures MISSING.
 * Reuse graded Platinum chrome, SimpleText (5,4) inset, Geneva9 TextEdit.
 * Fold footprint is an authored functional adaptation of the page corner
 * in period-gui-suite Sec 4, NOT claimed as native pixel fidelity. */
#define DA_WIN_L 160
#define DA_WIN_T 80
#define DA_WIN_R 480
#define DA_WIN_B 360
#define DA_VIEW_W 293
#define DA_VIEW_H 237
#define DA_TEXT_H 209
#define DA_FOLD 20
#endif
