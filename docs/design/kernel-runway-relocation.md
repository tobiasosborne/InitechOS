<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->
<!-- Minted 2026-08-24 by the ox-alpha 1M-context design seat (tool-less, 201KB corpus:
     stage2.asm, kernel.ld, kstart.asm, memory_map.h, ADR-0001/0003, loader excerpts),
     orchestrator-reviewed. Beads initech-5dr8/-yrzo; BLOCKS the R3 Finder slices.
     STATUS: DRAFT pending ratification; section-0 gaps are honest corpus limits
     (verify IDT-only dispatch, PSP vector derivation, serial-marker greps, BSS
     zeroing assumptions against the real tree before implementation). -->

# Kernel Runway Relocation Design

**Beads:** initech-5dr8 / -yrzo · **Blocks:** GUI-remediation R3/R4 · **Status:** DESIGN (read-only seat; no tools; all claims cited to inlined corpus)

---

## Section 0 — Corpus gaps (flagged honestly, Law 1)

1. **IDT / interrupt-dispatch source is not inlined.** The K1(b) conclusion ("nothing must stay low") rests on vectors being IDT-based flat-32 linear addresses (CLAUDE.md: "INT vectors are IDT-based, flat 32-bit -- physical position of handlers is free"). No `idt.c`/gate-wiring file appears in the corpus. **Verify before S2:** no IVT-style real-mode far-pointer dispatch survives in the FLAIR kernel.
2. **`psp.c` (`psp_build`) is not inlined.** ADR-0003 App. B.2 puts an `INT 21h` far-entry at PSP:0x50h and saved INT 22h/23h/24h vectors at 0x0Ah–0x15h. If any of these are baked *low literals* rather than runtime-computed linear addresses, K1(b)/K2 break. Expected: runtime values (the loader builds the PSP at EXEC time from live kernel state, `loader.c` :: `psp_params`). **Verify.**
3. **`blockdev` backend not inlined.** If any disk path programs raw floppy DMA (not INT 13h), DMA buffers carry a <16 MiB + no-64 KiB-boundary-crossing constraint. KERNEL_BASE 0x500000 is <16 MiB (physically legal) but the 64 KiB rule would need a bounce. Corpus evidence (stage2 INT 13h CHS everywhere, `mbr.asm`, `stage2.asm`) says BIOS-mediated; **confirm no raw-DMA site exists.**
4. **`.bss` carriage in the `.bin` is contradictory in-corpus.** `kernel.ld` emits `.bss` before `_kernel_end`; the Makefile guard prose (excerpt :207–209) says "_kernel_end (end of .bss) is NOT in the .bin," implying objcopy drops NOBITS — yet `kstart.asm` performs **no BSS zeroing**, so today's kernel implicitly relies on emulator zero-init RAM (cf. `spec/memory_map.h`: "the emulator zero-inits RAM"). This design pins explicit zeroing regardless (K2), making the ambiguity moot.
5. **Harness gate-matching style unknown.** If any boot gate diffs the *entire* serial stream rather than grepping per-marker (PRD: "serial marks each boot stage"), the two appended markers in K2/K3 re-key exactly one golden. Assumed grep-per-marker; **audit `harness/emu/*_main.c` at S4.**
6. **`seed/rt/start_dos.asm` and TPS codegen bases are not inlined.** The prime directive (program window byte-identical) is asserted from the brief + `spec/memory_map.h` (PROGRAM_IMAGE=0x40100 baked into `org`s per the header's NASM note) — not re-verified against the actual seed sources.
7. **Pending-feature sizes are extrapolated**, not measured (see K1(d)); the corpus gives analogues (manager set ~51 KiB, chrome-fidelity delta 163,960−159,000 ≈ 4.9 KiB), not the R3/R4 objects themselves.
8. **86Box driver not built** (`harness/emu/86Box` PLANNED, initech-x0i; CLAUDE.md file map). "Tri-emulator" today is honestly QEMU + Bochs; 86Box legs land with x0i.
9. **FLAIR heap allocator not yet written** (`spec/memory_map.h`, ALLOCATOR DEFERRED note) — convenient here: no baked heap addresses exist, so heap immobility costs nothing; but confirm no FLAIR .c bakes a `0x100000`-derived literal outside the header.

---

## Part K1 — Options, scored

### (a) Load the whole kernel HIGH (extended memory)

**What stage2 already does (analysis per brief).** The load loop (`stage2.asm` :: `.kload_loop`) reads KERNEL_SECTORS=352 sectors via INT 13h CHS into ES:BX starting at `kload_seg=0x1000` (phys 0x10000), chunked per queried track geometry (`int 0x13` AH=08h), advancing `kload_seg += chunk*32`. It is **destination-agnostic**: retargeting the destination is a one-line change. Critically, the loop runs **in real mode, before A20/PE** — so it cannot write above 1 MiB directly. But A20 is enabled *before* the PM switch (`stage2.asm` :: `.a20_done`, port 0x92), and the PM entry (`pm_entry`) is flat-32 with a full 4 GiB data selector — so a **post-PE copy-up** needs no unreal-mode tricks, no bounce-buffer juggling in real mode, and no reordering of the minefield sequence CLAUDE.md warns about. The vacated kernel window [0x10000,0x40000) becomes the natural bounce/staging area (192 KiB ≥ 176 KiB image).

**Placement sub-choice.** Two candidate high homes:
- **0x500000 (above the FLAIR heap)** — `FLAIR_HEAP_*` ([0x100000,0x500000), DEC-03, LOCKED) untouched; disjointness proofs in `spec/memory_map.h` survive verbatim; cost: minimum installed RAM rises 4 MiB → 5 MiB (one derived gate).
- 0x100000 with the heap moved — forces a Rule-8 edit to DEC-03's "FIXED and spec-locked" window and re-derives every FLAIR_HEAP proof; if the heap keeps its 4 MiB size and moves to 0x200000, the requirement rises to **6 MiB** — strictly worse. **Rejected.**

**Scores (a@0x500000):** period authenticity GOOD (an ext-mem-resident flat-32 kernel is exactly ADR-0001's model; 5 MiB 386-class machines are period-plausible; the probe-gate pattern already exists, DEC-03/FO-G). Implementation risk LOW-MED (the only new privileged-mode code is a `rep movsd`; the real→protected sequence order is untouched; A20 already precedes PE). Blast radius LOW (program window, seed/TPS `.org 0x40100`, DEC-08a prose, every `.COM` fixture: untouched — prime directive holds by construction; FLAIR heap untouched; changes are additive constants + one gate). Oracle-ability HIGH (all serial contracts preserved; placement is trivially assertable — K3). Rule 11 determinism HIGH (fixed constants; the probe gates boot, never shapes the map — the DEC-03 pattern). Headroom: link-address runway 0x500000→0x600000 = **1 MiB** (~830 KiB net over today's 2.4 KiB); disk-image growth remains the routine KERNEL_SECTORS equate bump (Makefile :180 history), now **decoupled from PROGRAM_BASE forever**.

### (b) Split kernel: low stub + high body

**What genuinely must stay low: the answer is NOTHING (kernel-code-wise).** Programs reach the kernel exclusively through vectors (INT 21h/20h/22h/23h/24h per ADR-0003 App. A/B.2) and through *data* the loader hands them (PSP at 0x40000, env at 0x6F000, stack top 0x7FFFC — `spec/memory_map.h`; `loader.c` :: `loader_prepare_core` sets `params.*` to these). In flat 32-bit protected mode with an IDT, a vector's handler linear address is arbitrary — 0x5xxxxx works identically to 0x1xxxx. Nothing in the loader contract hands a program a *kernel code* address. The only legitimate low residents are pre-boot artifacts stage2 itself places (boot_info 0x500, font 0x1000, VBE bufs 0x7000/0x7200, E820 buf 0x7400, stage2 at 0x8000 — `stage2.asm` constants), which are not kernel body. **Therefore the "low stub" is empty, and (b) degenerates into (a) plus a second link script, a stub/body ABI, and far-thunk bookkeeping — strictly dominated. Rejected.** (Caveat: gaps 0.1/0.2 — confirm IDT-only dispatch and runtime-computed PSP vectors.)

### (c) Raise PROGRAM_BASE

Dead three ways. (i) **No room:** the gap proof (`spec/memory_map.h`, initech-re30.2 note) shows free gap = 0 KiB — the kernel stack tops at 0x9FFFC, butting the 0xA0000 VGA aperture; re30.2 explicitly recorded this as "the MAXIMUM PROGRAM_BASE raise possible under the conventional-memory scheme." (ii) **Prime-directive violation:** PROGRAM_BASE/PROGRAM_IMAGE are baked into `test_program.asm`-class `org`s, TPS codegen bases, DEC-08a prose, and every `.COM` fixture (header's DELIBERATE-ACT notes; brief). (iii) Even a hypothetical raise buys one-shot KiB, not a runway. **Rejected.**

### (d) Diet + defer

Numbers against 0x9C0 (~2.4 KiB) of headroom (`_kernel_end=0x3f640` vs 0x40000):

| Pending item | Plausible size | Basis |
|---|---|---|
| Resident Finder tenant (F2: icon grid, selection, rubber-band, drag save-under, menus, file-ops UI) | ~20–35 KiB | Larger than any single Toolbox manager; the 12-manager set linked at ~51 KiB (re30.1 note, `kernel.ld`/`memory_map.h`); chrome-fidelity glyphs alone added ~4.9 KiB |
| Disk-tenant launch (INT-0x81 gate + loader plumbing) | ~3–6 KiB | Loader/MZ machinery already resident (`loader.c` #includes `mz.c`); the gate is new but thin |
| 4 bundled apps (R4) | ~20–50 KiB | HELLO/NOTES-scale tenants × 4 |
| 640x480x8 present path (zqy3) | ~5–10 KiB | Blit/present + palette routing |

Total plausible need **~50–100 KiB vs 2.4 KiB held** — a 20–40× shortfall. The DOS-shell escape hatch (delinking dead FLAIR objects, which took kernel_shell 0x3f640-class → 0x31660) is spent: the FLAIR kernels cannot shed the Toolbox (brief; the managers are load-bearing linked code per re30.2). **Rejected with prejudice.**

### RECOMMENDATION: **(a), kernel high at 0x500000, post-PE copy-up.**

### Exact new layout constants (for `spec/memory_map.h`, Rule-8 deliberate act)

```c
#define KERNEL_BASE        0x00500000u  /* link == load == physical (kernel.ld)      */
#define KERNEL_RUNWAY      0x00100000u  /* window [0x500000, 0x600000)               */
#define KERNEL_CEIL        0x00600000u  /* ASSERT(_kernel_end < KERNEL_CEIL)         */
#define KERNEL_STACK_BOT   0x004F0000u  /* 64 KiB kernel stack, moved up with kernel */
#define KERNEL_STACK_TOP   0x004FFFFCu  /* kstart.asm ESP                            */
#define KERNEL_BOUNCE_BASE 0x00010000u  /* stage2 real-mode load target (vacated)    */
#define KERNEL_BOUNCE_CAP  0x00030000u  /* 192 KiB -> 384 sectors hard cap           */
#define INITECH_MIN_EXT_KB (((KERNEL_CEIL) - 0x00100000u) / 1024u)  /* = 5120 */
```

- **FLAIR heap does NOT move:** [0x100000,0x500000) byte-identical; `flair_heap_ram_ok` and all DEC-03 proofs untouched. Kernel sits immediately above it; disjointness is positional.
- **Program window does NOT move:** PROGRAM_BASE…LOAD_STAGING… all literals byte-identical (prime directive).
- **Conventional [0x10000,0x40000):** vacated by the kernel; repurposed as the stage2 load bounce. Below PROGRAM_BASE, above stage2/font/VBE/E820 scratch — disjoint from every locked region. Documented as kernel-owned bounce; no other consumer.
- **Kernel stack moves** [0x90000,0xA0000) → [0x4F0000,0x500000): kstart.asm is edited anyway; this permanently retires the "stack butts VGA, 0 KiB slack" fragility (re30.2 note) and keeps all kernel-private state contiguous high. Stage2's own transient PM stack anchor (`pm_entry` `mov esp,0x90000`) is left as-is — the region is free post-handoff (same argument `kstart.asm` already documents).
- **boot_info handoff change:** one **append-only** field, `uint32_t kernel_base` at offset 28 (struct 28→32 bytes), written by stage2, consumed by the kernel's fail-loud placement guard — the exact backward-compatible-append pattern `ext_mem_kb` established (`boot_info.h` header note). Nothing else in the handoff changes; the far-jump target literal moves 0x00010000 → 0x00500000 (`stage2.asm` :: `jmp dword CODE_SEL:0x00010000`).

---

## Part K2 — The stage2 delta

### Strategy: real-mode load UNCHANGED into the bounce; copy-up POST-PE (minimal-delta)

The entire real-mode load path — geometry query (AH=08h), CHS conversion, track-chunk loop, error paths — is **byte-for-byte untouched**. Only the 32-bit tail of `pm_entry` changes. This is deliberately chosen over (i) A20-early + unreal-mode copy (more asm cleverness in the exact region CLAUDE.md's hallucination callout flags) and (ii) loading directly high via unreal mode (same objection). The new privileged code is one flat-32 `rep movsd` — the least risky possible addition downstream of the far-jump flush.

### `stage2.asm` changes

1. **Load destination:** `kload_seg` init stays `0x1000` (comment updated: bounce = vacated kernel window, `KERNEL_BOUNCE_BASE`). Loop untouched.
2. **New capacity invariant:** KERNEL_SECTORS (352) ≤ 384 (bounce cap). Add a comment + Makefile guard (below). At 384 the disk window equals the bounce; further image growth is the routine KERNEL_SECTORS/IMG_SECTORS bump *plus* a bounce-policy decision — not needed within the 1 MiB runway (4× today's image).
3. **`pm_entry`, after the LFB/OK markers, before the kernel jump** — insert the copy-up:
   ```asm
   ; Copy the kernel image bounce -> high residence. Full padded window
   ; (KERNEL_SECTORS*512) copied deterministically, padding zeros included.
   cld
   mov esi, KERNEL_BOUNCE_BASE        ; 0x00010000
   mov edi, KERNEL_BASE               ; 0x00500000
   mov ecx, (KERNEL_SECTORS*512)/4    ; 45056 dwords at 352 sectors
   rep movsd
   mov esi, msg_khi32                 ; new marker "KHI\n"
   call serial_puts32
   ```
4. **boot_info append:** `mov dword [BOOT_INFO_ADDR + 28], KERNEL_BASE` in the real-mode boot_info build block (field 8 of 8; struct 28→32 bytes).
5. **Final jump:** `jmp dword CODE_SEL:0x00010000` → `jmp dword CODE_SEL:KERNEL_BASE`.
6. **A20 ordering: no change.** A20 is enabled before LGDT/PE today (`.a20_done`); the copy-up crosses the 1 MiB line *after* PE with A20 already on — the Bochs legs (which catch A20/prefetch sins per CLAUDE.md) exercise exactly this path.
7. **Serial contract:** append-only. Existing markers (S1/JMP2/S2/VBE/FONT/E820|E801|E88/A20/GDT/PM/LFB/OK/KLOAD) all preserved in order; two new markers (`KHI` post-copy; `KAT` from the kernel, K3). Subject to gap 0.5.

### `kernel.ld` changes

- `. = 0x00010000;` → `. = 0x00500000;` (update the ground-truth Sec 3.1–3.2 citation comment: link addr == load addr == physical still holds, at the new base).
- `ASSERT(_kernel_end < 0x40000, ...)` → `ASSERT(_kernel_end < 0x60000 * 16, ...)` i.e. `< 0x00600000` with updated message (the old message's "raise PROGRAM_BASE" advice is obsolete — the conventional gap is spent; the new failure advice is "grow within the runway or bump KERNEL_SECTORS").
- Optional hygiene: emit `KERNEL_BASE`-assertion `ASSERT(. == 0x00500000)` at the dot assignment so a stray preceding section fails at link.

### `kstart.asm` changes

- `mov esp, 0x0009FFFC` → `mov esp, KERNEL_STACK_TOP` (0x004FFFFC); rewrite the region comment (old [0x90000,0xA0000) rationale retired; new stack [0x4F0000,0x500000) sits immediately below the kernel image, disjoint from everything).
- **Explicit BSS/runway zeroing** (resolves gap 0.4, belt-and-suspenders over emulator zero-RAM): after setting ESP,
  ```asm
  ; Zero the post-image runway slack: [KERNEL_BASE + KERNEL_SECTORS*512, KERNEL_CEIL)
  mov edi, KERNEL_BASE + (KERNEL_SECTORS*512)
  mov ecx, (KERNEL_RUNWAY - KERNEL_SECTORS*512)/4
  xor eax, eax
  rep stosd
  ```
  Deterministic, ~0.85 MiB of `stosd` — microseconds. Guarantees `_kernel_end`-through-runway is zero regardless of objcopy NOBITS behavior.
- Entry address comment: stage2 far-jumps to `CODE_SEL:0x00500000`.

### Makefile guard changes

- **Ceiling (replaces the PROGRAM_BASE-targeted check, :206–239):** kernel-end guard now asserts `_kernel_end < 0x600000` per kernel ELF (trivially green today; the guard's real job becomes catching runaway growth against the runway).
- **NEW floor guard (the prime-directive teeth):** assert (a) the lowest section/symbol address of each kernel ELF is `>= KERNEL_BASE` (catches a LOAD_LOW relapse at link time), and (b) `PROGRAM_BASE == 0x40000 && PROGRAM_IMAGE == 0x40100 && ENV_BLOCK == 0x6F000 && PROGRAM_STACK_TOP == 0x7FFFC` via a byte-golden spec test (any accidental touch of the locked program-window literals goes RED — this is the mutation-proven floor).
- **NEW bounce-capacity guard:** `KERNEL_SECTORS <= 384` (build-time RED on overflow — the deterministic catch for the BOUNCE_OVERLAP mutant class; see K3).
- **Unchanged:** KERNEL_SECTORS=352, IMG_SECTORS geometry (IMG_MIN = 1+16+352 = 369 ≤ current multiple-of-64 image), pad recipes, dd seek=17 layout. **The on-disk image layout does not change at all.**
- **DOS program window [0x40000,0x80000): byte-identical behavior.** No constant, no `org`, no fixture, no loader arithmetic moves. The only code that ever writes near it is stage2's bounce — now *bounded* by the new capacity guard so it provably cannot reach 0x40000 (352·512 = 0x2C000 ends at 0x3C000 < 0x40000; even at cap, 384·512 ends exactly at 0x40000, exclusive).

---

## Part K3 — Oracle set

**Main proof — every existing boot gate re-runs unchanged.** Serial contracts identical (append-only markers aside): `test-boot`, `test-boot-bochs`, `test-flair-desktop` (+`-bochs`), `test-flair-appswitch` (+`-mutant`, +`-bochs`), plus the non-boot oracles untouched by construction: `test-loader*` (host-side, layout math against `memory_map.h`), `test-arena-disjoint-mutant`, `test-region`, `test-fat`, `test-dbase`, `test-seed-fpc-diff`, `make selfhost`/`ddc` (factory-side; Rule 11 unaffected — all new constants are fixed literals).

**NEW boot leg — physical-placement assertion (the load-bearing new gate):**
1. stage2 prints `KHI` after the copy-up (proves the copy ran).
2. `kernel_main` prints `KAT 0x00500000-0x000xxxxx` derived from linker symbols `_kernel_start`/`_kernel_end` (proves execution *at* the high base, not merely a successful copy).
3. In-kernel fail-loud guard (Rule 2): `if (*(volatile uint32_t*)(BOOT_INFO_ADDR+28) != KERNEL_BASE) panic();` — the boot_info append makes placement self-checking independent of the serial harness.
4. Harness greps `KAT`, asserts start == 0x500000 and end < 0x600000. Wired into `make test` (and the Bochs variant — see below).

**Tri-emulator:** QEMU dev loop first; **Bochs strict real→protected legs are mandatory** for this change specifically — the copy-up is the first code this kernel has ever run that crosses the 1 MiB boundary, which is precisely the A20/prefetch sin class CLAUDE.md's callout reserves Bochs for (`test-boot-bochs` + the new placement leg under Bochs). 86Box: deferred with initech-x0i (gap 0.8) — stated, not hidden.

**Mutants (Rule 6 — each proven RED before merge):**

| Mutant | Perturbation | RED signal (named mechanism) |
|---|---|---|
| `LOAD_LOW` relapse | Delete the copy-up; jump to 0x10000 | **Double-caught:** (1) the `KAT` gate — kernel reports 0x10000, range assert RED; (2) the boot_info guard — stage2 wrote `kernel_base=0x500000`, the low-linked kernel's constant disagrees → in-guest panic. Note the trap: the bounce *contains* a valid image, so the machine boots fine — only the placement leg catches it. This is why the leg exists. |
| `BOUNCE_OVERLAP` | KERNEL_SECTORS > 384 (bounce overruns PROGRAM_BASE) | Primary: **build-time** bounce-capacity guard RED (deterministic, Rule 2). Honest secondary note: there is **no post-boot RAM image-hash gate in the corpus today** — the actual mechanisms are the golden diffs (`test-fat` mtools/python image diff, screendump/structural oracles, the Makefile kernel-end guard, `test_loader` layout asserts), none of which hash conventional RAM at boot. Once R4 disk-launch lands, an EXEC-then-PSP-bytes check inherits coverage; until then the capacity guard IS the gate, and it is mutation-proven. |
| `STACK_LOW` relapse | kstart keeps ESP=0x9FFFC | Functionally harmless (flat model) — caught by review + the layout doc, not a runtime gate; optionally a debug-flag self-check `ESP ∈ [0x4F0000,0x500000)`. Declared weak on purpose. |
| `BOOTINFO-STALE` | Omit the offset-28 write | `KAT` leg RED (guard panics / marker missing). |

---

## Part K4 — Migration plan + ADR

Ordered slices, each independently green (Red→Green per Rule 1; the RED for S1/S2 is the new guards failing on the *old* layout, for S3/S4 the missing `KAT`):

| # | Size | Slice | Independent-green criterion |
|---|---|---|---|
| S1 | M | **Spec + ADR.** `spec/memory_map.h` Rule-8 edit (constants above + DELIBERATE-ACT note citing 5dr8/-yrzo); `boot_info.h` append `kernel_base` (28→32 bytes, append-only precedent); draft ADR-0003-AMENDMENT-DEC-08b (below). | Spec gates green; zero artifact behavior change; floor-guard byte-golden for PROGRAM_* lands here and is mutation-proven. |
| S2 | M | **Link + entry.** `kernel.ld` → 0x500000 + new ASSERT; `kstart.asm` stack + runway zeroing; Makefile: ceiling→0x600000, NEW floor guard, NEW bounce-capacity guard. | Build green; kernel-end guard green at new ceiling; all host oracles green (they never boot). Boot intentionally broken at this point (stage2 still jumps low) — acceptable mid-state only within this slice boundary; S3 lands in the same session. |
| S3 | M | **stage2.** Copy-up block, boot_info offset-28 write, `KHI` marker, jump retarget. | `test-boot` (QEMU) full serial contract + `test-boot-bochs`. |
| S4 | S | **Placement oracle.** `KAT` print in kernel_main; harness leg (grep + range assert) in `make test` + Bochs variant; mutants `LOAD_LOW` and `BOUNCE_OVERLAP` proven RED then restored. | New leg green; mutants bite (Rule 6). |
| S5 | S | **Ext-mem gate.** Kernel boot check `ext_mem_kb >= INITECH_MIN_EXT_KB` (5120) → panic below (FO-G pattern); verify QEMU/Bochs default RAM clears it. | Boot gates green; a low-RAM boot leg (if cheap) asserts the panic. |
| S6 | M | **Full-vector regression.** Entire `make test` matrix incl. appswitch/desktop/-bochs; operator Law-4 eyeball (pixels bit-identical — no rendering path touched). | Whole vector green; R3/R4 unblocked. |

**Gate re-keying: expect NONE beyond the new placement leg.** Verified against the corpus gate list: boot gates key on per-stage markers (all preserved, append-only — subject to gap 0.5: if any gate is full-stream-equality, that one golden is the sole sanctioned re-key); loader/arena/spec oracles read `memory_map.h` constants that did not move; FAT/dBASE/compiler/self-host gates are orthogonal. The kernel-end guard is *retargeted* (0x40000→0x600000) — a strengthening, not a weakening (Stop-Condition compliant).

### ADR draft — ADR-0003 Amendment DEC-08b (DEC block, ready to ratify)

> **DEC-08b — High Kernel Relocation (Extended-Memory Residence).** The resident kernel remains a flat binary handed off by stage2 (DEC-08, unchanged in format and handoff *structure*). Its residence moves from conventional [0x10000, PROGRAM_BASE) to the extended-memory window **[KERNEL_BASE, KERNEL_CEIL) = [0x00500000, 0x00600000)**, linked == loaded == physical per ADR-0001's flat model. stage2 loads the image real-mode into the vacated conventional bounce [0x10000, 0x40000) (capacity-capped at 384 sectors, fail-loud at build), then copies it up **after** the CR0.PE transition with A20 already enabled (existing sequence order preserved; the copy is the only new post-PE code). The kernel stack relocates to [0x4F0000, 0x500000). The FLAIR heap window [0x100000, 0x500000) (DEC-03) is UNCHANGED; the program window [0x40000, 0x80000) and every constant, fixture, and codegen base derived from it are UNCHANGED (prime directive). Minimum installed extended memory rises 4096 → **5120 KiB**, gated fail-loud via the existing `ext_mem_kb` probe (DEC-03/FO-G pattern). `boot_info_t` gains one append-only field, `kernel_base`, consumed by an in-kernel placement guard. Rationale: the conventional gap is exhausted (re30.2 record, 0 KiB free); pending R3/R4 need ~50–100 KiB against 2.4 KiB held; alternatives rejected — split-kernel (nothing must stay low; strictly dominated), PROGRAM_BASE raise (no room; breaks the seed/TPS/.COM contract), diet (20–40× shortfall; the delink lever is spent). Verification: all boot serial contracts unchanged; new placement leg (`KHI`/`KAT` + boot_info guard) under QEMU and Bochs strict-transition; mutants `LOAD_LOW`/`BOUNCE_OVERLAP` proven RED.

---

*Triage honored: K1 → K2 → K4 → K3. Every numeric claim above traces to `spec/memory_map.h`, `os/boot/stage2.asm`, `os/milton/kernel.ld`, `os/milton/kstart.asm`, `os/milton/boot_info.h`, `os/milton/loader.c`, the Makefile excerpts (:126–239), ADR-0003 and its amendments, or the measured figures in the brief; the seven soft spots are enumerated in Section 0 with their verifying slices.*
