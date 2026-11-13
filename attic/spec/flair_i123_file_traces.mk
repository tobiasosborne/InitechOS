# initech-tdnl.91 slice 2: deliberate locked keystroke traces.
# Geometry/typing: flair_i123_traces.mk (the independently minted MINT01 row).
# Commands: R2.2 Reference Manual pp. 1-13/14, 5-19..22; 123.RI resource
# offsets 0x15f (File menu), 0x2f43 (Retrieve prompt), 0x2f92 (Save prompt).
# Authored filenames/actions, no local reference. x -> x.wk1 by p. 5-21.
# 53 tokens exceed a comfortable 250-tick margin: use the existing 500-tick
# gate kernel (same desktop/ABI, only the test pump lifetime differs).
FLAIR_I123_FILE_LAUNCH := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:20,l1,l0,l1,l0
FLAIR_I123_FILES := $(FLAIR_I123_TYPE_SPEC),kslash,kf,ks,kx,kret,kctrl-q,l1,l0,l1,l0,kslash,kf,kr,kx,kret
FLAIR_I123_CORPUS := $(FLAIR_I123_FILE_LAUNCH),kslash,kf,kr,km,ki,kn,kt,k0,k1,kret
# Visible unsupported command, back out, file menu, then real Retrieve prompt.
FLAIR_I123_FILE_UI := $(FLAIR_I123_FILE_LAUNCH),kslash,kw,kesc,kslash,kf,kr
FLAIR_I123_FILE_ERROR := $(FLAIR_I123_TYPE_SPEC),kslash,kf,kr,kz,kret
# BAD.WK1 has only the valid BOF and no EOF (wk1_format.h framing contract).
# After the refusal, Esc/Home forces the old row to repaint from the model.
FLAIR_I123_FILE_BAD := $(FLAIR_I123_TYPE_SPEC),kslash,kf,kr,kb,ka,kd,kret,kesc,khome
# Existing Save: Cancel keeps disk bytes, then Replace updates to "Changed".
# Start by retrieving the real MINT01; edit A1, name the same file, cancel,
# then save again, replace. Later oracle grades both disk legs independently.
FLAIR_I123_CANCEL := $(FLAIR_I123_FILE_LAUNCH),kslash,kf,kr,km,ki,kn,kt,k0,k1,kret,khome,kshift-c,kh,ka,kn,kg,ke,kd,kret,kslash,kf,ks,kret,kc
FLAIR_I123_REPLACE := $(FLAIR_I123_CANCEL),kslash,kf,ks,kret,kr
