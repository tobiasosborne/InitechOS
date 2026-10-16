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
# Same launch; an edit to A1, then a menu-key Quit tears down BOTH windows.
FLAIR_CALC_EDIT_SPEC := $(FLAIR_CALC_SPEC),k7,k3,kret,kctrl-q
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
