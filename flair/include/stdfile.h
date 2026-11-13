/*
 * os/flair/stdfile.h -- FLAIR Standard File (SFGetFile): the shell-owned
 * Open-file modal's navigate/select/return logic.
 *
 * beads: initech-gymo (FLAIR Phase 4.5 -- SFGetFile first cut).
 *
 * LAW 1 ACQUISITION GATE (committee flag): there is NO SFGetFile spec in the
 * decomp corpus, so the model is GROUNDED from Inside Macintosh + the LOCAL
 * FLAIR building blocks. Cited sources:
 *
 *   THE MODEL -- Inside Macintosh, Volume I, "The Standard File Package"
 *   (IM-I, the Standard File chapter; SFGetFile / SFPutFile / SFReply):
 *     SFGetFile presents a MODAL dialog showing a SCROLLABLE list of the files
 *     in the current directory, with Open / Cancel (and Drive, in the floppy
 *     era) buttons. The user either selects a file and clicks Open, or
 *     DOUBLE-CLICKS A FOLDER to navigate into it, or clicks Cancel. The result
 *     is an SFReply record:
 *        SFReply = RECORD
 *          good:    BOOLEAN;   { TRUE iff the user confirmed (Open), not Cancel }
 *          copy:    BOOLEAN;
 *          fType:   OSType;
 *          vRefNum: INTEGER;
 *          version: INTEGER;
 *          fName:   Str255     { the chosen file's name }
 *        END;
 *     FOLDER-ON-OPEN RULE (IM-I): the Open button NAVIGATES into a selected
 *     folder (it does not "return" the folder); it RETURNS only when a FILE is
 *     chosen. We implement navigate-on-folder, return-on-file (see
 *     FlairSF_doOpen). [Ref: IM-I Standard File Package, SFGetFile + SFReply.]
 *
 *   WIN31 ACCENT -- GetOpenFileName (Windows 3.1 commdlg) is BEHAVIORALLY
 *   EQUIVALENT (a modal list of the current directory + Open/Cancel, folder
 *   double-click to descend). We DO NOT import the Win API names (era ceiling):
 *   the names here are the Macintosh Standard File names.
 *
 *   ADR-0013 Sec 3.6 -- "Standard File is a shell modal helper like FILE COPY,
 *   NOT a tenant." The shell OWNS this modal (the FILE COPY box is the
 *   precedent; os/flair/dialog.h FileCopyDialog).
 *
 *   LOCAL BUILDING BLOCKS we BUILD ON (cite, do not reinvent):
 *     os/flair/list.h  -- the List Manager. The file list IS a single-column
 *       FlairList (FlairList_init / FlairLAddRow / FlairLSetCell / FlairLGetCell
 *       / FlairLClick / FlairLGetSelect / FlairLSetSelect). Cell = flair_point_t
 *       (h=col, v=row); ListBounds = rgn_rect_t.
 *     os/flair/dialog.h -- the Dialog Manager (ModalDialog; the FILE COPY
 *       shell-owned-modal precedent). The LIVE rendered ModalDialog wiring is a
 *       FOLLOW-UP (this first cut is the navigate/select/return LOGIC).
 *     os/milton/fat12.h -- the directory source ON THE REAL SYSTEM
 *       (fat12_read_root_dir + fat12_format_83 over dir_entry_t). We DO NOT call
 *       FAT here: the directory is abstracted behind a provider callback
 *       (FlairSFEnumProc) so the module is host-testable and decoupled from real
 *       FAT I/O. The real provider wraps fat12_read_root_dir + fat12_format_83;
 *       the oracle injects a hand-authored fixture.
 *
 *   CLAUDE.md Law 1 (ground truth before code -- this header IS the citation),
 *     Law 2 (the oracle is truth; the golden is INDEPENDENT, never
 *     by-construction -- test_stdfile.c), Law 3 (artifact C, freestanding,
 *     dual-compile, no libc), Rule 2 (fail loud), Rule 8 (locked first-cut
 *     caps), Rule 11 (deterministic), Rule 12 (ASCII-clean).
 *
 * ARTIFACT code: freestanding. Dual-compiles for the host oracle
 * (cc -std=c11 -Wall -Wextra -Werror) AND the kernel type-check
 * (gcc -m32 -ffreestanding -nostdlib -std=c11 -Wall -Wextra -Werror).
 * Only <stdint.h> + <stddef.h> (+ list.h, itself freestanding). No malloc, no
 * libc (a small static byte-copy/compare handles the 8.3 names; no string.h).
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_STDFILE_H
#define INITECH_OS_FLAIR_STDFILE_H

#include <stdint.h>
#include <stddef.h>

#include "list.h"   /* FlairList, Cell, rgn_rect_t, FlairL* API (-Ios/flair) */

/* ==========================================================================
 * First-cut reduction caps (LOCKED spec-data; CLAUDE.md Rule 8).
 *
 * Raise only with a beads issue + worklog note -- never a silent bump to make
 * one test pass.
 *
 *   NAME_MAX 13   : a formatted 8.3 name "NAME.EXT" + NUL = 8 + 1 + 3 + 1.
 *                   Matches FAT12_NAME83_MAX (os/milton/fat12.h).
 *   MAX_ENTRIES 32: one directory's worth of entries; pinned <= the List
 *                   Manager's row cap (FLAIR_LIST_MAX_ROWS) so every entry maps
 *                   to one FlairList row.
 *   PATH_MAX 128  : the current-directory path buffer ("/" + components).
 * ========================================================================== */
#define FLAIR_SF_NAME_MAX     13   /* 8 + '.' + 3 + NUL (== FAT12_NAME83_MAX)   */
#define FLAIR_SF_MAX_ENTRIES  32   /* entries per directory (first-cut cap)     */
#define FLAIR_SF_PATH_MAX    128   /* current-path buffer bytes (first-cut cap) */

/* Each entry occupies exactly one FlairList row, so the entry cap may not
 * exceed the List Manager's row cap (else FlairLAddRow would fail loud). */
_Static_assert(FLAIR_SF_MAX_ENTRIES <= FLAIR_LIST_MAX_ROWS,
               "SF entries must fit in FlairList rows (FLAIR_LIST_MAX_ROWS)");
/* A formatted 8.3 name must fit in one FlairList cell. */
_Static_assert(FLAIR_SF_NAME_MAX <= FLAIR_LIST_CELL_MAX,
               "SF 8.3 name must fit in one FlairList cell (FLAIR_LIST_CELL_MAX)");
/* The path buffer must hold at least "/" + a maximal 8.3 component. */
_Static_assert(FLAIR_SF_PATH_MAX >= (1 + FLAIR_SF_NAME_MAX),
               "SF path buffer must hold root '/' + one 8.3 component");

/* ==========================================================================
 * Fail-loud error codes (CLAUDE.md Rule 2). All negative; 0 == success.
 * Mirrors the os/flair/list.h convention (FLAIR_LIST_ERR_*).
 * ========================================================================== */
#define FLAIR_SF_OK            0   /* success                                   */
#define FLAIR_SF_ERR_NULL    (-1)  /* NULL pointer argument                     */
#define FLAIR_SF_ERR_PROVIDER (-2) /* provider callback returned < 0            */
#define FLAIR_SF_ERR_OVERFLOW (-3) /* provider returned count > MAX_ENTRIES     */
#define FLAIR_SF_ERR_RANGE   (-4)  /* entry index out of [0, count)             */
#define FLAIR_SF_ERR_NOTDIR  (-5)  /* navigate target is not a folder           */
#define FLAIR_SF_ERR_PATH    (-6)  /* child path would overflow PATH_MAX        */
#define FLAIR_SF_ERR_LIST    (-7)  /* an underlying FlairList op failed loud     */

/* ==========================================================================
 * FlairSFEntry -- one directory entry (the provider's unit of output).
 *
 *   name   : a NUL-terminated formatted 8.3 name, e.g. "REPORT.DBF" (the same
 *            shape fat12_format_83 produces).
 *   is_dir : 1 if this entry is a FOLDER (double-click / Open navigates into
 *            it), 0 for a regular file.
 *   size   : the file size in bytes (0 for a folder); carried for the future
 *            live list (size column) -- not used by the navigate/select logic.
 * ========================================================================== */
typedef struct FlairSFEntry {
    char     name[FLAIR_SF_NAME_MAX];  /* NUL-terminated 8.3 name               */
    uint8_t  is_dir;                   /* 1 = folder, 0 = file                  */
    uint32_t size;                     /* file size in bytes (0 for a folder)   */
} FlairSFEntry;

/* ==========================================================================
 * FlairSFReply -- the SFGetFile result (the IM-I SFReply analogue).
 *
 *   good   : 1 if Open was clicked on a FILE (confirmed); 0 if Cancel, or if
 *            Open navigated into a folder (not-yet-confirmed). [IM-I SFReply.good]
 *   fName  : the chosen entry's NUL-terminated 8.3 name (IM-I SFReply.fName).
 *   is_dir : 1 if the chosen entry is a folder, 0 if a file (FLAIR extension to
 *            make the folder-on-Open rule observable to the oracle/caller).
 * ========================================================================== */
typedef struct FlairSFReply {
    int16_t  good;                      /* 1 = Open-on-file; 0 = Cancel/navigated */
    char     fName[FLAIR_SF_NAME_MAX];  /* chosen 8.3 name (IM-I SFReply.fName)   */
    uint8_t  is_dir;                    /* 1 = folder, 0 = file                   */
} FlairSFReply;

/* ==========================================================================
 * FlairSFEnumProc -- the directory provider (DECOUPLES the module from FAT I/O).
 *
 * Fill out[0..n) with the entries of directory `path` and RETURN n (the entry
 * count), or a NEGATIVE value on error. `max` is out[]'s capacity; the provider
 * MUST NOT write more than `max` entries (return < 0 if the directory has more).
 * `user` is the caller's opaque cookie.
 *
 * On the real system the provider wraps fat12_read_root_dir + fat12_format_83
 * (os/milton/fat12.h) over the directory at `path`; the oracle injects a
 * hand-authored fixture. THIS is what makes the module host-testable (Law 2 /
 * Law 3): no FAT, no block device, no emulator in the unit test.
 * ========================================================================== */
typedef int (*FlairSFEnumProc)(const char *path, FlairSFEntry *out,
                               int16_t max, void *user);

/* ==========================================================================
 * FlairStandardFile -- the modal's navigate/select/return state.
 *
 *   list     : the single-column FlairList shown in the dialog (one row per
 *              entry; the entry's 8.3 name lives in column 0). Built ON the
 *              real List Manager (no second list type).
 *   entries  : the current directory's entries, as the provider returned them.
 *   count    : how many of entries[]/list rows are live.
 *   path     : the current-directory path, a "/"-rooted component stack
 *              (e.g. "/" or "/DATA" or "/DATA/SUB"); NUL-terminated.
 *   path_len : length of path (not counting the NUL).
 *   provider : the directory provider + its opaque user cookie.
 *   cellSize : the FlairList cell pixel size (re-applied on every re-enumerate).
 *   rView    : the FlairList display rect, pixel coords (likewise re-applied).
 * ========================================================================== */
typedef struct FlairStandardFile {
    FlairList       list;                       /* the file list (List Manager)  */
    FlairSFEntry    entries[FLAIR_SF_MAX_ENTRIES];
    int16_t         count;                      /* live entry/row count          */
    char            path[FLAIR_SF_PATH_MAX];    /* "/"-rooted current path       */
    int16_t         path_len;                   /* strlen(path)                  */
    FlairSFEnumProc provider;                   /* directory provider            */
    void           *user;                       /* provider cookie               */
    Cell            cellSize;                    /* list cell pixel size          */
    rgn_rect_t      rView;                       /* list display rect (pixels)    */
} FlairStandardFile;

/* ==========================================================================
 * API -- the SFGetFile subset (Inside Macintosh Standard File Package).
 * ========================================================================== */

/*
 * FlairSF_open -- initialize the modal at the ROOT ("/").
 *
 * Enumerates "/" via `provider` into entries[] and populates the FlairList (one
 * row per entry, the 8.3 name in column 0), binding the list to `cellSize` /
 * `rView`. Fail loud (Rule 2): NULL sf/provider -> FLAIR_SF_ERR_NULL; a provider
 * error -> FLAIR_SF_ERR_PROVIDER; count > FLAIR_SF_MAX_ENTRIES ->
 * FLAIR_SF_ERR_OVERFLOW; an underlying FlairList failure -> FLAIR_SF_ERR_LIST.
 * Returns FLAIR_SF_OK on success. Ref: IM-I SFGetFile (initial display).
 */
int FlairSF_open(FlairStandardFile *sf, FlairSFEnumProc provider, void *user,
                 Cell cellSize, rgn_rect_t rView);

/*
 * FlairSF_count -- the number of entries in the CURRENT directory.
 * Returns 0 for a NULL sf (defensive, mirroring FlairLClick's NULL guard).
 */
int16_t FlairSF_count(const FlairStandardFile *sf);

/*
 * FlairSF_select -- select list row `index` (single-selection; mirrors the List
 * Manager lOnlyOne policy: clear every row, then select `index`).
 * Defensive no-op for a NULL sf or an out-of-range index (the modal owns a
 * single live selection; an invalid index selects nothing). Ref: IM-I -- a
 * single highlighted item in the file list.
 */
void FlairSF_select(FlairStandardFile *sf, int16_t index);

/*
 * FlairSF_click -- hit-test the file list with local pixel point `localPt` (via
 * FlairLClick, which applies lOnlyOne single-selection) and select the hit row.
 * Returns the hit row index, or -1 on a miss / NULL sf.
 *
 * NOTE: `localPt` is a flair_point_t (the QuickDraw Point, h=x/v=y) -- the same
 * pixel-point type FlairLClick consumes. (The brief named "rgn_point_t"; no such
 * type exists in the codebase -- flair_point_t is THE QuickDraw Point used for
 * both Cell coords and pixel coords.) Ref: IM-I -- click selects a list item.
 */
int FlairSF_click(FlairStandardFile *sf, flair_point_t localPt);

/*
 * FlairSF_navigate -- DOUBLE-CLICK-FOLDER behavior: if entry `index` is a
 * FOLDER, push it onto the path and RE-ENUMERATE that subdirectory via the
 * provider, rebuilding the FlairList. Fail loud (Rule 2): NULL sf ->
 * FLAIR_SF_ERR_NULL; index out of range -> FLAIR_SF_ERR_RANGE; a non-folder
 * target -> FLAIR_SF_ERR_NOTDIR; a path overflow -> FLAIR_SF_ERR_PATH; a
 * provider/list error propagated. Returns FLAIR_SF_OK on success.
 * Ref: IM-I -- double-clicking a folder opens (descends into) it.
 */
int FlairSF_navigate(FlairStandardFile *sf, int16_t index);

/*
 * FlairSF_up -- pop one path component and re-enumerate the parent directory
 * (rebuilding the FlairList). A NO-OP at the root ("/"). Fail loud on a
 * provider/list error. Returns FLAIR_SF_OK on success (including the root
 * no-op). Ref: IM-I -- the pop-up directory menu / "up" navigation.
 */
int FlairSF_up(FlairStandardFile *sf);

/*
 * FlairSF_doOpen -- the Open button. Reads the CURRENT selection from the
 * FlairList (the List Manager is the source of truth) and fills `reply`:
 *
 *   FOLDER-ON-OPEN RULE (IM-I; navigate-on-folder, return-on-file):
 *     - selected entry is a FILE   -> reply.good = 1, reply.fName = name,
 *                                     reply.is_dir = 0 (CONFIRMED).
 *     - selected entry is a FOLDER -> NAVIGATE into it (FlairSF_navigate) and
 *                                     leave reply.good = 0 (not-yet-confirmed),
 *                                     reply.fName = the folder name, is_dir = 1.
 *     - nothing selected           -> reply.good = 0, reply.fName = "" (Open
 *                                     with no selection confirms nothing).
 *
 * Defensive no-op for a NULL sf/reply. Ref: IM-I SFGetFile -- Open returns on a
 * file, descends on a folder; SFReply.good / SFReply.fName.
 */
void FlairSF_doOpen(FlairStandardFile *sf, FlairSFReply *reply);

/*
 * FlairSF_doCancel -- the Cancel button: reply.good = 0, reply.fName = "",
 * is_dir = 0 (the IM-I SFReply for a cancelled SFGetFile). Defensive no-op for
 * a NULL reply. Ref: IM-I SFReply.good = FALSE on Cancel.
 */
void FlairSF_doCancel(FlairStandardFile *sf, FlairSFReply *reply);

#endif /* INITECH_OS_FLAIR_STDFILE_H */
