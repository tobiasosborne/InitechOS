# spec/flair_appswitch_trace.mk -- the LOCKED O-5 app-switch input trace + the
# two-capture harness contract for `test-flair-appswitch` (ADR-0013 FLAIR App
# Contract; Wave-4 gate O-5).  Locked spec-data (CLAUDE.md Rule 8, Rule 11):
# deterministic, versioned, never silently edited to make a test pass.
#
# This is a Makefile FRAGMENT in the SAME shape FLAIR_DRAG_SPEC / FLAIR_MENU_SPEC
# take (a comma-separated QMP relative-mouse spec).  The O-5 recipe is wired by
# the orchestrator (Wave-4 Step 5); this file holds the locked trace + documents
# exactly what the harness must capture so that wiring is mechanical.  To use:
#   include spec/flair_appswitch_trace.mk
# (or copy the FLAIR_APPSWITCH_SPEC line into the Makefile verbatim).
#
# ---------------------------------------------------------------------------
# THE TRACE.  QEMU rel->cursor is 1:1 with x-positive=right and y-INVERTED
# (rel +y -> cursor UP), exactly as the FLAIR_DRAG_SPEC comment records:
#   cursor_x = 320 + sum(dx)   cursor_y = 240 - sum(dy)   (screen center 320,240)
# PS/2 deltas are int8 (a >127 component sets the packet overflow bits and is
# DROPPED), so the move is SPLIT into <=int8 hops (cf. FLAIR_MENU_SPEC).
#
# Target = HELLO's visible sliver = FLAIR_TEN_HELLO_CLICK_X/Y = (150,150)
# (spec/flair_tenants_demo.h): inside HELLO content [60,360)x[60,260), and x<260
# so LEFT of the NOTES overlap -- a click here lands on the BACKGROUND tenant.
#   needed: sum(dx) = 150-320 = -170 ; sum(dy) = 240-150 = +90
#   split : m-85:45, m-85:45  (each component |c|<=127, int8-safe) -> (150,150)
#   click : l1, l0            (button down + up at the sliver: the activating click)
# Net: move to (150,150) then click -> the pump FindWindow's the background HELLO,
# raises its group to front, repaints the exposed overlap (updateEvt), fires the
# activate/deactivate pair, swaps the menubar, and emits "FLAIR-DISPATCH app=HELLO".
FLAIR_APPSWITCH_SPEC := m-85:45,m-85:45,l1,l0

# ---------------------------------------------------------------------------
# WHAT THE HARNESS MUST CAPTURE (two deterministic runs of the SAME reproducible
# -DFLAIR_LIVE_TENANTS image; no harness change needed -- both use the existing
# qemu.c CLI, which screendumps ONCE per run):
#
#   PRE  (the co-resident scene, BEFORE the click): boot, wait FLAIR-LIVE-READY,
#        screendump immediately -- NO --mouse.  Captures NOTES on top (overlap =
#        NOTES_FILL) and HELLO inactive (accent block = HELLO_FILL).
#          $(HARNESS_BIN) --disk $(FLAIRLIVE_TENANTS_IMG) --name flair_appswitch_pre \
#            --out $(BUILD) --keys-after FLAIR-LIVE-READY \
#            --screendump --screendump-after "FLAIR-LIVE-READY" --timeout-ms 15000
#
#   POST (after the switch): boot, wait FLAIR-LIVE-READY, inject the locked trace,
#        screendump AFTER the dispatch marker.  Captures HELLO raised+repainted
#        (overlap = HELLO_FILL), active (accent block = ACTIVE_ACCENT), menubar
#        swapped.
#          $(HARNESS_BIN) --disk $(FLAIRLIVE_TENANTS_IMG) --name flair_appswitch_post \
#            --out $(BUILD) --mouse "$(FLAIR_APPSWITCH_SPEC)" \
#            --keys-after FLAIR-LIVE-READY \
#            --screendump --screendump-after "FLAIR-DISPATCH app=HELLO" --timeout-ms 15000
#
# Then grade the differential:
#          $(PPM_FLAIR_APPSWITCH_CHECK_BIN) $(BUILD)/flair_appswitch_pre.ppm \
#                                           $(BUILD)/flair_appswitch_post.ppm
#
# Serial asserts the recipe SHOULD also make (Law 2, like test-flair-drag):
#   1. no triple-fault;
#   2. FLAIR-LIVE-READY (the pump armed);
#   3. FLAIR-DISPATCH app=HELLO (the click dispatched the activating switch);
#   4. ppm_flair_appswitch_check PASS (TIER-A overlap NOTES_FILL->HELLO_FILL +
#      TIER-B accent FILL->ACTIVE_ACCENT + MENU-BAND title strip differs).
# The MUTANT gate (Rule 6) reuses the SAME trace against the no-raise / skip-
# activate / menubar-no-swap mutant image(s); the grader MUST go RED (proven
# against the hand-made pre/post pair in Step 2; see ppm_flair_appswitch_check.c).
