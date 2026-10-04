# spec/flair_trash_traces.mk -- the LOCKED input traces for the TRASH emulator
# gates: the Trash WINDOW (bead initech-tdnl.39, audit F04) and Special > Empty
# Trash (bead initech-6k12). Locked spec-data (CLAUDE.md Rule 8, Rule 11):
# deterministic, versioned, never silently edited to make a test pass.
#
# Conventions are spec/flair_file_ops_traces.mk's verbatim (QMP relative moves
# from the screen centre (320,240), y DOWN-positive, every component int8-safe
# |c| <= 100; "l1"/"l0" left button; "k<chord>" a keystroke in the mouse
# stream; every trace ends PARKED at (620,460)). Shared geometry (derived
# there, never re-read from the artifact):
#   desktop: VOLUME "INITECH" sprite (584,48) centre (600,64);
#            Trash sprite (584,404) centre (600,420).
#   ROOT window (slot 0) frame (20,60)..(380,280): README.TXT cell 0 sprite
#            (39,86) centre (55,102); APPS cell 1 sprite (107,86).
#
# THE TRASH WINDOW'S GEOMETRY, DERIVED (os/flair/finder_windows.h Sec 2/3,
# window.c CalcDocContentRect = (L+1, T+22)..(R-21, B-21)): the root window
# holds slot 0, so the Trash window takes slot 1 at the cascaded default
#     frame (20+20, 60+20) = (40,80)..(400,300)
#     content (41,102)..(379,279)
#     cell 0 sprite (41+18, 102+4) = (59,106), centre (75,122)
# titled "Trash" (FINDER_WIN_TRASH_TITLE), the window FRONT (active).
#
# THE SHARED PREFIX (= spec/flair_file_ops_traces.mk FLAIR_SVC_TRASH_SPEC's
# first 22 tokens):
#   A.  volume double-click: m100:-88, m100:-88, m80:0 -> (600,64); l1,l0,l1,l0
#       -> FINDER-OPEN-VOLUME win=0 n=2
#   B.  -> README centre (55,102): m-100:8 x3, m-100:7 x2, m-45:0
#   GO. l1 ; m100:64 x4, m100:62, m45:0 -> (600,420) ; m0:2 -> (600,422) ; l0
#       -> FINDER-TRASH name=README.TXT origin=0 ; DESKTOP-DB-SAVE n=3
#   OPEN. l1,l0,l1,l0 at (600,422), inside the Trash sprite (584..616 x
#       404..436): click 1 selects the Trash, click 2 opens the Trash window
#       -> FINDER-OPEN-TRASH win=1 n=1 singleton=0
#
# ===========================================================================
# 1. FLAIR_TRASHWIN_OPEN_SPEC -- stage README.TXT, open the Trash window, park.
#    The screendump (after FINDER-OPEN-TRASH) is graded by
#    tools/ppm_flair_disk_windows_check leg `trashwin`: the window at (40,80)
#    with its chrome, the "Trash" title centred, README.TXT's DOC strike in
#    cell 0, cells 1-3 bare.
#    PARK (600,422) -> (620,460): m20:38.
# ===========================================================================
FLAIR_TRASHWIN_OPEN_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,l1,l0,l1,l0,m20:38

# ===========================================================================
# 2. FLAIR_TRASHWIN_PUTBACK_SPEC -- the same, then PUT BACK: README.TXT is
#    dragged out of the Trash window onto the VOLUME icon (= the root).
# ===========================================================================
# Serial chain: FINDER-OPEN-VOLUME win=0 n=2 ; FINDER-TRASH name=README.TXT
#   origin=0 ; DESKTOP-DB-SAVE n=3 ; FINDER-OPEN-TRASH win=1 n=1 singleton=0 ;
#   FINDER-DROP-HILITE name=INITECH ; FINDER-MOVE name=README.TXT from=<T>
#   to=0 ; DESKTOP-DB-SAVE n=2 -- where <T> is \TRASH's first cluster AS
#   mshowfat REPORTS IT on the same image (never our FAT code's view).
# mtools: ::/README.TXT back in the root, bytes == the fixture; ::/TRASH
#   empty; \DESKTOP.DB 8 + 2*24 = 56 bytes (the origin record dropped).
# PUT BACK. (600,422) -> README in the Trash window (75,122): sum (-525,-300):
#   m-100:-60 x5, m-25:0 ; l1 ; -> the volume centre (600,64): sum (+525,-58):
#   m100:-12 x4, m100:-10, m25:0 ; m0:2 -> (600,66) ; l0.
# PARK (600,66) -> (620,460): sum (+20,+394): m5:99 x3, m5:97.
FLAIR_TRASHWIN_PUTBACK_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,l1,l0,l1,l0,m-100:-60,m-100:-60,m-100:-60,m-100:-60,m-100:-60,m-25:0,l1,m100:-12,m100:-12,m100:-12,m100:-12,m100:-10,m25:0,m0:2,l0,m5:99,m5:99,m5:99,m5:97

# ===========================================================================
# 3. SPECIAL > EMPTY TRASH (bead initech-6k12). Shared geometry (derived in
#    spec/flair_finder_cmds_traces.mk / spec/flair_disk_windows_traces.mk):
#    band-2 Special title x[142,202) -> click (170,30); Special item 2 Empty
#    Trash owns screen y [57,73) -> release at (170,65). THE ALERT (measured
#    off ../system7-decomp/goldens/captures/s8_alert_modal.png, see
#    os/milton/kmain.c finder_trash_confirm): frame (133,87)..(507,191), OK
#    button (435,158)..(494,178) centre (464,168), Cancel (363,158)..(422,178).
#    GO2 stages the APPS folder too, so the Trash holds a FOLDER WITH CONTENTS
#    (APPS\TENANTFX.EXE) and the purge must recurse:
#      from (600,422) -> APPS centre (123,102): sum (-477,-320):
#        m-100:-64 x4, m-77:-64 ; l1 ; back to (600,420): sum (+477,+318):
#        m100:64 x4, m77:62 ; m0:2 -> (600,422) ; l0
#        -> FINDER-TRASH name=APPS origin=0 ; DESKTOP-DB-SAVE n=4
#    MENU (from (600,422)) -> (170,30): sum (-430,-392): m-100:-98 x4, m-30:0 ;
#      l1 ; m0:35 -> (170,65) ; l0 -> FINDER-CMD id=11 name=EMPTY_TRASH ...
#      FINDER-TRASH-ALERT n=<items> k=<K>
# ===========================================================================

# 3a. FLAIR_EMPTY_ALERT_SPEC -- stage README.TXT, choose Empty Trash, and LEAVE
#     the alert up (the screendump after FINDER-TRASH-ALERT n=1 is graded:
#     the alert at the corpus geometry, both buttons, the message ink, and the
#     desktop Trash drawn FULL). PARK (170,65) -> (620,460): m90:79 x5.
FLAIR_EMPTY_ALERT_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,m-100:-98,m-100:-98,m-100:-98,m-100:-98,m-30:0,l1,m0:35,l0,m90:79,m90:79,m90:79,m90:79,m90:79

# 3b. FLAIR_EMPTY_CANCEL_SPEC -- the same, then ESCAPE (the alert's Cancel):
#     FINDER-TRASH-EMPTY-CANCEL and NOTHING purged (mtools: README.TXT still
#     in ::/TRASH, byte-identical). Escape is a keystroke in the mouse stream
#     ("kesc"), sent while the alert's modal loop owns the event queue.
FLAIR_EMPTY_CANCEL_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,m-100:-98,m-100:-98,m-100:-98,m-100:-98,m-30:0,l1,m0:35,l0,kesc,m90:79,m90:79,m90:79,m90:79,m90:79

# 3c. FLAIR_EMPTY_OK_SPEC -- stage README.TXT AND the APPS folder, open the
#     Trash window (FINDER-OPEN-TRASH win=1 n=2), choose Empty Trash, CLICK OK:
#     FINDER-TRASH-ALERT n=3 ; FINDER-TRASH-EMPTIED purged=3 refused=0 (the
#     file, the folder's file, then the folder -- depth-first) ;
#     DESK-TRASH-ICON empty ; DESKTOP-DB-SAVE n=2 (both origins dropped).
#     mtools: ::/TRASH empty; README.TXT / APPS / TENANTFX.EXE nowhere;
#     fsck.fat finds no lost clusters (the chains were FREED, not leaked).
#     OK: (170,65) -> (464,168): m98:51, m98:52, m98:0 ; l1 ; l0.
#     PARK (464,168) -> (620,460): m52:97, m52:97, m52:98.
FLAIR_EMPTY_OK_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,m-100:-64,m-100:-64,m-100:-64,m-100:-64,m-77:-64,l1,m100:64,m100:64,m100:64,m100:64,m77:62,m0:2,l0,l1,l0,l1,l0,m-100:-98,m-100:-98,m-100:-98,m-100:-98,m-30:0,l1,m0:35,l0,m98:51,m98:52,m98:0,l1,l0,m52:97,m52:97,m52:98
