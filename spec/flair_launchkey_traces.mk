# initech-rex3: deliberate Rule-8 addition, PRD 6.3/6.5; DEC-LAUNCH-1.
# Ref: Lotus 1-2-3 R2.2 Reference Manual p. 1-17 (PDF 33): backslash
# repeats the label's characters to fill the cell. Existing locked 123
# launch geometry is flair_i123_traces.mk A/B/C123 (APPS cell 2).
# Store backslash-minus in A1, then the system chord runs SAMIR. QUIT
# returns to the SAME sheet; Right proves the resumed tenant still takes keys.
# Rule-8 ordering: wait for FLAIR-RESUME before Right. A key sent while
# SAMIR still owns the screen is its input and is correctly drained on return.
FLAIR_LAUNCHKEY_OPEN := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:20,l1,l0,l1,l0
FLAIR_LAUNCHKEY_PLAIN_SPEC := $(FLAIR_LAUNCHKEY_OPEN),kbsl,kminus,kret
FLAIR_LAUNCHKEY_RESUME_SPEC := $(FLAIR_LAUNCHKEY_PLAIN_SPEC),kctrl-alt-bsl,kq,ku,ki,kt,kret,WFLAIR-RESUME,kright
