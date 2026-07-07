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
# Target = NOTES's visible sliver = FLAIR_TEN_NOTES_CLICK_X/Y = (460,300)
# (spec/flair_tenants_demo.h; bead initech-4w15: HELLO is now the boot foreground,
# so the O-5 switch activates the BACKGROUND NOTES). (460,300) is inside NOTES
# content [260,560)x[120,340), with x>=360 (right of HELLO's right edge) AND
# y>=260 (below HELLO's bottom edge) -- doubly clear of HELLO, so a click here
# lands on the BACKGROUND tenant.
#   needed: sum(dx) = 460-320 = +140 ; sum(dy) = 240-300 = -60
#   split : m70:-30, m70:-30  (each component |c|<=127, int8-safe) -> (460,300)
#   click : l1, l0            (button down + up at the sliver: the activating click)
# Net: move to (460,300) then click -> the pump FindWindow's the background NOTES,
# raises its group to front, repaints the exposed overlap (updateEvt), fires the
# activate/deactivate pair, swaps band 2's menubar (System-7 <- Photoshop), and
# emits "FLAIR-DISPATCH app=NOTES".
FLAIR_APPSWITCH_SPEC := m70:-30,m70:-30,l1,l0

# ---------------------------------------------------------------------------
# WHAT THE HARNESS MUST CAPTURE (two deterministic runs of the SAME reproducible
# -DFLAIR_LIVE_TENANTS image; no harness change needed -- both use the existing
# qemu.c CLI, which screendumps ONCE per run):
#
#   PRE  (the co-resident scene, BEFORE the click): boot, wait FLAIR-LIVE-READY,
#        screendump immediately -- NO --mouse.  Captures HELLO on top (overlap =
#        HELLO_FILL), NOTES inactive (accent block under HELLO = HELLO_FILL), and
#        the DISTINCT chimera (band 1 System-7 != band 2 Photoshop; Law 4).
#          $(HARNESS_BIN) --disk $(FLAIRLIVE_TENANTS_IMG) --name flair_appswitch_pre \
#            --out $(BUILD) --keys-after FLAIR-LIVE-READY \
#            --screendump --screendump-after "FLAIR-LIVE-READY" --timeout-ms 15000
#
#   POST (after the switch): boot, wait FLAIR-LIVE-READY, inject the locked trace,
#        screendump AFTER the dispatch marker.  Captures NOTES raised+repainted
#        (overlap = NOTES_FILL), active (accent block = ACTIVE_ACCENT), band 2's
#        menubar swapped (Photoshop -> System-7), band 1 UNCHANGED.
#          $(HARNESS_BIN) --disk $(FLAIRLIVE_TENANTS_IMG) --name flair_appswitch_post \
#            --out $(BUILD) --mouse "$(FLAIR_APPSWITCH_SPEC)" \
#            --keys-after FLAIR-LIVE-READY \
#            --screendump --screendump-after "FLAIR-DISPATCH app=NOTES" --timeout-ms 15000
#
# Then grade the differential:
#          $(PPM_FLAIR_APPSWITCH_CHECK_BIN) $(BUILD)/flair_appswitch_pre.ppm \
#                                           $(BUILD)/flair_appswitch_post.ppm
#
# Serial asserts the recipe SHOULD also make (Law 2, like test-flair-drag):
#   1. no triple-fault;
#   2. FLAIR-LIVE-READY (the pump armed);
#   3. FLAIR-DISPATCH app=NOTES (the click dispatched the activating switch);
#   4. ppm_flair_appswitch_check PASS (DISTINCT-CHIMERA band1!=band2 at boot +
#      TIER-A overlap HELLO_FILL->NOTES_FILL + TIER-B accent FILL->ACTIVE_ACCENT +
#      BAR1-STATIC band1 unchanged + MENU-BAND band2 title strip differs).
# The MUTANT gate (Rule 6) reuses the SAME trace against the no-raise / skip-
# activate / menubar-no-swap mutant image(s); the grader MUST go RED (proven
# against the hand-made pre/post pair in Step 2; see ppm_flair_appswitch_check.c).
