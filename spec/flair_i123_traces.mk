# spec/flair_i123_traces.mk -- the LOCKED input traces of test-flair-i123
# (beads initech-9u8w / initech-w96l: Initech 123 opens from the Finder as a
# disk application and takes typed entries). Locked spec-data (CLAUDE.md Rule
# 8, Rule 11); bound by every convention of spec/flair_app_launch_traces.mk
# (cursor starts (320,240); "m<dx>:<dy>" relative, y down, int8-safe; "l1"/
# "l0" left button; "k<chord>" one keystroke, "kshift-h" a capital H).
#
# THE SCENE: $(FLAIRTENANTS_IMG) + a gate-local copy of $(FLAIR_DATA_IMG),
# whose APPS folder holds, in directory order, TENANTFX.EXE (grid cell 0),
# CTENANT.EXE (cell 1) and 123.EXE (cell 2).
#
# GEOMETRY (derived, never measured off a render):
#   grid cell 2: sprite_x = 41 + 18 + 68*2 = 195, sprite_y = 106
#     => 123.EXE sprite (195,106)..(227,138), centre (211,122)
#   the 123 window frame (0,40)..(640,480) (face_a.c WIN_*), content
#     (1,62)..(619,459) by CalcDocContentRect; Face A cell (col c, line L)
#     at content-local (1 + 8c, 16L) -> GLOBAL (2 + 8c, 62 + 16L).
#   line 0 panel 1 (mode indicator cols 72..76 -> x 578..618, y 62..78)
#   line 3 the column border (y 110..126); line 4 = row 1 (y 126..142)
#   row 1 cells (width 9, after the 4-char border): A cols 4..12 -> x 34..106,
#     B 13..21, C 22..30, D cols 31..39 -> x 250..322
#
# WAYPOINTS: A, B as flair_app_launch_traces.mk (the volume, then APPS), then
#   C123 (123,102) -> 123.EXE (211,122): m88:20
#
# 1. FLAIR_I123_TYPE_SPEC -- the MINT01 first row, typed: A, B, C123 + double-
#    click (the launch), then "Hello" (shift-h e l l o: a LABEL, default
#    prefix '), Right (stores it, pointer to B1), "123", Right, "1.5", Right,
#    "-7", Enter. Row 1 must then read exactly as the real 1-2-3 showed it
#    (../lotus123-decomp/goldens/minted/MINT01.shots/02_entered.png) and the
#    pointer sits on D1 in READY. 38 tokens.
FLAIR_I123_TYPE_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:20,l1,l0,l1,l0,kshift-h,ke,kl,kl,ko,kright,k1,k2,k3,kright,k1,kdot,k5,kright,kminus,k7,kret

# 2. FLAIR_I123_QUIT_SPEC -- launch, then Ctrl-Q (the File > Quit ^Q of its own
#    bar, delivered as TBX_EVT_MENU, spec/toolbox_gate.h Sec 6a): 123 exits
#    and the frame returns to the never-launched baseline (3). 22 tokens.
FLAIR_I123_QUIT_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:20,l1,l0,l1,l0,kctrl-q

# 3. FLAIR_I123_PRE_SPEC -- the RESTORE baseline: A, B, C123 and ONE click (the
#    icon selected, never launched). 19 tokens.
FLAIR_I123_PRE_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:20,l1,l0
