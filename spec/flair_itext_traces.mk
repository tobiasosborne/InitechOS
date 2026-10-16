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
#     origin (+5,+4) = GLOBAL (15,70); Geneva 9 lines are 11 px: line L
#     occupies y [70 + 11L, 81 + 11L).
#   "Saving tables to disk": the Geneva 9 advances (spec/assets/geneva9.h)
#     put "Saving " at 0..38 and "tables" at 38..69 => the drag (48,75) ->
#     (79,75) selects offsets 7..13 = "tables" exactly (boundaries 38 and 69
#     are hit on the boundary itself) -- GLOBAL (53,75) -> (84,75).
#   "PC LOAD LETTER" is 85 px: a click at (110,86) is past its end -> the
#     caret at the end of line 1 (the end of the text).
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
#    (Save); then a last drag selects "PC LOAD" on line 1 for the screendump:
#    to (15,86) [line 1 x 0], press, m44:0 -> (59,86) ("PC LOAD" ends at
#    x 44: P 6, C 7, space 4, L 6, O 7, A 7, D 7), release; park on the
#    window's right margin (580,300) so no Finder icon is hovered.
FLAIR_ITEXT_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m20:72,l1,l0,l1,l0,w400,kshift-s,ka,kv,ki,kn,kg,kspc,w120,kt,ka,kb,kl,ke,ks,kspc,w120,kt,ko,kspc,w120,kd,ki,ks,kk,kret,w120,kshift-p,kshift-c,kspc,w120,kshift-l,kshift-o,kshift-a,kshift-d,kspc,w120,kshift-l,kshift-e,kshift-t,kshift-t,kshift-e,kshift-r,w300,m-90:-99,l1,m31:0,l0,w150,kctrl-c,w150,m26:11,l1,l0,w150,kret,w120,kctrl-v,w200,kctrl-s,w600,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,km,ke,km,ko,kdot,kt,kx,kt,kret,w120,w600,m-95:0,l1,m44:0,l0,m100:100,m100:100,m100:14,m100:0,m100:0,m21:0

# 2. FLAIR_ITEXT_EDIT_SPEC -- the itext_edit clip (Rule 14): launch, type,
#    drag-select, Copy, click, Return, Paste (no save).
FLAIR_ITEXT_EDIT_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m20:72,l1,l0,l1,l0,w400,kshift-s,ka,kv,ki,kn,kg,kspc,w120,kt,ka,kb,kl,ke,ks,kspc,w120,kt,ko,kspc,w120,kd,ki,ks,kk,kret,w120,kshift-p,kshift-c,kspc,w120,kshift-l,kshift-o,kshift-a,kshift-d,kspc,w120,kshift-l,kshift-e,kshift-t,kshift-t,kshift-e,kshift-r,w300,m-90:-99,l1,m31:0,l0,w150,kctrl-c,w150,m26:11,l1,l0,w150,kret,w120,kctrl-v
