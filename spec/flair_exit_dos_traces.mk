# spec/flair_exit_dos_traces.mk -- the LOCKED input traces for Special > Exit
# to InitechDOS and the TPS Report Generator journey (bead initech-3p1a).
# Locked spec-data (CLAUDE.md Rule 8, Rule 11): deterministic, versioned, never
# silently edited to make a test pass.
#
# Conventions are spec/flair_disk_windows_traces.mk's verbatim (relative QMP
# moves from the screen centre (320,240), y screen-down-positive, |c| <= 100;
# "l1"/"l0" left button; "k<chord>" a keystroke in the mouse stream) plus the
# harness's "W<marker>" (harness/emu/qemu.c: hold until the GUEST prints
# <marker> on serial -- the keys after the exit must reach COMMAND.COM, so
# their ordering is the guest's SHELL-READY, never a guessed delay).
#
# THE MENU GEOMETRY (os/flair/menu.h: band-2 panel rows from y 41, item 16 px,
# divider 6 px; spec/flair_finder_cmds_traces.mk's hand derivation, extended):
#   Special 1 Clean Up [41,57) | 2 Empty Trash [57,73) | div [73,79) |
#   4 Erase Disk [79,95) | div [95,101) | 6 Restart [101,117) |
#   7 Shut Down [117,133) | 8 Sleep [133,149) | div [149,155) |
#   10 Exit to InitechDOS [155,171) c163.
# THE ALERT (os/milton/kmain.c flair_live_alert_open, s8_alert_modal.png
# geometry): Exit = item 1 at OK's place (435,158)..(494,178) centre
# (464,168), the default (Return); Cancel = item 2 (363,158)..(422,178)
# (Escape).
#
# THE SHARED PREFIX (X):
#   A. volume double-click -> (600,64): m100:-88, m100:-88, m80:0 ;
#      l1,l0,l1,l0 -> FINDER-OPEN-VOLUME win=0 n=2 (the Finder is foreground:
#      band 2 is the Finder bar from here on)
#   B. -> the band-2 Special title (170,30): sum (-430,-34): m-100:-8 x4,
#      m-30:-2 ; l1 (the Special menu drops)
#   C. m0:100, m0:33 -> (170,163) ; l0 -> FLAIR-MENU menu=515 item=10
#      (sel=0x0203000A) ; FINDER-CMD id=16 name=EXIT_DOS src=mouse sel=0 ;
#      the alert ; FINDER-EXIT-DOS-ALERT
FLAIR_EXIT_DOS_PREFIX := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:-8,m-100:-8,m-100:-8,m-100:-8,m-30:-2,l1,m0:100,m0:33,l0

# 1. FLAIR_EXIT_DOS_ALERT_SPEC -- the alert LEFT UP (screendump graded).
#    PARK (170,163) -> (620,460): sum (+450,+297): m90:59 x5, m0:2.
FLAIR_EXIT_DOS_ALERT_SPEC := $(FLAIR_EXIT_DOS_PREFIX),m90:59,m90:59,m90:59,m90:59,m90:59,m0:2

# 2. FLAIR_EXIT_DOS_CANCEL_SPEC -- Escape cancels; the desktop lives on.
#    kesc -> FINDER-EXIT-DOS-CANCEL ; then kctrl-n -> the live Finder makes a
#    NEWFOLD in the front (root) window (FINDER-CMD id=2 name=NEW_FOLDER ...),
#    which a desktop that had gone would never print.
FLAIR_EXIT_DOS_CANCEL_SPEC := $(FLAIR_EXIT_DOS_PREFIX),kesc,kctrl-n

# 3. FLAIR_EXIT_DOS_SPEC -- Exit: the desktop goes, the prompt comes.
#    D. -> the Exit button (464,168): sum (+294,+5): m100:2, m100:2, m94:1 ;
#       l1,l0 -> FINDER-POWER name=EXITDOS views=0 ; DESKTOP-DB-SAVE ;
#       FLAIR-EXIT-TO-DOS ; COMMAND-TRANSIENT-OK ; SHELL-READY ; A:\>
#    E. WSHELL-READY ; DIR <Enter> -> COMMAND.COM lists drive A: (the
#       flagship volume: " Volume in drive A is INITECH", README.TXT, APPS,
#       the " bytes free" footer).
FLAIR_EXIT_DOS_SPEC := $(FLAIR_EXIT_DOS_PREFIX),m100:2,m100:2,m94:1,l1,l0,WSHELL-READY,kd,ki,kr,kret

# 4. FLAIR_TPS_JOURNEY_SPEC -- the north-star joke, end to end: exit the
#    desktop, CD APPS, TPSREP TPS.DBF, and the whole operating procedure,
#    keyed EXACTLY as test-tps-report leg A keys it (harness/diff/tps/
#    keys_a.txt: 6 presses refused, 8 refused, 7 accepted; the report number
#    keyed twice wrong then right; the wrong memo then 88-114; X ignored, Y).
#    The trailing W holds the stream (and the clip's last frame) until the
#    generator has written TPSREPT.TXT and said so.
FLAIR_TPS_KEYS_CD  := kc,kd,kspc,ka,kp,kp,ks,kret
FLAIR_TPS_KEYS_CMD := kt,kp,ks,kr,ke,kp,kspc,kt,kp,ks,kdot,kd,kb,kf,kret
FLAIR_TPS_KEYS_A   := kspc,kspc,kspc,kspc,kspc,kspc,kret,kspc,kspc,kspc,kspc,kspc,kspc,kspc,kspc,kret,kspc,kspc,kspc,kspc,kspc,kspc,kspc,kret,k4,k4,k1,k7,kret,k4,k4,k7,k1,kret,k4,k4,k1,k7,kret,k4,k4,k1,k7,kret,k8,k8,kminus,k1,k1,k3,kret,k8,k8,kminus,k1,k1,k4,kret,kx,ky
FLAIR_TPS_JOURNEY_SPEC := $(FLAIR_EXIT_DOS_PREFIX),m100:2,m100:2,m94:1,l1,l0,WSHELL-READY,$(FLAIR_TPS_KEYS_CD),$(FLAIR_TPS_KEYS_CMD),$(FLAIR_TPS_KEYS_A),WTPS REPORT GENERATOR TERMINATED

# 5. FLAIR_TPS_SIX_SPEC -- the ritual mutant (Rule 6): the SAME journey with
#    the accepted seven presses replaced by SIX (the third attempt). TPSREP
#    must refuse it ("FORM NOT ALIGNED (6 PRESSES)") and never print FORM
#    ALIGNED., so no report may appear.
FLAIR_TPS_KEYS_SIX := kspc,kspc,kspc,kspc,kspc,kspc,kret,kspc,kspc,kspc,kspc,kspc,kspc,kspc,kspc,kret,kspc,kspc,kspc,kspc,kspc,kspc,kret,k4,k4,k1,k7,kret,k4,k4,k7,k1,kret,k4,k4,k1,k7,kret,k4,k4,k1,k7,kret,k8,k8,kminus,k1,k1,k3,kret,k8,k8,kminus,k1,k1,k4,kret,kx,ky
FLAIR_TPS_SIX_SPEC := $(FLAIR_EXIT_DOS_PREFIX),m100:2,m100:2,m94:1,l1,l0,WSHELL-READY,$(FLAIR_TPS_KEYS_CD),$(FLAIR_TPS_KEYS_CMD),$(FLAIR_TPS_KEYS_SIX)
