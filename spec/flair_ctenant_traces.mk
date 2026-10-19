# spec/flair_ctenant_traces.mk -- the LOCKED input trace of test-flair-ctenant
# (bead initech-w96l: a C application is a disk tenant; bead initech-8zii: what
# a tenant draws for a plain keyDown / mouseDown reaches the screen).
# Locked spec-data (CLAUDE.md Rule 8, Rule 11); bound by every convention of
# spec/flair_app_launch_traces.mk (cursor starts (320,240); "m<dx>:<dy>"
# relative, y down, int8-safe; "l1"/"l0" left button; "k<chord>" a key).
#
# THE SCENE: $(FLAIRTENANTS_IMG) + a gate-local copy of $(FLAIR_DATA_IMG).
# APPS holds, in directory order, TENANTFX.EXE (grid cell 0) and CTENANT.EXE
# (cell 1) -- the Finder lays a folder out in record order.
#
# GEOMETRY (derived, never measured off a render):
#   APPS window content.left 41, content.top 102 (flair_app_launch_traces.mk)
#   grid cell i: sprite_x = 41 + 18 + 68 i, sprite_y = 102 + 4 = 106
#     => CTENANT.EXE (i=1) sprite (127,106)..(159,138), centre (143,122)
#   the CTENANT window frame (180,170)..(470,330) (ctenant.c WIN_*); content
#     by CalcDocContentRect (frame 1, title 22, scroll bars 16 + body 4):
#     (181,192)..(449,309)
#   its draws (content-local -> global):
#     keyDown   TEXTDRAW  "Key q" at (8,40)  -> (189,232), 16 tall
#               DRAWCELLS "Key q" at (8,64)  -> (189,256)..(229,272) (5 cells)
#     mouseDown DRAWCELLS "Click" at (8,96)  -> (189,288)..(229,304)
#
# FLAIR_CTENANT_SPEC: A, B (flair_app_launch_traces.mk), then
#   C1 (123,102) -> CTENANT (143,122): m20:20 ; l1,l0,l1,l0 -> the launch
#   K  kq: a plain keyDown 'q' to the foreground tenant
#   D  (143,122) -> inside the content (250,300): m107:89,m0:89 ; l1,l0
#   P  park (250,300) -> (620,460): m100:40,m100:40,m100:40,m70:40
# 30 tokens: inside the flagship pump's 250-tick budget.
FLAIR_CTENANT_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m20:20,l1,l0,l1,l0,kq,m107:89,m0:89,l1,l0,m100:40,m100:40,m100:40,m70:40

# ===========================================================================
# THE REPAINT TRACES (bead initech-6qsk, audit L117): the readouts a keyDown
# and a mouseDown left in the CTENANT window must survive every updateEvt --
# a title drag and an obscure/reveal -- because the tenant repaints them from
# retained state (Inside Macintosh: Macintosh Toolbox Essentials, Window
# Manager "Updating the Content" / Event Manager update events: the
# application must be able to redraw ANY part of its content at any time;
# ADR-0013 App Contract: the tenant receives updateEvt through the gate).
# Both run on the HOLD image (FLAIR_TEN_TICK_BUDGET=1500, see
# test-flair-held-gestures): 34 / 45 tokens do not fit the 250-tick demo on a
# loaded host. Same scene and prefix as FLAIR_CTENANT_SPEC (A, B, C1, K, D),
# which leaves the cursor at (250,300) inside the content.
#
# GEOMETRY (derived, never measured off a render):
#   T  title drag: CTENANT title bar y [170,192) (frame top 170 + title 22),
#      press at (300,180) -- right of the go-away box (184..196), left of the
#      right-hand title widgets: (250,300) -> m50:-120 ; l1 ; m25:25,m25:25
#      ; l0 at (350,230) -> FLAIR-DRAG ... (180,170)->(230,220)
#      CTENANT now (230,220)..(520,380); its content left = frame.left + 6
#      = 236 (initech-tdnl.51), top 220 + 22 = 242, so the readouts sit at
#        TEXTDRAW  "Key q" (8,40) -> (244,282)..(284,298)
#        DRAWCELLS "Key q" (8,64) -> (244,306)..(284,322)
#        DRAWCELLS "Click" (8,96) -> (244,338)..(284,354)
#      and the white reference (440,300) moves to (490,350).
#   O  obscure: the APPS window (40,80)..(400,300) title bar y [80,102) is
#      clear of CTENANT (top 220): (350,230) -> m-100:-100,m-30:-40 = (220,90)
#      ; l1 ; m0:50,m0:50 ; l0 -> APPS (40,180)..(400,400), raised over the
#      Finder switch, covering CTENANT x [230,400) y [220,380) -- ALL three
#      readout boxes (x <= 284 < 400) but not the reference x 490.
#   R  reveal: CTENANT's title bar is still visible at x [400,520) y
#      [220,242): (220,190) -> m100:20,m100:20,m10:0 = (430,230) ; l1,l0 --
#      a title click: activation only, no content mouseDown, so a "Click"
#      re-drawn after it can only come from the repaint.
#   P  park at (620,460): from (350,230) m100:100,m100:100,m70:30; from
#      (430,230) m100:100,m90:100,m0:30.
#
# FLAIR_CTENANT_DRAG_SPEC: prefix, T, P (the drag repaint, dumped alone).
FLAIR_CTENANT_DRAG_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m20:20,l1,l0,l1,l0,kq,m107:89,m0:89,l1,l0,m50:-120,l1,m25:25,m25:25,l0,m100:100,m100:100,m70:30
# FLAIR_CTENANT_REVEAL_SPEC: prefix, T, O, R, P (drag, then obscure/reveal).
FLAIR_CTENANT_REVEAL_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m20:20,l1,l0,l1,l0,kq,m107:89,m0:89,l1,l0,m50:-120,l1,m25:25,m25:25,l0,m-100:-100,m-30:-40,l1,m0:50,m0:50,l0,m100:20,m100:20,m10:0,l1,l0,m100:100,m90:100,m0:30
