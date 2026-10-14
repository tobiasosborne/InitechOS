# spec/flair_fonts_traces.mk -- the LOCKED input traces of test-flair-fonts
# (bead initech-tdnl.68, Font Manager slice 2: synthesized text styles, the
# ellipsis-truncated title, text clipped to its window, a Style menu drawn in
# its own styles). Locked spec-data (CLAUDE.md Rule 8, Rule 11); bound by every
# convention of spec/flair_app_launch_traces.mk (cursor starts (320,240);
# "m<dx>:<dy>" relative, y down, int8-safe; "l1"/"l0" left button; "w<ms>" a
# pause).
#
# THE SCENE: $(FLAIRTENANTS_IMG) (the flagship desktop) + the gate's own data
# volume $(FLAIR_DATA_FONTS_IMG): the flagship volume's recipe with FONTS.EXE
# (os/apps/fonts/fonts.c) shipped in APPS grid cell 3 under the name WORD.EXE
# (flair-data-rules' fourth slot; the gate-local-variant pattern), so the
# launch reuses spec/flair_iword_traces.mk's waypoints unchanged.
#
# GEOMETRY (derived, never measured off a render):
#   launch: A, B (flair_app_launch_traces.mk), CWORD (123,102) -> cell 3
#     (279,122): m78:10,m78:10, double-click (flair_iword_traces.mk).
#   the FONTS window frame (300,170)..(520,400) (fonts.c WIN_*); content by
#     CalcDocContentRect (frame 1, title 22, scroll bars 16 + body 4):
#     (301,192)..(499,379). Title bar: cell top 170 + FLAIR_CHROME_TITLE_
#     TEXT_TOP_OFF 3 = 173; stripe run [300+21, 520-38) = [321,482).
#   specimen lines (fonts.c paint), content-local (6, y) -> GLOBAL (307, 192+y),
#     one 16-row Chicago cell each, baseline cell row 12:
#     L0 y=2   "Hamburger" plain        -> (307,194)
#     L1 y=20  "Hamburger" bold         -> (307,212)
#     L2 y=38  "Hamburger" italic       -> (307,230)
#     L3 y=56  "jumpy gyp" plain        -> (307,248)
#     L4 y=74  "jumpy gyp" underline    -> (307,266)
#     L5 y=92  "Hamburger" bold+italic+underline -> (307,284)
#     L6 y=116 "Saving tables to disk... PC LOAD LETTER" bold -> (307,308),
#        wider than the content: clipped at x = 499.
#   the tenant bar in band 2 (y 20..40; has_apple: the Apple slot is 20 px):
#     File slot [20, 20+7+23+7) = [20,57) (F7 i4 l4 e8, NFNT 5478), Style
#     slot [57, 57+7+33+7) = [57,104) (S7 t6 y8 l4 e8); its press point
#     (80,30). The Style panel drops below band 2 at x = 57.
#
# 1. FLAIR_FONTS_SPEC -- launch, then park the cursor on the desktop (279,460)
#    clear of the FONTS window: m0:100 x3, m0:38. 27 tokens: inside the
#    flagship pump's 250-tick budget. The dump is taken after FLAIR-LIVE-OK.
FLAIR_FONTS_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m78:10,m78:10,l1,l0,l1,l0,m0:100,m0:100,m0:100,m0:38

# 2. FLAIR_FONTS_MENU_SPEC -- launch exactly as 1, then the Style menu is
#    reached by a CROSS-MENU DRAG: (279,122) -> the File title (38,30):
#    m-100:-46,m-100:-46,m-41:0; press (File, menu 129, drops); drag right to
#    the Style title (80,30): m42:0 -> FLAIR-MENU-XDROP menu=130, and the dump
#    is taken there, while held, no item hilited. (A FLAIR-MENU-DROP-gated dump
#    would make the harness wait up to 750 ms after EVERY button-down --
#    harness/emu/qemu.c capture_wait_after_event -- splitting the launch's
#    double-clicks; an XDROP-gated dump waits only on held moves.) Then hold
#    1 s, leave the panel (180,130) and release: a cancel. 29 tokens.
FLAIR_FONTS_MENU_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m78:10,m78:10,l1,l0,l1,l0,m-100:-46,m-100:-46,m-41:0,l1,m42:0,w1000,m100:100,l0
