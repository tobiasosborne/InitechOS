# LOCKED B06 / initech-3qtt oversized-column trace (Rule 8). Deliberately added.
# Ref: audit L008 (docs/audits/2026-10-23-i123/REPORT.md: "Put 1 in A1. At A1
# choose /Worksheet Column Set-Width, enter 240." -- heading, value and
# pointer vanished); the REAL Lotus 1-2-3 R2.2 under DOSBox-X,
# fixtures/i123-width/width*.steps (05_w240: A clipped to the 72-character
# area; 06_w240_right: Right puts B first; 15_back_a1: Left brings the
# clipped A back). Launch geometry + conventions: flair_i123_traces.mk
# (cursor starts (320,240); "m<dx>:<dy>" relative, y down, int8-safe),
# flair_i123_file_traces.mk (the launch).
#
# GEOMETRY (derived, never measured off a render):
#   after FLAIR_I123_FILE_LAUNCH the cursor sits on 123.EXE, (211,122);
#   Face A cell (char c, line L) at GLOBAL (7 + 8c, 62 + 16L); A2 is line 5
#   (y 142..157); area char 60 of the clipped A is char 64 (x 519..526), the
#   click point (523,150) -- inside A, past every whole-column boundary the
#   default width 9 would put there.
#
# Scene: launch; 1 Enter (A1); /Worksheet Column Set-Width 240 Enter; Right
# (B1, B first); Left (A1, the clipped A first again); click (523,150) -> A2;
# park the cursor below. W tokens are completion handshakes on rendered text,
# never timeout allowances; the gate, not the barrier, decides what was drawn.
#   (211,122) +m100:28 +m100:0 +m100:0 +m12:0 -> (523,150)
#   park m0:100                               -> (523,250)
FLAIR_I123_WIDE := $(FLAIR_I123_FILE_LAUNCH),WA1:,\
k1,kret,WA1: 1,\
kslash,kw,kc,ks,k2,k4,k0,kret,\
kright,WB1:,\
kleft,\
m100:28,m100:0,m100:0,m12:0,l1,l0,WA2:,\
m0:100
