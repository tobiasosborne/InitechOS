# spec/flair_input_traces.mk -- the LOCKED input traces for the input-handling
# emulator gates of the second FLAIR desktop audit (beads initech-tdnl.58 /
# .59 / .60; docs/audits/2026-10-04-flair-gui-codex-pass2/REPORT.md findings
# G11, G01, G02). Locked spec-data (CLAUDE.md Rule 8, Rule 11): deterministic,
# versioned, never silently edited to make a test pass.
#
# Conventions are spec/flair_disk_windows_traces.mk's verbatim (QMP relative
# moves from the screen centre (320,240), y DOWN-positive, every component
# int8-safe |c| <= 100; "l1"/"l0" left button; "k<chord>" a keystroke). Two
# tokens are new with this file (harness/emu/qemu.c qmp_inject_mouse):
#   "K<key>:<0|1>" one explicit key transition (press-and-hold / release),
#                  the audit's `keyheld ctrl true` / `keyheld ctrl false`;
#   "w<ms>"        hold every input state for <ms> ms (the audit's `pause`).
# Geometry of the root disk window (opened by the volume double-click at
# (600,64), unmoved default frame (20,60)..(380,280)), from
# spec/flair_file_ops_traces.mk: README.TXT centre (55,102), APPS centre
# (123,102).
#
# ===========================================================================
# 1. FLAIR_MODREL_SPEC -- G11, modifiers released stay released (tdnl.58).
# ===========================================================================
#   open the root window            m100:-88,m100:-88,m80:0,l1,l0,l1,l0
#   LEFT Ctrl press, release, n     Kctrl:1,Kctrl:0,kn
#        (audit: plain N created NEWFOLD -- must be NO FINDER-CMD)
#   RIGHT Ctrl press, release, w    Kctrl_r:1,Kctrl_r:0,kw
#        (E0 1D / E0 9D; audit: plain W closed the root -- must stay open)
#   Shift press, release            Kshift:1,Kshift:0
#   click README.TXT, then APPS     m-100:10 x3,m-100:8,m-77:0 -> (123,102);
#                                   m-68:0 -> (55,102) l1,l0 ; m68:0 l1,l0
#        (audit: count=2, Shift-extend latched -- must be count=1)
#   POSITIVE CONTROL: Ctrl-N chord  kctrl-n -> the ONE FINDER-CMD,
#        FINDER-CMD id=2 name=NEW_FOLDER src=key sel=1 (APPS selected), so the
#        window was open and the chord path still works.
FLAIR_MODREL_SPEC := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,Kctrl:1,Kctrl:0,kn,Kctrl_r:1,Kctrl_r:0,kw,Kshift:1,Kshift:0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,m-68:0,l1,l0,m68:0,l1,l0,kctrl-n
