# spec/flair_disk_windows_traces.mk -- the LOCKED input traces for the R3.3
# Finder DISK WINDOWS emulator gate (bead initech-tdnl.10, emu-wiring half).
# Locked spec-data (CLAUDE.md Rule 8, Rule 11): deterministic, versioned, never
# silently edited to make a test pass.
#
# SIBLING of spec/flair_desktop_icons_traces.mk, which owns the R3.2 DESKTOP
# traces and states the shared conventions in full. A separate file, not an
# edit, for the same reason spec/assets/finder_icons.h is a sibling of
# desk_icons.h: the R3.2 bytes are locked and adding is cheaper than touching
# (Rule 8). Everything below obeys that file's conventions verbatim:
#   * comma-separated QMP specs for qemu_harness --mouse; cursor starts at the
#     screen centre (320,240); "m<dx>:<dy>" is a RELATIVE move, y screen-DOWN-
#     POSITIVE; every component is int8-safe (|c| <= 100 here);
#   * "l1"/"l0" are left button down/up;
#   * PARK CONVENTION: every trace ENDS with the pointer parked at desktop
#     (620,460), outside every probe rect, because the pointer is real since
#     R0.1 and a stranded arrow reads as a grader failure.
#
# ---------------------------------------------------------------------------
# NEW THIS SLICE: "k<chord>" -- A KEYSTROKE IN THE MOUSE STREAM
# ---------------------------------------------------------------------------
# spec/flair_desktop_icons_traces.mk lines 60-71 recorded that the harness rail
# "has NO modifier tokens ... there is no --keys/--mouse interleave that could
# hold Shift across a button event". That note stands for shift-CLICK and is
# now PARTLY superseded for KEY CHORDS (bead initech-tdnl.10):
#
#   QMP `send-key` takes an ARRAY of keys pressed TOGETHER and then released.
#   So [ctrl, n] is ONE command that emits the real PS/2 SET-1 sequence
#   0x1D (Ctrl make) 0x31 (n make) + breaks, and os/flair/event.c's scancode
#   cooker sets FLAIR_EVT_MOD_CONTROL_KEY on the Ctrl make -- exactly the bit
#   os/milton/kmain.c :: flair_live_finder_key tests. No modifier state has to
#   survive BETWEEN two QMP commands, which is why this works where shift-click
#   still cannot: a shift-CLICK needs the modifier held ACROSS an
#   input-send-event mouse button, and send-key cannot express that.
#
# harness/emu/qemu.c therefore gained (a) chord tokens ("ctrl-n") in
# token_to_qcode/qmp_send_key_chord and (b) a "k<chord>" token in the MOUSE
# grammar, because the two injectors run in a FIXED order (all --keys, then all
# --mouse) and a chord that must arrive AFTER a click was otherwise
# unreachable. One ordered stream, one grammar. --keys is unchanged for every
# gate that uses it.
#
# ---------------------------------------------------------------------------
# CLEAN UP HAS NO EMU LEG, AND HERE IS WHY (Law 1 honesty, not an oversight)
# ---------------------------------------------------------------------------
# FCMD_CLEANUP's row in os/flair/finder_cmd.c carries cmd_char == 0 -- the
# period Finder gave Clean Up no Command-key equivalent, and F4.2's menu
# resource reproduces that. The only OTHER route to a Finder command is the
# menu bar, and the Finder's bar does not exist until bead initech-tdnl.12
# (os/milton/kmain.c :: flair_live_finder_key says so at length: band 2 belongs
# to the foreground TENANT, and promoting the Finder early would re-key every
# locked band-2 gate). There is therefore NO input this harness can inject that
# reaches finder_win_cleanup on the booted system, and a FLAIR_CLEANUP_SNAP_SPEC
# would be a trace that cannot fire.
#
# It is HOST-COVERED by harness/proptest/test_finder_windows.c (the row-major
# snap + the moved count) and mutation-proven there by
# FINDER_WIN_MUT_CLEANUP_UNSORTED. The emu leg lands with tdnl.12's menu bar,
# at which point the mouse path (MenuSelect -> finder_dispatch) reaches it.
# Recorded here rather than silently skipped.
#
# ---------------------------------------------------------------------------
# THE SCENE these traces are authored against
# ---------------------------------------------------------------------------
# $(FLAIRTENANTS_IMG) + a GATE-LOCAL copy of $(FLAIR_DATA_IMG), i.e. the same
# scene spec/flair_desktop_icons_traces.mk documents (HELLO (60,60)-(360,260),
# NOTES (260,120)-(560,340), two menu bars over y [0,40), the VOLUME desktop
# icon sprite (584,48)..(616,80) and the TRASH sprite (584,404)..(616,436)),
# PLUS -- once a double-click opens one -- a real Finder disk window.
#
# THE ROOT WINDOW'S GEOMETRY, DERIVED (os/flair/finder_windows.h Sec 3 + the
# window.c :: CalcDocContentRect seam + spec/chrome_metrics.h):
#   frame(slot 0) = (FINDER_WIN_DEFAULT_L, FINDER_WIN_DEFAULT_T)
#                   .. + (FINDER_WIN_DEFAULT_W, FINDER_WIN_DEFAULT_H)
#                 = (20,60)..(380,280)
#   content.top    = 60 + FLAIR_CHROME_TITLEBAR_H(22)                   =  82
#   content.left   = 20 + FLAIR_CHROME_FRAME(1)                         =  21
#   content.bottom = 280 - FLAIR_CHROME_FRAME(1)                        = 279
#   content.right  = 380 - FLAIR_CHROME_FRAME(1) - FLAIR_CHROME_BODY_BAR(4)
#                        - FLAIR_CHROME_SCROLLBAR_W(16)                 = 359
#   => content (21,82)..(359,279), 338 x 197
#
# THE INVISIBLE GRID (finder_windows.h Sec 2), evaluated over that content:
#   cols = (338 - FINDER_GRID_INSET_X(18)) / FINDER_GRID_PITCH_X(68)
#        = 320 / 68 = 4          (integer division; >= 1)
#   sprite_x(i) = 21 + 18 + (i % 4) * 68 = 39 + (i % 4) * 68
#   sprite_y(i) = 82 +  4 + (i / 4) * 52 = 86 + (i / 4) * 52
#
# THE ENUMERATION ORDER, from an INDEPENDENT reference (Law 2). The Finder
# walks the FAT12 root in DIRECTORY-SLOT order and SKIPS the volume label
# (finder_win_skip_entry, design F1.1); hidden entries are NOT skipped (the
# stated scope note in finder_windows.h). `mdir -a -i build/flair_data.img ::`
# on a booted gate copy prints the slots -- mtools, never our own FAT code:
#     slot 0  INITECH        (volume label)   -> SKIPPED by the Finder
#     slot 1  README.TXT     ($(FLAIR_DATA_IMG) recipe: mcopy)
#     slot 2  APPS   <DIR>   ($(FLAIR_DATA_IMG) recipe: mmd)
#     slot 3  DESKTOP.DB     (first boot: desktop_db_bootstrap, kmain order)
#     slot 4  TRASH  <DIR>   (first boot: desktop_trash_ensure, kmain order)
# so the four window icons, in row-major grid order, are
#     i=0 README.TXT  DOC    sprite (39,86)..(71,118)
#     i=1 APPS        FOLDER sprite (107,86)..(139,118)
#     i=2 DESKTOP.DB  DOC    sprite (175,86)..(207,118)
#     i=3 TRASH       FOLDER sprite (243,86)..(275,118)
# (kind by finder_win_kind_of: DIR_ATTR_DIRECTORY -> FOLDER, else DOC; no .EXE
# on this volume, so no APP strike appears and this gate does not probe one.)
# That is also where the LOCKED "FINDER-OPEN-VOLUME win=0 n=4" of
# test-flair-desktop-icons leg 5 comes from. Add a file to $(FLAIR_DATA_IMG)
# and BOTH gates go red until the counts are deliberately updated -- the Rule 8
# posture.
#
# Grader: tools/ppm_flair_disk_windows_check.c (legs rootwin / movedwin /
# newfolder). Serial markers: os/flair/finder_windows.h Sec "SERIAL MARKERS".
#
# ===========================================================================
# 1. FLAIR_FOLDER_NAV_SPEC -- open the volume, open a folder INSIDE it, and
#    prove the SPATIAL SINGLETON by re-opening that folder.
# ===========================================================================
# Serial chain this trace must produce, in order:
#     FINDER-OPEN-VOLUME win=0 n=4
#     FINDER-WIN-SELECT win=0 name=APPS count=1
#     FINDER-OPEN-FOLDER name=APPS win=1 singleton=0
#     FLAIR-DRAG win 0 (40,80)->(250,250)
#     FINDER-WIN-SELECT win=0 name=APPS count=1
#     FINDER-OPEN-FOLDER name=APPS win=1 singleton=1
#
# WHY THE DRAG IS IN THE MIDDLE (the load-bearing design choice, stated).
# Proving `singleton=1` needs the SAME folder opened TWICE. The second open has
# to come from the same icon in window 0 -- but window 1 opens at the cascaded
# frame (40,80)..(400,300) (finder_win_default_frame(1) = base + 1*(20,20)),
# and that rectangle COVERS window 0's whole icon row (the APPS cell is
# (107,86)..(139,133), well inside it). A second double-click at the same point
# would land in window 1's content and report FINDER-WIN-DESELECT-ALL win=1.
#
# Two cheaper routes were tried FIRST and are recorded as rejected, because
# both would have produced a green-looking trace that proves nothing:
#   (a) Re-double-click the DESKTOP volume icon. finder_open_volume's marker
#       (kmain.c) carries win= and n= but NOT singleton=, and a raise does not
#       re-enumerate, so the second line is byte-identical to the first. That
#       tests the singleton only by the ABSENCE of a win=1 line -- strictly
#       weaker than the field the contract defines.
#   (b) Click window 0's TITLE BAR to raise it, then re-double-click. MEASURED
#       on the real guest: the click is dispatched (FLAIR-DRAG win 0
#       (20,60)->(20,60)) but window 0 does NOT come forward -- both windows
#       belong to the same (Finder) tenant, so flair_app_dispatch's DQ5
#       click-to-activate has no tenant to switch and flair_live_do_drag never
#       calls SelectWindow. The following double-click still went to win=1.
#       That is a REAL gap in R3.3's raise story (a background window's title
#       click should raise it) and it belongs to the core lane / tdnl.12, not
#       to a trace that papers over it.
# So the trace MOVES window 1 out of the way with the drag verb that already
# works, and then opens APPS again from window 0 -- the honest gesture a user
# would make, and the one that exercises the singleton field itself.
#
# --- waypoint arithmetic -------------------------------------------------
# A. (320,240) -> the VOLUME sprite CENTRE (600,64) = (584+16, 48+16):
#      sum(dx) = +280 ; sum(dy) = -176
#      split: m100:-88, m100:-88, m80:0 -> (420,152),(520,64),(600,64)
#    l1,l0,l1,l0 = the double-click (the FLAIR_ICON_OPEN_SPEC pacing note:
#    qmp_inject_mouse drains 40 ms between tokens, so the second mouseDown
#    lands ~8-9 PIT ticks after the first -- inside FINDER_DBLCLICK_TICKS=20 --
#    and both clicks are at the IDENTICAL point, so 0 <= FINDER_DBLCLICK_SLOP).
#    -> the root window opens at (20,60)..(380,280) with 4 icons.
# B. (600,64) -> the APPS sprite CENTRE (123,102) = (107+16, 86+16):
#      sum(dx) = 123-600 = -477 ; sum(dy) = 102-64 = +38
#      split: m-100:10, m-100:10, m-100:10, m-100:8, m-77:0
#             -> (500,74),(400,84),(300,94),(200,102),(123,102)
#      (-100*4 - 77 = -477 ; 10+10+10+8+0 = 38 ; every component int8-safe)
#    l1,l0 selects APPS (FINDER-WIN-SELECT win=0 name=APPS count=1); l1,l0
#    classifies DOUBLE -> FINDER-OPEN-FOLDER name=APPS win=1 singleton=0.
# C. (123,102) -> window 1's TITLE BAR at (300,90):
#      sum(dx) = +177 ; sum(dy) = -12
#      split: m100:-12, m77:0 -> (223,90),(300,90)
#    (300,90) clearance: window 1's frame is (40,80)..(400,300) and its title
#    band is y [80, 80+22) = [80,102), so y=90 is in the band; x=300 is clear
#    of the go-away box (footprint (44,84)..(56,96)) and of the right-hand
#    widget slots (FLAIR_CHROME_ZOOM_RIGHT_OFF 32 / COLLAPSE_RIGHT_OFF 16 from
#    x=400). Window 1 is frontmost, so FindWindow returns inDrag on IT, not on
#    window 0 whose content also contains the point.
# D. drag window 1 by (+210,+170) and release:
#      l1 ; m100:85, m100:85, m10:0 -> (400,175),(500,260),(510,260) ; l0
#      (85+85+0 = 170 ; 100+100+10 = 210)
#      new frame = (40,80)+(210,170) = (250,250)..(610,470)
#    Clearance of the new rect: it is clear of window 0's icon row (left 250 >
#    the APPS cell's right 139), of the DESKTOP volume cell (577,48)-(624,95)
#    (top 250 > 95), and of the cursor PARK rect (620,460)-(636,476) (right
#    610 < 620). It is fully on the 640x480 raster (right 610, bottom 470), so
#    the DQ6 drag clamp does not bite and the committed origin is exactly
#    (250,250) -- which the FLAIR-DRAG line pins.
# E. (510,260) -> back to the APPS sprite centre (123,102):
#      sum(dx) = -387 ; sum(dy) = -158
#      split: m-100:-40, m-100:-40, m-100:-40, m-87:-38
#             -> (410,220),(310,180),(210,140),(123,102)
#    l1,l0,l1,l0 -> the second double-click. Window 1 no longer covers the
#    point, so FindWindow returns window 0; finder_win_find(APPS) HITS ->
#    FINDER-OPEN-FOLDER name=APPS win=1 singleton=1 (a raise, no re-enumeration).
# F. PARK: (123,102) -> (620,460): sum(dx) = +497 ; sum(dy) = +358
#      split: m100:72, m100:72, m100:72, m100:71, m97:71
#             -> (223,174),(323,246),(423,318),(523,389),(620,460)
#      (100*4 + 97 = 497 ; 72*3 + 71 + 71 = 358)
# This trace is graded on SERIAL only (no screendump), so its intermediate
# window rects need no probe clearance -- only the park does.
FLAIR_FOLDER_NAV_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m100:-12,m77:0,l1,m100:85,m100:85,m10:0,l0,m-100:-40,m-100:-40,m-100:-40,m-87:-38,l1,l0,l1,l0,m100:72,m100:72,m100:72,m100:71,m97:71

# ===========================================================================
# 2. FLAIR_WINDOW_DRAG_PERSIST_SPEC -- open the root window, DRAG it to a new
#    origin, CLOSE it (the Finder-owned go-away path), and let the kind=4 view
#    record reach \DESKTOP.DB.
# ===========================================================================
# Serial chain this trace must produce, in order:
#     FINDER-OPEN-VOLUME win=0 n=4
#     FLAIR-DRAG win 0 (20,60)->(120,180)
#     FINDER-CLOSE-WINDOW win=0
#     DESKTOP-DB-SAVE n=3
# n=3 is derived, not observed: finder_desk_persist encodes the DESKTOP icon
# records first and then the kind=4 view records, and prints
# (len - FINDER_DB_HEADER_SIZE) / FINDER_DB_RECORD_SIZE. The scene has 2
# desktop icons (FINDER-DESKTOP-ICONS n=2) and this trace closes exactly ONE
# disk window, so 2 + 1 = 3.
#
# The REBOOT half of the leg replays FLAIR_ICON_OPEN_SPEC (the locked R3.2
# trace, unchanged) against the SAME, now-written data image and must produce
#     DESKTOP-DB-OK   (never CREATE / REGEN / POS-SKIP)
#     DESKTOP-DB-VIEWS n=1
#     FINDER-OPEN-VOLUME win=0 n=4
# with the window's frame now at the SAVED origin (120,180) -- graded by the
# `movedwin` leg of ppm_flair_disk_windows_check.
#
# --- waypoint arithmetic -------------------------------------------------
# A. the volume double-click, byte-for-byte trace 1 step A.
# B. (600,64) -> window 0's TITLE BAR at (200,70):
#      sum(dx) = -400 ; sum(dy) = +6
#      split: m-100:2, m-100:2, m-100:2, m-100:0
#             -> (500,66),(400,68),(300,70),(200,70)
#    (200,70) is in the title band y [60,82); clear of the go-away footprint
#    (24,64)..(36,76) and of the centred title run (see below).
# C. drag window 0 by (+100,+120) and release:
#      l1 ; m100:100, m0:20 -> (300,170),(300,190) ; l0
#      new frame = (20,60)+(100,120) = (120,180)..(480,400)
#    THE DROP RECT AND ITS CLEARANCE (the load-bearing choice):
#      * fully on the raster: right 480 <= 640, bottom 400 <= 480, top 180 >=
#        the usable-desktop top 40 -- so the DQ6 clamp does not bite and the
#        committed origin is exactly (120,180)
#      * clear of the DESKTOP volume cell (577,48)-(624,95): right 480 < 577,
#        so the second boot's double-click at (600,64) is never covered
#      * clear of the TRASH cell (583,404)-(617,451): right 480 < 583
#      * clear of the cursor PARK rect (620,460)-(636,476)
#      * clear of this gate's own bare-teal control points (450,440) and
#        (600,300): 440 > the rect's bottom 400, and 600 > its right 480
# D. (300,190) -> the MOVED window's go-away box centre (130,190):
#      sum(dx) = -170 ; sum(dy) = 0 ; split: m-100:0, m-70:0 -> (200,190),(130,190)
#    The go-away footprint is (frame.left + FLAIR_CHROME_CLOSE_LEFT_OFF(4),
#    frame.top + FLAIR_CHROME_WIDGET_TOP_OFF(4)) + FLAIR_CHROME_WIDGET_BOX(12)
#    = (124,184)..(136,196), so (130,190) is its centre.
#    l1,l0 -> FindWindow returns inGoAway on a FINDER-owned window, so kmain
#    takes flair_live_do_finder_close (design F2-3: DisposeWindow only; the
#    always-resident Finder survives) and the view state is saved.
# E. PARK: (130,190) -> (620,460): sum(dx) = +490 ; sum(dy) = +270
#      split: m100:68, m100:68, m100:68, m100:66, m90:0
#             -> (230,258),(330,326),(430,394),(530,460),(620,460)
#      (100*4 + 90 = 490 ; 68*3 + 66 + 0 = 270)
FLAIR_WINDOW_DRAG_PERSIST_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:2,m-100:2,m-100:2,m-100:0,l1,m100:100,m0:20,l0,m-100:0,m-70:0,l1,l0,m100:68,m100:68,m100:68,m100:66,m90:0

# ===========================================================================
# 3. FLAIR_NEW_FOLDER_SPEC -- open the root window, then Ctrl-N: a REAL FAT12
#    directory is created and appears as a fifth icon.
# ===========================================================================
# Serial chain this trace must produce, in order:
#     FINDER-OPEN-VOLUME win=0 n=4
#     FINDER-CMD id=2 name=NEW_FOLDER src=key sel=0
#     FINDER-NEW-FOLDER name=NEWFOLD parent=0
# and the mtools differential on the gate-local data image must then show
#     NEWFOLD      <DIR>
# in the root -- a real directory, not a claim on serial (Law 2).
#
# NEWFOLD is the LOCKED first rung of the name ladder (finder_windows.h Sec 4:
# FINDER_NEWFOLDER_BASE, taken because no NEWFOL* name exists on this volume).
# parent=0 is the fixed root's "cluster" sentinel.
#
# THE FIFTH ICON, DERIVED: after the mkdir the window re-populates and the root
# holds 5 non-label entries, so i=4 -> col 4%4 = 0, row 4/4 = 1 ->
# sprite (39, 86+52) = (39,138)..(71,170), FOLDER strike, label band beneath.
# That is the `newfolder` grader leg.
#
# --- waypoint arithmetic -------------------------------------------------
# A. the volume double-click, byte-for-byte trace 1 step A.
# B. "kctrl-n" -- ONE QMP send-key carrying the qcode array [ctrl, n] (see the
#    banner). The chord reaches os/flair/event.c as Ctrl-make + n-make, so the
#    keyDown for 'n' carries FLAIR_EVT_MOD_CONTROL_KEY; kmain's
#    flair_live_finder_key sees a Finder window frontmost (finder_win_front_slot
#    >= 0, true because step A just opened one) and routes the chord through
#    the ONE command spine. finder_cmd_key_lookup('n') matches the F4.2 row
#    whose cmd_char is 'N' (the match is case-insensitive).
#    NO PARK IS NEEDED BEFORE THE CHORD: a keystroke does not move the pointer.
# C. PARK: (600,64) -> (620,460), the FLAIR_ICON_SELECT_SPEC split verbatim:
#      sum(dx) = +20 ; sum(dy) = +396
#      split: m20:99, m0:99, m0:99, m0:99
#             -> (620,163),(620,262),(620,361),(620,460)
FLAIR_NEW_FOLDER_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,kctrl-n,m20:99,m0:99,m0:99,m0:99
