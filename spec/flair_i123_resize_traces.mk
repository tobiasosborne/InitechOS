# LOCKED B12 / initech-89rc resized-worksheet trace (Rule 8). Deliberately added.
# Ref: audit L018 (docs/audits/2026-10-05-astra-full-drive/REPORT.md: "Shrink
# the 640x440 123 window by about 288x194 with its grow box. Move Right to
# E1." -- the current cell must stay visible, AUTHORED expectation); Lotus
# 1-2-3 R2.2 Reference Manual pp. 1-3/1-4 (the screen). Launch geometry +
# conventions: flair_i123_traces.mk (cursor starts (320,240); "m<dx>:<dy>"
# relative, y down, int8-safe), flair_i123_file_traces.mk (the launch).
#
# GEOMETRY (derived, never measured off a render):
#   after FLAIR_I123_FILE_LAUNCH the cursor sits on 123.EXE, (211,122);
#   the 123 frame (0,40)..(640,480): grow cell (620,460)..(638,478) (frame
#   right/bottom - 20 .. - 2, flair_grow_box_trace.mk), centre (629,469);
#   a frame W x H has content (W - 27) x (H - 43) (CalcDocContentRect: the
#   640 x 440 frame's 613 x 397, tdnl.51), (W - 28) / 8 whole characters
#   and (H - 43) / 16 whole lines (face_a.c / test_i123_resize.c table);
#   the grow cell moves with the frame's bottom-right corner, so the release
#   point of one grow is the centre of the next grow cell.
#
# Scene:
#   1 SHRINK  (211,122) +m100:100 x3 +m118:47 -> (629,469); press, drag
#             m-100:-100 m-100:-94 m-88:0 -> (341,275); release: frame 352 x
#             246 (the audit's size; FLAIR-GROW (640,440)->(352,246)), 40 x 12
#             characters, A..D shown;
#   2 L018    Right x4 -> E1: the view scrolls to B..E;
#   3 GROW    press (341,275) (the new grow cell), drag m100:50 x2 ->
#             (541,375): frame 552 x 346, 65 x 18, B..G shown, no scroll;
#   4 SHRINK  press (541,375), drag m-100:-50 x2 m-72:0 -> (269,275): frame
#             280 x 246, 31 x 12, three columns: the view scrolls to C..E;
#   park m0:100 -> (269,375), below the window.
# W tokens are completion handshakes on the kernel's grow records / rendered
# text, never timeout allowances; each grow barrier names its own new width
# ("->(352": the trace is comma-separated, so the full "(w,h)" cannot be a
# token; the width alone is unique in this scene).
FLAIR_I123_RESIZE := $(FLAIR_I123_FILE_LAUNCH),WA1:,\
m100:100,m100:100,m100:100,m118:47,l1,m-100:-100,m-100:-94,m-88:0,l0,W->(352,\
kright,kright,kright,kright,WE1:,\
l1,m100:50,m100:50,l0,W->(552,\
l1,m-100:-50,m-100:-50,m-72:0,l0,W->(280,\
m0:100
