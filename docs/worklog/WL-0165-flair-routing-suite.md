# WL-0165 -- the R3.7 host routing suite over disk_tenant_event

Lane: `lane/routing`, from main `60794f14`. Bead: initech-2svz (R3.7
follow-up to initech-tdnl.14). Commits `cb5f4e25`, `9ba6af05`. No beads
edits, no push.

## Context

R3.7 (tdnl.14, WL-0091 item 8) shipped the disk-tenant host with an emulator
gate only. Two guest bugs were fixed on the real guest without a host oracle
or a mutant pinning them:

1. `chrome.c` drew a window's title text unclipped. A repaint of a partly
   covered window stamped its Chicago cell run over the window in front. The
   app-launch RESTORE leg caught 176 wrong pixels.
2. A tenant that REGISTERs without a window must not become the foreground.
   `FlairProcess_admit` links it behind the head. The first NEWWINDOW is
   affirmed through `FlairProcess_activate` after the tenant returns
   (DEC-AC3-4).

The reconciliation names the missing oracle in DEC-AC3-3: "O-1-style host
routing suite over `disk_tenant_event`". Its DEC-AC3-2 names a second
missing oracle: "host property suite over the gate dispatcher (AX demux,
frame-relative arg reads, negative-error convention)". Before this lane, no
host test compiled `tbxgate.c`, so neither existed.

## What changed

- `harness/proptest/test_flair_routing.c` links the real code. That is
  `os/flair/tbxgate.c` over the real Layer-5 spine (`process.c`, `window.c`,
  `heap.c`, the region engine) and the real `psp.c`/`sft.c`. Only three seams
  are host stand-ins:
  - `loader_load_tenant` carves through the kernel's own callback and writes
    a link-synthesized FlairTenantRec.
  - `tbx_tenant_call`/`tbx_tenant_abort` stand in for the asm trampolines.
    They call the host tenant program; a triaged fault unwinds with longjmp.
  - The tenant's `int 0x81` is a 68-byte frame image handed to the real
    `tbx_gate_dispatch`.

  `service()` mirrors kmain `flair_live_tenant_service`: affirm first, then
  reap. The tenant program logs what it reads out of its own FlairEvent
  buffer. Every expected sequence is hand-authored from ADR-0013 Sec 3.3/3.4
  and DEC-AC3-3/-4.
- `harness/proptest/flair_routing_stubs.c` stubs every surface the suite
  does not grade. Each stub aborts if reached (Rule 2), and each includes its
  artifact header so its prototype is checked.
- Legs:
  - S0 proves the -no-pie address model: every tenant block is below 4 GiB.
  - A: a windowless REGISTER leaves the Finder foreground.
  - B: the affirmation waits until the entry returns, then sends exactly one
    deactivate and then one activate, through `disk_tenant_event`.
  - C: keyDown, content and title clicks, and click-to-activate in both
    directions.
  - G: a second launch while the slot is busy.
  - D: EXIT from inside a handler. The dying tenant hears nothing more; the
    Finder gets one activate; the next key goes to the Finder.
  - E: a fault inside the eventProc is a kill; a fault outside the image is
    not triaged.
  - H: an entry that never REGISTERs.
  - P: an event for the wrong app panics, fail-loud.
  - T: the title of a partly covered window writes no pixel in front. This
    leg uses the real chrome drawer over the render skeleton, and its
    unclipped reference render proves the title extends under the front
    window.
  - X: the dispatcher. All 65504 non-callable AX values give BADCODE; AX is
    EAX's low word; args are read from frame+68 under a poisoned frame;
    errors come back negative in EAX; a stray int 81h gives NOTTENANT; the
    TENANT-GATE lines match exactly.
- Four artifact mutants (Rule 6). Each is defined only by its mutant build:
  - `PROC_MUT_ADMIT_IS_FOREGROUND` (`process.c`)
  - `CHROME_MUT_TITLE_UNCLIPPED` (`chrome.c`)
  - `TBX_MUT_AX_FULL_EAX` (`tbxgate.c`)
  - `TBX_MUT_ARG_OFFSET` (`tbxgate.c`)
- `tbxgate.c`'s i386 MenuBar ABI `_Static_assert`s now apply only where that
  ABI exists: `UINTPTR_MAX == 0xFFFFFFFF`, which covers every kernel build.
  If `UINTPTR_MAX` is undefined, the build stops with `#error`. Without this,
  a 64-bit host build cannot compile the file.
- `test-flair-routing` and `test-flair-routing-mutant` join TEST_UNIT_GATES.
- Makefile root fix: `build/text_mut_%.o` did not depend on the generated
  `chicago_packed_data.h`. A fresh worktree's parallel test-kernel-headroom
  build raced it and failed.

## Frictions

- `sft_close_process` aborted on the host until `sft_init()` ran. kmain runs
  it at boot, and the teardown walks the inherited CON JFT.
- `bd` was locked by the PM's embedded backend, so the bead was not read
  (`initech-2svz` text came from the brief and the 2026-10-17 audit row).

## Acceptance

- `make test-flair-routing`: 61 checks, 0 failures.
- `make test-flair-routing-mutant`: all four mutants are RED on their named
  line:
  - ADMIT_IS_FOREGROUND -> leg(A1)
  - TITLE_UNCLIPPED -> leg(T1)
  - AX_FULL_EAX -> leg(X2)
  - ARG_OFFSET -> leg(X3)
- The existing knobs also bite this suite:
  - TBX_MUT_NO_SLOT_GUARD -> G1/G2
  - FLAIR_LIVE_MUTATE_SKIP_ACTIVATE -> B2/C4/C6
  - FLAIR_LIVE_MUTATE_IGNORE_REFCON -> C4/C5/C6
- `test-kernel-headroom` is green: 241 kernels; flagship headroom 10,412 B,
  binding 9,132 B. The flagship `kernel_flairtenants.bin` is byte-identical
  to main.
- These gates are green: test-process (+mutant), test-process-activate,
  test-chrome, test-tbx-menu (+mutant), test-makefile-vars, and
  test-gate-honesty (+mutant).
- No emulator gate was re-run, because the flagship kernel is byte-identical
  to main. No clip is needed: this is a host suite over existing behaviour.

## Pointers

- The suite does not cover SETMBAR, the drawing verbs, files, scrap, alerts
  or Standard File. On the 64-bit host they need a 4-byte-pointer MenuBar or
  more real Toolbox objects. Each is a fail-loud stub today and a natural
  next leg.
- `PROC_MUT_FRONT_STAYS_ACTIVE` is not caught here: no leg closes the
  foreground app's last window. test-process leg(g) owns that.
