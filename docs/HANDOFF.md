<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# HANDOFF — Programme Continuity Briefing (InitechOS / STAPLER)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Continuity Briefing (living document; supersede in place)
**Last Reconciled:** 2026-06-08

> Incoming agent: read this top to bottom, then `CLAUDE.md`, then run `bd ready`. This briefing tells you *where the Programme stands and what to do next*; `CLAUDE.md` tells you *how to work*; the PRD and the ADRs tell you *what to build*.

---

## 1. Read order (do this first)

1. `CLAUDE.md` — the Laws & Rules (oracle-is-truth; fail loud; Red→Green; ASCII source; beads-only tracking).
2. `InitechOS-PRD.md` — the product spec (now reconciled to the ADRs).
3. `docs/adr/` — **ratified, authoritative decisions.** `ADR-0003` (InitechDOS, the active milestone) is the one to know cold. `CDR-0001` records the interim-toolchain deviation. ADRs **govern**; where the PRD/CLAUDE.md ever diverge, the ADR wins and the divergence is reconciled.
4. This briefing (current state + next steps).
5. `bd ready` / `bd show <id>` — the live work queue. Run `bd prime` for the tracker workflow.

## 2. What the Programme is

A bootable, period-plausible OS for emulated 386+ PCs — a DOS-3.3 personality (`MILTON`) under a System-7-style Toolbox (`FLAIR`), reproducing the *Office Space* "Saving tables to disk…" frame, with a dBASE-alike that really runs and a Pascal self-hosting compiler (`Turbo Initech`) as the finale. Built by an agent swarm whose fitness signal is the emulator itself.

**Design stance (governs every naming/structure decision):** the blandness is deliberate and rigorous. Keep every canonical name and every vestigial structure, in full, with a straight face. InitechDOS is not a parody of DOS — it is DOS with the soul extracted and the legacy lovingly preserved. Corporate software accretes and never deletes.

## 3. Binding decisions in force

| Decision | Ruling | Source |
|---|---|---|
| **First deliverable** | **InitechDOS** (M2, codename MILTON). Toolbox/GUI (M3/M4) deferred behind it. | operator + ADR-0003 |
| **OS implementation language** | **C** (kernel, InitechDOS, Toolbox, bundled apps for now). | ADR-0002 / PNC-1 |
| **Pascal** | Reserved for **Turbo Initech** (self-host compiler, ADR-0007 *pending*) and programs it compiles. The seed compiler (`seed/`) is its genesis, NOT the OS bootstrap. | PNC-1 |
| **Self-host fixpoint** | Concerns Turbo Initech (`K₂==K₃`), not the C kernel (which the factory rebuilds). | PRD §7 |
| **Toolchain** | Target `i686-elf` (ADR-0002). **Interim: host `gcc -m32 -ffreestanding -nostdlib` + `nasm` + `ld`** until dev moves to a more capable device. | CDR-0001 |
| **Executable format** | Flat binary kernel; flat `.COM`-equivalent apps; **MZ `.EXE` deferred**. | ADR-0003 DEC-08 |
| **Documents** | All new docs in enterprise corporate-committee ("Initech") house style (NFR-7). | operator |
| **Tracking** | `bd` (beads) only; `bd remember` for persistent knowledge. No TodoWrite/markdown TODOs. No GitHub CI. | CLAUDE.md |

## 4. What is built and green (do not redo)

Six beads closed; all verified by mechanical gates. The factory and a thin tracer thread already run end-to-end:

- `tse` — repo skeleton + self-documenting `Makefile` (two-tier stubs: action stubs exit 0; **gate stubs fail loud**).
- `uba` — QEMU 8.2.2 / nasm / bochs provisioned.
- `znb` — seed C-hosted Pascal→x86 compiler (lexer/parser/AST + stack-machine codegen). *Re-cast as Turbo Initech's genesis.*
- `f2s` — QEMU oracle harness (`harness/emu/`): serial capture, triple-fault detect (via `-d` log, not reset-count), QMP screendump of a live guest, wall-clock timeout. CLI `build/qemu_harness`.
- `f8v.1` — `make smoke`: seed-built ELF boots, emits a serial marker, CI exit code.
- `f8v.2` — real boot chain (`os/boot/mbr.asm` + `stage2.asm`): MBR → A20/GDT/protected-flat → VESA LFB → seafoam fill, pixel-verified.

**Gates that must stay green** (run any time): `make test-spec test-seed test-seed-codegen test-harness test-tracer-boot smoke`. (`make factory` builds everything; `make` alone prints help.)

## 5. Next work — the InitechDOS vertical slice

The M0.5 tracer is now an **InitechDOS slice**: boot → `InitechDOS  Version 3.30` banner → `COMMAND.COM` → `DIR`/`TYPE` over a FAT volume → serial markers + ECH differential (`f8v.4` keystone). All of it is buildable now with the interim host toolchain. Recommended order (all unblocked or near-front):

1. `509.1` — diagnostic-message-catalogue enforcement (P0; land **before** any message-emitting kernel code — messages come from `spec/dos_messages.json` only).
2. `509.2` — `IO.SYS` + `INITDOS.SYS` two-file kernel + SYSINIT (CONFIG.SYS parsing).
3. `adf` — ATA PIO + FAT12 read; then `509.11` (FAT write + byte-identity oracle).
4. `bea` — correct banner to the byte-exact Appendix D.1 text (`spec/dos_banner.txt`).
5. `7pc` — `COMMAND.COM` (`DIR`/`TYPE`/`CD` + `$P$G`), then the rest of `509.x` (PSP, JFT/SFT, INT 21h dispatcher, MCB, devices, handlers, FCB).
6. `f8v.3` → `f8v.4` — compose the slice; keystone green unblocks M3+ (GUI/db/compiler).

The controlled spec-data in `spec/` is the contract for all of the above; the ECH (`build/qemu_harness`) is the oracle (boot a raw image with `--disk`).

## 6. Where things live

```
CLAUDE.md            how to work (Laws/Rules)
InitechOS-PRD.md     what to build
docs/adr/            ADR-0003 (DOS, authoritative), CDR-0001 (toolchain deviation)
docs/worklog/        WL-0001 (foundations), WL-0002 (this reconciliation)
docs/HANDOFF.md      this briefing
spec/                LOCKED spec-as-data: int21h_register.json, dos_structs.h,
                     dos_messages.json, dos_{banner,config_sys,autoexec_bat}.txt,
                     chrome_metrics.json, assets/ (palette/glyph work, deferred)
seed/                C seed Pascal->x86 compiler (= Turbo Initech genesis)
harness/             C factory: emu/ (QEMU oracle harness), factory_smoke.c
os/boot/             C+asm boot chain (MBR -> protected -> LFB)
os/{milton,flair,samir,tps,apps}  the OS (C; tps/ will hold Pascal/Turbo Initech)
Makefile             factory + gates; CC interim = host gcc -m32 -ffreestanding
build/               artifacts (gitignored)
```

Beads conventions: issues are `initech-*`; epics carry `m0`..`m8`/`m0.5`/`stretch` + `adr-0003` labels; M2 children are `509.x`. Vestigial-but-required structures carry the `vestigial` label and are implemented **in full** (design stance).

## 7. Gotchas (learned the hard way)

- **Oracle is truth, not the agent's report.** Re-run the gate yourself; verify subagent claims. Mutation-prove goldens (perturb → must go red → restore).
- **Stub honesty.** Gate/oracle Makefile targets exit non-zero when unimplemented; only action targets (image/run) exit 0 when stubbed.
- **Banner/message bytes are controlled vocabulary** (ADR-0003 DEC-13/App D): exact spacing (`InitechDOS  Version 3.30` — double space) is load-bearing and enforced by `test-spec`.
- **Triple-fault detection** keys on QEMU `-d` log strings, NOT `cpu_reset` count (SeaBIOS resets ~2×/boot).
- **Screendump needs a live guest** (race if the guest clean-exits fast; bead `initech-xcg`). The DOS keystone guest loops, so it's fine.
- The reference frame still (`spec/assets/preview.webp`) is a **local-only reference fixture** (gitignored); derive palette/metrics from it, never embed it in committed source.
- Open follow-up beads worth knowing: `6pm` (i686-elf, deferred), `79s` (author ADR-0007), `xcg` (screendump race), `7r0` (michael_bolton.conf spec), `ta2` (M1 boot robustness).

---

*— End of Briefing —*

<!-- Tedium certified compliant with NFR-7. If you have received this briefing in error, please shred it and notify the Help Desk (ext. 2504). -->
