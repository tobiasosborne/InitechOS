# spec/flair_fg_close_traces.mk -- the LOCKED input trace for the "foreground
# follows the front window after a close" emulator gate (bead initech-tdnl.40;
# audit docs/audits/2026-10-03-flair-gui-codex/REPORT.md F06 + TRIAGE.md).
# Locked spec-data (CLAUDE.md Rule 8, Rule 11). Conventions are
# spec/flair_disk_windows_traces.mk's verbatim (relative QMP moves from the
# screen centre (320,240), y down-positive, |c| <= 100; "l1"/"l0"; "k<chord>"
# a keystroke in the mouse stream; the trace ends PARKED at (620,460)).
#
# THE AUDITED STATE. The volume double-click opens the root disk window; the
# Finder becomes the foreground tenant (FLAIR-DISPATCH app=FINDER) and band 2
# becomes its bar. Ctrl-W closes the window. Before the fix nothing demoted the
# Finder: band 2 kept the Finder bar while HELLO -- the boot front window,
# re-hilited by DisposeWindow -- was drawn ACTIVE, and a press on band 2's
# first title dropped the FINDER's File menu (512) over HELLO.
#
# EXPECTED after the close (the operator ruling of 2026-10-03: at rest HELLO
# is the foreground tenant and band 2 is its Photoshop bar):
#     FINDER-CLOSE-WINDOW win=0
#     FLAIR-DISPATCH app=HELLO        (the owner of the new front window)
#     a band-2 press on the first title drops HELLO's Photoshop File menu
#     (menu id 256, os/flair/shell.c bar_photoshop; the solid leg E grader
#     names the same id), never the Finder's File (512)
# and the final frame: band 2 == the Photoshop bar (ppm_flair_app_launch_check
# bar-photoshop), HELLO's title active, NOTES's inactive (ppm_flair_solid_check
# leg K).
#
# --- waypoint arithmetic -------------------------------------------------
# A. the volume double-click (spec/flair_disk_windows_traces.mk trace 1 A):
#      m100:-88, m100:-88, m80:0 -> (600,64) ; l1,l0,l1,l0
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
#      press discriminates only WHICH bar is live. l1 ; l0 -> the panel drops
#      and closes, item 0.
# D. PARK (32,30) -> (620,460): sum (+588,+430): m100:72 x5, m88:70.
FLAIR_FG_CLOSE_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,kctrl-w,m-100:-6,m-100:-6,m-100:-6,m-100:-6,m-100:-6,m-68:-4,l1,l0,m100:72,m100:72,m100:72,m100:72,m100:72,m88:70
