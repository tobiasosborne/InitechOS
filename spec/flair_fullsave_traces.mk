# spec/flair_fullsave_traces.mk -- the LOCKED input traces of
# test-flair-fullsave and test-flair-notepad-keep (bead initech-mcep: a save
# that fails on a full disk must leave the last saved file exactly as it was;
# audits docs/audits/2026-10-23-iword L001/L002, -das L003/L008, -i123 L001).
# Locked spec-data (CLAUDE.md Rule 8, Rule 11); bound by every convention of
# spec/flair_app_launch_traces.mk (cursor starts (320,240); "m<dx>:<dy>"
# relative, y down, int8-safe; "l1"/"l0" left button; "k<chord>" one
# keystroke; "w<ms>" a hold that changes no event the app sees).
#
# THE SCENE: $(FLAIR_IWORD_GATE_IMG) (the flagship desktop, pump life 1500
# ticks) + a gate-local copy of $(FLAIR_FULLSAVE_DATA) (harness/emu/
# fullsave.mk): the flagship volume + ORIGINAL.WP5, LONG.WP5, KEEP.TXT,
# KEEP.WK1, MINT01.WK1 and a FILLER.BIN that leaves two clusters free for
# the desktop's own DESKTOP.DB / TRASH. Every save below needs more clusters
# than its target and the free space together hold, so each one FAILS.
#
# LAUNCH PREFIXES (copied EXACTLY, never re-derived): WORD.EXE = the first 22
# tokens of FLAIR_IWORD_SPEC (spec/flair_iword_traces.mk, APPS cell 3); TEXT.EXE
# = FLAIR_ITEXT_SPEC through its "m-68:0,w400" (spec/flair_itext_traces.mk,
# APPS cell 6); 123.EXE = FLAIR_I123_FILE_LAUNCH (spec/flair_i123_file_
# traces.mk, APPS cell 2).
#
# 1. FLAIR_FULLSAVE_IWORD_SPEC -- InitechWord (audit iword L001 S02 and L002):
#    Shift-F10 "long.wp5" Return (Retrieve, 2,679 bytes); F10, "original.wp5"
#    over the offered name, Return, y ("Replace A:\ORIGINAL.WP5? No (Yes)"):
#    ERROR: Disk Full; F1 (any key), F1 (Cancel); F10 "new.wp5" Return:
#    ERROR: Disk Full again; F1. Prompts: WP 5.1 Reference Manual PDF 213/763.
FLAIR_FULLSAVE_IWORD_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m78:10,m78:10,l1,l0,l1,l0,w400,kshift-f10,kl,ko,kn,kg,kdot,kw,kp,k5,kret,w600,kf10,ko,kr,ki,kg,ki,kn,ka,kl,kdot,kw,kp,k5,kret,w300,ky,w600,kf1,kf1,w200,kf10,kn,ke,kw,kdot,kw,kp,k5,kret,w600,kf1,w300
#
# 2. FLAIR_FULLSAVE_ITEXT_SPEC -- InitechText (audit das L003): type
#    "pc load letter. " (16 characters), then seven doublings (Ctrl-A, Ctrl-C,
#    Right = the caret to the end, a 150 ms hold -- a Ctrl-V right behind
#    the Right was once dropped by a loaded host --, Ctrl-V): 2,048 characters; Ctrl-S (untitled
#    -> the Save As box, "UNTITLED" typed over: 8 Backspaces, "keep.txt"),
#    Return, Return (the Standard File replace question's default: Replace):
#    "The document could not be saved."; Return (OK).
FLAIR_FULLSAVE_ITEXT_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:72,l1,l0,l1,l0,m-68:0,w400,kp,kc,kspc,w120,kl,ko,ka,kd,kspc,w120,kl,ke,kt,kt,ke,kr,kdot,kspc,w120,kctrl-a,kctrl-c,kright,w150,kctrl-v,w200,kctrl-a,kctrl-c,kright,w150,kctrl-v,w200,kctrl-a,kctrl-c,kright,w150,kctrl-v,w200,kctrl-a,kctrl-c,kright,w150,kctrl-v,w200,kctrl-a,kctrl-c,kright,w150,kctrl-v,w200,kctrl-a,kctrl-c,kright,w150,kctrl-v,w200,kctrl-a,kctrl-c,kright,w150,kctrl-v,w300,kctrl-s,w600,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,kk,ke,ke,kp,kdot,kt,kx,kt,kret,w600,kret,w800,kret,w300
#
# 3. FLAIR_FULLSAVE_I123_SPEC -- Initech 123 (audit i123 L001): /File
#    Retrieve "mint01" Return (the real MINT01.WK1, 1,609 bytes); /File Save
#    "keep" Return (x -> x.wk1, Reference Manual p. 5-21); the existing file
#    asks Cancel / Replace / Backup (p. 5-22): r (Replace): "File I/O error".
FLAIR_FULLSAVE_I123_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:20,l1,l0,l1,l0,w400,kslash,kf,kr,km,ki,kn,kt,k0,k1,kret,w600,kslash,kf,ks,kk,ke,ke,kp,kret,w300,kr,w800
#
# 4. FLAIR_NOTEPAD_KEEP_SPEC -- Note Pad over a store whose ONLY defect is its
#    checksum (audit das L008; volume $(DA_BADSUM_DATA), harness/emu/
#    notepad.mk, NOT full): open Note Pad from the Apple menu (the first 13
#    tokens of FLAIR_NOTEPAD_OPEN_SPEC, spec/flair_notepad_traces.mk), type x,
#    Ctrl-W (Close): the damaged-store alert; Return (OK): Note Pad ends.
FLAIR_NOTEPAD_KEEP_SPEC := m-100:-100,m-100:-70,m-50:0,l1,l0,w300,m-50:-61,l1,w200,m40:73,l0,m0:-16,w600,kx,w200,kctrl-w,w600,kret,w600
