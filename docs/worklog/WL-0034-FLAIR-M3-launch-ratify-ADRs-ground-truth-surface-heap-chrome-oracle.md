<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# WL-0034 -- FLAIR M3 LAUNCH: ADRs ratified, GUI ground truth, surface + heap + the chrome oracle (the GUI built on REAL measurements)

**Milestone:** M3 (FLAIR Toolbox) launch -- governance + foundations + the crown-jewel chrome oracle.
**Governing decisions:** ADR-0004 (FLAIR) + ADR-0005 (ATKINSON) RATIFIED (ADR-by-committee); DEC-03 (FLAIR heap) / DEC-04 (defer 86Box).
**Date:** 2026-06-19/20. **Branch:** command-com-default (pushed to origin).

## Context

Operator opened the FLAIR GUI as the next consequential work and directed an orchestrated build: the GUI is the **crown jewel of corporate design** -- built on REAL measurements + reverse-engineering / source-code-as-spec + golden generation from real era software; 100% usable; "obviously a bland ripoff of System 7 + Windows" (the deadpan chimera). See `bd memories` operator-directive-2026-06-19-flair-is-the.

## What changed (4 commits)

**Governance (7118d6f, 6a9917d).** ADR-by-committee (4 reviewers + opus chair, no gridlock) RATIFIED ADR-0004 (FLAIR, amendments AM-1..AM-9) + ADR-0005 (ATKINSON as-is). **DEC-03**: FLAIR heap = dedicated extended-memory region [0x100000,0x500000) (4 MiB), fixed+spec-locked, stage2 INT15h probe -> boot_info.ext_mem_kb, panic-below-min (NOT the per-program MCB arena). **DEC-04**: defer 86Box pixel-capture (q0gy blocks M4); QEMU+Bochs via host-model-per-mode (frozen, AM-6). A 5-lane research workflow produced `docs/research/gui-ground-truth.md` (System 7.0/7.1 target; Apple WDEF assembly source + Inside Macintosh as authoritative native-pixel source; Win 3.1 flat-2D; the 11-element chimera map; the minting toolchain). chrome_metrics.json v0->v2 + chimera_element_map.json locked from FIRST-HAND-FETCHED Apple WDEF source.

**Wave 1 (033e2fd).** The ONE surface module `os/flair/surface.{c,h}` (k8o5.6, ADR-0004 D-2) lifted from console.c; console.c is now a client (public API byte-identical); `test-fbagree` mutation-proven; surface.o wired into all 18 kernel object-lists. FLAIR-heap spec-lock + stage2 INT15h E820/E801/88h probe + kmain panic-below-min + FO-G oracle (k8o5.5/DEC-03). Two real E820 bugs fixed by live boot.

**Wave 2 (5580ef7).** FLAIR heap ALLOCATOR `os/flair/heap.{c,h}` (k8o5.5): bump + per-class LIFO free-list, 16-byte align, fail-loud, dual-compile, deterministic; `test-flair-heap` + 2 mutants. GrafPort/imaging spec `spec/grafport.h`+`imaging.h` (k8o5.3): verbatim Inside Mac, 47 static_asserts. Canon assets `spec/assets/cursors.h` (HOURGLASS canon -- NOT the watch) + `menu_canon.h` (frozen Photoshop string) (zaqj). New `test-flair-headers` compile-contract gate.

**Wave 3 (91fadda) -- CROWN JEWEL.** `harness/render/` host-render skeleton (k8o5.7, parameterized by boot_info geometry, AM-1, 32bpp+8bpp -> deterministic PPM). `os/flair/chrome.{c,h}` (freestanding) draws a real System-7 window (pinstripe title bar 19px, close/zoom boxes, 1px double-line frame, 16px scrollbar) via GrafPort+surface+region clipping. **`test-chrome`** (k8o5.8, FO-2/AM-3): chrome_metrics.h==json consistency + STRUCTURAL render-vs-metrics (30 checks) + 3 named mutants all RED. Visually audited: `build/chrome_window.ppm` IS a Mac window. Event/window spec `spec/event_model.h`+`window_record.h`+`ssim_params.h` (k8o5.4): verbatim IM EventRecord/WindowRecord/part-codes; SSIM guide-not-gate.

## Why

Law 1 + the operator directive: the GUI's geometry now derives from Apple's own WDEF source (real source-as-spec), and `test-chrome` makes "looks right" a mechanical gate. The region engine's internal format still has no golden (QuickDraw body unpublished, ADR-0005); everything ON SCREEN now has one.

## Frictions / lessons

- **Shared-tree concurrent agents + the Makefile.** Only ONE agent edits the Makefile per wave; the others deliver source + self-verify by direct gcc (to /tmp), and the orchestrator wires gates + re-grades. Selective `git add` lets a disjoint sub-wave (chrome_metrics+chimera) commit while other agents' WIP stays unstaged.
- **The integration seam.** console.c -> surface_* meant surface.o had to join ALL 18 kernel object-lists; a targeted sed on `:=` list lines (excluding the build-rule line, which has no `:=`) wired it cleanly.
- Always re-grade independently (`make clean && make test-unit` + emu) -- never trust the agent's report (Law 2 / Rule 4). Look at the screendump (Law 4): the PPM confirmed the window before trusting green.

## Acceptance

- `make clean && make test-unit`: **194 host gates ALL GREEN** (was 184).
- `make test-emu`: **35 QEMU gates** + `make test-boot-bochs` PASS; triple_fault=0 both BIOSes; ext_mem probe QEMU 129920 / Bochs 31744 KiB (Wave 1 boot-path re-grade).
- All 4 commits pushed to origin/command-com-default. FLAIR epic 15/33 (45%).

## Pointers / next

- Remaining FLAIR (bd ready): i50 (region-clipped blitter), kg5 (Chicago/Geneva text), k8o5.10 (canon oracle), ch81 (palette desktop_bg -> SEAFOAM -- the render bg is currently v0 gray), then the interactive arc 26d (PS/2 mouse) -> 8b7 (event loop) -> 9qf (Window Manager) -> **87a (M3 drag gate, AM-8)** -> M4 Managers (n3e/8h9/qcc/k8o5.12) -> desktop shell (859).
- OPERATOR-gated (non-blocking): pvo4 (Mac 68K ROM for Basilisk II rendered goldens), 77wz (Win 3.1 via dosbox-x, needs `apt install xvfb xdotool`), u9gf (Bochs RFB capture), q0gy (86Box, blocks M4 sign-off). The Tier-2 pixel-perfect SCREENSHOT goldens wait on these; Tier-1 (WDEF-source metrics) is live.
- The chrome desktop is HOST-rendered today (the oracle). Wiring FLAIR INTO the kernel (boot -> draggable desktop in the emulator) is the M3/M4 in-OS integration.
