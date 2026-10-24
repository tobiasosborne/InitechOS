<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->
# WL-0181 -- Retirement of the round-3 arrival cap

Issuing Body: Initech Systems Corporation, Platform Engineering
Date: 2026-10-24. Authority: operator ruling, bead initech-oiygj.

## Context

The round-3 lane's reclaim target became a permanent all-kernel cap:
182732 B arrival binding minus 10240 B reclaim = 172492 B. WL-0127 and
WL-0128 record that round's measurements and acceptance. The operator ruled
on 2026-10-24 that this arrival criterion must retire; it is not a physical
limit and cannot govern every subsequent feature. The paperwork is amended.

## What changed

Removed test-kernel-size-r3, test-kernel-size-r3-mutant, their grader,
KERNEL_R3_LOADED_MAX, the r3 size_pad_one recipe and two TEST_UNIT_GATES
entries from harness/proptest/kernel_size.mk. The round-3 comment records
the ruling and the live size gates. No other prerequisites referenced them.

Kept test-kernel-size-r3-emu and test-kernel-size-r3-emu-mutant under their
existing names, with identical recipes and TEST_EMU_GATES registration.
These grade desktop pixels, text traces and DOS/FCB/FAT behaviour.
Updated the round-8 policy, HANDOFF Sec 5 and later worklog references;
preserved WL-0127/WL-0128 and historical verdicts.

## Why

test-kernel-headroom protects every kernel with the 4096 B floor against
the 196608 B bounce window. test-kernel-size-r4 protects the flagship's
16384 B headroom. The PM's 2026-10-17 comment in kernel_size_r4.mk explains
the distinction: a headroom floor allows later features to spend bytes;
a fixed arrival cap would fail every merged feature by construction.
The forward capacity programme is overlays epic initech-x8gsi.

## Frictions

The local bead lookup returned no issue matching initech-oiygj; the ruling
reproduced in LANE-BRIEF.md supplied the authority. No bead state changed.
Make reports existing duplicate mouse-object recipe warnings. The stdin
count recipe lost its target here; --eval supplied an equivalent count
without stashing or changing the checkout.

## Acceptance

The make database has zero retired cap targets and both emulator targets.
TEST_UNIT_GATES: 798 before, 796 after, delta -2.
Headroom (+mutant), r4 (+mutant), repro, GC (+mutant/roots) and spec pass.
All 309 kernels keep the floor; flagship r4 margin remains 16 B.
Targeted gate commands and verbatim verdicts are recorded in CODEX-OUT.md.
No full make test, Bochs leg, commit or push was run, per the lane brief.

## Pointers

- LANE-BRIEF.md: operator ruling 2026-10-24, initech-oiygj.
- docs/worklog/WL-0127-kernel-size-round3.md; WL-0128-kernel-size-round3-part2.md.
- harness/proptest/kernel_size.mk; kernel_size_r4.mk (PM 2026-10-17 comment).
- docs/design/kernel-size-policy-round8.md; docs/HANDOFF.md Sec 5.
- Epic initech-x8gsi: overlays programme; CODEX-OUT.md: lane evidence.
