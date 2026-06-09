<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0010 — Programme Engineering Work Record (PEWR)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Engineering Work Record (Worklog)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | PE-WL-0010 |
| Title | The Aggregate `make test` Gate, Deterministic Screendump Capture, and the Multi-Tenant File-I/O Epic |
| Version | 1.0 |
| Status | Issued |
| Classification | Internal Use Only |
| Period Covered | 2026-06-09 |
| Recording Function | Build Orchestration (supervised multi-agent; one bead per delegated subagent, orchestrator-verified per Law 2 / Rule 4) |
| Related | WL-0009; beads initech-4mc, -3pe (closed); epic initech-6qy with children -lq2, -snk, -0qh, -6hk (all closed); follow-ups -vtk, and the FAT/file-I/O fuzzer bead |

---

## 1. Purpose

This Record memorializes an **orchestration session** whose object was to make
InitechDOS "absolutely rock solid" — a trustworthy foundation for the GUI
(FLAIR), the database (InitechBase), and the resident compiler (Turbo Initech)
— *before* any of those are built on top. The operator's directive: work out a
priority list, then orchestrate it (take a bead, granular-plan it, delegate each
step to a focused subagent, monitor, verify every "green" independently, file
beads for anything surfaced), landing milestones serially.

The headline outcome: **there is now a single deterministic `make test`** that
runs the entire green oracle vector (50 gates), and **the single-file,
whole-file-into-one-64-KiB-buffer file model is gone** — InitechDOS holds
arbitrarily many files open concurrently with independent positions and reads
files far larger than the old 64 KiB cap. That was the load-bearing prerequisite
for InitechBase and Turbo Initech.

## 2. The priority list (ground truth, not the worklog's feature checklist)

Verification of the actual sources (not WL-0009 prose) reframed the work from
"finish M2's features" to "harden what the next three milestones stand on." The
ranked landmines:

- **P0-1 — no aggregate gate.** `make test` was the M0 `stub_fail`; the ~22-gate
  sweep was run by hand. "Rock solid" was unassertable.
- **P0-2 — the single-tenant file model.** One file open at a time; whole-file
  read into one 64 KiB static buffer at OPEN; EXEC staging reusing that region.
  Fatal for dBASE (`.dbf` + `.mdx` open at once, random seek, >64 KiB) and the
  compiler (read source + write object). Split across four under-prioritized
  P2 beads.
- **P0-3 — INT 21h reentrancy (initech-xk2).** Trap-gate dispatch with IF set;
  "safe only by zero-shared-state." A latent corruption landmine for the
  cooperative event loop. **Still open — next.**

## 3. What landed (in order, each oracle-verified by the orchestrator)

1. **Aggregate `make test` (initech-4mc).** `test-unit` (host unit + mutant
   gates) + `test-emu` (QEMU keystones) + `test` (both, fail-loud on first red).
   Milestone stubs (region M3 / dbase M6 / compiler M7 / selfhost+ddc M8) and the
   unbuilt Bochs/86Box gates are excluded from the green aggregate. Mutation-
   proven to bite.
2. **Deterministic screendump (initech-3pe).** Running the whole vector exposed
   a wall-clock race: the screendump keystones grabbed the framebuffer at a fixed
   6 s deadline and, under back-to-back QEMU load, dumped a blank frame (RED). A
   non-deterministic gate is worse than none. Fix: a harness `--screendump-after
   MARKER` option waits for the guest's paint-complete serial marker (`BANNER`
   for tracer-boot/boot, `DIR-OK` for fs) before grabbing; the wall clock remains
   a hard backstop and on marker-miss it dumps anyway so a guest that never paints
   still fails **honestly**. Verified green under 6-core CPU saturation; negative
   control (`NEVERPAINT`) terminates at the backstop without a hang.
3. **Multi-tenant file I/O — epic initech-6qy (4 children):**
   - **lq2 — `fat12_read_partial`:** positioned cluster-chain read, no whole-file
     buffer. Differential vs an independent python reference + mtools/dd (204
     checks); 2 mutants bite.
   - **snk — `fat12_write_partial`:** positioned write (read-modify-write at
     cluster granularity, extend-by-allocation, zero-hole, both-FAT sync) with an
     **allocate-then-commit disk-full rollback** that leaves the volume re-mountable
     and prior files intact. Triple-checked (byte model + mtools + python) incl. a
     disk-full integrity leg; 2 mutants bite.
   - **0qh — positioned multi-tenant backend (the keystone):** the
     `int21_file_backend_t` vtable made positioned (`open`=locate, `read_at` /
     `write_at` over the step-1/2 primitives); the whole-file buffer and all
     `g_buf_in_use`/`g_write_in_use` single-buffer logic deleted; `sft.h`
     `file_data` removed, `root_slot` added; `int21.c` READ/WRITE/LSEEK/CLOSE
     rewired; EXEC staging given its own `memory_map.h` region. Proven by the new
     `test-multiopen`: two files open concurrently with independent positions
     (no cross-talk) and a 96 KiB file read via LSEEK at offset 80000 (past the
     old cap). Host mock rewritten to the positioned vtable; mutants still bite.
   - **6hk — SFT teardown on EXIT:** `sft_close_process` releases an exiting
     process's FILE-kind SFT slots (device slots 0-3 preserved) on 4Ch/00h/INT 20h
     before the exit hook, so EXEC chains no longer leak handles. Proven by
     `test-exit-handles` (6 leaky EXEC runs x 4 opens = 24 > 16 slots, all succeed
     via reclaim); mutant (elide the close) exhausts the SFT at run 5 -> RED.

## 4. Oracles of Record (Law 2 — all re-run by the orchestrator, all green)

`make test` = **35 host + 15 emu = 50 gates**, green, **verified under 6-core CPU
load** (the condition that previously made the screendump gates flake). New gates
this period: `test-fat-partial(-mutant)`, `test-fat-write-partial(-mutant)`,
`test-multiopen`, `test-exit-handles(-mutant)`. Every new oracle is mutation-
proven (Rule 6).

## 5. Decisions / Frictions of Record

- **The SFT was already the handle table.** The clean design fell out of the
  existing code: the SFT entry already carried `file_offset` + a `dir_entry` copy,
  so multi-tenancy was not a "buffer pool" but "make the backend positioned and
  let each SFT slot be its own handle." Simpler than a pool and DOS-authentic.
- **Same-file coherence is out of scope (documented).** Two handles on the *same*
  file hold independent `dir_entry` copies and don't see each other's writes until
  reopened. Distinct files are fully independent — exactly InitechBase's need.
- **One shared cluster/sector scratch is safe today only because INT 21h is
  cooperative/single-threaded.** This is the seam that **initech-xk2** (reentrancy)
  must harden before any interrupt-context I/O.
- **A subagent mis-narrated provenance.** The 6hk agent claimed its artifact code
  "was already present from a prior session"; the git diff against the prior commit
  showed it was this step's work. Verified by git, not by the agent's prose (Rule 4).

## 6. Follow-ups raised

- **initech-vtk (P2):** `test-shell` keystroke-injection gate flakes only under
  pathological 12/12-core oversubscription (different mechanism from 3pe).
- **FAT / file-I/O fuzzer (P1, new):** we mandate + apply mutation-proving and FAT
  differential, but have **no generative fuzzing** for the file layer (the only
  property/fuzz mandates — region engine 6dy/M3, xBase gmo/M6 — are unbuilt and
  don't cover it). Filed a standing seeded-fuzzer-with-shrinking gate to close it.

## 7. Phase Disposition — NEXT AGENT START HERE

Tier-0 landmines: **P0-1 done, P0-2 done.** Remaining: **P0-3 = initech-xk2 (INT
21h reentrancy hardening)** — the last Tier-0 item, and the seam the multi-tenant
I/O now leans on. Then Tier-1 confidence: **initech-x0i** (tri-emulator: get
InitechDOS booting on Bochs, the emulator-ism detector — everything is QEMU-only)
and the **FAT/file-I/O fuzzer**. Structural M2 work (initech-509.2 SYSINIT, 509.7
CLOCK$, 4tw/509.8 Ctrl-C/critical-error) follows.

## 8. Verification of Record

At close of period `make test` is green at 50 gates on QEMU with
`triple_fault=0`, verified under CPU load; the FAT positioned read/write and the
multi-open path round-trip through real `mtools`. Bochs/86Box agreement
(initech-x0i) and INT 21h reentrancy (initech-xk2) are the principal outstanding
foundation debts.

---

*— End of Record —*

<!-- Tedium certified compliant with NFR-7. -->
