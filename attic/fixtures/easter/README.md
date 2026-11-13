# Jump to Conclusions: authored regression stills

Source: operator brief initech-2ter part (a), PRD 6.3/6.4 and Appendix B,
and the authored `spec/easter/jump_to_conclusions.json` contract. These are
SELF-MINTED FROM THE QEMU GATE, not independent historical goldens. The
operator explicitly authorized this mint. Historical Platinum chrome and
Chicago retain their existing independent subsystem oracles.

Before minting jtc_mat.png, the agent viewed the gate capture and recorded:
all ten pads and labels are readable; CONCLUSIONS is above the grid; the
Platinum document has a close box and no grow box; the last conclusion
wraps within its own pad. The cursor is outside this crop.

Before minting jtc_alert.png, the agent viewed the gate capture and recorded:
the Platinum alert overlays the mat, reads "That Would Be Great.", has one
ringed OK button, and leaves the cursor at pad 8's centre (218,372) below it.

The gate captures the mat and alert at the ends of separate LOCKED traces
(`spec/flair_egg_jtc_traces.mk`). Ordinary harness screenshot markers are
post-injection markers, so using the complete clip trace for an alert still
would incorrectly capture the already dismissed alert.

The C grader compares every RGB byte of the fixed (100,70) 450x370 crop:
166500 pixels, zero tolerance, no renderer headers. `--mint` is an explicit
manual operation and never a prerequisite of a test target. Rule 6 uses
EGG_MUT_PAD_MISSING for the host table and EGG_MUT_TRIGGER_OFF_BY_ONE for
the emulator trigger. No real implementation file is edited for a mutant.
