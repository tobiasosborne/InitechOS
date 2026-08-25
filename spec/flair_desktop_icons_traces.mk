# spec/flair_desktop_icons_traces.mk -- the LOCKED input traces for the R3.2
# Finder DESKTOP MANAGER emulator gate (bead initech-tdnl.9, emu-wiring half).
# Locked spec-data (CLAUDE.md Rule 8, Rule 11): deterministic, versioned, never
# silently edited to make a test pass.
#
# Same shape as spec/flair_appswitch_trace.mk / spec/flair_solid_traces.mk:
# comma-separated QMP relative-mouse specs for qemu_harness --mouse. The cursor
# starts at the screen centre (320,240); "m<dx>:<dy>" is a relative move with y
# screen-DOWN-POSITIVE (the post-rgt8 producer convention -- verified against
# FLAIR_SOLID_CLOSE_SPEC, whose m-100:-100,m-100:-70,m-46:0 reaches HELLO's
# go-away box at (74,70) = (320-246, 240-170)); every component is int8-safe
# (|c| <= 100 here). "l1"/"l0" are left button down/up.
#
# PARK CONVENTION (spec/flair_solid_traces.mk lines 10-13): every trace whose
# graded dump samples pixels ENDS with the pointer parked at desktop (620,460),
# outside every probe rect. The pointer is REAL since R0.1 (bead initech-tdnl.1)
# and a stranded arrow reads as a grader failure. The park motion is injected
# AFTER the marker that gates the screendump, exactly like FLAIR_SOLID_RAISE_
# SPEC: harness/emu/qemu.c drains 100 ms at the end of injection and a further
# 150 ms after the marker is seen before the QMP grab, so the parked cursor is
# always the one in the dump (the leg-H precedent, green since 2026-08-23).
#
# ---------------------------------------------------------------------------
# THE SCENE these traces are authored against
# ---------------------------------------------------------------------------
# $(FLAIRTENANTS_IMG), the flagship co-resident scene:
#   HELLO struct (60,60)..(360,260)   -- boot foreground
#   NOTES struct (260,120)..(560,340) -- background; right edge 560, so any
#                                        x >= 566 is window-free
#   two menu bars occupy y [0,40); the usable desktop is (0,40)..(640,480)
# The R3.2 Finder shell tenant seeds two desktop icons (os/flair/finder_desktop.h
# Sec 1; os/milton/kmain.c placement note):
#   VOLUME sprite (584,48)..(616,80)    label (577,82)..(624,95)   "INITECH"
#   TRASH  sprite (584,404)..(616,436)  label (583,438)..(617,451) "Trash"
# (The label rects are the finder_desk_label_rect rule -- sum of geneva9
# advances + 2*FINDER_LABEL_PAD_X, centered on the sprite, clamped into bounds --
# evaluated over the LOCKED spec/assets/geneva9.h strike: "INITECH" measures 41
# px -> band 47 wide -> (577..624); "Trash" measures 28 px -> band 34 wide ->
# (583..617). NOTE: an earlier lane REPORT (not the shipped header -- checked)
# quoted the Trash label with a stale right edge of 627; the width is 34, not
# 44. The traces below depend only on the SPRITE columns [584,616), which lie
# inside every one of these bands, so the nit was inert either way.)
# Derived cell (hit + damage) rects, sprite UNION label bbox:
#   VOLUME cell (577,48)..(624,95)     TRASH cell (583,404)..(617,451)
#
# NO-GO SET honoured by every waypoint below (probe points and window rects of
# the OTHER locked traces/graders that share this image, plus this gate's own):
#   HELLO (60,60)-(360,260); NOTES (260,120)-(560,340) and its post-drag
#   (200,180)-(500,400); solid/appswitch probes (300,185), (80,94),
#   (261,142)-(273,154), (559,200), (559,300), (400,339), (500,339),
#   (400,279)-(400,280); window-ops waypoints (10,100), (100,60..130), (155,90),
#   (156,90), (200,100), (359,100), (500,60); the cursor PARK rect
#   (620,460)-(636,476); and this gate's four bare-teal control points
#   (570,60), (630,60), (570,420), (632,420).
#
# Grader: tools/ppm_flair_desktop_icons_check.c (legs default / selected /
# moved). Serial markers: os/flair/finder_desktop.h lines 54-66.
#
# ---------------------------------------------------------------------------
# SHIFT-CLICK: DELIBERATELY ABSENT FROM THE EMU TRACES (Law 1 honesty)
# ---------------------------------------------------------------------------
# The harness injection rail has NO modifier tokens: qmp_inject_mouse
# (harness/emu/qemu.c) understands exactly "m<dx>:<dy>", "l0/l1", "r0/r1",
# "M0/M1" and nothing else, and there is no --keys/--mouse interleave that could
# hold Shift across a button event. So finder_desk_select_extend (the shift-
# EXTENDS arm of flair_live_do_desk) CANNOT be exercised from an emulator trace
# on today's harness. It is HOST-COVERED by harness/proptest/test_finder_
# desktop.c (test-finder-desktop), whose hand-authored event streams drive the
# extend path directly, and mutation-proven there. Adding a modifier token to the
# QMP rail is a separate, larger change than this bead; recorded here rather than
# silently skipped.
#
# ===========================================================================
# 1. FLAIR_ICON_SELECT_SPEC -- click the volume icon; graded SELECTED.
# ===========================================================================
# From (320,240) to the volume sprite CENTRE (600,64) = (584+16, 48+16):
#   needed sum(dx) = 600-320 = +280 ; sum(dy) = 64-240 = -176
#   split (int8-safe, |c| <= 100): m100:-88, m100:-88, m80:0
#     (420,152) -> (520,64) -> (600,64)
# l1,l0 = the selecting click. finder_desk_hit takes sprite UNION label, so
# (600,64) is a sprite hit -> finder_desk_select_only -> the label band INVERTS
# (design F1.1) and kmain emits "FINDER-ICON-SELECT name=INITECH count=1".
# Then PARK: (600,64) -> (620,460): sum(dx)=+20, sum(dy)=+396
#   split: m20:99, m0:99, m0:99, m0:99  -> (620,163),(620,262),(620,361),(620,460)
FLAIR_ICON_SELECT_SPEC := m100:-88,m100:-88,m80:0,l1,l0,m20:99,m0:99,m0:99,m0:99

# ===========================================================================
# 2. FLAIR_ICON_DESELECT_SPEC -- select, then click bare desktop; graded DEFAULT.
# ===========================================================================
# The SELECT half above verbatim, then a click on bare desktop at (450,440).
#   from (600,64): sum(dx) = 450-600 = -150 ; sum(dy) = 440-64 = +376
#   split: m-100:94, m-50:94, m0:94, m0:94 -> (500,158),(450,252),(450,346),(450,440)
# (450,440) clearance: x=450 is left of BOTH icon cells (577 / 583) and right of
# HELLO (right 360); y=440 is below NOTES (bottom 340) and below its post-drag
# rect (bottom 400); it is outside the park rect (left 620) and outside all four
# bare-teal control points. FindWindow therefore returns inDesk with v >= 40 and
# finder_desk_hit returns -1 -> "FINDER-ICON-DESELECT-ALL", and the label band
# must repaint back to its un-inverted state (the DEFAULT leg, again).
# Then PARK: (450,440) -> (620,460): sum(dx)=+170, sum(dy)=+20
#   split: m100:20, m70:0 -> (550,460),(620,460)
FLAIR_ICON_DESELECT_SPEC := m100:-88,m100:-88,m80:0,l1,l0,m-100:94,m-50:94,m0:94,m0:94,l1,l0,m100:20,m70:0

# ===========================================================================
# 3. FLAIR_RUBBER_BAND_SPEC -- marquee both icons; expects FINDER-MARQUEE n=2.
# ===========================================================================
# Press on bare desktop ABOVE-LEFT of the volume cell, then sweep down-right past
# the Trash cell. DEVIATION FROM THE BRIEF, STATED: the brief suggested pressing
# at (570,100). finder_band_rect normalises the anchor and the release point, so
# a band anchored at y=100 has top=100 and CANNOT reach the volume icon at all
# (its cell bottom is 95) -- the marquee would report n=1 and the leg would be a
# lie. The anchor is therefore raised to (570,44): 4 px below the second menu
# bar (usable desktop top = 40), 7 px left of the volume cell (left 577), and 10
# px right of NOTES's right edge + shadow (560/562).
#   from (320,240): sum(dx) = 570-320 = +250 ; sum(dy) = 44-240 = -196
#   split: m100:-98, m100:-98, m50:0 -> (420,142),(520,44),(570,44)
# l1 = press on bare desktop (finder_desk_hit -> -1, the marquee arm).
# Sweep to (632,455): sum(dx) = +62 ; sum(dy) = +411
#   split: m62:83, m0:82, m0:82, m0:82, m0:82
#          -> (632,127),(632,209),(632,291),(632,373),(632,455)
#   (83 + 4*82 = 411; every component int8-safe.)
# l0 = release. finder_band_rect((570,44),(632,455)) = (570,44)..(633,456)
# (half-open, +1 on right/bottom so the release pixel is INSIDE -- the
# FINDER_DESK_MUT_MARQUEE_OFFBYONE tooth). That band meets BOTH sprite rects
# (584..616 horizontally; 48..80 and 404..436 vertically) -> n=2.
# Then PARK: (632,455) -> (620,460): m-12:5
FLAIR_RUBBER_BAND_SPEC := m100:-98,m100:-98,m50:0,l1,m62:83,m0:82,m0:82,m0:82,m0:82,l0,m-12:5

# ===========================================================================
# 4. FLAIR_ICON_DRAGDROP_SPEC -- drag the volume icon to a clear cell; graded MOVED.
# ===========================================================================
# Press the volume sprite centre (600,64) exactly as trace 1, then drag to
# (416,416) and release.
#   press route: m100:-88, m100:-88, m80:0 -> (600,64) ; l1
#   drag: sum(dh) = 416-600 = -184 ; sum(dv) = 416-64 = +352
#   split: m-92:88, m-92:88, m0:88, m0:88
#          -> (508,152),(416,240),(416,328),(416,416)
#   (first hop is 92,88 -- far beyond FINDER_DRAG_SLOP=3, so the gesture is a
#   DRAG, not a click; the save-under outline arms on that hop.)
# l0 = drop. finder_desk_drag_commit translates the sprite origin by the clamped
# delta: (584,48) + (-184,+352) = (400,400). Serial: "FINDER-ICON-DRAG
# name=INITECH x=400 y=400" then "DESKTOP-DB-SAVE n=2".
#
# THE DROP CELL AND ITS CLEARANCE (the load-bearing choice).
#   new sprite (400,400)..(432,432)
#   new label  (393,434)..(440,447)   [ "INITECH" band 47 wide, centred on 416 ]
#   new cell   (393,400)..(440,447)
# Clearance argument, every member of the no-go set:
#   * HELLO (60,60)-(360,260)                 : cell top 400 > 260, and left 393 > 360
#   * NOTES (260,120)-(560,340)               : cell top 400 > 340 (+shadow ~342)
#   * NOTES post-drag (200,180)-(500,400)     : cell top 400 == that rect's
#                                               EXCLUSIVE bottom -> disjoint
#                                               (and NOTES is never dragged on
#                                               this gate's boots anyway)
#   * TRASH cell (583,404)-(617,451)          : cell right 440 < 583
#   * VOLUME's own old cell (577,48)-(624,95) : disjoint in both axes
#   * cursor PARK rect (620,460)-(636,476)    : cell right 440 < 620, bottom 447 < 460
#   * solid/appswitch probes (300,185),(80,94),(261,142)-(273,154),(559,200),
#     (559,300),(400,339),(500,339),(400,279)-(400,280): every one has y <= 339
#     or x >= 500 with y <= 300 -> all above the cell's top row 400 or right of
#     its right edge 440
#   * window-ops waypoints (10,100),(100,60..130),(155,90),(156,90),(200,100),
#     (359,100),(500,60)                      : all y <= 130, cell top is 400
#   * this gate's bare-teal controls (570,60),(630,60),(570,420),(632,420):
#     all x >= 570 > 440
#   * usable desktop (0,40)-(640,480)         : cell fits with 33 px of bottom
#                                               margin, so finder_desk_drag_commit
#                                               does NOT clamp -- x=400,y=400 is
#                                               the exact committed origin
# Then PARK: (416,416) -> (620,460): sum(dx)=+204, sum(dy)=+44
#   split: m100:44, m100:0, m4:0 -> (516,460),(616,460),(620,460)
FLAIR_ICON_DRAGDROP_SPEC := m100:-88,m100:-88,m80:0,l1,m-92:88,m-92:88,m0:88,m0:88,l0,m100:44,m100:0,m4:0

# ===========================================================================
# 5. FLAIR_ICON_OPEN_SPEC -- double-click the volume; expects
#    "FINDER-OPEN-VOLUME win=0 n=4" (RE-KEYED at bead initech-tdnl.10).
# ===========================================================================
# Same approach route to (600,64), then FOUR button tokens back-to-back:
# l1,l0,l1,l0. qmp_inject_mouse drains only 40 ms between tokens, so the second
# mouseDown reaches the guest roughly 80-90 ms (8-9 PIT ticks at 100 Hz) after
# the first -- inside FINDER_DBLCLICK_TICKS (20) with margin, and both clicks are
# at the IDENTICAL point so |dh|,|dv| = 0 <= FINDER_DBLCLICK_SLOP (4).
# The MEASURED delta is recorded in the bead report; if a future harness change
# slows the pacing past 20 ticks the fix is the harness or the trace, NEVER
# raising FINDER_DBLCLICK_TICKS (that constant golden-resolves against a period
# Mouse control panel, finder_desktop.h Sec 7).
# Click 1 selects (FINDER-ICON-SELECT name=INITECH count=1); click 2 classifies
# DOUBLE and -- since bead initech-tdnl.10 (R3.3 disk windows) -- really OPENS
# the root disk window over the mounted volume, emitting
# "FINDER-OPEN-VOLUME win=0 n=4". The R3.2 line this replaces was
# "FINDER-OPEN-VOLUME NYI"; the re-key is strictly stronger (it additionally
# pins the window slot and the enumerated icon count). The count is the flagship
# volume's root AFTER first boot: README.TXT, APPS, DESKTOP.DB and TRASH -- the
# volume LABEL is skipped by the Finder (design F1.1) while hidden entries are
# not (the stated scope note in os/flair/finder_windows.h).
#
# THE TRACE ITSELF IS UNCHANGED. The waypoints, the button tokens and the park
# are byte-for-byte what R3.2 locked; only what the guest DOES at the end of
# them grew. Note also that this trace now leaves a WINDOW OPEN when the boot
# ends -- it never closes it, so no kind=4 view record is written and
# \DESKTOP.DB stays the 56 bytes leg 7 asserts.
# Then PARK: (600,64) -> (620,460), the same split as trace 1.
FLAIR_ICON_OPEN_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m20:99,m0:99,m0:99,m0:99
