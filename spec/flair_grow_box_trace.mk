# h6y1 locked mouse trace. Ref: finder_windows.h default cascade/grid;
# sys8/window-chrome.md Sec 5 grow cell; window.c ConstrainWindowSize.
# Cursor starts (320,240); all hops int8-safe; y is down.
# Open volume at (600,64), then APPS at (128,102). APPS slot1 frame:
# (40,80)..(400,300), grow cell (380,280)..(398,298).
# Press (390,290), drag (+40,+40), release (430,330): frame400x260,
# new cell (420,320)..(438,338). Park (620,460), clear of the gadget.
# Finder slot1 is live marker win0: kmain.c flair_live_window_index delegates
# disk windows to WindowMgr_window_index (front-to-back rank), not Finder slot.
# No file is added to the shipped data volume.
FLAIR_GROW_BOX_OPEN := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-72:0,l1,l0,l1,l0
FLAIR_GROW_BOX_SPEC := $(FLAIR_GROW_BOX_OPEN),m100:94,m100:94,m62:0,l1,m40:40,l0,m100:100,m90:30
