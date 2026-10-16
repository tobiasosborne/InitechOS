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
# PM MERGE RE-KEY 2026-10-17 (carried into lane/frameapps verbatim from main's
# working tree): APPS holds TENANTFX, CTENANT, 123.EXE, WORD.EXE, 123.MNU,
# CHKDSK, ATTRIB, LABEL, XCOPY, TREE, CALC -- CALC.EXE is cell 10 = row 3,
# col 2, centre (211,226): +88,+124 from the APPS double-click (123,102). The
# park keeps its absolute end point (143,460): -68,+234 from the new click.
# initech-bf2 / tdnl.20: PAINT.EXE, LEDGER.TXT, MICHAEL.CFG append AFTER Calc
# (cells 11..13), so CALC.EXE's cell 10 does not move.
FLAIR_CALC_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:100,m0:24,l1,l0,l1,l0,m-68:100,m0:100,m0:34
# Same launch; an edit to A1, then a menu-key Quit tears down BOTH windows.
FLAIR_CALC_EDIT_SPEC := $(FLAIR_CALC_SPEC),k7,k3,kret,kctrl-q
# initech-bf2: the same launch, then Cmd/Ctrl-S -- the shell's MenuKey
# resolves it to File > Save (enabled, cmdChar 'S'; spec/toolbox_gate.h
# Sec 6a) and Calc asks the FILE COPY box (Sec 12). The cursor is parked at
# (143,460), clear of the box (140,200)-(500,280).
FLAIR_FILE_COPY_SPEC := $(FLAIR_CALC_SPEC),kctrl-s
# initech-tdnl.20 (bar carrier): PAINT.EXE is APPS cell 11 = row 3, col 4
# (1-based), centre (279,226) = +156,+124 from the APPS double-click (123,102)
# (cell n centre = 75+68*(n%4), 122+52*(n/4)). Same disk/APPS prefix as Calc.
#   FLAIR_PAINT_SPEC: launch, park straight down at (279,460) -- band 2 shows
#     PAINT's bar, the canvas (330,150)-(630,400) is up, nothing pulled.
#   FLAIR_PAINT_MENU_SPEC: launch; to the File title (20,30) in band 2 (no
#     Apple slot; File slot x 0..41, Edit 41..84 = StringWidth+18, NFNT 5478):
#     -259,-196; press (File drops); drag right +42 onto Edit (62,30): the
#     cross-drag drops Edit (FLAIR-MENU-XDROP menu=129, the mid-track capture --
#     an XDROP capture waits only on held moves, so the launch double-clicks
#     keep their timing); drag to Cut (item 2, row centre 41+16+8 = 65): +35;
#     release on a GRAYED item: nothing is delivered. Then File (20,30):
#     -42,-35; press, drag to Quit (item 5, centre 41+4*16+8 = 113): +83;
#     release -> EXIT.
# PM MERGE RE-KEY 2026-10-17: PAINT.EXE is cell 13 (row 4, col 1), centre (143,278): +20,+176 from the
# APPS click; the park keeps its absolute end point (279,460): +136,+182 from the new click.
FLAIR_PAINT_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m20:100,m0:76,l1,l0,l1,l0,m100:100,m36:82
# PM MERGE RE-KEY 2026-10-17: same cell-13 launch; the move to the File menu (20,30) is -123,-248 from (143,278).
FLAIR_PAINT_MENU_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m20:100,m0:76,l1,l0,l1,l0,m-100:-100,m-23:-100,m0:-48,l1,m42:0,m0:35,l0,m-42:-35,l1,m0:83,l0
