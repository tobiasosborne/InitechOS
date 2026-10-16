# spec/flair_calc_traces.mk -- the LOCKED input traces of test-flair-apple-
# menu and test-flair-calc (beads initech-tdnl.47 / initech-tdnl.17). Locked
# spec-data (CLAUDE.md Rule 8, Rule 11); bound by every convention of
# spec/flair_app_launch_traces.mk (cursor starts (320,240); "m<dx>:<dy>"
# relative, y down, int8-safe; "l1"/"l0" left button; "k<qcode>" one key in
# stream order; "w<ms>" a pause).
#
# THE SCENE: the flagship desktop (HELLO front, NOTES behind, band 1 the
# system bar WITH the Apple glyph) + the flagship data volume (APPS\CALC.EXE).
#
# GEOMETRY (derived, never measured off a render):
#   the band-1 Apple glyph: the slot [0, 33) x [0, 20) (chrome_metrics.h
#     FLAIR_CHROME_MENU_APPLE_W); press at (20,9) -- the point the Mac OS 8.1
#     mint pressed (s8_apple_menu.png), so both frames carry the pointer at
#     the same hotspot: m-100:-77 x3 from (320,240).
#   the Apple panel (menu.c, menu.h Sec 3a): left x 9, top row 19, two-row
#     top anatomy, item rows 16 px, divider 6 px: About This Computer...
#     rows 21..36, the divider 37..42, Calculator 43..58. Release on About at
#     (60,28) = m40:19; on Calculator at (60,50) = m40:41.
#   HELLO's close box: HELLO's frame (60,60) (spec/flair_tenants_demo.h),
#     the Platinum close widget L+4 / T+4, 13 px: press at (70,70) =
#     m-125:-85 x2 from (320,240). HELLO closes so nothing but the desktop
#     lies under the Calculator's rounded corners (the capture's corners show
#     its desktop).
#   the Calculator (spec/calc/calc_layout.h): frame (267,94) 106x168, content
#     origin (268,113); key centres = content + key l/t + (9,8):
#       '9' (329,175)  '-' (351,175)  '2' (307,219)  '=' (307,153)
#   a desktop point clear of every window: (150,400).
#
# 1. FLAIR_APPLE_DROP_SPEC -- press the glyph, HOLD 1 s (the dump is taken at
#    FLAIR-MENU-DROP menu=1, the panel up, no row hilited), drag to the
#    Calculator row, hold, release: Calculator launches.
FLAIR_APPLE_DROP_SPEC := m-100:-77,m-100:-77,m-100:-77,l1,w1000,m40:41,w600,l0,w800

# 2. FLAIR_APPLE_ABOUT_SPEC -- the same press, release on About This
#    Computer...: the alert (dump at APPLE-ABOUT up).
FLAIR_APPLE_ABOUT_SPEC := m-100:-77,m-100:-77,m-100:-77,l1,w300,m40:19,w300,l0,w1500

# 3. FLAIR_CALC_KEYS_SPEC -- close HELLO, Apple > Calculator, then TYPE
#    1 2 + 3 = (the keyboard path; '+' is shift-equal on the PC). The
#    readout must read 15: s8_calc_12plus3eq.png, the Mac after the same keys.
FLAIR_CALC_KEYS_SPEC := m-125:-85,m-125:-85,l1,l0,w400,m-50:-61,l1,w300,m40:41,w300,l0,w1000,k1,w150,k2,w150,kshift-equal,w150,k3,w150,kequal,w600

# 4. FLAIR_CALC_CLICKS_SPEC -- close HELLO, Apple > Calculator, then CLICK
#    9 - 2 = (the click path) and click the desktop (the Calculator goes
#    inactive). The readout must read 7, inactive: s8_calc_inactive.png.
FLAIR_CALC_CLICKS_SPEC := m-125:-85,m-125:-85,l1,l0,w400,m-50:-61,l1,w300,m40:41,w300,l0,w1000,m90:42,m90:42,m89:41,l1,l0,w300,m22:0,l1,l0,w300,m-44:44,l1,l0,w300,m0:-66,l1,l0,w300,m-79:124,m-78:123,l1,l0,w800
