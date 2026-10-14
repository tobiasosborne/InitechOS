# spec/flair_i123_quit_traces.mk -- the LOCKED input traces of
# test-flair-i123-quitguard (bead initech-fvnj; astra audit L008 "Quit discards
# an edited worksheet without an unsaved-work warning"). Locked spec-data
# (CLAUDE.md Rule 8, Rule 11); bound by every convention of
# spec/flair_app_launch_traces.mk (cursor starts (320,240); "m<dx>:<dy>"
# relative, int8-safe; "l1"/"l0" left button; "k<chord>" one keystroke).
#
# THE SCENE: $(FLAIR_I123_FILE_GATE_IMG) (the flagship desktop, 500-tick pump
# life) + a gate-local copy of $(BUILD)/flair_i123_quit_data.img: the
# tdnl.91 data volume (APPS\123.EXE, MINT01.WK1, BAD.WK1) plus RO.WK1, a
# byte copy of MINT01.WK1 with the DOS read-only attribute (lane-dosprot:
# CREAT over it is refused, Access denied -- the honest Save failure).
#
# THE PREFIXES (spec/flair_i123_traces.mk, spec/flair_i123_file_traces.mk):
#   $(FLAIR_I123_TYPE_SPEC)   launch 123 from APPS and type the MINT01 first
#                             row "Hello 123 1.5 -7" (four committed cells);
#   $(FLAIR_I123_FILE_LAUNCH) launch only (a clean sheet).
# Both leave the cursor on the 123.EXE icon centre (211,122): every later
# step is a key, so a relaunch is a plain double-click there.
#
# GEOMETRY (derived, never measured off a render):
#   the 123 window frame (0,40)..(640,480) (face_a.c WIN_*); its go-away box =
#     frame + (4,4)..(16,16) (the spec/flair_app_launch_traces.mk rule) =
#     (4,44)..(16,56), centre (10,50): from (211,122) m-100:-36,m-101:-36;
#   the alert (spec/toolbox_gate.h Sec 11): Save (default) 435..494, Cancel
#     363..422, Don't Save 211..295, buttons y 158..178 -> Don't Save centre
#     (253,168): from (211,122) m42:46. Return = Save, Escape = Cancel.
#
# THE KEYS: kctrl-q = File > Quit of 123's own bar (TBX_EVT_MENU, Sec 6a);
#   /qy = the R2.2 /Quit Yes (Reference Manual p. 9-1); the save prompt is the
#   tdnl.91 /File Save prompt (123.RI 0x2f92); "q" -> q.wk1 (p. 5-21).
#
# 1. ALERT     dirty sheet, Ctrl-Q: the alert is LEFT UP (screendump).
FLAIR_I123_QG_ALERT := $(FLAIR_I123_TYPE_SPEC),kctrl-q
# 2. CANCEL    dirty, Ctrl-Q, Escape, Ctrl-Q again, Escape: never exits.
FLAIR_I123_QG_CANCEL := $(FLAIR_I123_TYPE_SPEC),kctrl-q,kesc,kctrl-q,kesc
# 3. DONTSAVE  dirty, Ctrl-Q, click Don't Save: exits, writes nothing.
FLAIR_I123_QG_DONTSAVE := $(FLAIR_I123_TYPE_SPEC),kctrl-q,m42:46,l1,l0
# 4. PENDING   clean sheet, "Hel" typed and NOT entered, Ctrl-Q (asks),
#              Escape, then "p" + Return: the kept entry commits "Help".
FLAIR_I123_QG_PENDING := $(FLAIR_I123_FILE_LAUNCH),kshift-h,ke,kl,kctrl-q,kesc,kp,kret
# 5. SAVE      dirty, Ctrl-Q, Return (Save), "q" Return (Q.WK1), exits;
#              relaunch, /File Retrieve q: the row is back.
FLAIR_I123_QG_SAVE := $(FLAIR_I123_TYPE_SPEC),kctrl-q,kret,kq,kret,l1,l0,l1,l0,kslash,kf,kr,kq,kret
# 6. SAVEFAIL  dirty, Ctrl-Q, Return (Save), "ro" Return (RO.WK1 exists:
#              Cancel/Replace/Backup), r (Replace): CREAT refused -> "File
#              I/O error", stays open; Escape, Ctrl-Q asks AGAIN, Escape.
FLAIR_I123_QG_SAVEFAIL := $(FLAIR_I123_TYPE_SPEC),kctrl-q,kret,kr,ko,kret,kr,kesc,kctrl-q,kesc
# 7. CLOSE     dirty, the close box: asks (no terminate); Escape.
FLAIR_I123_QG_CLOSE := $(FLAIR_I123_TYPE_SPEC),m-100:-36,m-101:-36,l1,l0,kesc
# 8. CLOSECLEAN  clean sheet, the close box: exits at once, never asks.
FLAIR_I123_QG_CLOSECLEAN := $(FLAIR_I123_FILE_LAUNCH),m-100:-36,m-101:-36,l1,l0
# 9. SLASH     dirty, /Quit Yes: asks; Escape (the second No): READY.
FLAIR_I123_QG_SLASH := $(FLAIR_I123_TYPE_SPEC),kslash,kq,ky,kesc
