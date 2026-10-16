# tdnl.44 LOCKED, PARTIAL mouse trace. Ref: flair_disk_windows_traces.mk;
# finder_menu.c View item 3; chrome_metrics.json platinum_menu_layout;
# finder_windows.h default frame/grid; tdnl.51 content-left +6 correction.
# This is NOT the missing list-view pixel golden and does not invent metrics.
#
# Cursor starts320,240. Open volume600,64. Root APPS sprite112,86 centre128,102:
# root frame20,60 + content-left6 + inset18 + pitch68 =112.
# Double-click APPS -> slot1. Move128,102 to View title140,30 (band2).
# Title slots: Apple33; File23+18=>74; Edit25+18=>117; View32+18=>167.
# Hold and move to140,80: item3 by Name, y[73,89), then release. Park620,460.
#
# STOP: no Mac OS 8.1 list capture exists in goldens/captures. This prelude
# does NOT inject a Size-header click. Both test-flair-list-view and the
# finder_list_view clip fail the capture prerequisite before injection.
# Required: s8_finder_list_view_size.png (Mac OS 8.1, Size selected),
# showing header edges, row pitch, 16px icon placement and sort underline.
# After independent measurement, complete the Size click + Restart trace and
# grade serial and a screendump. Do not lift the host arithmetic's synthetic
# widths or an icon view as the missing oracle (Law 2 / ADR-0010).
FLAIR_LIST_VIEW_PRELUDE_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-72:0,l1,l0,l1,l0,m12:-72,l1,m0:50,l0,m120:95,m120:95,m120:95,m120:95
