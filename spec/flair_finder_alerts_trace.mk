# LOCKED injected traces: tdnl.65/.88. PRD 6.3; s8_alert_modal.png.
# Use the existing disk launch/Trash routes. Initial cursor (320,240).
# APP_LAUNCH_DOUBLE raises APPS over TenantFix, then activates TenantFix
# through its exposed y310 content. That last click must not quit it.
FLAIR_ACTIVATE_ONLY_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m-48:20,l1,l0,l1,l0,m4:0,m-4:0,l1,l0,l1,l0,m65:99,m0:89,l1,l0
# Empty alert ends parked at (620,460). Press OK (464,168), drag outside
# to (564,168), release. Alert must survive. Chrome compares to the PNG.
FLAIR_DIALOG_TRACK_SPEC := $(FLAIR_EMPTY_ALERT_SPEC),m-78:-98,m-78:-98,m0:-96,l1,m100:0,l0
FLAIR_DIALOG_HELD_SPEC := $(FLAIR_EMPTY_ALERT_SPEC),m-78:-98,m-78:-98,m0:-96,l1
# Root fixture: README.TXT, APPS, DOS, AUTOEXEC.BAT, F00..F68 (73 items). Type the last
# name, Open, Close, Select All, Open. Right twice first selects APPS;
# Open/Close proves arrow selection reaches the real folder before typing. The final command must exhaust the
# actual Finder records arena with a visible alert after opening >4 folders.
FLAIR_FINDER_LIMITS_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,kright,kright,kctrl-o,kctrl-w,kf,k6,k8,kctrl-o,kctrl-w,kctrl-a,kctrl-o,m10:98,m10:98,m0:100,m0:100
FLAIR_FINDER_DEMO_SPEC := m-100:-100,m-100:-100,m-110:-10,l1,m0:22,l0
FLAIR_FINDER_REFUSED_SPEC := $(FLAIR_FILEOPS_REFUSE_SPEC)
FLAIR_FINDER_REPLACE_SPEC := $(FLAIR_FILEOPS_INTO_SPEC)
FLAIR_FINDER_REPLACE_OK_SPEC := $(FLAIR_FINDER_REPLACE_SPEC),kret
FLAIR_FINDER_RESTORE_SPEC := $(FLAIR_TRASHWIN_OPEN_SPEC),kr,ke,ka,kd,km,k0,kctrl-y
FLAIR_FINDER_DISKFULL_SPEC := m100:100,m100:100,m100:20
