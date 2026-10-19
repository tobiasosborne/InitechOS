# LOCKED B31 / initech-umfi cell hit-testing trace (Rule 8). Deliberately added.
# Ref: the audit L017 expectation (a click on a cell selects it, AUTHORED);
# Lotus 1-2-3 R2.2 Reference Manual pp. 1-3/1-4 (the screen), p. 1-10 (Tab /
# Shift-Tab one screen). Launch geometry + conventions: flair_i123_traces.mk
# (cursor starts (320,240); "m<dx>:<dy>" relative, y down, int8-safe).
#
# GEOMETRY (derived, never measured off a render):
#   after FLAIR_I123_FILE_LAUNCH the cursor sits on 123.EXE, (211,122);
#   Face A cell (char c, line L) at GLOBAL (7 + 8c, 62 + 16L); row 4 is line
#   7 (y 174..190); the fifth area column is chars 40..48 (x 327..399), its
#   centre (363,182) -- E4 unscrolled, M4 after Tab (column I first);
#   the 123 frame (0,40)..(640,480): grow cell (620,460)..(638,478) (frame
#   right/bottom - 20 .. - 2, flair_grow_box_trace.mk); press (629,469),
#   drag (-200,-100), release (429,369): frame 440 x 340 (FLAIR-GROW
#   (640,440)->(440,340)); (363,182) stays inside the shrunk content.
#
# Scene: launch; Tab (I1, column I first); click (363,182) -> M4; Shift-Tab
# (A4, column A first); grow-drag the window smaller; click (363,182) -> E4;
# park the cursor below the sheet. W tokens are completion handshakes on
# rendered text / the kernel's grow record, never timeout allowances. The
# first click's barrier is "4:", the tail of the row-4 address that click
# draws first on line one (M4: clean; E4: under the mutant), so a mutant
# whose click lands elsewhere still runs the rest of the trace in the pump
# life; the gate, not the barrier, decides which cell it was.
#   (211,122) +m100:60 +m52:0            -> (363,182)
#   (363,182) +m100:100 +m100:100 +m66:87 -> (629,469)  the grow cell
#   drag m-100:-50 m-100:-50             -> (429,369)
#   (429,369) +m-66:-100 +m0:-87          -> (363,182)
#   park m0:100                           -> (363,282)
FLAIR_I123_HIT := $(FLAIR_I123_FILE_LAUNCH),WA1:,\
ktab,WI1:,\
m100:60,m52:0,l1,l0,W4:,\
kshift-tab,WA4:,\
m100:100,m100:100,m66:87,l1,m-100:-50,m-100:-50,l0,WFLAIR-GROW win,\
m-66:-100,m0:-87,l1,l0,WE4:,\
m0:100
