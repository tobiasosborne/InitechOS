# spec/flair_scroll_traces.mk -- the LOCKED input traces for the WORKING
# SCROLL BARS emulator gate (bead initech-tdnl.35; audit
# docs/audits/2026-10-03-flair-gui-codex/REPORT.md F02, the P0 "scroll bar
# input drags the window").
# Locked spec-data (CLAUDE.md Rule 8, Rule 11): deterministic, versioned, never
# silently edited to make a test pass.
#
# Conventions are spec/flair_disk_windows_traces.mk's verbatim (QMP relative
# moves from the screen centre (320,240), y DOWN-positive, every component
# int8-safe |c| <= 100; "l1"/"l0" left button; "w<ms>" a held state; every
# trace ends PARKED at (620,460), clear of every probe).
#
# THE VOLUME (Makefile FLAIR_SCROLL_DATA_IMG): the flagship recipe plus 18
# empty files F00.TXT..F17.TXT, so the root lists README.TXT, APPS, F00..F17
# and the boot-created DESKTOP.DB and TRASH: 22 entries -- the audit's own
# overflow count. The scrollfit legs use the flagship 4-entry volume.
#
# THE GEOMETRY, by hand (finder_windows.h Sec 2/3, window.c
# CalcDocContentRect, os/flair/winscroll.h, spec/chrome_metrics.h):
#   root frame (20,60)..(380,280); content (21,82)..(359,259), 177 high
#   vertical bar x [359,375) y [81,260): up arrow y [81,97), down [244,260);
#   thumb track lo 97, span 132; 6 rows -> content 315 -> max 138, page 161;
#   line step 16.
#   The audit clicked the down arrow at (367,251) and the up arrow at
#   (367,89): those are the points used here.
#
# --- waypoint arithmetic -------------------------------------------------
# A. the volume double-click (flair_disk_windows_traces.mk trace 1 A):
#      m100:-88, m100:-88, m80:0 -> (600,64) ; l1,l0,l1,l0
#    -> FINDER-OPEN-VOLUME win=0 n=22 (n=4 on the flagship volume).
# B. (600,64) -> the DOWN arrow (367,251): sum (-233,+187):
#      m-100:100, m-100:87, m-33:0
# C. (600,64) -> the track below the thumb (367,200): sum (-233,+136):
#      m-100:100, m-100:36, m-33:0
# D. (600,64) -> the thumb at value 0, y [97,112), point (367,104):
#      sum (-233,+40): m-100:40, m-100:0, m-33:0
# E. (367,251) after three down clicks (value 48: F06.TXT, index 8, row 2
#    col 0, sprite (39, 86+104-48 = 142), centre (55,158)): sum (-312,-93):
#      m-100:-93, m-100:0, m-100:0, m-12:0
# F. (367,251) -> the horizontal bar (200,267): sum (-167,+16):
#      m-100:16, m-67:0
# PARKS to (620,460):
#   from (367,251): (+253,+209): m100:100, m100:100, m53:9
#   from (367,200): (+253,+260): m100:100, m100:100, m53:60
#   from (367,170): (+253,+290): m100:100, m100:100, m53:90
#   from (200,267): (+420,+193): m100:100, m100:93, m100:0, m100:0, m20:0
#
# THE TRACES
#   SCROLL_ARROW  A, B, three clicks -> value 16, 32, 48; park.
#                 graded: leg scrollarrow; no FLAIR-DRAG anywhere.
#   SCROLL_SELECT A, B, three clicks, E, one click on F06.TXT where it is
#                 DRAWN -> FINDER-WIN-SELECT win=0 name=F06.TXT count=1.
#   SCROLL_PAGE   A, C, one click below the thumb -> 0+161 clamps to 138.
#                 graded: leg scrollpage.
#   SCROLL_THUMB  A, D, press on the thumb, drag +66 (m0:33 x2), release:
#                 thumb leading edge 97 -> 163 -> value ceil(66*138/132) = 69.
#                 graded: leg scrollthumb.
#   SCROLL_HOLD   A, B, press and HOLD the down arrow 1500 ms: one step, then
#                 the held repeat (winscroll.h: after 30 ticks, every 5) runs
#                 the view to its end -> value 138; graded: leg scrollpage.
#   SCROLL_FIT    (flagship volume) A, B, click -> FLAIR-SCROLL-IGNORED win 0 v;
#                 F, click -> FLAIR-SCROLL-IGNORED win 0 h; park.
#                 graded: leg scrollfit (DISABLED bars, icons unmoved).
FLAIR_SCROLL_ARROW_SPEC  := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:100,m-100:87,m-33:0,l1,l0,l1,l0,l1,l0,m100:100,m100:100,m53:9
FLAIR_SCROLL_SELECT_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:100,m-100:87,m-33:0,l1,l0,l1,l0,l1,l0,m-100:-93,m-100:0,m-100:0,m-12:0,l1,l0
FLAIR_SCROLL_PAGE_SPEC   := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:100,m-100:36,m-33:0,l1,l0,m100:100,m100:100,m53:60
FLAIR_SCROLL_THUMB_SPEC  := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:40,m-100:0,m-33:0,l1,m0:33,m0:33,l0,m100:100,m100:100,m53:90
FLAIR_SCROLL_HOLD_SPEC   := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:100,m-100:87,m-33:0,l1,w1500,l0,m100:100,m100:100,m53:9
FLAIR_SCROLL_FIT_SPEC    := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:100,m-100:87,m-33:0,l1,l0,m-100:16,m-67:0,l1,l0,m100:100,m100:93,m100:0,m100:0,m20:0
