# spec/flair_trash_edges_traces.mk -- the LOCKED input traces for the Finder
# Trash EDGE gate test-flair-trash-edges (bead initech-vnl4; audit
# docs/audits/2026-10-23-desktop/REPORT.md L001 / L006 / L007 / L008).
# Locked spec-data (CLAUDE.md Rule 8, Rule 11): deterministic, versioned, never
# silently edited to make a test pass.
#
# Conventions are spec/flair_file_ops_traces.mk's verbatim (QMP relative moves
# from the screen centre (320,240), y DOWN-positive, every component int8-safe
# |c| <= 100; "l1"/"l0" left button; "k<chord>" a keystroke in the mouse
# stream). Shared geometry (derived in spec/flair_trash_traces.mk, never re-read
# from the artifact):
#   desktop: VOLUME "INITECH" centre (600,64); Trash sprite (584,404) centre
#            (600,420).
#   ROOT window (slot 0) frame (20,60)..(380,280): the 68-px row-major grid,
#            cell 0 sprite (39,86) centre (55,102) = README.TXT; cell 3 centre
#            (55 + 3*68, 102) = (259,102) = AUTOEXEC.BAT (the stock volume's
#            disk order README.TXT, APPS, DOS, AUTOEXEC.BAT, per mdir).
#   THE SHARED PREFIX (spec/flair_trash_traces.mk A/B/GO):
#     A.  m100:-88, m100:-88, m80:0 -> (600,64); l1,l0,l1,l0 -> the root opens
#     B.  -> README centre (55,102): m-100:8 x3, m-100:7 x2, m-45:0
#     GO. l1 ; m100:64 x4, m100:62, m45:0 -> (600,420) ; m0:2 -> (600,422) ; l0
#
# THE FULL VOLUME ($(FINDER_FULL_DATA): the flagship volume + FILL.BIN sized by
# mtools to leave ZERO free clusters, and no \TRASH -- the audit's
# evidence/prepare_fixtures.py full-data.img recipe). At mount
# desktop_trash_ensure fails (TRASH-INIT-FAIL rc=-11) and the boot notice "The
# disk is full." is up before any input: every full-volume trace OPENS with
# "kret" to dismiss it (the audit's step "Dismiss the boot alert with Return").
#
# ===========================================================================
# 1. FLAIR_TRASH_EDGE_FULL_ALERT_SPEC (L001) -- kret ; A ; B ; GO: README.TXT
#    dropped on the Trash of the full volume. No \TRASH can be made (TRASH-
#    RESERVE-FAIL rc=-11), so the Delete Now confirm comes up
#    (FINDER-DELETE-ALERT name=README.TXT) and STAYS up; the screendump after
#    that marker is graded (two-button alert chrome + the CAUTION icon).
#    PARK (600,422) -> (620,460): m20:38.
# ===========================================================================
FLAIR_TRASH_EDGE_FULL_ALERT_SPEC := kret,m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,m20:38

# ===========================================================================
# 2. FLAIR_TRASH_EDGE_FULL_DELETE_SPEC (L001) -- the same, then "kret" (the
#    confirm's default, OK): FINDER-DELETE-NOW name=README.TXT purged=1 and, in
#    the cluster just freed, TRASH-CREATE. Then OPEN at (600,422): l1,l0,l1,l0
#    on the Trash sprite -> FINDER-OPEN-TRASH win=1 n=0 singleton=0 (the Trash
#    exists again and is empty). PARK m20:38.
# ===========================================================================
FLAIR_TRASH_EDGE_FULL_DELETE_SPEC := kret,m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,kret,l1,l0,l1,l0,m20:38

# ===========================================================================
# 3. FLAIR_TRASH_EDGE_NOTRASH_OPEN_SPEC (L007) -- kret ; (320,240) -> the
#    Trash (600,422): sum (+280,+182): m100:61, m100:61, m80:60 ; l1,l0,l1,l0:
#    the open finds no \TRASH and no room to make one -> FINDER-WIN-OPEN-FAIL
#    rc=-13 and the notice naming THE TRASH (FINDER-ALERT reason=notrash),
#    never "could not be moved". PARK m20:38.
# ===========================================================================
FLAIR_TRASH_EDGE_NOTRASH_OPEN_SPEC := kret,m100:61,m100:61,m80:60,l1,l0,l1,l0,m20:38

# ===========================================================================
# 4. FLAIR_TRASH_EDGE_PUTAWAY_SPEC (L006 + L008) -- on the STOCK volume:
#    A ; B ; GO (README.TXT staged, origin 0) ;
#    -> AUTOEXEC.BAT (259,102): sum (-341,-320): m-100:-80 x3, m-41:-80 ; l1 ;
#       back to (600,420): sum (+341,+318): m100:80 x3, m41:78 ; m0:2 ; l0
#       -> FINDER-TRASH name=AUTOEXEC.BAT origin=0 ;
#    OPEN the Trash at (600,422): l1,l0,l1,l0 -> FINDER-OPEN-TRASH win=1 n=2 ;
#    kctrl-n  with the Trash window front: New Folder is DARK (L008) -- no
#             FINDER-NEW-FOLDER, nothing made in \TRASH ;
#    kctrl-a  -> FINDER-SELECT-ALL win=1 n=2 ;
#    kctrl-y  -> FINDER-PUT-AWAY n=2: BOTH items home (L006) ;
#    kctrl-w  -> the Trash window closes, the root is front again ;
#    kctrl-n  the positive control: New Folder is live in the root ->
#             FINDER-NEW-FOLDER name=NEWFOLD parent=0 ;
#    PARK (600,422) -> (620,460): m20:38.
# ===========================================================================
FLAIR_TRASH_EDGE_PUTAWAY_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,m-100:-80,m-100:-80,m-100:-80,m-41:-80,l1,m100:80,m100:80,m100:80,m41:78,m0:2,l0,l1,l0,l1,l0,kctrl-n,kctrl-a,kctrl-y,kctrl-w,kctrl-n,m20:38
