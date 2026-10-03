# spec/flair_file_ops_traces.mk -- the LOCKED input traces for the R3.4a Finder
# FILE-OPERATIONS emulator gate (bead initech-34dh: drag-move + drag-to-Trash
# staging; GUI remediation plan R3.4, reslice 1/3 of initech-tdnl.11).
# Locked spec-data (CLAUDE.md Rule 8, Rule 11): deterministic, versioned, never
# silently edited to make a test pass.
#
# Conventions are spec/flair_disk_windows_traces.mk's verbatim (QMP relative
# moves from the screen centre (320,240), y DOWN-positive, every component
# int8-safe |c| <= 100; "l1"/"l0" left button; "k<chord>" a keystroke in the
# mouse stream; every trace ends PARKED at (620,460)). Every trace starts with
# that file's FLAIR_ICON_OPEN_SPEC step A -- the volume double-click -- so the
# ROOT disk window opens at its UNMOVED default frame (20,60)..(380,280) with
# the four icons, row-major (derived there, mtools-ordered):
#     README.TXT  DOC    sprite (39,86)   centre (55,102)
#     APPS        FOLDER sprite (107,86)  centre (123,102)
#     DESKTOP.DB  DOC    sprite (175,86)  centre (191,102)
#     TRASH       FOLDER sprite (243,86)  centre (259,102)
# and the desktop icons: VOLUME "INITECH" sprite (584,48) centre (600,64);
# Trash sprite (584,404) centre (600,420).
#
# NO WINDOW IS EVER DRAGGED in these traces (operator audit 2026-10-03: a
# moved disk window's icon layer does not follow it yet -- a separate lane's
# bug). Every gesture runs against windows at their default cascaded frames.
#
# CLUSTERS (from mtools on the pristine $(FLAIR_DATA_IMG), never our own FAT
# code): README.TXT start cluster 2, APPS start cluster 3 (`mdir`/the raw
# root-dir slots printed by the gate). So a move into APPS reports to=3.
#
# THE OUTLINE AND THE HIGHLIGHT. Pressing on an icon and moving past
# FINDER_DRAG_SLOP (3 px) draws the gray save-under outline of the icon cell at
# the pointer offset; a folder / the volume / the Trash under the pointer is
# drawn HIGHLIGHTED and the kernel prints FINDER-DROP-HILITE name=<n> once per
# target lit (os/flair/finder_ops.h "WHAT A USER SEES").
#
# ===========================================================================
# 1. FLAIR_FILEOPS_INTO_SPEC -- drag README.TXT onto the APPS folder icon.
# ===========================================================================
# Serial chain: FINDER-OPEN-VOLUME win=0 n=4 ; FINDER-DROP-HILITE name=APPS ;
#               FINDER-MOVE name=README.TXT from=0 to=3
# mtools: README.TXT in ::/APPS (its bytes == the fixture), absent from ::/.
# A. volume double-click -> (600,64)                    (disk_windows trace 1 A)
# B. (600,64) -> README centre (55,102): sum (-545,+38):
#      m-100:8 x3, m-100:7 x2, m-45:0          (FLAIR_FINDER_MENU_SPEC step B)
# C. l1 ; m34:0 -> (89,102) bare content between the two icons (outline only,
#    no highlight) ; m34:0 -> (123,102) over APPS (HIGHLIGHT) ; m0:4 ->
#    (123,106) still over APPS (a second frame of the lit target) ; l0.
# D. PARK (123,106) -> (620,460): sum (+497,+354): m100:71 x4, m97:70.
FLAIR_FILEOPS_INTO_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m34:0,m34:0,m0:4,l0,m100:71,m100:71,m100:71,m100:71,m97:70

# ===========================================================================
# 2. FLAIR_FILEOPS_BETWEEN_SPEC -- drag TENANTFX.EXE out of the APPS window
#    into the ROOT window, then raise the root window to show it arrived.
# ===========================================================================
# Serial chain: FINDER-OPEN-VOLUME win=0 n=4 ;
#               FINDER-OPEN-FOLDER name=APPS win=1 singleton=0 ;
#               FINDER-MOVE name=TENANTFX.EXE from=3 to=0 ;
#               FLAIR-DRAG win 0 (20,60)->(20,60)
# mtools: TENANTFX.EXE in ::/ (bytes == the shipped build), absent from ::/APPS.
# A. volume double-click -> (600,64).
# B. (600,64) -> APPS centre (123,102): m-100:10 x3, m-100:8, m-77:0
#    (disk_windows trace 1 B); l1,l0,l1,l0 opens APPS in slot 1 at its
#    cascaded frame (40,80)..(400,300), content (41,102)..(379,299), so its one
#    icon TENANTFX.EXE sits at (41+18, 102+4) = (59,106), centre (75,122).
# C. (123,102) -> (75,122): m-48:20.
# D. l1 ; m-45:39 -> (30,161) ; m0:39 -> (30,200) ; m0:2 -> (30,202) ; l0.
#    x=30 is inside the ROOT window's content (21..359) and LEFT of the APPS
#    window's frame (x 40), so FindWindow says inContent on the ROOT window:
#    a WINDOW target (window bodies are not highlighted). The icon lands where
#    dropped, content-relative: sprite origin (59-45, 106+80) = (14,186),
#    clamped to the content's left edge 21 -> (21,186).
# E. (30,202) -> the ROOT title band at (38,70): m8:-100, m0:-32. x=38 is
#    clear of the go-away box (24..36) and left of the APPS frame (40); the
#    click RAISES the root window (tpzf) so the arrival is visible.
#    l1,l0 -> FLAIR-DRAG win 0 (20,60)->(20,60).
# F. PARK (38,70) -> (620,460): sum (+582,+390): m100:65 x5, m82:65.
FLAIR_FILEOPS_BETWEEN_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m-48:20,l1,m-45:39,m0:39,m0:2,l0,m8:-100,m0:-32,l1,l0,m100:65,m100:65,m100:65,m100:65,m100:65,m82:65

# ===========================================================================
# 3. FLAIR_FILEOPS_REFUSE_SPEC -- two REFUSED drops; the volume is unchanged.
# ===========================================================================
# Serial chain: FINDER-DROP-HILITE name=INITECH ;
#               FINDER-MOVE-REFUSED reason=samedir name=README.TXT ;
#               FINDER-DROP-HILITE name=Trash ;
#               FINDER-MOVE-REFUSED reason=cycle name=TRASH
# and NO FINDER-MOVE / FINDER-TRASH line. mtools: the root still lists
# README.TXT, APPS, DESKTOP.DB, TRASH; \TRASH is empty.
# A. volume double-click -> (600,64); B. -> README centre (55,102) as trace 1.
# C. l1 ; drag back to the VOLUME centre: sum (+545,-38): m100:-8 x3,
#    m100:-7 x2, m45:0 -> (600,64) ; m0:2 -> (600,66) ; l0. README lives in the
#    root and the volume icon IS the root: SAMEDIR, refused by the Finder
#    before the backend; the outline zooms back.
# D. (600,66) -> the TRASH folder icon centre (259,102): sum (-341,+36):
#    m-100:9 x3, m-41:9.
# E. l1 ; drag to the desktop Trash centre: sum (+341,+318): m100:80 x3,
#    m41:78 -> (600,420) ; m0:2 -> (600,422) ; l0. The \TRASH folder into the
#    Trash is a folder into ITSELF: CYCLE.
# F. PARK (600,422) -> (620,460): m20:38.
FLAIR_FILEOPS_REFUSE_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:-8,m100:-8,m100:-8,m100:-7,m100:-7,m45:0,m0:2,l0,m-100:9,m-100:9,m-100:9,m-41:9,l1,m100:80,m100:80,m100:80,m41:78,m0:2,l0,m20:38

# ===========================================================================
# 4. FLAIR_FILEOPS_TRASH_SPEC -- drag to the Trash three times; the third
#    collides and is suffixed.
# ===========================================================================
# Serial chain (in order):
#     FINDER-TRASH name=README.TXT origin=0      DESKTOP-DB-SAVE n=3
#     FINDER-NEW-FOLDER name=NEWFOLD parent=0
#     FINDER-TRASH name=NEWFOLD origin=0         DESKTOP-DB-SAVE n=4
#     FINDER-NEW-FOLDER name=NEWFOLD parent=0
#     TRASH-RENAME from=NEWFOLD to=NEWFO001
#     FINDER-TRASH name=NEWFO001 origin=0        DESKTOP-DB-SAVE n=5
# mtools: ::/TRASH holds README.TXT (bytes == the fixture), NEWFOLD <DIR>,
# NEWFO001 <DIR>; the root holds none of them; \DESKTOP.DB is 8 + 5*24 = 128
# bytes whose last 72 are the three hand-authored kind=5 origin records.
# WHY NEWFOLD REAPPEARS AT README's CELL: staging README.TXT frees root slot 1;
# Ctrl-N's fat12_mkdir claims the LOWEST free slot (slot 1) and New Folder
# re-populates the window in slot order, so NEWFOLD is icon 0 at (39,86) --
# the same centre (55,102). The same holds after the second staging. So the
# whole "go to the Trash and come back" gesture is ONE repeated segment.
# A. volume double-click -> (600,64); B. -> README centre (55,102).
# GO  (55,102) -> Trash centre: sum (+545,+318): l1 ; m100:64 x4, m100:62,
#     m45:0 -> (600,420) ; m0:2 -> (600,422) ; l0.
# BACK (600,422) -> (55,102): sum (-545,-320): m-100:-64 x5, m-45:0.
# Sequence: GO, kctrl-n, BACK, GO, kctrl-n, BACK, GO, PARK m20:38.
FLAIR_FILEOPS_TRASH_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,kctrl-n,m-100:-64,m-100:-64,m-100:-64,m-100:-64,m-100:-64,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,kctrl-n,m-100:-64,m-100:-64,m-100:-64,m-100:-64,m-100:-64,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,m20:38

# ===========================================================================
# 5. FLAIR_FILEOPS_HOVER_SPEC -- the MID-DRAG frame: README.TXT held over the
#    APPS folder, button STILL DOWN. Screendumped after
#    FINDER-DROP-HILITE name=APPS and graded by tools/ppm_flair_drop_target_check
#    (APPS highlighted, the other icons not, the gray outline on screen).
# ===========================================================================
# Trace 1 steps A-B, then l1 ; m34:0 ; m34:0 -> (123,102), and NO release:
# the dump is taken while the drag loop is live (FLAIR_LIVE_DRAG_TRACK_TICKS
# then ends it; that later drop is not graded by this leg). No park: the
# pointer IS the subject of this frame, and the grader's probes avoid its
# 16x16 footprint at (123,102)..(139,118).
#   outline: README's cell translated by (+68,0); its top edge is row y=86
#   from x=107 (sprite left 39+68) -- over APPS's transparent rows 0..2.
FLAIR_FILEOPS_HOVER_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m34:0,m34:0
