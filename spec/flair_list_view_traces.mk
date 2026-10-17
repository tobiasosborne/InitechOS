# tdnl.44 LOCKED mouse traces. Sources: finder_windows.h default frame/grid;
# flair_grow_box_trace.mk; mint-results-008 Sec6. Int8-safe hops, cursor320,240.
# Volume600,64; APPS128,102; move APPS frame40,80 ->8,60 via title200,90.
# Grow358,270 ->629,390: frame8,60..639,400; content14,82..618,379.
# View140,30 -> item3 by Name140,80. Header Size461,92. Park620,460.
FLAIR_LIST_OPEN := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-72:0,l1,l0,l1,l0
FLAIR_LIST_POSITION := $(FLAIR_LIST_OPEN),m72:-12,l1,m-32:-20,l0,m95:100,m95:100,l1,m100:60,m100:60,m71:0,l0,m-100:-90,m-100:-90,m-100:-90,m-100:-90,m-89:0,l1,m0:50,l0
FLAIR_LIST_NAME_SPEC := $(FLAIR_LIST_POSITION),m120:95,m120:95,m120:95,m120:95
FLAIR_LIST_SIZE_SPEC := $(FLAIR_LIST_POSITION),m100:4,m100:4,m100:4,m21:0,l1,l0,m100:100,m59:100,m0:100,m0:68
# Restart: same Size click, then Special170,30 -> Restart170,109.
FLAIR_LIST_RESTART_SPEC := $(FLAIR_LIST_POSITION),m100:4,m100:4,m100:4,m21:0,l1,l0,m-100:-31,m-100:-31,m-91:0,l1,m0:79,l0
# Fresh boot: existing views restore APPS at8,60, default360x220.
FLAIR_LIST_REOPEN_SPEC := $(FLAIR_LIST_OPEN),m120:95,m120:95,m120:95,m120:95,m12:-22

# Root Name list: first alphabetical entry APPS is a folder. This separately
# grades all native16 folder colours/mask against the independent folder cell.
FLAIR_LIST_ROOT_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:-8,m-100:-8,m-100:-8,m-100:-8,m-60:-2,l1,m0:50,l0,m120:95,m120:95,m120:95,m120:95
