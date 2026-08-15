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
# x >= 360).  Grader: tools/ppm_flair_solid_check.c (legs A/B/C/G).
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
