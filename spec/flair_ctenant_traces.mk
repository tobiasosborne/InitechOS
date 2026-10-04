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
