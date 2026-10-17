# spec/flair_cmd_traces.mk -- the LOCKED input traces for FLAIR as a command
# (bead initech-tdnl.106; ADR-0003 Amendment DEC-19). Locked spec-data
# (CLAUDE.md Rule 8, Rule 11): deterministic, versioned, never silently
# edited to make a test pass.
#
# Conventions are spec/flair_exit_dos_traces.mk's (relative QMP moves from
# the screen centre, "l1"/"l0" the left button, "k<chord>" a keystroke in the
# mouse stream, "W<marker>" hold until the guest prints <marker>) plus
# "W<n>:<marker>" -- hold until <marker> has appeared <n> times
# (harness/emu/qemu.c): every desktop prints the same FLAIR-LIVE-READY and
# every return to the prompt the same SHELL-READY, so the n-th one is the
# guest's own ordering signal, never a guessed delay. Every desktop starts
# with the pointer at the centre (flair_event_init), so the SAME exit trace
# leaves every one of them.
#
# THE PROMPT: COMMAND.COM prints "A:\>" ($P$G, ADR-0003 DEC-11) on the
# console, which the INT 21h CON sink mirrors to serial; the boot prompt is
# its first occurrence. The streams start at COMMAND-TRANSIENT-OK (SYSINIT's
# transient read), so the first token waits for that prompt.
FLAIR_CMD_PROMPT := WA:\>
# FLAIR <Enter> at the prompt (COMMAND.COM finds \DOS\FLAIR.COM on PATH).
FLAIR_CMD_TYPE   := kf,kl,ka,ki,kr,kret
# Special > Exit to InitechDOS, then Exit in the alert: the exit-to-DOS gate's
# own prefix + Exit click (spec/flair_exit_dos_traces.mk FLAIR_EXIT_DOS_SPEC
# minus its DIR), verbatim.
FLAIR_CMD_EXIT   := $(FLAIR_EXIT_DOS_PREFIX),m100:2,m100:2,m94:1,l1,l0

# 1. FLAIR_FROM_DOS_SPEC -- the round trip, twice, then a third desktop:
#    A:\> ; FLAIR -> desktop 1 ; Exit -> SHELL-READY 1 ; FLAIR -> desktop 2 ;
#    Exit -> SHELL-READY 2 ; FLAIR -> desktop 3 (left to run).
FLAIR_FROM_DOS_SPEC := $(FLAIR_CMD_PROMPT),$(FLAIR_CMD_TYPE),WFLAIR-LIVE-READY,$(FLAIR_CMD_EXIT),WSHELL-READY,$(FLAIR_CMD_TYPE),W2:FLAIR-LIVE-READY,$(FLAIR_CMD_EXIT),W2:SHELL-READY,$(FLAIR_CMD_TYPE)

# 2. FLAIR_FROM_DOS_SWITCH_SPEC -- the same, then on desktop 3 the appswitch
#    gate's own click on the NOTES sliver (spec/flair_appswitch_trace.mk),
#    so test-flair-appswitch's grader judges the third desktop.
FLAIR_FROM_DOS_SWITCH_SPEC := $(FLAIR_FROM_DOS_SPEC),W3:FLAIR-LIVE-READY,$(FLAIR_APPSWITCH_SPEC)

# 3. FLAIR_TWICE_SPEC -- a second FLAIR while the desktop runs: the system
#    hotkey '\' runs the text tenant (kmain flair_launch_text_tenant), which
#    on this gate's volume IS FLAIR.COM; it must be refused, the desktop must
#    resume, and then Exit and FLAIR must still work (the refusal changed
#    nothing).
FLAIR_TWICE_SPEC := kbsl,WFLAIR-RESUME,$(FLAIR_CMD_EXIT),WSHELL-READY,$(FLAIR_CMD_TYPE)

# 4. FLAIR_CMD_RELOAD_SPEC -- on the shipped volume (AUTOEXEC.BAT ends with
#    FLAIR): Exit from desktop 1, then FLAIR again at the prompt.
FLAIR_CMD_RELOAD_SPEC := $(FLAIR_CMD_EXIT),WSHELL-READY,$(FLAIR_CMD_TYPE)
