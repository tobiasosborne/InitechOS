# WL-0110: sampled button and modal-alert chrome

## Context

Lane `lane-chrome-controls`, based on `00a8e83`. PM supplied
initech-tdnl.99 (M011), initech-tdnl.100 (M012), and L023. No bd commands,
push, full make test, Bochs, or clip recording in this lane.

Authority: PRD 6.3/6.5; look-pass6 REPORT M011/M012 and TRIAGE batch 2;
astra-full-drive REPORT L022/L023; system7-decomp specs/sys8/controls.md
5.1, 5.2, 6, 8; historical goldens/captures/s8_alert_modal.png. The sibling
corpus was read only. System 7 dialog/alert specs were read as heritage.

## What changed

- M011: restrict opaque Chicago label painting to the inner button face.
  PNG x365..418 at y160 is 54 white pixels; at y175 it is 53 C0C0C0 and
  one E7E7E7. Both complete rows now match with the Cancel label present.
- M012: remove the outer gray fringe and restore the missing lower-right
  dark pixel. All four patches (x363..366/418..421,
  y158..161/174..177) match 64/64. The complete measured four-patch
  transcription and shade legend are commented in control.c.
- L023: replace black slab paint with the measured outline and two bevel
  rows. At x300, y87..90 is 000000/FFB3B3/FFFFFF/E7E7E7;
  y187..190 is E7E7E7/B3B3B3/FF8787/000000. The corresponding sides
  and corner joins match too. The heritage layout inset remains unchanged.
- Host and QEMU oracles decode the historical PNG at runtime, independently
  of renderer constants. Host checks cover both 8-bit and 32-bit surfaces.
  The frame check covers the entire seven-pixel perimeter, including face
  pixels inward of the bevel, to reject a residual black slab.
- Add test-flair-alert-chrome and its three booted mutants using the existing
  locked dirty-123 quit trace. Strengthen the shared Trash/tenant alert grader.

## Why

The Chicago cell erase crossed the bevel even though its ink did not. The
corner transcription carried thirteen pixels absent from the actual capture.
The old dialog frame used heritage layout thickness as solid-black art.

## Frictions

A direct-color-only tint with neutral indexed fallbacks failed QEMU on exactly
946 tint pixels. A solo rerun reproduced that result. The desktop always
composes into an indexed offscreen, even on a direct-color display.

Deliberate Rule-8 data change for L023: append sampled color-canon slots 9/10
for FFB3B3/FF8787 and regenerate the header with the C generator. These slot
numbers are product allocations, not claimed Mac palette indices. They replace
two unnamed gray-ramp values; slots 0..8 and all named gray-ramp rows remain
unchanged. The unnamed ramp now begins at 11. No fallback remapping or tolerance
was added to the capture oracle.

The sandbox mounts the primary clone's .git read only: staging/commit fails
creating .git/index.lock. The finished change is committed on branch
lane-chrome-controls in build/lane-delivery.git and exported as
build/lane-chrome-controls.bundle (base prerequisite 00a8e83). The primary
clone's branch ref could not be advanced. PM must import the bundle; no push
was attempted. build/lane-report.md has the exact commit and import command.

## Acceptance

Host: test-control, test-dialog, test-chrome-fidelity and their full mutant
sets green. Additional canon generation/value/mutant, mechanism-policy,
colorblind, CLUT, shell/mutant, and frozen-skin gates green.

QEMU with INITECH_QMP_STDIO=1 and make -j2: test-flair-alert-chrome green
(108/108 row pixels, 64/64 corners, 6496/6496 perimeter pixels).
CTRL_MUT_LABEL_OPAQUE, CTRL_MUT_BTN_FRINGE, DIALOG_MUT_ALERT_SLAB independently
fail their named host and booted pixel checks. Separate mutant builds leave
production sources restored. Existing nine-leg test-flair-i123-quitguard and
three-leg test-flair-empty-trash also green. ASCII and git diff --check pass.

PM acceptance remains: Bochs and Rule-14 clips are unavailable in this lane.
L022 / tdnl.87 caution art remains explicitly out of scope. The known label
font difference is unchanged. The capture's drop shadow outside the structure
(x507, y89..191; row y191) remains absent; e.g. the quit-alert guest has navy at
(507,120) and white at (300,191), where controls.md 6 specifies black. This
outside-frame residual was reported, not repaired.

## Pointers

- os/flair/control.c; os/flair/dialog.c; os/flair/flair_look.c/.h
- spec/assets/color_canon.json/.h; tools/color_canon_extract.c
- harness/proptest/test_control.c; harness/proptest/test_dialog.c
- harness/render/alert_capture.h; tools/ppm_flair_trash_check.c; Makefile
- build/lane-report.md: exact commands, final gate lines, evidence paths.
