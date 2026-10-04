# spec/flair_finder_follow_traces.mk -- the LOCKED input traces for the
# "window contents follow the window" emulator gate (bead initech-tdnl.34;
# audit docs/audits/2026-10-03-flair-gui-codex/REPORT.md F01, P0).
# Locked spec-data (CLAUDE.md Rule 8, Rule 11): deterministic, versioned, never
# silently edited to make a test pass.
#
# Conventions are spec/flair_disk_windows_traces.mk's verbatim (QMP relative
# moves from the screen centre (320,240), y DOWN-positive, every component
# int8-safe |c| <= 100; "l1"/"l0" left button; every trace ends PARKED).
#
# THE AUDIT GESTURE, made repeatable. The reviewer dragged a Finder window and
# saw it go blank; collapse/expand did not repair it; zoom showed an icon at
# its OLD screen position; restore blanked it again (F01 "Saw"). Each trace
# below is a PREFIX of the next, so each boot stops at one state and is
# screendumped there (the harness takes one dump per boot, after injection):
#
#   FOLLOW_DRAG    open the volume, drag the root window by (+100,+120)
#                  -> graded by ppm_flair_disk_windows_check leg `movedwin`
#   FOLLOW_ZOOM    ... collapse, expand, zoom to the standard state
#                  -> graded by leg `zoomedwin`
#   FOLLOW_RESTORE ... zoom back to the user state (120,180)
#                  -> graded by leg `movedwin` again
#   FOLLOW_MOVE    ... click APPS where it is NOW (it must SELECT), then drag
#                  README.TXT onto APPS (it must MOVE INTO APPS; mtools judges)
#
# WHERE THE EXPECTED POSITIONS COME FROM (Law 2). The grader derives every
# icon cell from the window's NEW frame through the hand-carried chrome +
# grid arithmetic (tools/ppm_flair_disk_windows_check.c, finder_windows.h
# Sec 2): it never reads the artifact's stored cells. After the drag the frame
# is (120,180)..(480,400), content (121,202)..(459,399), so the four root
# icons (mtools order, spec/flair_disk_windows_traces.mk) sit at
#     README.TXT sprite (139,206) centre (155,222)
#     APPS       sprite (207,206) centre (223,222)
#     DESKTOP.DB sprite (275,206)
#     TRASH      sprite (343,206)
# and after the zoom the frame is (4,40)..(636,476), content (5,62), sprites
# (23,66) (91,66) (159,66) (227,66) (8 grid columns; still one row).
#
# THE WIDGET BOXES of the moved frame (spec/chrome_metrics.h: box 12, top off
# 4, ZOOM_RIGHT_OFF 32, COLLAPSE_RIGHT_OFF 16; the HELLO zoom_toggle trace in
# spec/flair_solid_traces.mk uses the same arithmetic):
#     zoom     (480-32, 184)..(460,196)  centre (454,190)
#     collapse (480-16, 184)..(476,196)  centre (470,190)
# and of the ZOOMED frame: zoom (636-32, 44)..(616,56), centre (610,50) --
# the same point zoom_toggle uses for HELLO's standard state.
#
# --- waypoint arithmetic -------------------------------------------------
# A. the volume double-click (spec/flair_disk_windows_traces.mk trace 1 A):
#      m100:-88, m100:-88, m80:0 -> (600,64) ; l1,l0,l1,l0
#    -> FINDER-OPEN-VOLUME win=0 n=4, root frame (20,60)..(380,280).
# B. (600,64) -> the root TITLE BAR (200,70): m-100:2 x3, m-100:0
#    (FLAIR_WINDOW_DRAG_PERSIST_SPEC step B, byte for byte).
# C. drag by (+100,+120): l1 ; m100:100, m0:20 -> (300,190) ; l0
#    -> FLAIR-DRAG win 0 (20,60)->(120,180)   (that trace's step C).
# D. (300,190) -> the collapse box (470,190): m100:0, m70:0.
#    l1,l0 -> FLAIR-COLLAPSE win 0 1 ; l1,l0 -> FLAIR-COLLAPSE win 0 0.
#    (The box does not move: a collapse keeps the title bar where it is.)
# E. (470,190) -> the zoom box (454,190): m-16:0.
#    l1,l0 -> FLAIR-ZOOM win 0 in ; frame (4,40)..(636,476).
# F. restore: (454,190) -> the ZOOMED zoom box (610,50): m100:-100, m56:-40.
#    l1,l0 -> FLAIR-ZOOM win 0 out ; frame (120,180)..(480,400) again.
# G. (610,50) -> APPS's centre in the moved window (223,222):
#      sum (-387,+172): m-100:43 x3, m-87:43.
#    l1,l0 -> FINDER-WIN-SELECT win=0 name=APPS count=1. Before the fix the
#    stored cell was (107,86) and this click reported DESELECT-ALL.
# H. (223,222) -> README's centre (155,222): m-68:0.
#    l1 ; m34:0, m34:0 -> (223,222) over APPS (lit: FINDER-DROP-HILITE
#    name=APPS) ; m0:4 -> (223,226) still over APPS ; l0
#    -> FINDER-MOVE name=README.TXT from=0 to=3 (APPS is cluster 3, mtools).
#
# PARKS (each trace's tail):
#   DRAG / RESTORE: (300,190) or (610,50) -> (620,460), the convention.
#     DRAG:    sum (+320,+270): m100:68 x3, m20:66
#     RESTORE: sum (+10,+410):  m10:100, m0:100 x3, m0:10
#   ZOOM: the zoomed window covers the convention's park point's probe-free
#     surroundings AND its bottom-right frame corner (635,475) sits under a
#     cursor parked at (620,460), so this trace parks INSIDE the zoomed body at
#     (400,400) -- white content that no zoomedwin probe reads (the icons are
#     in row 0, y < 114; the deep-body fill probe is (284,220)).
#     sum from (454,190): (-54,+210): m-54:100, m0:110
#   MOVE: sum from (223,226) (+397,+234): m100:59 x3, m97:57.
FLAIR_FOLLOW_DRAG_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:2,m-100:2,m-100:2,m-100:0,l1,m100:100,m0:20,l0,m100:68,m100:68,m100:68,m20:66
FLAIR_FOLLOW_ZOOM_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:2,m-100:2,m-100:2,m-100:0,l1,m100:100,m0:20,l0,m100:0,m70:0,l1,l0,l1,l0,m-16:0,l1,l0,m-54:100,m0:110
FLAIR_FOLLOW_RESTORE_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:2,m-100:2,m-100:2,m-100:0,l1,m100:100,m0:20,l0,m100:0,m70:0,l1,l0,l1,l0,m-16:0,l1,l0,m100:-100,m56:-40,l1,l0,m10:100,m0:100,m0:100,m0:100,m0:10
FLAIR_FOLLOW_MOVE_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:2,m-100:2,m-100:2,m-100:0,l1,m100:100,m0:20,l0,m100:0,m70:0,l1,l0,l1,l0,m-16:0,l1,l0,m100:-100,m56:-40,l1,l0,m-100:43,m-100:43,m-100:43,m-87:43,l1,l0,m-68:0,l1,m34:0,m34:0,m0:4,l0,m100:59,m100:59,m100:59,m97:57
