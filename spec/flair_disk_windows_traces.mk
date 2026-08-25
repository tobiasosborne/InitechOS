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
#       [SUPERSEDED 2026-08-25, bug initech-tpzf / bead initech-tdnl.12: THE GAP
#       IS FIXED. os/flair/process.c :: flair_app_dispatch now runs
#       SelectWindow(w) whenever the clicked window is not wm->front -- title
#       AND content, same-tenant AND cross-tenant -- so route (b) really would
#       work now (host-graded by test_process.c leg (f), mutant
#       PROC_MUT_NO_SAMETENANT_RAISE). The trace below is NOT rewritten to use
#       it: it is a locked trace with locked expectations (Rule 8), the drag it
#       performs is load-bearing for the FLAIR-DRAG assertion, and re-keying a
#       green trace to exercise a different verb buys nothing. Recorded so the
#       "rejected because broken" note is not read later as "still broken".]
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
# and, from the Cmd-I that PRECEDES the Cmd-N (bead initech-tdnl.12), NO line
# mentioning GET_INFO anywhere in the capture -- see step B1.
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
# B1. "kctrl-i" -- THE NEGATIVE MenuKey LEG (bead initech-tdnl.12). Nothing is
#    selected at this point (the double-click in step A opened the volume; it
#    selected no icon INSIDE the new window), so File > Get Info's enable byte
#    is 0 and menu.h's MenuKey -- which returns the FIRST *ENABLED* non-divider
#    match -- hands out NOTHING. kmain's chord arm therefore returns "not
#    consumed" and no FINDER-CMD line is emitted at all.
#
#    THIS IS THE PROOF THAT THE KEYBOARD PATH REALLY MOVED TO MenuKey. tdnl.10
#    resolved a chord by scanning the COMMAND TABLE (finder_cmd_key_lookup),
#    which has no notion of an enable byte: it would have found the Get Info row
#    regardless and finder_dispatch would have printed
#        FINDER-CMD id=5 name=GET_INFO src=key sel=0
#        FINDER-CMD-DISABLED name=GET_INFO
#    So "no GET_INFO line at all" is a state ONLY the new path can produce, and
#    the assertion is a real differential rather than a tautology. The POSITIVE
#    half (Cmd-I WITH a selection, which must dispatch) rides
#    FLAIR_FINDER_MENU_SPEC step H.
#
#    NO PARK IS NEEDED BEFORE THE CHORD and no pixel moves, so the newfolder
#    screendump this trace also feeds is unaffected.
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
FLAIR_NEW_FOLDER_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,kctrl-i,kctrl-n,m20:99,m0:99,m0:99,m0:99

# ===========================================================================
# 4. FLAIR_FINDER_MENU_SPEC -- THE REAL FINDER MENU BAR, DRIVEN BY THE MOUSE
#    (bead initech-tdnl.12 / GUI remediation R3.5 stage 1)
# ===========================================================================
# THE CLEAN UP UNLOCK. The banner above ("CLEAN UP HAS NO EMU LEG, AND HERE IS
# WHY") recorded that FCMD_CLEANUP carries cmd_char == 0 and that the Finder's
# menu bar "does not exist until bead initech-tdnl.12", so no injectable input
# could reach finder_win_cleanup on the booted system. The bar exists now
# (os/flair/finder_menu.c) and the Finder really takes the foreground when it
# opens a disk window, so band 2 IS the Finder's bar and the MOUSE path reaches
# Clean Up. That paragraph is superseded for Clean Up specifically; everything
# else it says about cmd_char == 0 still holds.
#
# Serial chain this trace must produce, in order:
#     FINDER-OPEN-VOLUME win=0 n=4
#     FLAIR-DISPATCH app=FINDER
#     FINDER-WIN-DRAG win=0 name=README.TXT x=189 y=166
#     FLAIR-MENU-DROP menu=515
#     FLAIR-MENU menu=515 item=1 (sel=0x02030001)
#     FINDER-CMD id=9 name=CLEANUP src=mouse sel=0
#     FINDER-CLEANUP win=0 moved=1
#     FINDER-WIN-SELECT win=0 name=README.TXT count=1
#     FINDER-CMD id=5 name=GET_INFO src=key sel=1
#
# WHY THE ICON IS DRAGGED FIRST (the load-bearing choice, stated). A freshly
# populated window is ALREADY clean -- finder_win_populate snaps every icon
# row-major from the top-left (os/flair/finder_windows.c) -- so Clean Up on it
# would report moved=0, which is indistinguishable from a Clean Up that ran and
# did nothing. Dragging README.TXT off its cell first makes moved=1 the ONLY
# honest outcome, so the leg proves the command really executed rather than
# merely dispatched.
#
# THE TWO Cmd-I CHORDS ARE THE MenuKey PROOF (F4.2 + F4.4). tdnl.10's keyboard
# path scanned the COMMAND TABLE, which knows nothing about enable bytes: Cmd-I
# with nothing selected found the Get Info row every time. tdnl.12 replaced that
# with menu.h's MenuKey over the LIVE bar, which returns the FIRST *ENABLED*
# match -- so the chord is a no-op while Get Info is grayed. This trace fires
# Cmd-I only AFTER selecting an icon, and gets the dispatch; the NEGATIVE half
# (Cmd-I with an empty selection producing NO GET_INFO line at all) rides
# FLAIR_NEW_FOLDER_SPEC below, where the selection is empty by construction.
#
# --- waypoint arithmetic -------------------------------------------------
# A. the volume double-click, byte-for-byte trace 1 step A -> (600,64).
#    The window opens AND the Finder is promoted to the foreground tenant, so
#    band 2 swaps from HELLO's Photoshop bar to the Finder's own bar.
# B. (600,64) -> the README.TXT sprite CENTRE (55,102) = (39+16, 86+16):
#      sum(dx) = -545 ; sum(dy) = +38
#      split: m-100:8, m-100:8, m-100:8, m-100:7, m-100:7, m-45:0
#             -> (500,72),(400,80),(300,88),(200,95),(100,102),(55,102)
# C. drag README.TXT by (+150,+80) and release:
#      l1 ; m100:80, m50:0 -> (155,182),(205,182) ; l0
#    new origin = (39,86)+(150,80) = (189,166). NOT clamped: fd_clamp_origin
#    holds the origin inside (bounds.right - FINDER_ICON_DIM(32)) = 359-32 = 327
#    and (bounds.bottom - FINDER_CELL_H(47)) = 279-47 = 232, and 189 <= 327,
#    166 <= 232. A drag does NOT select (finder_desk_drag_commit is reached
#    through the `moved` branch of flair_live_do_surface, which never calls
#    finder_desk_select_only), which is why the CLEANUP line below reads sel=0.
# D. (205,182) -> the SPECIAL title in BAND 2 at (190,30):
#      sum(dx) = -15 ; sum(dy) = -152 ; split: m-15:-76, m0:-76
#             -> (190,106),(190,30)
#    Special's title slot is x[158,228) -- hand-derived in
#    harness/proptest/test_finder_menu.c leg B from os/flair/menu.h Sec 5:
#    first title at FLAIR_MENU_APPLE_W(20), slot = 8*len + 2*FLAIR_MENU_TITLE_PAD
#    (Chicago is a fixed 8px cell), so File[20,66) Edit[66,112) View[112,158)
#    Special[158,228) Help[228,274). Band 2 is rows [SHELL_MENUBAR2_TOP(20),40).
#    (190,30) is the slot's middle, well clear of both neighbours. y=30 is
#    ABOVE window 0's frame top (60), so FindWindow returns inDesk and the
#    band-2 arm takes the click.
# E. pull down and release on SPECIAL item 1 (Clean Up):
#      l1 ; m0:18 -> (190,48) ; l0
#    The panel drops with its top at the bar baseline: item rows begin at
#    FLAIR_MENUBAR_H(20) - FLAIR_MENU_PANEL_FRAME(1) + FLAIR_MENU_PANEL_INSET(2)
#    = 21 in band-local coordinates, i.e. screen y 41, and each row is
#    FLAIR_MENU_ITEM_H(16) tall -- so item 1 (Clean Up) owns screen y [41,57)
#    and y=48 is its middle. The panel is x[158,278): menu_panel_w takes the
#    widest item, "Empty Trash" (11 Chicago cells = 88) + FLAIR_MENU_ITEM_LPAD
#    (20) + FLAIR_MENU_ITEM_RPAD (12) = 120, and no Special item carries a
#    command key. x=190 is inside it.
#    -> MenuSelect returns MenuResult(515,1) = 0x02030001, kmain hands it to
#    finder_dispatch(..., "mouse") and the shell's exec hook runs Clean Up.
# F. (190,48) -> back to README.TXT's RESTORED cell centre (55,102):
#      sum(dx) = -135 ; sum(dy) = +54 ; split: m-100:40, m-35:14
#             -> (90,88),(55,102)
#    It is back at (39,86) because Clean Up just snapped it there -- so this
#    waypoint only exists if the command really ran.
# G. l1,l0 selects it: the window's click tracker has not classified a click
#    since finder_click_reset at open (the drag in C took the `moved` branch,
#    which never calls finder_click_classify), so this is an unambiguous SINGLE.
# H. "kctrl-i" -- ONE QMP send-key carrying [ctrl, i]. Get Info's enable byte
#    was just recomputed to 1 by finder_menu_refresh_enables (selection_count
#    == 1), so MenuKey hands out MenuResult(512,6) and the command dispatches.
# I. PARK: (55,102) -> (620,460): sum(dx) = +565 ; sum(dy) = +358
#      split: m100:72, m100:72, m100:72, m100:71, m100:71, m65:0
#             -> (155,174),(255,246),(355,318),(455,389),(555,460),(620,460)
#      (100*5 + 65 = 565 ; 72*3 + 71 + 71 + 0 = 358)
# Graded on SERIAL only.
FLAIR_FINDER_MENU_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:80,m50:0,l0,m-15:-76,m0:-76,l1,m0:18,l0,m-100:40,m-35:14,l1,l0,kctrl-i,m100:72,m100:72,m100:72,m100:71,m100:71,m65:0

# ===========================================================================
# 5. FLAIR_FINDER_MENU_CANCEL_SPEC -- the band-2 CANCEL restores the frame
#    EXACTLY (bead initech-tdnl.12; the solid-leg-D pattern)
# ===========================================================================
# Open the volume (so band 2 is the Finder's bar), pull the FILE menu down over
# the disk window and the desktop, drag off the panel entirely and release on
# bare desktop -- a CANCEL, MenuSelect returns 0, nothing dispatches -- then
# park. The resulting frame must be BYTE-IDENTICAL to the frame of a boot that
# only opened the volume and parked ($(FLAIR_DW_ROOT_NAME).ppm, leg 1). That is
# the whole-frame version of the "PRE == POST after a cancel" property
# test-flair-solid leg D applies to the single-bar pump: a pull-down is
# TEMPORARY INK and every pixel it covered has to come back, including the ones
# it covered ON A WINDOW.
#
# Serial chain this trace must produce, in order:
#     FINDER-OPEN-VOLUME win=0 n=4
#     FLAIR-DISPATCH app=FINDER
#     FLAIR-MENU-DROP menu=512
#     FLAIR-MENU menu=512 item=0 (sel=0x00000000)
# and NO FINDER-CMD line at all (sel == 0 is IM's "nothing chosen", which
# finder_dispatch returns from before tracing -- os/flair/finder_cmd.c).
#
# WHY THE FILE MENU. It is the TALLEST menu in the F4.2 resource -- 14 items,
# 12 normal rows (FLAIR_MENU_ITEM_H 16) + 2 dividers (FLAIR_MENU_DIV_H 6) =
# 204, plus FLAIR_MENU_PANEL_INSET 2 = 206 -- so its panel spans screen
# y[39,245) and x[20,176) (menu_panel_w: "Close Window" is 12 Chicago cells =
# 96, and it carries a command key, so 20 + 96 + FLAIR_MENU_CMD_GAP(8) +
# 2*8 + FLAIR_MENU_CMD_RPAD(16) = 156). That rectangle covers the disk window's
# title bar, its go-away box, part of its icon row AND bare desktop, so a
# restore that is right for the desktop but wrong over the window cannot pass.
#
# --- waypoint arithmetic -------------------------------------------------
# A. the volume double-click, byte-for-byte trace 1 step A -> (600,64).
# B. (600,64) -> the FILE title in band 2 at (40,30):
#      sum(dx) = -560 ; sum(dy) = -34
#      split: m-100:-7, m-100:-7, m-100:-7, m-100:-7, m-100:-6, m-60:0
#             -> (500,57),(400,50),(300,43),(200,36),(100,30),(40,30)
#    File's slot is x[20,66) (see trace 4 step D for the derivation); (40,30) is
#    inside it and inside band 2's rows [20,40).
# C. pull down, drag CLEAR of the panel, release on bare desktop at (400,400):
#      l1 ; m90:93, m90:93, m90:92, m90:92 -> (130,123),(220,216),(310,308),
#      (400,400) ; l0
#      (90*4 = 360 = 400-40 ; 93+93+92+92 = 370 = 400-30)
#    (400,400) clearance: outside the File panel x[20,176) y[39,245); outside
#    the disk window (20,60)..(380,280) (x 400 > 380 AND y 400 > 280); outside
#    the VOLUME cell (577,48)-(624,95) and the TRASH cell (583,404)-(617,451)
#    (x 400 < 577); outside the cursor PARK rect (620,460)-(636,476).
#    MenuInfo_item_at returns -1 for a point outside the panel, so MenuSelect
#    returns 0 -- the cancel.
#    The mouseUp is consumed INSIDE the menu track loop, and the pump's inDesk
#    arm tests the ORIGINAL mouseDown (y=30 < 40), so the desktop gesture
#    handler never runs and the selection state is untouched.
# D. PARK: (400,400) -> (620,460): sum(dx) = +220 ; sum(dy) = +60
#      split: m100:30, m100:30, m20:0 -> (500,430),(600,460),(620,460)
FLAIR_FINDER_MENU_CANCEL_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:-7,m-100:-7,m-100:-7,m-100:-7,m-100:-6,m-60:0,l1,m90:93,m90:93,m90:92,m90:92,l0,m100:30,m100:30,m20:0
