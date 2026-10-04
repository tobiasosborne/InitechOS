# spec/flair_app_launch_traces.mk -- the LOCKED input traces for the R3.7
# APP LAUNCH FROM DISK emulator gate (bead initech-tdnl.14).
# Locked spec-data (CLAUDE.md Rule 8, Rule 11): deterministic, versioned, never
# silently edited to make a test pass.
#
# Sibling of spec/flair_disk_windows_traces.mk and bound by ALL of its
# conventions (cursor starts at (320,240); "m<dx>:<dy>" relative, y down; every
# component int8-safe; "l1"/"l0" left button; "k<chord>" a keystroke in stream
# order; every trace PARKS the pointer at (620,460)).
#
# THE SCENE: $(FLAIRTENANTS_IMG) + a gate-local copy of $(FLAIR_DATA_IMG), whose
# APPS folder now holds TENANTFX.EXE (os/apps/tenantfx.asm). The root listing is
# unchanged (n=4: README.TXT, APPS, DESKTOP.DB, TRASH), so every R3.2/R3.3
# locked trace and count still holds.
#
# GEOMETRY (derived, never measured off a render):
#   root window (slot 0) frame (20,60)..(380,280); APPS icon sprite
#     (107,86)..(139,118), centre (123,102)   [flair_disk_windows_traces.mk B]
#   APPS window (slot 1) frame = finder_win_default_frame(1) = (40,80)..(400,300)
#     content.left = 40 + FLAIR_CHROME_FRAME(1) = 41
#     content.top  = 80 + FLAIR_CHROME_TITLEBAR_H(22) = 102
#     grid i=0: sprite_x = 41 + FINDER_GRID_INSET_X(18) = 59
#               sprite_y = 102 + 4 = 106
#     => TENANTFX.EXE (the only entry after "." / "..", which the Finder skips)
#        APP sprite (59,106)..(91,138), centre (75,122)
#   the TENANT window frame (100,160)..(400,340) (tenantfx.asm WIN_*)
#     content (101,182)..(379,319) by CalcDocContentRect (title 22, frame 1,
#     body bar 4, scrollbar 16 -- the bottom stops at the horizontal scroll
#     bar since bead initech-tdnl.35, comment re-keyed from 339; no trace byte
#     depends on it); go-away box footprint = frame + (4,4)..(16,16)
#     = (104,164)..(116,176), centre (110,170)
#
# BUDGET NOTE (why the hops are short): the flagship pump halts after
# FLAIR_TEN_TICK_BUDGET = 250 ticks (2.5 s) and the harness drains ~40 ms per
# token, so a serial leg must stay near 30 tokens with room for two launches on
# a loaded host. The tenant window therefore sits right beside the icon (one
# move each way), and the serial-only legs (DOUBLE/CYCLE/CLOSE) do not park --
# they take no screendump, so the stranded-arrow rule has nothing to protect.
#
# WAYPOINTS (shared by every trace below):
#   A (320,240) -> VOLUME (600,64): m100:-88,m100:-88,m80:0 ; l1,l0,l1,l0
#       -> FINDER-OPEN-VOLUME win=0 n=4
#   B (600,64) -> APPS in win 0 (123,102): m-100:10,m-100:10,m-100:10,
#       m-100:8,m-77:0 ; l1,l0,l1,l0 -> FINDER-OPEN-FOLDER name=APPS win=1
#   C (123,102) -> TENANTFX.EXE in win 1 (75,122): m-48:20 ; l1,l0,l1,l0
#       -> TENANT-LOAD ... the launch
#   D (75,122) -> inside the tenant content (140,210): m65:88 ; l1,l0
#       -> TENANT-EVT what=1, TENANT-EXIT rc=0 via=exit
#   D' (140,210) -> back to the icon (75,122): m-65:-88
#   G (75,122) -> the tenant go-away box (110,170): m35:48
#   P park: from (140,210): m120:100,m120:100,m120:50,m120:0 (dx 480, dy 250)
#       ; from (75,122): m110:113,m110:113,m110:112,m110:0,m105:0 (545, 338)
#
# ===========================================================================
# 1. FLAIR_APP_LAUNCH_SPEC -- THE POSITIVE LEG (and the app_launch clip):
#    A, B, C (launch), D (click inside -> clean exit), P.
# ===========================================================================
FLAIR_APP_LAUNCH_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m-48:20,l1,l0,l1,l0,m65:88,l1,l0,m120:100,m120:100,m120:50,m120:0

# ===========================================================================
# 2. FLAIR_APP_LAUNCH_DOUBLE_SPEC -- DOUBLE_LAUNCH: A, B, C (launch), then a
#    small jiggle (so the re-click is a fresh gesture, not the tail of C's
#    double) and a SECOND double-click on the same icon while the tenant is
#    resident -> the first click switches the foreground back to the Finder,
#    the double asks for a second launch -> TENANT-SLOT-BUSY; then D (the click
#    re-activates the tenant and exits it) -- but at (140,320), NOT D's
#    (140,210): the Finder's activation raised the APPS window (40,80)..
#    (400,300) over the tenant, whose only visible content is now the strip
#    y [300,319); from (75,122): m65:99,m0:89 -> (140,310). RE-KEYED (bead
#    initech-tdnl.35, stated): this click was at (140,320), which the strip
#    then reached to y 339 -- but y [319,335) is the tenant window's
#    HORIZONTAL SCROLL BAR, and since tdnl.35 a scroll-bar click activates
#    the window without reaching its content (FindWindow inContent ->
#    WindowScrollBand; IM: the bars are controls). The tenant exits on a
#    CONTENT click, so the point moves 10 px up into the content strip.
#    Serial-only: no park.
# ===========================================================================
FLAIR_APP_LAUNCH_DOUBLE_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m-48:20,l1,l0,l1,l0,m4:0,m-4:0,l1,l0,l1,l0,m65:99,m0:89,l1,l0

# ===========================================================================
# 3. FLAIR_APP_LAUNCH_CYCLE_SPEC -- EXIT_LEAK: TWO full launch/exit cycles:
#    A, B, C, D, D', relaunch (l1,l0,l1,l0), D. Two TENANT-HEAPAVAIL lines.
#    Serial-only: no park.
# ===========================================================================
FLAIR_APP_LAUNCH_CYCLE_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m-48:20,l1,l0,l1,l0,m65:88,l1,l0,m-65:-88,l1,l0,l1,l0,m65:88,l1,l0

# ===========================================================================
# 4. FLAIR_APP_LAUNCH_CLOSE_SPEC -- CLOSE_STALE_FOCUS: A, B, C, G (click the
#    tenant's go-away box -> the anti-8fhu terminate), then ONE plain keystroke
#    ("x": no Ctrl, so no Finder chord intercepts it -- it goes through the
#    Layer-5 keyDown route to whoever is foreground), then back to the icon
#    (m-35:-48) and a RELAUNCH double-click. The relaunch is what makes a
#    close that merely HID the window observable in THIS scene: R3.5's
#    Finder-foreground rule promotes the Finder the moment the front visible
#    window is a disk window, which masks the keystroke half of the initech-8fhu
#    symptom -- but a tenant the close box did not terminate still holds the
#    one slot, so the relaunch reports TENANT-SLOT-BUSY on a windowless zombie.
#    Clean: TENANT-EXIT via=close, the key reaches no tenant, the relaunch
#    LOADs and REGISTERs. Serial-only: no park.
# ===========================================================================
FLAIR_APP_LAUNCH_CLOSE_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m-48:20,l1,l0,l1,l0,m35:48,l1,l0,kx,m-35:-48,l1,l0,l1,l0

# ===========================================================================
# 5. FLAIR_APP_LAUNCH_SHOW_SPEC -- THE WINDOW-APPEARS PIXEL LEG: A, B, C
#    (launch) and park, with NO exit click, so the post-budget screendump
#    (FLAIR-LIVE-OK, pointer hidden) shows the resident tenant's window at the
#    front with its text run. Park P from (75,122).
# ===========================================================================
FLAIR_APP_LAUNCH_SHOW_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m-48:20,l1,l0,l1,l0,m110:113,m110:113,m110:112,m110:0,m105:0

# ===========================================================================
# 6. FLAIR_APP_LAUNCH_PRE_SPEC -- the RESTORE baseline: A, B, then ONE click on
#    TENANTFX.EXE (select, no launch) and park. Its post-budget dump must be
#    BYTE-IDENTICAL to FLAIR_APP_LAUNCH_SPEC's (launch + clean exit): an exit
#    that leaves one pixel of the tenant behind, or restores the wrong
#    foreground/band-2 bar, cannot pass. (Both dumps are taken after
#    FLAIR-LIVE-OK, when the pointer is hidden, so the park point is moot.)
# ===========================================================================
FLAIR_APP_LAUNCH_PRE_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m-48:20,l1,l0,m110:113,m110:113,m110:112,m110:0,m105:0

# ===========================================================================
# 7. FLAIR_APP_MENUBAR_SPEC -- THE TENANT'S OWN BAR IS LIVE (bead initech-cnpm,
#    SETMBAR; spec/toolbox_gate.h Sec 9). A, B, C (launch), then a click on the
#    tenant's "Fixture" title in BAND 2 -> the Menu Manager drops the panel
#    read straight out of the tenant image (FLAIR-MENU-DROP menu=131, the
#    menuID tenantfx.asm authors), the release on the title selects nothing
#    (FLAIR-MENU menu=131 item=0), then D (click inside the content -> clean
#    exit). Serial-only in the gate (no park: the leg takes no screendump);
#    also the app_menubar record-flair clip (Rule 14).
#    GEOMETRY (derived, never measured off a render): band 2 = rows [20,40);
#    the tenant bar has the Apple slot, so (menu.h Sec 5, pad 7, PROPORTIONAL
#    Chicago 12 -- the REAL NFNT 5478 advances, bead initech-tdnl.33):
#    File F7 i4 l4 e8 = 23 -> [20,57), Edit E7 d8 i4 t6 = 25 -> [57,96),
#    Fixture F7 i4 x8 t6 u8 r6 e8 = 47 -> 47+14 = 61 -> [96,157); the click
#    point (147,30) is inside it (was the fixed-cell slot [112,182), whose
#    centre it was; the trace bytes did not need to move). From the icon
#    (75,122): m72:-92. Back to
#    the content (140,210): m-7:90,m0:90 (each component int8-safe).
#    30 tokens: inside the flagship pump's budget (see BUDGET NOTE above).
# ===========================================================================
FLAIR_APP_MENUBAR_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m-48:20,l1,l0,l1,l0,m72:-92,l1,l0,m-7:90,m0:90,l1,l0
