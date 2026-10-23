# LOCKED traces, initech-tdnl.20. Ref: flair_app_launch_traces.mk and
# flair_iword_traces.mk. Calc is APPENDED in APPS directory cell 5 (123.MNU
# occupies cell 4); four columns, click (143,174). A/B unchanged. With the
# Platinum content.left=46, cell5 sprite is (132,158)-(164,190): this point
# is inside it (68x52 pitch,18/4 inset; finder_windows.c).
# Window placement is authored product policy: ledger (10,65)-(390,360),
# chart (400,100)-(635,360). No missing-film geometry is claimed.
# Chrome is Window Manager Platinum, independently sampled in
# ../system7-decomp/goldens/captures/s8_doc_window_active.png.
# Content origins: ledger (16,87), chart (406,122), CalcDocContentRect.
# Authored content-layout policy (all coordinates CONTENT-LOCAL):
#   ledger field353x252; column headings A(8,2), B(204,2); three known rows
#   at y24+24*r, label x8 and amount x204. Gray rules span x4..344 at
#   y18+24*r (r0..3), one-pixel column divider x196/y18..91. Selection is a
#   one-pixel black line x4 or200, y20+24*r, width140. These are grid UI,
#   not claims about the missing film's cell count or measured row pitch.
#   Unlocated negative specimen: heading (8,104), cell (204,128).
#   Remaining-data/pending-entry line (8,164), PRD footer total (204,194).
#   chart field208x217; mathematical disk centre(70,75), radius48; three
#   existing Toolbox role shades BLACK/GRAY/NAVY/BLACK/GRAY in slice order.
#   Percentages (120,20+22*i), total (120,134); adjacent shades differ at
#   all five boundaries. PRD-stated weights remain40/35/18/14/9, total116.
#   Typography is PRD6.4/6.5 Geneva9 using the existing Font Manager strike.
# initech-zc68: APPS cell 5 = (143,174), delta (+20,+72) from (123,102).
# Cell n centre = (75+68*(n%4),122+52*(n/4)); park stays (143,460),
# delta (0,+286). Ref: Makefile flair-data-rules, spec/dos_folder_apps.txt.
FLAIR_CALC_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m20:72,l1,l0,l1,l0,m0:100,m0:100,m0:86
# Same launch; an edit to A1, then a menu-key Quit asks (initech-qxft) and Don't Save tears down BOTH windows.
FLAIR_CALC_EDIT_SPEC := $(FLAIR_CALC_SPEC),k7,k3,kret,kctrl-q,m100:-100,m10:-100,m0:-92,l1,l0
# initech-bf2: the same launch, then Cmd/Ctrl-S -- the shell's MenuKey
# resolves it to File > Save (enabled, cmdChar 'S'; spec/toolbox_gate.h
# Sec 6a) and Calc asks the FILE COPY box (Sec 12). The cursor is parked at
# (143,460), clear of the box (140,200)-(500,280).
FLAIR_FILE_COPY_SPEC := $(FLAIR_CALC_SPEC),kctrl-s
# initech-zc68: PAINT cell 7 = (279,174), delta (+156,+72) from APPS.
# Park stays (279,460), delta (0,+286). File title (20,30) is (-259,-144)
# from this launch; later menu moves keep their absolute targets.
FLAIR_PAINT_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m100:72,m56:0,l1,l0,l1,l0,m0:100,m0:100,m0:86
FLAIR_PAINT_MENU_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m100:72,m56:0,l1,l0,l1,l0,m-100:-72,m-100:-72,m-59:0,l1,m42:0,m0:35,l0,m-42:-35,l1,m0:83,l0

# initech-qxft (audit 2026-10-23-das L006/L007/L009): InitechCalc's exit guard,
# its Save content and its replace question. Locked input traces of
# test-flair-calc-quit (harness/frameapps.mk). Same launch as FLAIR_CALC_SPEC
# (cursor parked at (143,460)); A1 := 73 (k7,k3,kret commit). The alert is the
# Toolbox alert (spec/toolbox_gate.h Sec 11): Save (default) / Cancel / Don't
# Save, buttons y 158..178, Don't Save centre (253,168) = (+110,-292) from the
# park -> m100:-100,m10:-100,m0:-92. Return = default, Escape = Cancel. The
# ledger window go-away box centre (20,75) = (-123,-385) from the park.
FLAIR_CALC_EDIT_A1 := $(FLAIR_CALC_SPEC),k7,k3,kret
FLAIR_CALC_DONTSAVE_CLICK := m100:-100,m10:-100,m0:-92,l1,l0
FLAIR_CALC_CLOSE_CLICK := m-100:-100,m-23:-100,m0:-100,m0:-85,l1,l0,l1,l0
# 1 ALERT      edit, Ctrl-Q: the alert is up (never exits).
FLAIR_CALC_QG_ALERT := $(FLAIR_CALC_EDIT_A1),kctrl-q
# 2 CANCEL     edit, Ctrl-Q, Escape, Ctrl-Q, Escape: never exits, asked twice.
FLAIR_CALC_QG_CANCEL := $(FLAIR_CALC_EDIT_A1),kctrl-q,kesc,kctrl-q,kesc
# 3 DONTSAVE   edit, Ctrl-Q, Don't Save: exits, TABLES.TXT never written.
FLAIR_CALC_QG_DONTSAVE := $(FLAIR_CALC_EDIT_A1),kctrl-q,$(FLAIR_CALC_DONTSAVE_CLICK)
# 4 SAVEQUIT   edit, Ctrl-Q, Return (Save): the FILE COPY runs, then exits;
#              TABLES.TXT holds the edit.
FLAIR_CALC_QG_SAVEQUIT := $(FLAIR_CALC_EDIT_A1),kctrl-q,kret
# 5 CLEAN      no edit, Ctrl-Q: exits at once, never asks.
FLAIR_CALC_QG_CLEAN := $(FLAIR_CALC_SPEC),kctrl-q
# 6 PENDING    "7" typed, NOT entered, Ctrl-Q: asks.
FLAIR_CALC_QG_PENDING := $(FLAIR_CALC_SPEC),k7,kctrl-q
# 7 CLOSE      edit, the close box: asks (no terminate).
FLAIR_CALC_QG_CLOSE := $(FLAIR_CALC_EDIT_A1),$(FLAIR_CALC_CLOSE_CLICK)
# 8 SAVE       edit, Ctrl-S, wait for the copy: TABLES.TXT holds the 73 (L006).
FLAIR_CALC_QG_SAVE := $(FLAIR_CALC_EDIT_A1),kctrl-s
# 9 REPLACEASK    (a volume that ALREADY holds APPS\TABLES.TXT, written by mtools)
#                  edit, Ctrl-S: the replace question is up.
FLAIR_CALC_QG_REPLACEASK := $(FLAIR_CALC_EDIT_A1),kctrl-s
# 10 REPLACECANCEL ... Escape: the existing TABLES.TXT is untouched.
FLAIR_CALC_QG_REPLACECANCEL := $(FLAIR_CALC_EDIT_A1),kctrl-s,kesc
# 11 REPLACEYES    ... Return (Replace): TABLES.TXT is the edited worksheet.
FLAIR_CALC_QG_REPLACEYES := $(FLAIR_CALC_EDIT_A1),kctrl-s,kret
