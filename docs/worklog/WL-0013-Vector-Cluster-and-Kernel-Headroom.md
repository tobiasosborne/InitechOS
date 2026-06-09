<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0013 — Programme Engineering Work Record (PEWR)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Engineering Work Record (Worklog)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | PE-WL-0013 |
| Title | The INT 22/23/24 Vector Cluster + SETVECT/GETVECT + PSP Vector Save (DEC-10), and the Kernel-Headroom Work That Had To Land First (u0a guard, 5pe PROGRAM_BASE raise) |
| Version | 1.0 |
| Status | Issued |
| Classification | Internal Use Only |
| Period Covered | 2026-06-09 |
| Recording Function | Build Orchestration (supervised multi-agent; orchestrator re-ran every oracle + independently mutation-proved, Law 2 / Rule 4 / Rule 6) |
| Related | WL-0012. beads CLOSED: u0a, 5pe, 509.8. Filed: (INT 24h real disk-error trigger). Forward: 4tw (^C -> INT 23h), 509.10. |

---

## 1. Purpose & headline

Continues WL-0012 (the DEC-13 message-catalogue enforcement). This Record covers
the next M2 personality item — **`509.8` (DEC-10): the INT 22h/23h/24h handlers +
SETVECT/GETVECT + PSP vector save/restore** — and the **two prerequisite pieces of
kernel-headroom work that had to land first**, surfaced while planning it:

- **`u0a`** — a build-time `_kernel_end < PROGRAM_BASE` guard (the silent ceiling
  the .bin-size guards could not see).
- **`5pe`** — raising `PROGRAM_BASE 0x20000 -> 0x30000`, because the kernel had
  filled its 64 KiB window and 509.8's handlers would have tripped the new guard.

**State at close:** `make test` = **45 host + 19 emu = 64 gates** (was 43+18=61),
deterministic, green. `_kernel_end = 0x20360` against a now-128 KiB window
(limit 0x2ff00). Working tree clean. Three commits this period (u0a, 5pe, 509.8).

## 2. The headroom detour (u0a -> 5pe) — why it came first

Planning 509.8 (interrupt handlers = new kernel `.text`), the orchestrator checked
the kernel margin and found the root problem: `_kernel_end` was **0x1fd20**, only
**~480 bytes** below `PROGRAM_BASE` (0x20000). The kernel had quietly filled its
64 KiB window (linked at 0x10000); `KERNEL_SECTORS` had grown to 96 and `.bss`
ate the slack. The `memory_map.h` "gap proof" comment still claimed 32 KiB of
headroom — **stale**. Any further kernel feature, not just 509.8, would collide
with the program-load region.

1. **`u0a` — make the ceiling loud.** The existing guards check only the loaded
   `.bin` vs `KERNEL_SECTORS*512`; `.bss` is not in the `.bin`, so a `.bss` growth
   past 0x20000 was invisible until runtime corruption. Added a guard on all 15
   kernel `.bin` recipes: `nm` extracts `_kernel_end`, asserts `< PROGRAM_BASE -
   256` (PROGRAM_BASE parsed from `spec/memory_map.h` so it stays in sync), fails
   the build with an actionable message. Mutation-proven (orchestrator,
   independently): a 0x1000 `.bss` bloat the `.bin` guard waved through pushed
   `_kernel_end` to 0x20d60 and the new guard went RED.

2. **`5pe` — give the kernel room.** With the ceiling now loud, raised
   `PROGRAM_BASE 0x20000 -> 0x30000` (and `PROGRAM_IMAGE -> 0x30100`), a 128 KiB
   kernel window. A deliberate LOCKED-spec change (Rule 8): the operator chose it
   over squeezing 509.8 or relocating `.bss`. It rippled into **all 11 baked
   `.asm` `org 0x00020100` constants** (nasm can't read the C header, so they are
   duplicated constants that move together — `datetime_program.asm` also moved its
   absolute scratch addresses 0x21000/0x22000 -> 0x31000/0x32000), the
   `test_loader` assertion, and the stale gap-proof comment (rewritten to current
   reality). The rest of the map (ENV_BLOCK 0x5F000, stack/staging) was unchanged
   and stayed disjoint/ordered; the image arena remains ample (~188 KiB).

   **Vindication:** after 509.8 landed, `_kernel_end = 0x20360` — i.e. *past the
   old 0x20000*. Without 5pe, 509.8 would have tripped the guard exactly as
   predicted.

## 3. `509.8` — the vector cluster (DEC-10)

Implemented in three delegated stages, each re-verified by the orchestrator:

1. **Handlers + dispatch (the kernel core).** asm trap stubs `int22_entry/
   int23_entry/int24_entry` in `isr.asm` (byte-shape-identical to `int21_entry`,
   vector sentinels 0x22/0x23/0x24), installed as IDT trap gates in
   `sysinit_early` next to 0x20/0x21 (DEC-04a leaves 0x22-0x24 free). C
   dispatchers in `int21.c` with the same irq-depth + InDOS bracket as 21h:
   - **INT 22h** — terminate (routes to the bound exit hook, like INT 20h).
   - **INT 23h** — control-break default: emit `INT23-BREAK`, terminate. (Keyboard
     `^C` detection is **not** wired here — that is bead `4tw`.)
   - **INT 24h** — critical error: loops presenting `MSG_DOS_0001` ("Abort, Retry,
     Fail?", the DEC-13 catalogue macro) + `conin_get_pb()` + the **pure**
     `crit_error_action()` (R->0/A->1/F->2, invalid->re-prompt) until valid; writes
     AL, clears CF, does NOT terminate (24h returns the action to its caller).
   - **AH=25h SETVECT / 35h GETVECT** — completed the already-listed-but-unimpl
     register surface (`ah_is_listed` already named them). SETVECT installs the IDT
     gate offset for vector AL from EDX; GETVECT returns the offset in EBX (ES=0,
     flat model). Both via a `g_setvect`/`g_getvect` seam bound in `kmain` so
     `int21.c` stays host-testable. ABI per `spec/int21h_calling_convention.json`.

2. **PSP vector save/restore (loader).** Pure helpers `psp_save_vectors` /
   `psp_load_vectors` in `psp.c` (little-endian into `saved_vectors` at PSP
   0x0A/0x0E/0x12; `psp_build` keeps zero-filling, the loader fills explicitly).
   `load_program` reads the live IDT 0x22/0x23/0x24 right after `psp_build` and
   saves them into the child PSP; on the EXIT path (after the program can no longer
   run, PSP still intact) it restores them into the IDT. DOS-authentic: a child's
   `SETVECT` does not leak past its exit. `sizeof(psp_t)==256` preserved.

3. **Oracles (Law 2 centerpiece).** Host `test-int24` (44 checks: A/R/F mapping;
   the int24 prompt + re-prompt loop; psp save/load byte-exact round-trip +
   neighbour-untouched; 25h/35h dispatch; 22h/23h terminate) + `test-int24-mutant`
   (4 source mutations — A/F swap, no-reprompt, psp vec-offset, getvect wrong
   register — each must bite). Emu `test-vect`: a baked `vect_program.asm`
   GETVECTs 0x24 (`V24PRE`), `int $0x24` runs the kernel handler which presents
   MSG-DOS-0001 and the harness injects 'a' (`CRIT-AL=1` = Abort), then SETVECTs
   0x24 to a bogus address and EXITs; the kernel GETVECTs 0x24 post-exit
   (`V24POST`) and the gate asserts `V24POST==V24PRE` — the loader restored the
   parent vector despite the child's SETVECT (the DEC-10 acceptance).

## 4. Orchestration & verification (Law 2 / Rule 4)

Five delegated stages (u0a, 5pe, handlers, save/restore, oracles); the orchestrator
re-ran every gate itself and **independently** mutation-proved the load-bearing
ones rather than trusting the scripted mutants:
- `crit_error_action` Retry mapping broken in the real source -> `test-int24` RED
  at the exact checks; restored identical.
- the `install_live_vector(0x24,...)` restore line commented out in the real
  `loader.c` -> `test-vect` RED with `V24POST=201268 != V24PRE` ("the child SETVECT
  leaked"); restored identical.
- (carried from the headroom work) the u0a `.bss`-bloat proof and the 5pe full
  program-emu-gate sweep.

This caught nothing wrong in the subagents' deliverables this period — but the
discipline is the point (WL-0011 recorded two sessions where subagents shipped
masked problems). One subagent also correctly reported that the working-tree git
snapshot was stale (prior in-tree 509.8 work) and left those files untouched.

## 5. Scope decisions

- **25h/35h folded into 509.8.** No separate bead existed and they were already in
  the locked register surface; they are the mechanism that makes the PSP
  save/restore meaningful and end-to-end testable, so implementing them here was
  completion, not creep.
- **Real critical-error trigger deferred.** 509.8 makes INT 24h invokable and
  correct, but no kernel path RAISES it yet (real DOS raises it from disk-I/O
  errors). Filed a new bead to wire `fat12.c`/`ata.c` failures to INT 24h and
  honor its A/R/F return (Retry re-issues, Abort terminates, Fail returns the
  error). Ignore=3 also deferred.
- **`^C` -> INT 23h deferred to `4tw`.** The INT 23h handler now exists, so `4tw`
  (CON input 01h/08h/0Ah detecting 0x03 and invoking INT 23h) is now unblocked.

## 6. Phase Disposition — NEXT AGENT

M2 personality continues to fill in. With DEC-13 (WL-0012) and DEC-10 (this Record)
done, and the kernel window doubled, the runway is clear. Per the operator order
(internals -> shell -> rest) and WL-0011 §5:

1. **`4tw`** — now unblocked: CON input 01h/08h/0Ah detect `^C` (0x03) and invoke
   the (now-existing) INT 23h handler. Small, high-value for interactive robustness.
2. **`x0i`** — tri-emulator (Bochs/86Box). All 19 emu gates are QEMU-only; this is
   the principal quality debt.
3. Remaining M2: the deferred INT 24h disk-error trigger (new bead), `509.7`
   device-driver chain, `509.6` MCB arena, `509.9` FCB, `509.10` CONFIG.SYS/
   AUTOEXEC baseline (unblocked by 509.1), `kod` two-file split, then `k6x`
   (COMMAND.COM default boot — the M2 finale).

**Working discipline (reconfirmed):** re-run the full 64-gate vector after anything
touching the kernel image; verify every subagent "green" yourself; the u0a guard
now makes a kernel-window overflow a loud build failure — if it fires, the fix is a
deliberate `PROGRAM_BASE` raise (5pe is the template) or `.bss` reclamation, never
weakening the guard.

## 7. Verification of Record

At close, `make test` passes all 64 gates on QEMU (`triple_fault=0`). Every new
oracle is mutation-proven (scripted + orchestrator-independent). The u0a guard is
live on all 15 kernel variants; the 5pe window raise is verified by `test-loader`
+ all 8 program emu keystones; 509.8's acceptance (MSG-DOS-0001 + A/R/F + vectors
saved/restored across EXEC/EXIT) is enforced by `test-vect`. Tree clean.

---

*— End of Record —*

<!-- Tedium certified compliant with NFR-7. -->
