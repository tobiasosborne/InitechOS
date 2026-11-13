# R4.5 locked traces. Start (320,240). Apple Control Panels row4 centre(60,66).
# DA frame (120,90)..(520,390), content (126,112). Dialog items below are
# composed from existing furniture, NOT a claim of native panel geometry.
# Chooser panel buttons centres x240,y172/200/228/256/284. All traces park
# outside the dialog. Normal tracking is 1:1; mouse test uses keys after OK.
CP_LAUNCH_SPEC := m-100:-77,m-100:-77,m-100:-77,l1,w300,m40:57,w300,l0,w600
CP_DATE_SPEC := $(CP_LAUNCH_SPEC),m100:53,m80:53,l1,l0,w200,m100:0,m30:0,l1,l0,w200
CP_PATTERNS_SPEC := $(CP_LAUNCH_SPEC),m100:67,m80:67,l1,l0,w200,m-34:12,l1,l0,w200
CP_MOUSE_SPEC := $(CP_LAUNCH_SPEC),m100:81,m80:81,l1,l0,w200,m-5:-8,l1,l0,w200
CP_SOUND_SPEC := $(CP_LAUNCH_SPEC),m100:95,m80:95,l1,l0,w200,m-5:-60,l1,l0,w200
CP_MEMORY_SPEC := $(CP_LAUNCH_SPEC),m100:109,m80:109,l1,l0,w200
# Display snapshots end with an out-of-window pointer. No commit yet.
CP_PARK := m100:60,m100:60,m100:60,m80:60,w400
FLAIR_CP_DATE_SPEC := $(CP_DATE_SPEC),$(CP_PARK)
FLAIR_CP_PATTERNS_SPEC := $(CP_PATTERNS_SPEC),$(CP_PARK)
FLAIR_CP_MOUSE_SPEC := $(CP_MOUSE_SPEC),$(CP_PARK)
FLAIR_CP_SOUND_SPEC := $(CP_SOUND_SPEC),$(CP_PARK)
FLAIR_CP_MEMORY_SPEC := $(CP_MEMORY_SPEC),$(CP_PARK)
# Persistence/RTC are independently observed after OK. Return to chooser,
# Escape quits; click bare desktop makes Finder foreground; Special Restart.
CP_SAVE_SPEC := $(CP_PATTERNS_SPEC),kret,w300,kesc,w300,m-53:104,m-53:104,l1,l0,w300,m70:-100,m0:-100,m0:-100,m0:-90,l1,m0:79,l0
CP_CLOCK_SAVE_SPEC := $(CP_DATE_SPEC),kret,w1200
CP_CANCEL_SPEC := $(CP_PATTERNS_SPEC),kesc,w300

CP_MOUSE_APPLY_SPEC := $(CP_MOUSE_SPEC),m100:0,m85:0,l1,l0,kret,w300,m1:0,w300
CP_SOUND_APPLY_SPEC := $(CP_SOUND_SPEC),kret,w300
CP_RESET_POINTER := m-127:-127,m-127:-127,m-127:-127,m-127:-127,m-127:-127
# Rule14: chooser and all five panels, Cancel between views; no changed pump.
CP_CLIP_SPEC := $(CP_LAUNCH_SPEC),m100:53,m80:53,l1,l0,w600,kesc,w300,$(CP_RESET_POINTER),m120:100,m120:100,l1,l0,w600,kesc,w300,$(CP_RESET_POINTER),m120:114,m120:114,l1,l0,w600,kesc,w300,$(CP_RESET_POINTER),m120:127,m120:127,m0:2,l1,l0,w600,kesc,w300,$(CP_RESET_POINTER),m120:127,m120:127,m0:30,l1,l0,w600,m100:44,m100:44,m100:44,m80:44,w400

CP_SAVE_ONLY_SPEC := $(CP_PATTERNS_SPEC),kret,w300
