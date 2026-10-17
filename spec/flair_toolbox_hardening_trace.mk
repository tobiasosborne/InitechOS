# Toolbox hardening valid-input regression and Rule-14 clip.
# Reuse the locked open/shrink/scroll history: no second injection mechanism.
# Ref: flair_scroll_look_traces.mk, sys8/scrollbars.md, ADR-0006 E-D5.
# Recursive expansion: the underlying locked trace is included later by Make.
FLAIR_TOOLBOX_HARDENING_SPEC = $(FLAIR_SCROLL_LOOK_CLICK_SPEC)
