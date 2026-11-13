# initech-tdnl.91: deliberate locked input addition. Authored fixture actions;
# Finder/CTENANT geometry from flair_ctenant_traces.mk, frame (180,170).
FLAIR_FILE_LAUNCH := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m20:20,l1,l0,l1,l0
FLAIR_FILE_ROUND := $(FLAIR_FILE_LAUNCH),kq
FLAIR_FILE_QUIT := $(FLAIR_FILE_LAUNCH),ko,l1,l0,l1,l0,ko
# Rule 8 note (initech-3j17, PM approved): the tdnl.65/.88 crash notice
# postdates this trace. Dismiss it with Return after EACH crash so the next
# clicks relaunch FILETEN; the gate requires both alerts and dismissals.
FLAIR_FILE_CRASH := $(FLAIR_FILE_LAUNCH),kx,kret,l1,l0,l1,l0,kx,kret
# Close centre (190,181), then back to icon (143,122), relaunch, close again.
FLAIR_FILE_CLOSE := $(FLAIR_FILE_LAUNCH),kc,m47:59,l1,l0,m-47:-59,l1,l0,l1,l0,kc,m47:59,l1,l0
