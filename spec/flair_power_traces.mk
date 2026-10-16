# spec/flair_power_traces.mk -- the LOCKED input traces for the Special >
# Restart and Special > Shut Down emulator gates (bead initech-tdnl.78).
# Locked spec-data (CLAUDE.md Rule 8, Rule 11): deterministic, versioned, never
# silently edited to make a test pass.
#
# Conventions are spec/flair_disk_windows_traces.mk's verbatim (relative QMP
# moves from the screen centre (320,240), y screen-down-positive, |c| <= 100;
# "l1"/"l0" left button; "k<chord>" a keystroke in the mouse stream).
#
# THE SHARED PREFIX (P) -- put something away that only Restart / Shut Down can
# save. The root window is opened and DRAGGED but never closed, so its view
# state lives only in memory (finder_win_close is the one other path that
# saves it). Steps A..C are spec/flair_disk_windows_traces.mk
# FLAIR_WINDOW_DRAG_PERSIST_SPEC steps A..C byte-for-byte:
#   A. volume double-click -> (600,64): m100:-88, m100:-88, m80:0 ; l1,l0,l1,l0
#      -> FINDER-OPEN-VOLUME win=0 n=4 (the Finder is foreground: band 2 is
#      the Finder bar from here on)
#   B. -> the title bar (200,70): m-100:2 x3, m-100:0
#   C. drag by (+100,+120): l1 ; m100:100, m0:20 -> (300,190) ; l0
#      -> FLAIR-DRAG win 0 (20,60)->(120,180); the frame is now (120,180)..
#      (480,400) and NO view record exists yet.
#   D. -> the band-2 Special title (170,30): sum (-130,-160): m-100:-80,
#      m-30:-80 -> (200,110),(170,30) ; l1 (the Special menu drops)
# THE MENU GEOMETRY (spec/flair_finder_cmds_traces.mk, hand-derived there):
#   Special 6 Restart [101,117) c109 ; 7 Shut Down [117,133) c125.
FLAIR_POWER_PREFIX := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:2,m-100:2,m-100:2,m-100:0,l1,m100:100,m0:20,l0,m-100:-80,m-30:-80,l1

# ===========================================================================
# 1. FLAIR_SHUTDOWN_SPEC -- Special > Shut Down, then PROBE the dead desktop.
# ===========================================================================
# E. m0:95 -> (170,125) ; l0 -> FLAIR-MENU menu=515 item=7 (sel=0x02030007);
#    FINDER-CMD id=13 name=SHUTDOWN src=mouse sel=0 ; DESKTOP-DB-SAVE n=3 (2
#    desktop icons + the root window's kind=4 view at (120,180)) ; the
#    shut-down screen ; FLAIR-SHUTDOWN-SAFE.
# F. THE PROBE (the session is over, so NOTHING may answer):
#    m0:-95 -> (170,30) ; l1,l0  -- on a live desktop this press drops the
#    Finder's Special menu (FLAIR-MENU-DROP / FLAIR-MENU lines);
#    PARK (170,30) -> (620,460): sum (+450,+430): m100:86 x4, m50:86 ;
#    kctrl-n LAST -- on a live desktop the Finder makes a NEWFOLD
#    (FINDER-CMD ... NEW_FOLDER). Its cooked keyDown (FLAIR-EVT what=3) is the
#    gate's completion marker: the shut-down screen and the live pump both
#    cook and print it, so the harness waits for the probe to be CONSUMED in
#    the real build and in the mutant alike (no timing guess).
FLAIR_SHUTDOWN_SPEC := $(FLAIR_POWER_PREFIX),m0:95,l0,m0:-95,l1,l0,m100:86,m100:86,m100:86,m100:86,m50:86,kctrl-n

# ===========================================================================
# 2. FLAIR_SHUTDOWN_RESTART_SPEC -- Special > Shut Down, then the shut-down
#    screen's own Restart button.
# ===========================================================================
# E. as trace 1 (Shut Down at (170,125)).
# G. -> the Restart button centre (497,233) (os/milton/kmain.c FSD_* geometry:
#    button (468,223)..(527,243)): sum (+327,+108): m100:36 x3, m27:0 ;
#    l1,l0 -> FLAIR-RESTART -> the 8042 pulse -> the machine reboots ->
#    FLAIR-LIVE-READY a SECOND time.
FLAIR_SHUTDOWN_RESTART_SPEC := $(FLAIR_POWER_PREFIX),m0:95,l0,m100:36,m100:36,m100:36,m27:0,l1,l0

# ===========================================================================
# 3. FLAIR_RESTART_SPEC -- Special > Restart.
# ===========================================================================
# E. m0:79 -> (170,109) ; l0 -> FLAIR-MENU menu=515 item=6 (sel=0x02030006);
#    FINDER-CMD id=12 name=RESTART src=mouse sel=0 ; DESKTOP-DB-SAVE n=3 ;
#    FLAIR-RESTART ; reboot ; the second boot reads DESKTOP-DB-OK and
#    DESKTOP-DB-VIEWS n=1 (the dragged window's view survived) and prints
#    FLAIR-LIVE-READY a second time. Nothing is injected after the reset.
FLAIR_RESTART_SPEC := $(FLAIR_POWER_PREFIX),m0:79,l0

# ===========================================================================
# 4. FLAIR_SHUTDOWN_APP_SPEC -- a disk application is running: Shut Down asks
#    it to quit first (PM directive 2026-10-15; os/milton/kmain.c
#    finder_power_ask_apps).
# ===========================================================================
# A, B, C1 are spec/flair_ctenant_traces.mk's verbatim: the volume, APPS
#   (FINDER-OPEN-FOLDER name=APPS win=1, frame (40,80)..(400,300)), then
#   CTENANT.EXE (143,122) double-clicked -> the CTENANT window
#   (180,170)..(470,330), CTENANT foreground (band 2 = its bar).
# H. (143,122) -> the APPS window's title bar (100,90) (title band y [80,102),
#    left of the CTENANT window x 180): m-43:-32 ; l1,l0 -> FLAIR-DISPATCH
#    app=FINDER (band 2 = the Finder bar again).
# I. -> Special (170,30): m70:-60 ; l1 ; m0:95 -> Shut Down (170,125) ; l0
#    -> FINDER-CMD id=13 name=SHUTDOWN src=mouse sel=1 (CTENANT.EXE is the
#    APPS window's selection) ; TENANT-MENU menu=129 item=1 src=quit (its own
#    File > Quit, ctenant.c MENU_FILE/ITEM_QUIT) ; TENANT-EXIT rc=0 via=exit ;
#    DESKTOP-DB-SAVE n=4 (2 desktop icons + the root and APPS views) ;
#    FLAIR-SHUTDOWN-SAFE.
# P. PARK (170,125) -> (620,460): sum (+450,+335): m100:67 x4, m50:67 ;
#    kctrl-n LAST (the completion marker, as trace 1).
FLAIR_SHUTDOWN_APP_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m20:20,l1,l0,l1,l0,m-43:-32,l1,l0,m70:-60,l1,m0:95,l0,m100:67,m100:67,m100:67,m100:67,m50:67,kctrl-n
