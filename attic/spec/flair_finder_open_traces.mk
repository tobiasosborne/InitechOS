# spec/flair_finder_open_traces.mk -- the LOCKED input traces of
# test-flair-finder-open (bead initech-fbo4, audit 2026-10-23 desktop L004):
# Finder File > Open and Ctrl-O launch a selected APPLICATION exactly as a
# double-click does, through the ONE launch verb (kmain flair_live_launch_app).
# Locked spec-data (CLAUDE.md Rule 8, Rule 11); bound by every convention of
# spec/flair_app_launch_traces.mk (cursor starts (320,240); "m<dx>:<dy>"
# relative, y down, int8-safe; "l1"/"l0" left button; "k<chord>" a key in
# stream order; "w<ms>" hold).
#
# GROUND TRUTH: Apple_Macintosh_System_Software_Users_Guide_V6.0.pdf PDF p. 45
# "Open the TeachText icon, either by double-clicking it or by selecting it and
# choosing Open from the File menu" (TeachText is an APPLICATION); PDF p. 38
# "open an icon by selecting it and choosing Open from the File menu. And
# there's another way to do the same thing -- a shortcut called
# double-clicking". The three routes are ONE operation.
#
# THE SCENE: $(FLAIRTENANTS_IMG) + a gate-local copy of $(FLAIR_DATA_IMG).
# APPS holds, in directory order, TENANTFX.EXE (cell 0) and CTENANT.EXE
# (cell 1) -- spec/flair_ctenant_traces.mk.
#
# GEOMETRY (derived, never measured off a render):
#   A (320,240) -> VOLUME (600,64); B (600,64) -> APPS in win 0 (123,102)
#     (flair_app_launch_traces.mk waypoints, verbatim)
#   S (123,102) -> CTENANT.EXE sprite centre (143,122): m20:20 ; l1,l0
#     -> FINDER-ICON-SELECT name=CTENANT.EXE count=1 (a SINGLE click)
#   F (143,122) -> the FILE title in band 2 (38,30): File's slot is x[20,57)
#     (flair_disk_windows_traces.mk D: first title at FLAIR_MENU_APPLE_W(20),
#     proportional Chicago 12), band 2 is rows [20,40): m-100:-92,m-5:0
#   O press, drag to the OPEN row, release: item rows begin at screen y 41 and
#     are FLAIR_MENU_ITEM_H(16) tall (flair_disk_windows_traces.mk E); File's
#     rows are New Folder (1) y[41,57), Open (2) y[57,73) (finder_menu.c F4.2,
#     no divider between them) -> l1 ; m0:35 -> (38,65) ; l0
#     -> MenuSelect = (512,2) -> FINDER-CMD id=3 name=OPEN src=mouse sel=1
#   K kctrl-o -> MenuKey over the Finder bar -> the SAME (512,2)
#     -> FINDER-CMD id=3 name=OPEN src=key sel=1
#   Q kctrl-q -> the foreground CTENANT's File > Quit (ctenant.c: 'Q')
#     -> TENANT-MENU menu=129 item=1 src=key ; TENANT-EXIT rc=0 via=exit ;
#     TENANT-HEAPAVAIL (teardown done; process.c promotes the Finder)
#   W kctrl-w -> THE RETURN, PROVED: the Finder is foreground again with a
#     live bar, so its own chord closes the APPS window
#     -> FINDER-CMD id=4 name=CLOSE_WINDOW src=key ; FINDER-CLOSE-WINDOW win=1
#   P park at (620,460): from (143,122) dx 477 dy 338: m120:85,m120:85,
#     m120:85,m117:83 ; from (38,65) dx 582 dy 395: m120:100,m120:100,
#     m120:100,m120:95,m102:0
#   CTENANT window frame (180,170)..(470,330) (ctenant.c WIN_*); with the APPS
#     window (40,80)..(400,300) raised over it, its title bar stays visible at
#     x[400,470) y[170,192): T (143,122) -> (435,181): m100:30,m100:29,m92:0
#
# 1. KEY: A, B, S, K (the launch), Q (quit), W (the return), P.
#    26 tokens. Also the finder_open_app clip (CLAUDE.md Rule 14).
FLAIR_FINDER_OPEN_KEY_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m20:20,l1,l0,kctrl-o,kctrl-q,kctrl-w,m120:85,m120:85,m120:85,m117:83
# 2. MENU: A, B, S, F, O (the launch by the mouse), Q, W, P. 31 tokens.
FLAIR_FINDER_OPEN_MENU_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m20:20,l1,l0,m-100:-92,m-5:0,l1,m0:35,l0,kctrl-q,kctrl-w,m120:100,m120:100,m120:100,m120:95,m102:0
# 3. DBL: A, B, S as a DOUBLE-click (the launch), Q, W, P. 27 tokens. The
#    third route, graded by the SAME oracle as 1 and 2 (L004: one operation).
FLAIR_FINDER_OPEN_DBL_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m20:20,l1,l0,l1,l0,kctrl-q,kctrl-w,m120:85,m120:85,m120:85,m117:83
# 4. BUSY (runs on the HOLD image, FLAIR_TEN_TICK_BUDGET=1500): A, B, S as a
#    double-click (CTENANT resident and foreground), then l1,l0 on the icon
#    again -- an activate-only content click: the Finder comes forward and
#    raises APPS over CTENANT (flair_app_launch_traces.mk DOUBLE precedent) --
#    w500 so the next click is a fresh gesture, l1,l0 selects CTENANT.EXE,
#    K asks for a SECOND launch -> TENANT-SLOT-BUSY name=CTENANT.EXE and the
#    launch notice (FINDER-ALERT reason=launch); kesc dismisses it; T clicks
#    the resident tenant's title bar (activation only); Q quits it; W proves
#    the return. Serial-only: no park. 35 tokens.
FLAIR_FINDER_OPEN_BUSY_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m20:20,l1,l0,l1,l0,l1,l0,w500,l1,l0,kctrl-o,kesc,m100:30,m100:29,m92:0,l1,l0,kctrl-q,kctrl-w
