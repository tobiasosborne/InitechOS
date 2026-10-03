# spec/flair_input_traces.mk -- the LOCKED input traces for the input-handling
# emulator gates of the second FLAIR desktop audit (beads initech-tdnl.58 /
# .59 / .60; docs/audits/2026-10-04-flair-gui-codex-pass2/REPORT.md findings
# G11, G01, G02). Locked spec-data (CLAUDE.md Rule 8, Rule 11): deterministic,
# versioned, never silently edited to make a test pass.
#
# Conventions are spec/flair_disk_windows_traces.mk's verbatim (QMP relative
# moves from the screen centre (320,240), y DOWN-positive, every component
# int8-safe |c| <= 100; "l1"/"l0" left button; "k<chord>" a keystroke). Two
# tokens are new with this file (harness/emu/qemu.c qmp_inject_mouse):
#   "K<key>:<0|1>" one explicit key transition (press-and-hold / release),
#                  the audit's `keyheld ctrl true` / `keyheld ctrl false`;
#   "w<ms>"        hold every input state for <ms> ms (the audit's `pause`).
# Geometry of the root disk window (opened by the volume double-click at
# (600,64), unmoved default frame (20,60)..(380,280)), from
# spec/flair_file_ops_traces.mk: README.TXT centre (55,102), APPS centre
# (123,102).
#
# ===========================================================================
# 1. FLAIR_MODREL_SPEC -- G11, modifiers released stay released (tdnl.58).
# ===========================================================================
#   open the root window            m100:-88,m100:-88,m80:0,l1,l0,l1,l0
#   LEFT Ctrl press, release, n     Kctrl:1,Kctrl:0,kn
#        (audit: plain N created NEWFOLD -- must be NO FINDER-CMD)
#   RIGHT Ctrl press, release, w    Kctrl_r:1,Kctrl_r:0,kw
#        (E0 1D / E0 9D; audit: plain W closed the root -- must stay open)
#   Shift press, release            Kshift:1,Kshift:0
#   click README.TXT, then APPS     m-100:10 x3,m-100:8,m-77:0 -> (123,102);
#                                   m-68:0 -> (55,102) l1,l0 ; m68:0 l1,l0
#        (audit: count=2, Shift-extend latched -- must be count=1)
#   POSITIVE CONTROL: Ctrl-N chord  kctrl-n -> the ONE FINDER-CMD,
#        FINDER-CMD id=2 name=NEW_FOLDER src=key sel=1 (APPS selected), so the
#        window was open and the chord path still works.
FLAIR_MODREL_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,Kctrl:1,Kctrl:0,kn,Kctrl_r:1,Kctrl_r:0,kw,Kshift:1,Kshift:0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,m-68:0,l1,l0,m68:0,l1,l0,kctrl-n

# ===========================================================================
# 2. G01 -- gestures are tracked until mouseUp, however long they are held
#    (bead initech-tdnl.59). Every hold is w3000: twice the old 150-tick
#    (~1.5 s) per-gesture guard, so the pre-fix kernel provably cuts it.
#    Both traces run on the HOLD image (FLAIR_TEN_TICK_BUDGET=1500, ~15 s of
#    pump life) because a 3 s hold cannot fit the default 250-tick demo.
# ===========================================================================
# 2a. FLAIR_HELD_DRAG_SPEC -- HELLO (struct (60,60)..(360,260),
#     spec/flair_tenants_demo.h; grow cell (349,249), flair_solid_traces.mk).
#   title press   m-100:-100,m-20:-70 -> (200,70) ; l1
#   first leg     m50:60 -> (250,130) ; HOLD w3000 (outline still live)
#   second leg    m80:50 -> (330,180) ; l0
#        -> FLAIR-DRAG ... (60,60)->(190,170)   (pre-fix: (110,120), the
#           second leg ignored -- the audit's "slow drag")
#   grow press    HELLO now (190,170)..(490,370): grow cell (479,359)
#                 m100:100,m49:79 -> (479,359) ; l1
#   first leg     m-60:-40 -> (419,319) ; HOLD w3000
#   second leg    m-60:-40 -> (359,279) ; l0
#        -> FLAIR-GROW ... (300,200)->(180,120)  (pre-fix: (240,160))
#   park          m100:100,m100:81,m61:0 -> (620,460)
FLAIR_HELD_DRAG_SPEC := m-100:-100,m-20:-70,l1,m50:60,w3000,m80:50,l0,m100:100,m49:79,l1,m-60:-40,w3000,m-60:-40,l0,m100:100,m100:81,m61:0

# 2b. FLAIR_HELD_MENU_SPEC -- the Finder (root window open, band 2 = the
#     Finder bar; File title at (42,28), item rows 16 px from y=41: New Folder
#     [41,57) centre 49, Close Window (item 4) [89,105) centre 97).
#   open the root window          m100:-88,m100:-88,m80:0,l1,l0,l1,l0 (600,64)
#   to README.TXT                 m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,
#                                 m-45:0 -> (55,102) ; l1
#   icon drag, first leg          m34:0 -> (89,102) ; HOLD w3000
#   second leg onto APPS          m34:0 -> (123,102) ; l0
#        -> FINDER-MOVE name=README.TXT from=0 to=3  (pre-fix: the drop
#           committed at (89,102), never reaching APPS)
#   to the band-2 File title      m-81:-74 -> (42,28) ; l1 (FLAIR-MENU-DROP)
#   onto New Folder               m35:22 -> (77,50) ; HOLD w3000 (the audit:
#                                 the menu vanished, New Folder EXECUTED)
#   down to Close Window          m0:47 -> (77,97) ; l0
#        -> FINDER-CMD id=4 name=CLOSE_WINDOW src=mouse ... and NO New Folder
#   park                          m100:100 x3,m100:63,m100:0,m43:0 -> (620,460)
FLAIR_HELD_MENU_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m34:0,w3000,m34:0,l0,m-81:-74,l1,m35:22,w3000,m0:47,l0,m100:100,m100:100,m100:100,m100:63,m100:0,m43:0

# ===========================================================================
# 3. FLAIR_BOX_CANCEL_SPEC -- G02, the title-bar boxes act on RELEASE INSIDE
#    (bead initech-tdnl.60; TrackGoAway / TrackBox: "returns TRUE if the
#    mouse button is released inside the box", system7-decomp
#    specs/toolbox/window-manager.md Sec 5). HELLO's widgets, sampled sys8
#    rules (flair_solid_traces.mk): close (74,70), zoom (332,70), collapse
#    (348,70); HELLO struct (60,60)..(360,260).
# ===========================================================================
#   close: press, drag OUT, release   m-100:-100,m-100:-70,m-46:0 -> (74,70) ;
#                                     l1 ; m100:100 -> (174,170) ; l0
#        (audit: TENANT-EXIT on the press -- must be cancelled, in=0)
#   zoom: press, drag out, release    m100:-100,m58:0 -> (332,70) ; l1 ;
#                                     m0:60 -> (332,130) ; l0   (in=0)
#   collapse: same                    m16:-60 -> (348,70) ; l1 ;
#                                     m-50:50 -> (298,120) ; l0 (in=0)
#   close: press, out, BACK IN, release
#                                     m-100:-50,m-100:0,m-24:0 -> (74,70) ;
#                                     l1 ; m60:60 ; m-60:-60 ; l0 (in=1:
#                                     FLAIR-CLOSE + FLAIR-TENANT-EXIT HELLO)
#   park                              m100:100 x3,m100:90,m100:0,m46:0 -> (620,460)
FLAIR_BOX_CANCEL_SPEC := m-100:-100,m-100:-70,m-46:0,l1,m100:100,l0,m100:-100,m58:0,l1,m0:60,l0,m16:-60,l1,m-50:50,l0,m-100:-50,m-100:0,m-24:0,l1,m60:60,m-60:-60,l0,m100:100,m100:100,m100:100,m100:90,m100:0,m46:0
