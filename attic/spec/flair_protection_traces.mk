# LOCKED trace for audit L118. Same volume/window geometry as
# spec/flair_trash_traces.mk: README.TXT (55,102) -> Trash (600,420).
# Ref: Apple_Macintosh_System_Software_Users_Guide_V6.0.pdf pp. 88, 112.
# A protected source must revert and display a notice; PARK at (620,460).
FLAIR_LOCKED_TRASH_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,m20:38

# The locked-full pixel oracle grades mint-008's UNSELECTED full can.
# After opening Trash, click bare desktop (620,360) to clear selection,
# then return to (600,422) before the unchanged Empty Trash menu path.
# Keep the normal confirm/empty trace's selected-can contract unchanged.
FLAIR_LOCKED_PURGE_OPEN := l1,l0,l1,l0,m-100:-98
FLAIR_LOCKED_PURGE_CLEAR := l1,l0,l1,l0,m20:-62,l1,l0,m-20:62,m-100:-98
FLAIR_LOCKED_PURGE_SPEC := $(subst $(FLAIR_LOCKED_PURGE_OPEN),$(FLAIR_LOCKED_PURGE_CLEAR),$(FLAIR_EMPTY_OK_SPEC))
