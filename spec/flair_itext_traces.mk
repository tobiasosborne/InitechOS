# spec/flair_itext_traces.mk -- the LOCKED input traces of test-flair-itext
# (bead initech-tdnl.16, R4.2 InitechText: the SimpleText-alike opens from the
# Finder, takes two lines, a drag selects a word, Copy, a click at the end,
# Return, Paste, Save -> the Standard File Save As box -> "MEMO.TXT" -> the
# file on the FAT volume). Locked spec-data (CLAUDE.md Rule 8, Rule 11); bound
# by every convention of spec/flair_app_launch_traces.mk (cursor starts
# (320,240); "m<dx>:<dy>" relative, y down, int8-safe; "l1"/"l0" left button;
# "k<chord>" one keystroke, "kshift-s" a capital S, "kctrl-c" the Copy chord
# MenuKey resolves over InitechText's own bar).
#
# THE SCENE: $(FLAIR_ITEXT_GATE_IMG) (the flagship kernel, pump life 1500
# ticks) + a gate-local copy of $(FLAIR_DATA_IMG), whose APPS folder holds, in
# directory order, TENANTFX.EXE (grid cell 0), CTENANT.EXE (1), 123.EXE (2),
# WORD.EXE (3), 123.MNU (4) and TEXT.EXE (5).
#
# GEOMETRY (derived, never measured off a render):
#   APPS window content.left 41, content.top 102 (flair_app_launch_traces.mk);
#   grid cell 5 = row 1, column 1 (4 columns): sprite_x = 41 + 18 + 68 = 127,
#     sprite_y = 102 + 4 + 52 (FINDER_GRID_PITCH_Y) = 158
#     => TEXT.EXE sprite (127,158)..(159,190), centre (143,174)
#   the TEXT window frame (4,44)..(588,476) (itext.c WIN_*), content corner
#     (10,66) (window.c CalcDocContentRect: left rail 6, title 22); the text
#     origin (+5,+4) = GLOBAL (15,70); SimpleText's Geneva 12 lines are 16 px
#     (bead initech-uelk; ../system7-decomp specs/sys8/simpletext-stdfile.md
#     Sec 1.2 [MEAS]): line L occupies y [70 + 16L, 86 + 16L).
#   "Saving tables to disk": the Geneva 12 advances ([MEAS] Sec 1.2/1.4, =
#     the real NFNT 12112 owTable) put "Saving " at 0..45 and "tables" at
#     45..84 => the drag text (45,8) -> (84,8) selects offsets 7..13 =
#     "tables" exactly (boundaries 45 and 84 are hit on the boundary itself)
#     -- GLOBAL (60,78) -> (99,78).
#   "PC LOAD LETTER" is 101 px ([MEAS]): a click at (130,94) is past its end
#     -> the caret at the end of line 1 (the end of the text).
#
# PACING: "w<ms>" holds (harness/emu/qemu.c) after the launch, after the
#   typing, around the drag / Copy / paste, and after Ctrl-S and the Save
#   (the Standard File box reads the FAT directory over ATA PIO): a step the
#   guest is still busy with must not have its next input land in a full ring
#   on a loaded host (a dropped Backspace or move is a harness artifact, not
#   the app's -- the waits change no event the app sees). Typing pauses
#   w120 after every space and Return: a burst of shifted chords at the
#   harness's ~40 ms pace outran a loaded host's guest (a dropped Shift break
#   left the shift stuck: "MEMO>TXT"), measured 2026-10-16.
#
# WAYPOINTS: A, B as flair_app_launch_traces.mk (the volume, then APPS, at
#   (123,102)), then CTEXT (123,102) -> TEXT.EXE (143,174): m20:72.
#
# 1. FLAIR_ITEXT_SPEC -- test-flair-itext and the itext_savefile clip: launch
#    (A, B, CTEXT + double-click); type "Saving tables to disk" Return
#    "PC LOAD LETTER"; to (53,75), press, m31:0, release (drag-select
#    "tables"); Ctrl-C (Copy); to (110,86) click (caret at the end); Return;
#    Ctrl-V (Paste); Ctrl-S (Save: untitled -> the Save As box, name
#    "UNTITLED" selected... typed over: 8 Backspaces, "MEMO.TXT"), Return
#    (Save); then a last drag selects "tables" on line 0 again -- the scene
#    of the independent golden s8_simpletext_doc.png ("tables" selected) --
#    to (60,78), press, m39:0 -> (99,78), release; park on the window's right
#    margin (580,300) so no Finder icon is hovered.
#    (initech-uelk re-key from Geneva 9: the drag m-90:-99,m31:0 became
#    m-83:-96,m39:0; the end click m26:11 became m31:16; the last drag
#    m-95:0,l1,m44:0 ("PC LOAD", line 1) became m-70:-16,l1,m39:0 ("tables",
#    line 0) and the park legs follow from (99,78).)
# initech-zc68: TEXT cell 6 = (211,174): (+88,+72) from APPS (123,102).
# Cell n centre = (75+68*(n%4),122+52*(n/4)). After launch move (-68,0)
# back to (143,174), so every document gesture and park keeps its target.
FLAIR_ITEXT_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:72,l1,l0,l1,l0,m-68:0,w400,kshift-s,ka,kv,ki,kn,kg,kspc,w120,kt,ka,kb,kl,ke,ks,kspc,w120,kt,ko,kspc,w120,kd,ki,ks,kk,kret,w120,kshift-p,kshift-c,kspc,w120,kshift-l,kshift-o,kshift-a,kshift-d,kspc,w120,kshift-l,kshift-e,kshift-t,kshift-t,kshift-e,kshift-r,w300,m-83:-96,l1,m39:0,l0,w150,kctrl-c,w150,m31:16,l1,l0,w150,kret,w120,kctrl-v,w200,kctrl-s,w600,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,km,ke,km,ko,kdot,kt,kx,kt,kret,w120,w600,m-70:-16,l1,m39:0,l0,m100:100,m100:100,m100:22,m100:0,m81:0

# 2. FLAIR_ITEXT_EDIT_SPEC -- the itext_edit clip (Rule 14): launch, type,
#    drag-select, Copy, click, Return, Paste (no save).
FLAIR_ITEXT_EDIT_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:72,l1,l0,l1,l0,m-68:0,w400,kshift-s,ka,kv,ki,kn,kg,kspc,w120,kt,ka,kb,kl,ke,ks,kspc,w120,kt,ko,kspc,w120,kd,ki,ks,kk,kret,w120,kshift-p,kshift-c,kspc,w120,kshift-l,kshift-o,kshift-a,kshift-d,kspc,w120,kshift-l,kshift-e,kshift-t,kshift-t,kshift-e,kshift-r,w300,m-83:-96,l1,m39:0,l0,w150,kctrl-c,w150,m31:16,l1,l0,w150,kret,w120,kctrl-v

# 3. ehq4 -- forty Returns enable the standard vertical scroll bar and keep
# the insertion point visible. Launch prefix copied EXACTLY from
# FLAIR_ITEXT_SPEC through w400, including cell12 and the cursor return.
# Each Return is paced as in the original trace. Expected 41 lines x 16
# (Geneva 12, initech-uelk) + 2*4 inset = doc664; the locked view389 gives
# range/value275. Caret global x14,y435..450; thumb at the far end of the
# bar (568,425)..(582,440).
# The same trace drives test-flair-itext-scroll and record-flair itext_scroll.
FLAIR_ITEXT_SCROLL_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:72,l1,l0,l1,l0,m-68:0,w400,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,kret,w120,w300

# 4. L017 (audit 2026-10-23-das; bead initech-oobx) -- an invalid 8.3 name in
# the Save As box is refused WITH an alert, and the box stays usable. Launch
# prefix copied EXACTLY from FLAIR_ITEXT_SPEC through w400; type "x"; Ctrl-S
# (the Save As box, name UNTITLED); 8 Backspaces; type "toolongname" (11
# letters, no dot: the field upper-cases it; 11 > the 8-character base);
# Return -> the bad-name alert (marker TENANT-SF badname TOOLONGNAME); Return
# (OK); 11 Backspaces; "memo.txt" Return -> the reply MEMO.TXT (good=1).
FLAIR_ITEXT_BADNAME_UP_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:72,l1,l0,l1,l0,m-68:0,w400,kx,w200,kctrl-s,w600,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,kt,ko,ko,kl,ko,kn,kg,kn,ka,km,ke,w200,kret,w1500,m100:0,m100:0,m100:0,m100:0,m0:100,w300
FLAIR_ITEXT_BADNAME_SPEC := $(FLAIR_ITEXT_BADNAME_UP_SPEC),kret,w600,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,km,ke,km,ko,kdot,kt,kx,kt,kret,w800,w600

# 5. L014 (audit 2026-10-23-das; bead initech-oobx) -- typing at the 4096-byte
# limit raises the "This document is full." notice. Launch prefix copied
# EXACTLY from FLAIR_ITEXT_SPEC through w400; Ctrl-O (Standard File Open),
# Down x NDOWN to K4.TXT (a gate-local 4096-byte file in the volume root),
# Return (Open); type "x" -> the notice (marker TENANT-ALERT up lines=2
# buttons=1). The trace ENDS with the notice up: the harness grabs the
# screendump after the last input, so the pixel leg sees the alert.
# (Likewise FLAIR_ITEXT_BADNAME_UP_SPEC ends at the bad-name alert; the full
# BADNAME_SPEC then dismisses it and enters a valid name.)
FLAIR_ITEXT_FULL_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:72,l1,l0,l1,l0,m-68:0,w400,kctrl-o,w1000,kdown,kdown,kdown,kdown,kdown,kret,w1000,kx,w1500,m100:0,m100:0,m100:0,m100:0,m0:100,w300
