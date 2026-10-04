# spec/flair_finder_cmds_traces.mk -- the LOCKED input traces for the "no
# command may be enabled and do nothing" emulator gate (bead initech-tdnl.36,
# with .37 / .38 / .50 / .67; audit 2026-10-03 F03 / F05 / F07 / F15 and pass
# 2 G10). Locked spec-data (CLAUDE.md Rule 8, Rule 11).
#
# Conventions are spec/flair_disk_windows_traces.mk's verbatim. Both traces
# open the volume first (that file's trace 1 step A): the root window at
# (20,60)..(380,280), content (21,82), icons README.TXT (39,86) centre
# (55,102), APPS (107,86), DESKTOP.DB (175,86), TRASH (243,86); the Finder is
# the foreground tenant, so band 2 is ITS bar.
#
# THE MENU GEOMETRY, hand-derived (os/flair/menu.h: panel rows start at screen
# y 41 under band 2 and y 21 under band 1; FLAIR_MENU_ITEM_H 16, a divider
# FLAIR_MENU_DIV_H 6; title slots from spec/flair_disk_windows_traces.mk
# trace 4 D: File[20,57) Edit[57,96) View[96,142) Special[142,202); the F4.2
# item lists of os/flair/finder_menu.c, typed independently in
# harness/proptest/test_finder_menu.c):
#   File    2 Open [57,73) c65 ; 6 Get Info [111,127) c119 ; 7 Duplicate
#           [127,143) c135   (rows 1-4 = 64 px, the divider 6 px)
#   Edit    6 Select All [121,137) c129
#   View    12 Arrange (by Name) [207,223) c215 (9 rows + divider + Clean Up)
#   Special 6 Restart [101,117) c109 ; 7 Shut Down [117,133) c125
#           (Clean Up, Empty Trash, div, Erase Disk, div)
#   band 1  File title x[20,57) y[0,20); item 1 About [21,37) c29
# The audit's own coordinates agree: Restart (216,108), Shut Down (216,124),
# Arrange (190,214), upper About (56,30).
#
# ===========================================================================
# 1. FLAIR_CMDS_FAKES_SPEC -- every formerly-fake item is tried, then Select
#    All really runs.
# ===========================================================================
# A. volume double-click -> (600,64): m100:-88, m100:-88, m80:0 ; l1,l0,l1,l0
# B. -> README centre (55,102): m-100:8 x3, m-100:7 x2, m-45:0 ; l1,l0
#    -> FINDER-WIN-SELECT win=0 name=README.TXT count=1 (a DOCUMENT selected)
# C. File > Get Info: m-15:-72 -> (40,30) ; l1 ; m0:89 -> (40,119) ; l0
#    File > Duplicate: m0:-89 -> (40,30) ; l1 ; m0:100, m0:5 -> (40,135) ; l0
#    File > Open:      m0:-100, m0:-5 -> (40,30) ; l1 ; m0:35 -> (40,65) ; l0
#    each -> FLAIR-MENU menu=512 item=0 (sel=0x00000000): drawn disabled, the
#    release selects NOTHING (menu.c MenuInfo_item_selectable).
# D. Special > Restart: m130:-35 -> (170,30) ; l1 ; m0:79 -> (170,109) ; l0
#    Special > Shut Down: m0:-79 -> (170,30) ; l1 ; m0:95 -> (170,125) ; l0
#    each -> FLAIR-MENU menu=515 item=0 (sel=0x00000000)
# E. band-1 File > About: m-100:-100, m-30:-15 -> (40,10) ; l1 ; m0:19 ->
#    (40,29) ; l0 -> FLAIR-MENU menu=128 item=0 (sel=0x00000000)
# F. kctrl-i, kctrl-d, kctrl-o -> MenuKey hands out nothing: no FINDER-CMD.
# G. Edit > Select All: m40:1 -> (80,30) ; l1 ; m0:99 -> (80,129) ; l0
#    -> FLAIR-MENU menu=513 item=6 ; FINDER-CMD ... name=SELECT_ALL src=mouse
#    sel=1 ; FINDER-SELECT-ALL win=0 n=4 ; every label drawn inverted.
# H. PARK (80,129) -> (620,460): sum (+540,+331): m100:66 x4, m100:67, m40:0
FLAIR_CMDS_FAKES_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,l0,m-15:-72,l1,m0:89,l0,m0:-89,l1,m0:100,m0:5,l0,m0:-100,m0:-5,l1,m0:35,l0,m130:-35,l1,m0:79,l0,m0:-79,l1,m0:95,l0,m-100:-100,m-30:-15,l1,m0:19,l0,kctrl-i,kctrl-d,kctrl-o,m40:1,l1,m0:99,l0,m100:66,m100:66,m100:66,m100:66,m100:67,m40:0

# ===========================================================================
# 2. FLAIR_CMDS_ARRANGE_SPEC -- View > Arrange (by Name) really arranges.
# ===========================================================================
# The audit's G10 set-up: README.TXT dragged well off its cell first, so an
# Arrange that did nothing would leave the gap and the stray icon (G10 "Saw").
# A. volume double-click -> (600,64) ; B. -> README centre (55,102).
# C. drag README by (+90,+100): l1 ; m90:100 -> (145,202) ; l0
#    -> FINDER-WIN-DRAG win=0 name=README.TXT x=129 y=186 (inside the clamp:
#    129 <= 359-32, 186 <= 279-47). A drag does not select.
# D. View > Arrange: m-25:-86, m0:-86 -> (120,30) ; l1 ; m0:100, m0:85 ->
#    (120,215) ; l0 -> FLAIR-MENU menu=514 item=12 ; FINDER-CMD id=10
#    name=ARRANGE_BY_NAME src=mouse sel=0 ; FINDER-ARRANGE win=0 moved=3
#    (APPS, DESKTOP.DB, README.TXT move; TRASH is already in cell 3).
# E. PARK (120,215) -> (620,460): m100:49 x5.
FLAIR_CMDS_ARRANGE_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m90:100,l0,m-25:-86,m0:-86,l1,m0:100,m0:85,l0,m100:49,m100:49,m100:49,m100:49,m100:49
