<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->
# WL-0182 -- Retirement of the SAMIR arrival-reclaim clause

Issuing Body: Initech Systems Corporation, Platform Engineering
Date: 2026-10-24. Authority: operator ruling, bead initech-9swot.

## Context

LANE-BRIEF.md retires the frozen arrival-reclaim acceptance criterion that
outlived its size lane. WL-0181 records the same ruling for the kernel r3
cap (initech-oiygj). This is a deliberate oracle-policy act (Rule 8).

## What changed

Removed arrival/recovery constants and the reclaim comparison from the size
oracle. Kept the 4096 B headroom floor and RED SAMIR_SIZE: diagnostic prefix.
Replaced the retired boundary.COM leg with a physical-floor pad test:
SAMIR_SIZE_PAD_4096 leaves 4095 B and must go RED; floor.COM leaves exactly
4096 B and must stay GREEN. Both retain the program and fit the link budget.
The mutant reads that budget from the oracle rather than duplicating it.
Updated live-policy prose; historical measurements and verdicts remain.

## Why

samir.ld requires loaded end + 0x10000 BSS + 0x4f70 PAL allocation + 0x20
MCB/alignment <= 0x6f000. With PROGRAM_IMAGE=0x40100, the loaded-byte budget
is 106352 B; the retained floor therefore caps the image at 102256 B.
samir.ld and the locked memory map are unchanged.

## Frictions

WL-0181 is absent in this checkout; read the supplied sibling copy at
../initech-os-cx-r3/docs/worklog/WL-0181-retire-r3-cap.md. The lane brief
supplies the ruling and issue context; no beads state was changed.

## Acceptance

Size, floor mutant, two-build repro, QEMU emu (+mutant) and test-samir pass.
SAMIR.COM is 97705 B: 8647 B headroom, 4551 B above the retained floor.
Targeted gate results and full verbatim verdicts are in CODEX-OUT.md.
Repro and emulator recipes are unchanged; no other gate or SAMIR source
was edited. No full make test, Bochs, commit or push, per the lane brief.

## Pointers

- LANE-BRIEF.md: operator ruling 2026-10-24, initech-9swot.
- WL-0181-retire-r3-cap.md: kernel precedent, initech-oiygj.
- WL-0171-samir-size-round-2.md; samir-size-round1.md: historical rounds.
- os/samir/boot/samir.ld; spec/memory_map.h; CODEX-OUT.md: acceptance evidence.
