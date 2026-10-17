/*
 * os/flair/finder_info.h -- the R3.4c Finder GET INFO window (THE ARTIFACT).
 *
 * beads: initech-p6st (GUI remediation R3.4, reslice 3/3 of initech-tdnl.11:
 *        File > Get Info / Cmd-I as a single-instance Finder-owned window).
 *
 * WHAT A USER SEES (Law 4; ../system7-decomp/goldens/captures/
 * s8_get_info_doc.png + s8_get_info_folder.png, minted under Basilisk II on
 * Mac OS 8.1 for this bead -- ../system7-decomp/re/mint-results-011.md):
 *   a titled window "<NAME> Info" with a CLOSE box and a COLLAPSE box, no
 *   zoom box, no grow box, NO scroll bars (the Platinum noGrowDocProc
 *   variant, spec/window_record.h = 4), the Platinum body rails on three
 *   sides, the E7 dialog face; inside, top-left, the item's 32x32 icon with
 *   its name beside it; then a column of BOLD Geneva 9 labels right-aligned
 *   to one edge with plain Geneva 9 values beside them:
 *       Kind:      Size:      Where:      Created:    Modified:   Version:
 *   then "Comments:" over an empty white framed box, and (files) the
 *   "Locked" and "Stationery Pad" checkboxes along the bottom. Folders have
 *   no Version row and no checkboxes, so their window is shorter.
 *
 * THE FIELDS (design F3.5, the LOCKED V1 set, and the capture's spellings):
 *   Kind      finder_win_kind_text: "folder" / "application program" /
 *             "Application document" (the list view's strings).
 *   Size      "<K>K on disk (<bytes> bytes)" -- K = ceil(on-disk / 1024),
 *             the on-disk bytes from the binding's CHAIN WALK (clusters x
 *             cluster size), never from the size field; "zero K" when 0
 *             (s8_get_info_folder.png "zero K on disk (0 bytes), for 0
 *             items"). Folders add ", for <n> items" and sum their listed
 *             files' sizes and chains ONE level down (the 8.1 Finder sums
 *             recursively; V1 does not -- stated).
 *   Where     "<VOLUME>:<chain>" -- the volume's label and the window's
 *             folder chain in the HFS spelling ("INITECH:", "INITECH:APPS:"),
 *             finder_win_path.
 *   Created / Modified
 *             BOTH the ONE write stamp a DOS 3.3 directory entry carries
 *             (there is no creation stamp before Win95's dirent extension;
 *             the period window has two rows and both show the stamp the
 *             volume stores -- never a synthesized one).
 *   THE DATE HONESTY RULE (F3.5): the text is the DECODED on-disk packed
 *             value, full stop. A calendar-valid stamp reads the way the 8.1
 *             Finder spells its own: "Fri, Jan 1, 1904, 12:00 am"
 *             (weekday, month name, day, year, 12-hour time, lowercase
 *             am/pm -- s8_get_info_doc.png, where 1904-01-01 IS the zero
 *             HFS stamp shown raw). A FAT stamp of 0x0000 decodes to month 0
 *             / day 0 / 1980, which is not a calendar date: it reads RAW and
 *             numeric, "0/0/1980, 12:00 am", exactly as DOS DIR prints its
 *             fields -- never promoted to "Jan 1, 1980" (the mutant
 *             FINDER_INFO_MUT_DATE_SYNTH does that and is RED).
 *   Version   "n/a" (files; the capture's own text for a plain document).
 *   Comments  the box, empty and inert (no storage home in V1 -- F3.5).
 *   Locked / Stationery Pad
 *             drawn DISABLED (F3.5 "grayed"); the READONLY mapping is the
 *             noted V1.5 candidate.
 *
 * SINGLE INSTANCE: one window; a second Get Info REPLACES it (lane J's
 * ruling on bead initech-tdnl.11). The content is a SNAPSHOT at open.
 *
 * GEOMETRY (content-relative; the capture's content origin is (13,47) on a
 * one-bar Mac screen, i.e. frame (7,25); InitechOS has two bars, so the
 * frame sits 20 px lower at (7,45) -- the same 5 px below its bar):
 *   icon sprite at (35,7); name pen x 77, cap top 18;
 *   label column right edge 71 (exclusive), value pen x 76;
 *   row cap tops: Kind 52, Size 68, Where 100, Created 145, Modified 162,
 *   Version 179, Comments 210 (file) / 188 (folder);
 *   comments box x [8,279), y [222,290) (file) / [200,262) (folder);
 *   checkboxes y 298, 12x12: Locked box x 8 + text pen 25, Stationery box
 *   x 176 + text pen 192;
 *   frame 299 x 346 (file) / 299 x 298 (folder).
 * Every number above was measured off the two captures by hand
 * (re/mint-results-011.md Sec 2) and is graded by tools/
 * ppm_flair_dupinfo_check.c against the capture pixels independently.
 *
 * MUTANT KNOBS (Rule 6; -D on finder_info.c only, never a real build):
 *   FINDER_INFO_MUT_TWO_INSTANCES -- a second Get Info opens a second window.
 *   FINDER_INFO_MUT_DATE_SYNTH    -- a zero stamp reads "Tue, Jan 1, 1980".
 *   FINDER_INFO_MUT_LABEL_SHIFT   -- the labels sit 1 px right.
 *   FINDER_INFO_MUT_SIZE_FROM_FIELD -- "on disk" is ceil(size/512)*512 from
 *                                      the size field, not the chain walk.
 *
 * Ref: docs/design/GUI-remediation-R3-finder-design.md F3.5 (fields, the
 *        date honesty rule), F5.3 (get_info_window); bead initech-tdnl.11
 *        NOTES (single-instance, reopening replaces);
 *      ../system7-decomp/re/mint-results-011.md; spec/window_record.h
 *        (noGrowDocProc = 4); os/flair/text.h (bold = the 1-px smear,
 *        golden-verified against the s7_get_info.png labels).
 *      CLAUDE.md Law 1, Law 2, Law 3, Law 4, Rule 2, Rule 6, Rule 11, Rule 12.
 */
#ifndef INITECH_OS_FLAIR_FINDER_INFO_H
#define INITECH_OS_FLAIR_FINDER_INFO_H

#include <stdint.h>

#include "finder_windows.h"

/* Screen frame (InitechOS two-bar desktop). */
#define FINDER_INFO_LEFT        7
#define FINDER_INFO_TOP        45
#define FINDER_INFO_W         299
#define FINDER_INFO_H_FILE    346
#define FINDER_INFO_H_FOLDER  298

/* Content-relative layout (the banner). */
#define FINDER_INFO_ICON_X     35
#define FINDER_INFO_ICON_Y      7
#define FINDER_INFO_NAME_X     77
#define FINDER_INFO_NAME_CAP   18
#define FINDER_INFO_LABEL_R    71
#define FINDER_INFO_VALUE_X    76
#define FINDER_INFO_ROW_KIND   52
#define FINDER_INFO_ROW_SIZE   68
#define FINDER_INFO_ROW_WHERE 100
#define FINDER_INFO_ROW_CREAT 145
#define FINDER_INFO_ROW_MOD   162
#define FINDER_INFO_ROW_VERS  179
#define FINDER_INFO_ROW_CMT_F 210
#define FINDER_INFO_ROW_CMT_D 188
#define FINDER_INFO_BOX_L       8
#define FINDER_INFO_BOX_R     279
#define FINDER_INFO_BOX_T_F   222
#define FINDER_INFO_BOX_B_F   290
#define FINDER_INFO_BOX_T_D   200
#define FINDER_INFO_BOX_B_D   262
#define FINDER_INFO_CHK_Y     298
#define FINDER_INFO_CHK_DIM    12
#define FINDER_INFO_LOCK_X      8
#define FINDER_INFO_LOCK_TX    25
#define FINDER_INFO_STAT_X    176
#define FINDER_INFO_STAT_TX   192

/* Text buffers. */
#define FINDER_INFO_DATE_MAX   32   /* "Wed, Sep 30, 2099, 12:59 pm" + NUL   */
#define FINDER_INFO_SIZE_MAX   48   /* "4294967295K on disk (4294967295 bytes), for 65535 items" fits */

/* Open (or REPLACE) the window for icon `idx` of window `slot`; snapshots the
 * fields through the binding. sh->info.open says whether it is up. */
void finder_info_open(finder_shell_t *sh, int slot, int idx);

/* The window's content paint, clipped (the updateEvt path). */
void finder_info_paint(finder_shell_t *sh, const bitmap_t *dst,
                       const region_t *clip);

/* The date text for one packed FAT stamp (the honesty rule). Pure. */
void finder_info_date_text(char *out, uint16_t date, uint16_t time);

/* The Size text. Pure. */
void finder_info_size_text(char *out, uint32_t on_disk, uint32_t bytes,
                           int is_folder, uint16_t items);

/* finder_info_is / finder_info_close are declared in finder_windows.h Sec 13
 * (the shell's event proc and Close Window reach them from there). */

#endif /* INITECH_OS_FLAIR_FINDER_INFO_H */
