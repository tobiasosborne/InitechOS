# spec/flair_fg_close_traces.mk -- the LOCKED input traces for the
# "foreground app, band 2 and active window agree" emulator gate (bead
# initech-tdnl.40; audit docs/audits/2026-10-03-flair-gui-codex/REPORT.md F06
# + TRIAGE.md). Locked spec-data (CLAUDE.md Rule 8, Rule 11). Conventions are
# spec/flair_disk_windows_traces.mk's verbatim (relative QMP moves from the
# screen centre (320,240), y down-positive, |c| <= 100; "l1"/"l0"; "k<chord>"
# a keystroke in the mouse stream; every trace ends PARKED at (620,460)).
#
# THE RULE (re-keyed 2026-10-04; Macintosh Toolbox Essentials, local copy
# ../system7-decomp/refs/MacintoshToolboxEssentials.pdf):
#   p.2-4  "The foreground process displays its menu bar, and its windows are
#          in front of the windows of all other applications."
#   p.4-16 "One way the user can switch applications is by clicking in a
#          window that belongs to a background process."
#   p.2-60 on a suspend an application "should deactivate the front window".
# The foreground changes only when the user activates another application.
# Closing the Finder's last window is NOT a switch: the Finder stays the
# foreground app with its bar in band 2 and NO window is active (HELLO and
# NOTES belong to background apps). A click on the desktop -- the Finder's
# (IM VI p.9-3) -- makes the Finder the foreground app; a click in HELLO's
# window makes HELLO the foreground again. (Commit 47f6202 had keyed this gate
# to the opposite rule -- "the foreground follows the front window" -- which
# also made test-flair-close-terminate-mutant's CLOSE_HIDE_ONLY look like a
# real terminate; that rule is now the gate's MUTANT, KMAIN_MUT_FG_FOLLOWS_FRONT.)
#
# --- waypoint arithmetic -------------------------------------------------
# A. the volume double-click (spec/flair_disk_windows_traces.mk trace 1 A):
#      m100:-88, m100:-88, m80:0 -> (600,64) ; l1,l0,l1,l0
#    A' is the SAME move with ONE click (l1,l0): select, never open.
# B. kctrl-w  -> Close Window through MenuKey (the keyboard half of F06's
#    "Close the root ... using Ctrl-W").
# C. (600,64) -> band 2's first title (32,30): sum (-568,-34):
#      m-100:-6 x5, m-68:-4. Band 2 is rows [20,40). os/flair/menu.c lays
#      titles out from x = FLAIR_MENU_APPLE_W (20) when the bar has an Apple
#      slot (the Finder's) and from x = 0 when it has none (the Photoshop bar,
#      shell.c has_apple = 0); each slot is StringWidth + 14 wide, and Chicago
#      "File" is 23 px (F7 i4 l4 e8: the NFNT 5478 advances tools/
#      ppm_flair_disk_windows_check.c carries for its title run), so the
#      slots are [20,57) and [0,37): x=32 is on "File" in BOTH bars, and the
#      press discriminates only WHICH bar is live: the Finder's File is menu
#      512, HELLO's Photoshop File is 256 (os/flair/shell.c bar_photoshop).
#      l1 ; l0 -> the panel drops and closes, item 0.
# D. PARK (32,30) -> (620,460): sum (+588,+430): m100:72 x5, m88:70.
# E. (32,30) -> HELLO's content (80,200): sum (+48,+170): m24:85 x2 (via
#    (56,115), left of HELLO's frame at x=60, so no title probe column is
#    crossed). HELLO's window is struct (60,60)-(360,260)
#    (spec/flair_tenants_demo.h FLAIR_TEN_HELLO_*); (80,200) is below its
#    title band and left of NOTES (x>=260): inContent of HELLO. l1 ; l0.
# F. PARK (80,200) -> (620,460): sum (+540,+260): m90:52 x5, m90:0 (the path
#    stays at y>=252, below every title probe row).
#
# TRACE 1 -- FLAIR_FG_CLOSE_SPEC: A, B, C, D. Expected:
#     FINDER-CLOSE-WINDOW win=0
#     NO FLAIR-DISPATCH line after it (the Finder keeps the foreground)
#     FLAIR-MENU-DROP menu=512 after it (the Finder's File)
#   final frame: band 2 == the Finder bar (ppm_flair_app_launch_check
#   bar-finder); no window active (ppm_flair_solid_check leg L).
FLAIR_FG_CLOSE_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,kctrl-w,m-100:-6,m-100:-6,m-100:-6,m-100:-6,m-100:-6,m-68:-4,l1,l0,m100:72,m100:72,m100:72,m100:72,m100:72,m88:70
#
# TRACE 2 -- FLAIR_FG_CLOSE_CLICK_SPEC: A, B, C, E, F. Expected: as trace 1
#   up to the 512 drop, then the FIRST FLAIR-DISPATCH after the close is
#   app=HELLO and it follows the drop; final frame: band 2 == the Photoshop bar
#   (bar-photoshop), HELLO active and NOTES inactive (leg K).
FLAIR_FG_CLOSE_CLICK_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,kctrl-w,m-100:-6,m-100:-6,m-100:-6,m-100:-6,m-100:-6,m-68:-4,l1,l0,m24:85,m24:85,l1,l0,m90:52,m90:52,m90:52,m90:52,m90:52,m90:0
#
# TRACE 3 -- FLAIR_FG_DESK_SPEC: A', C, D. Only HELLO's and NOTES's windows
#   are on screen (no Finder window). Expected:
#     FLAIR-DISPATCH app=FINDER (the desktop click activated the Finder)
#     FLAIR-MENU-DROP menu=512 after it (band 2 is the Finder's bar)
#     NO further FLAIR-DISPATCH (the Finder keeps the foreground)
#   final frame: bar-finder; no window active (leg L: HELLO deactivated).
FLAIR_FG_DESK_SPEC := m100:-88,m100:-88,m80:0,l1,l0,m-100:-6,m-100:-6,m-100:-6,m-100:-6,m-100:-6,m-68:-4,l1,l0,m100:72,m100:72,m100:72,m100:72,m100:72,m88:70
#
# TRACE 4 -- FLAIR_FG_DESK_CLICK_SPEC: A', C, E, F. Expected: as trace 3, then
#   the first FLAIR-DISPATCH after app=FINDER is app=HELLO and it follows the
#   512 drop; final frame: bar-photoshop, leg K.
FLAIR_FG_DESK_CLICK_SPEC := m100:-88,m100:-88,m80:0,l1,l0,m-100:-6,m-100:-6,m-100:-6,m-100:-6,m-100:-6,m-68:-4,l1,l0,m24:85,m24:85,l1,l0,m90:52,m90:52,m90:52,m90:52,m90:52,m90:0
