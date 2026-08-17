# spec/flair_solid_traces.mk -- the LOCKED input traces for the FLAIR
# live-desktop SOLIDITY gate legs (epic initech-av7s; beads initech-gofc /
# initech-rqz5).  Locked spec-data (CLAUDE.md Rule 8, Rule 11): deterministic,
# versioned, never silently edited to make a test pass.
#
# Same shape as spec/flair_appswitch_trace.mk: comma-separated QMP
# relative-mouse specs for qemu_harness --mouse.  Cursor starts at the screen
# centre (320,240); m<dx>:<dy> with y screen-DOWN-positive (the post-rgt8
# producer convention); every component int8-safe (|c| <= 100 here).
# Geometry: spec/flair_tenants_demo.h (HELLO struct 60,60..360,260 with go-away
# box at ~(69..80, 64..75); NOTES struct 260,120..560,340, visible title
# x >= 360).  Grader: tools/ppm_flair_solid_check.c (legs A/B/C/D/E/G/H).
#
# Leg A -- CLOSE-EXPOSE CONTENT.  Click HELLO's go-away box at (74,70):
#   delta from centre = (-246,-170), split m-100:-100, m-100:-70, m-46:0;
#   l1,l0 = the closing click.  Post: the exposed NOTES overlap must read
#   NOTES_FILL (the owner repainted), graded by solid_check leg A on a dump
#   taken after the FLAIR-CLOSE marker.
FLAIR_SOLID_CLOSE_SPEC := m-100:-100,m-100:-70,m-46:0,l1,l0

# Leg B -- DRAG PRESERVES CONTENT.  First the O-5 activating click on NOTES's
# sliver at (460,300) (same waypoint as the locked FLAIR_APPSWITCH_SPEC:
# m100:60,m40:0,l1,l0), then grab NOTES's title bar at (450,130)
# (delta (-10,-170): m-10:-100, m0:-70), l1, drag (-60,+60) -> struct
# (260,120)->(200,180), l0.  The grader's SOLID_B_NOTES_L/T mirror the
# (-60,+60) delta -- keep in sync with ppm_flair_solid_check.c.
FLAIR_SOLID_DRAG_SPEC  := m100:60,m40:0,l1,l0,m-10:-100,m0:-70,l1,m-60:60,l0

# Leg C -- ACTIVATION CHROME.  The locked O-5 app-switch trace verbatim
# (spec/flair_appswitch_trace.mk): activate the background NOTES; the dump
# (after FLAIR-DISPATCH app=NOTES) must show NOTES's title band ACTIVE across
# its full width, HELLO's flat inactive, and no stale HELLO edge inside
# NOTES's title band.
FLAIR_SOLID_SWITCH_SPEC := m70:30,m70:30,l1,l0

# Leg D -- MENU CANCEL RESTORE (beads initech-b3hl/-j0vt; DQ2). HELLO is
# foreground from boot, so band 2 owns its Photoshop bar. Reuse leg E's exact
# int8-safe route from (320,240) to File at (30,30): (-100,-100),(-100,-100),
# (-90,-10). Press to drop File (menuID 256), then move (+30,0) within band 2
# to (60,30). Photoshop has no Apple slot; File is x[0,46), so x=60 is inside
# the DIFFERENT Edit title x[46,92) (menuID 257). This forces the cross-title
# old-panel erase while the button remains held. Release
# there, still in the bar (band-2-local y=10), so no item row is under the
# release and MenuSelect returns sel=0 (cancel). The solid gate captures a
# no-input PRE boot and this POST boot only after FLAIR-LIVE-OK; byte-equality
# therefore requires both the mid-track File erase and the track-end Edit erase
# to restore the exact tenant-owned frame through the DQ2 damage spine.
FLAIR_SOLID_MENUCANCEL_SPEC := m-100:-100,m-100:-100,m-90:-10,l1,m30:0,l0

# Leg E -- BAND-2 ACTIVE-TENANT MENU. HELLO is foreground from boot, so band 2
# owns its Photoshop bar (File menuID 256). From cursor start (320,240), move to
# File at (30,30): delta (-290,-210), split into int8-safe packets
# (-100,-100),(-100,-100),(-90,-10). Press, then move (+15,+35) to (45,65).
# In band-2-local coordinates (subtract SHELL_MENUBAR2_TOP=20), release is
# (45,45): the panel starts at local y=20, its 1px frame ends at y=21, item 1 is
# [21,37), and item 2 is [37,53), so y=45 selects Photoshop File item 2. Every
# packet component has |c| <= 100. The trace stays inside ONE menu; leg D owns
# the cross-title and track-end restore contract for beads initech-b3hl/-j0vt.
FLAIR_SOLID_MENU2_SPEC := m-100:-100,m-100:-100,m-90:-10,l1,m15:35,l0

# Leg G -- DRAG CLAMP. First activate NOTES at (460,300), then grab its title
# bar at (450,130), exactly as leg B. Drag the cursor to the on-screen waypoint
# (5,5): delta (-445,-125), split into int8-safe packets as
# (-100,-100),(-100,-25),(-100,0),(-100,0),(-45,0). The unconstrained NOTES
# struct would be (260,120)+(-445,-125)=(-185,-5). DQ6 clamps its title band
# below the two menu bars, yielding struct (-185,40); the horizontal proposal
# already leaves 115 px of the 300 px title band on-screen (well above the
# Inside-Macintosh 4 px reachability margin). Grader leg G and the serial tooth
# in test-flair-solid independently lock that result (bead initech-r8r7).
FLAIR_SOLID_CLAMP_SPEC := m100:60,m40:0,l1,l0,m-10:-100,m0:-70,l1,m-100:-100,m-100:-25,m-100:0,m-100:0,m-45:0,l0

# Leg H -- RAISE ON TITLE CLICK. From the screen centre (320,240), move
# DIRECTLY to NOTES's visible title segment at (450,130): delta (+130,-110),
# split into int8-safe packets (+100,-100),(+30,-10). Press there with NO prior
# content-activation click, drag (-60,+60), and release. The title mouseDown on
# background NOTES must take the existing content-click foreground-switch path
# FIRST (FLAIR-DISPATCH app=NOTES), then drag NOTES from struct (260,120) to
# (260-60,120+60)=(200,180), in front and active. Grader leg H independently
# locks the new geometry, active full-width title, and NOTES-over-HELLO overlap
# result (bead initech-haaq; DQ5).
FLAIR_SOLID_RAISE_SPEC := m100:-100,m30:-10,l1,m-60:60,l0
