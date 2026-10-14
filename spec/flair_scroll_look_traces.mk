# Locked trace, 2026-10-15, initech-tdnl.82/.86/.96/.97/.101.
# Root: frame (20,60)..(380,280), two visible entries. Content extent136x55.
# Shrink at grow point(370,270) by(-232,-100) =>128x120, view106x77:
# H max30, span56; thumb x41 (value0), x70 (value16). V disabled.
# Right arrow(120,167). Restore grow(138,170) by(+232,+100) =>360x220:
# both max0, value0. Request60x40 at grow point(370,270) by(-300,-180):
# derived bar-fit clamp96x88, view74x45 => H max62, V max10.
# Min V bar (95,81)..(111,128), thumb(96,97)..(110,112), tiles y82/113.
# Min H bar (25,127)..(96,143), thumb(41,128)..(56,142), tiles x26/81.
# Click visible min down arrow(103,120) =>value10. PARK always(620,460).
# Measured parts: system7-decomp/specs/sys8/scrollbars.md 1,2.1,2.4.
# No measured historical Finder minimum exists in that corpus. The 88px floor
# is derived from47px of parts plus FLAIR's41px title/grow reservation.
SCROLL_LOOK_OPEN := m100:-88,m100:-88,m80:0,l1,l0,l1,l0
SCROLL_LOOK_SHRINK := $(SCROLL_LOOK_OPEN),m-100:100,m-100:100,m-30:6,l1,m-100:-50,m-100:-50,m-32:0,l0
SCROLL_LOOK_CLICK := $(SCROLL_LOOK_SHRINK),m-18:-3,l1,l0
SCROLL_LOOK_RESTORE := $(SCROLL_LOOK_CLICK),m18:3,l1,m100:50,m100:50,m32:0,l0
SCROLL_LOOK_MIN := $(SCROLL_LOOK_RESTORE),l1,m-100:-90,m-100:-90,m-100:0,l0,m33:30,l1,l0
FLAIR_SCROLL_LOOK_SHRINK_SPEC := $(SCROLL_LOOK_SHRINK),m100:100,m100:100,m100:90,m100:0,m82:0
FLAIR_SCROLL_LOOK_CLICK_SPEC := $(SCROLL_LOOK_CLICK),m100:100,m100:100,m100:93,m100:0,m100:0
FLAIR_SCROLL_LOOK_RESTORE_SPEC := $(SCROLL_LOOK_RESTORE),m100:100,m100:90,m50:0
FLAIR_SCROLL_LOOK_SPEC := $(SCROLL_LOOK_MIN),m100:100,m100:100,m100:100,m100:40,m100:0,m17:0
