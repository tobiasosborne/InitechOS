<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# WL-0036 -- FLAIR M4: Menu + Control + Dialog Managers + the desktop shell that REPRODUCES THE FRAME

**Milestone:** M4 (FLAIR Toolbox Managers) -- the four core Managers + the Layer-5 desktop shell capstone.
**Date:** 2026-06-20. **Branch:** command-com-default (pushed). **Continues:** WL-0035.

## What changed (4 commits: 8636dcf, 8fb1620, 6672368, 9f8167a)

Continuing the orchestrated waves (host-tested freestanding artifact; orchestrator owns Makefile + independent re-grade + commit/push per wave). The four core M4 Managers + the desktop shell, all built on the WL-0034/0035 foundation (surface/heap/region/blitter/chrome/text/window/event/desktop):

- **`n3e` MENU MANAGER** `os/flair/menu.{c,h}`: MenuInfo/MenuBar, 20px bar with PROPORTIONAL item layout (Chicago `text_measure`), pull-down panels, `MenuSelect` (deterministic point-sequence tracking -> menuID<<16|item), `MenuKey`, divider/disabled non-selectable. The InitechPaint bar is built from the FROZEN `menu_canon.h` (Law 4). `test-menu` 41 checks + FIXED_WIDTH/SELECT_DISABLED mutants.
- **`8h9` CONTROL MANAGER** `os/flair/control.{c,h}`: push/check/radio buttons, the 16px scrollbar (proportional thumb, value<->thumb-Y INVERTIBLE), the FILE COPY determinate progress bar; `TestControl`/`TrackControl` part-codes + value clamping. `test-control` 143 checks + THUMB_OFF/NO_CLAMP mutants.
- **`qcc` DIALOG MANAGER** `os/flair/dialog.{c,h}`: DialogRecord (embeds WindowRecord@0) + DITL items, `ModalDialog` (cooperative WaitNextEvent loop; Return/Escape -> default/cancel; statText/disabled never returned), dBoxProc 7px frame, and `FileCopyDialog` = the canon "Saving tables to disk..." modal (`FLAIR_CANON_FILECOPY_MSG`, Law 4 byte-exact-enforced) + progress bar. `test-dialog` 73 checks + BORDER/HIT_STATIC/FILECOPY_MSG mutants. (Orchestrator fixed 2 Rule-12 non-ASCII the subagent left + validated its Makefile wiring.)
- **`k8o5.12` + `859` DESKTOP SHELL (the M4 CAPSTONE)** `os/flair/shell.{c,h}`: composes the Office Space frame -- seafoam desktop + the chimera **two stacked menu bars** (System-7 Apple/File/Edit/View/Special AND the canon Photoshop bar via an offset sub-bitmap view) + 2 z-ordered System-7 windows + the FILE COPY modal on top -- via EVERY Manager + the ONE surface module (D-2). `test-flair-shell` 38 structural checks (seafoam, both bars byte-exact incl. Layer/View chimera tells, window chrome vs chrome_metrics, FILE COPY centered + byte-exact, modal on top, determinism) + ONE_MENUBAR/NO_MODAL/MODAL_BEHIND mutants. Writes `build/desktop_scene.ppm`.

## Frictions / lessons

- **Gate-name COLLISION caught at integration (Rule 4).** The shell agent's `test-shell` collided with the EXISTING `test-shell` (the COMMAND.COM boot emu gate). `make test-shell` silently ran the OLD emu test; my recipe was overridden. RENAMED to `test-flair-shell`. Always grep the Makefile for an existing target before adding a gate; the full clean re-grade + reading the gate's OUTPUT (not just exit 0) surfaced it.
- **A subagent misreported (Rule 4).** The dialog agent claimed ASCII-clean + "Makefile already wired" -- both false (2 non-ASCII section-signs; it had edited the Makefile against instructions). Independent verification caught + fixed both. NEVER trust the agent's self-report; re-grade + grep yourself.
- **VISUAL audit refined a fidelity note (Law 4).** test-flair-shell is green and verifies the FILE COPY modal structurally; in the rendered PPM the modal IS visible and legible -- its 7px black dBoxProc border makes "Saving tables to disk..." + the progress bar stand out against the windows. The gap is narrower than first reported: the window/dialog INTERIOR "white" is the gray 0x7F7F86 (not crisp white), so the modal reads gray-on-gray-ish rather than the frame's white box. Bumped bead `bmih` (palette-unify + content-white) to P1 -- a polish, not a missing render. (First pass mis-called it "invisible"; re-rendering build/desktop_scene.ppm at the gallery scale showed the modal clearly -- always look at the screendump, Law 4.)

## Acceptance

- `make clean && make test-unit`: **216 host gates ALL GREEN** (WL-0035 left 208; +8 = menu/control/dialog/shell x2 each).
- Boot path + emu UNTOUCHED (no FLAIR module linked into the kernel image yet; the 35 QEMU + Bochs boot gates from WL-0034 stand). FLAIR epic 27/33 (81%).
- 4 commits pushed to origin/command-com-default.

## Pointers / next

- **P1 fidelity (bmih, now visible in the capstone):** content-body WHITE -- fix CIDX_WHITE/INITECH_WINDOW_WHITE_RGB across the lockstep (chrome.c + render.c + palette.json canonical -> palette.h single source) so windows + the FILE COPY modal render white (the modal stands out); re-verify test-chrome/test-flair-shell + re-audit the PPM. Ideally calibrate from a real System-7 screendump.
- **THE IN-OS ARC (the big next phase):** everything is HOST-rendered (the oracles). Wire the FLAIR stack INTO the kernel + boot to a live desktop in the emulator -- `26d` PS/2 mouse (IRQ12, dual-PIC EOI, Bochs-verified) + hourglass cursor + a desktop kmain path + an emu screendump gate. This makes the host-tested logic the actual bootable product (operator's "100% usable").
- **Other:** `u9gf` Bochs RFB capture (fb-agree Bochs leg); `k8o5.11` test-fb-agree dual-target; autoKey. OPERATOR-gated (non-blocking): `pvo4` Mac 68K ROM (Basilisk II rendered System-7 SSIM goldens + the real content-white + pinstripe RGBs), `77wz` Win 3.1 (`apt install xvfb xdotool`), `q0gy` 86Box (blocks M4 sign-off).
- M5 apps (InitechCalc with the 116% pie, etc.) consume these Managers next.
