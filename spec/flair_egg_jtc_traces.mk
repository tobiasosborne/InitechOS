# LOCKED, operator-authored initech-2ter (a). Start (320,240).
# Apple (16,10), hold Option/PC Alt, three clicks. No menu row changes.
# From (16,10), pad 8 corner (136,358), not its centre: mouseDown must
# WARP the Event Manager AND Cursor Manager to (218,372). Return dismisses
# the one-button alert. The pauses expose both stills and the Rule-14 clip.
FLAIR_EGG_JTC_OPEN_SPEC := m-100:-77,m-100:-77,m-100:-76,m-4:0,Kalt:1,l1,l0,l1,l0,l1,l0,Kalt:0,w800
FLAIR_EGG_JTC_ALERT_SPEC := $(FLAIR_EGG_JTC_OPEN_SPEC),m60:100,m60:100,m0:100,m0:48,l1,w800,l0
FLAIR_EGG_JTC_SPEC := $(FLAIR_EGG_JTC_ALERT_SPEC),kret,w500
FLAIR_EGG_JTC_SHORT_SPEC := m-100:-77,m-100:-77,m-100:-76,m-4:0,Kalt:1,l1,l0,l1,l0,Kalt:0
