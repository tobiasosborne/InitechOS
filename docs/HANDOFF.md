<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# HANDOFF — Programme Continuity Briefing (InitechOS / STAPLER)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Continuity Briefing (living document; supersede in place)
**Last Reconciled:** 2026-06-13

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

The Programme is well past foundations: **InitechDOS boots from disk, prints its
banner, mounts a real FAT12 filesystem, and drops to an interactive COMMAND.COM
`A:\>` prompt** — on QEMU *and* Bochs. Everything below is verified by a
mechanical gate (re-run any time; see §4.1).

**Foundations / boot (M0–M1):**
- `tse`/`uba`/`znb` — repo + Makefile, toolchain, seed Pascal→x86 compiler.
- `f2s` — QEMU oracle harness (`harness/emu/`): serial, triple-fault detect (via
  `-d` log, not reset-count), live-guest QMP screendump, timeout. Now also `--disk2`.
- `f8v.1`/`f8v.2` — `make smoke` + the real boot chain (`os/boot/`): MBR → A20/GDT/
  protected-flat → VESA LFB.
- `dt9`/`a9w`/`slz` — closed (the tracer already did MBR-load / protected / VBE-LFB).
- `d00` — **stage2 → C kernel handoff**: captures the VGA ROM 8×16 font + a
  `boot_info` block, loads the flat C kernel from disk, far-jumps into it.
- `yqb` — **8×16 LFB text console** (`os/milton/console.c`): glyph blit (bpp 24/32),
  putc/puts/newline/scroll.
- `bea` — **the InitechDOS banner** prints (via `int 0x21` AH=09h), byte-exact vs
  `spec/dos_banner.txt`. (Open only for the *tri-emulator* clause → `x0i`.)

**DOS internals (M2 / `509.x`):**
- `8e7` — `bpb_t` locked into `spec/dos_structs.h`.
- `adf` — **FAT12 read** (`os/milton/fat12.c`): mount/BPB, 12-bit decode + chain
  walk (anti-hang), root-dir enumerate, file read. **ATA PIO backend (`ata.c`) now
  validated on the emulator.** `5cu` — FAT differential oracle vs mtools/python.
- `a5a` — **interrupt foundation** (`idt.c`/`isr.asm`/`pic.c`/`panic.c`): IDT,
  exception handlers → fail-loud panic (not triple-fault), 8259 remap to **0x28/0x30**
  + masked.
- `509.5` (partial) / `1f9` — **INT 21h dispatcher** (`int21.c`): literal `int 0x21`
  trap gate, AH dispatch, controlled scope; CON functions 02h/09h/40h/30h/4Ch.
  Calling convention **ratified as ADR-0003 amendment DEC-04a** (by a delegated ARB
  committee; `spec/int21h_calling_convention.json`).
- `509.4` — **PSP** 256-byte construction (`psp.c`). Program **loader** (`loader.c`):
  lays out PSP + image, runs a flat `.COM`, returns to the loader on 4Ch/INT 20h.
- `saw` — **FAT12 mount over ATA + proto-DIR** + FAT-sourced `.COM` load/EXEC.
  See WL-0007.

**M2-finale, file handles, shell, devices (WL-0008–WL-0013):**
- `509.3`/`509.5` — **SFT/JFT + file-handle INT 21h** (OPEN/READ/WRITE/CLOSE/
  LSEEK/FINDFIRST/NEXT, DUP/DUP2); `fileio_fat.c` binds the FAT backend; FAT12
  **write** path + multi-open. `509.8` — INT 22/23/24 + SETVECT/GETVECT.
- `3rs`/`n62` — **PS/2 keyboard (IRQ1) + CON input**; `yv9` — MC146818 RTC;
  `509.2` — **SYSINIT + CONFIG.SYS** (FILES= cap). `xk2` — INT 21h reentrancy
  under an IRQ storm. `509.1` — diagnostic-message catalogue.
- `7pc` — **COMMAND.COM REPL** (`command.c`): `$P$G` prompt, DIR/TYPE/CD/CLS/
  VER/ECHO/EXIT + external `.COM` EXEC, all via real `int 0x21`.

**Kernel hardening + tri-emulator boot (WL-0014–WL-0016, this push):**
- `bcg` — **kernel hardening**: wave-A/B robustness fixes, edge/error-path
  suite, FAT-corruption + CONFIG.SYS + cmdline fuzzers, reproducible-build gate,
  INT 21h user-pointer guards (ADR-0003 DEC-14). See WL-0014.
- `6pj` — **standard-VGA mode-0x13 fallback** (`stage2.asm` `.vga_fallback` +
  `console.c` 8bpp renderer + `kmain.c` DAC palette): Bochs has no VBE LFB, so
  stage2 falls back to mode 0x13 and the OS boots there. See WL-0015.
- `564` — **C Bochs oracle** (`harness/emu/bochs.{c,h}`) + `test-boot-bochs`:
  the dual-emulator boot differential (RFB unblock in C, serial assertion, no
  triple-fault), mutation-proven. See WL-0015.
- `k6x` — **COMMAND.COM is the DEFAULT boot**: the real boot drops to `A:\>`
  after the banner; baked PROGRAM/TYPE/DIR demos moved to `DEMO_IMG`. See
  WL-0016.

**Kernel hardening sweep (WL-0017, `initech-bcg.1..11`):** a grounded read-only
audit of all nine kernel subsystems (0 P0/P1 escaped; 20 P2 + 20 P3 confirmed,
26 rejected) drove **11 fixes**, each RED->GREEN, mutation-proven, committed:
all four P1 correctness bugs (RDWR-write denied `bcg.1`; AH=59h stale error
`bcg.2`; FAT12 out-of-range cluster `bcg.3`; FAT16 mis-decode `bcg.4`) plus the
fail-loud/wedge P2s (do_puts guard `bcg.5`; spurious-vector resume `bcg.6`;
8259A spurious-IRQ EOI `bcg.7`; bounded fail-loud serial `bcg.8`; CONFIG.SYS
honor-first-1KB `bcg.9`; loader -O2-safe entry jump `bcg.10`; console geometry
`bcg.11`). Two new emu gates (`test-spurious`, `test-sysinit-oversize`); new
error codes `FAT12_ERR_UNSUPPORTED`, `CONSOLE_ERR_GEOMETRY`. **Remaining bcg
children (all P2-infra / P3, NOT correctness bugs): `bcg.12` (ata error-path
oracles + BSY/DRDY-before-command), `bcg.13` (shell msg-catalogue scanner),
`bcg.14` (P3 robustness/test-gap sweep), `bcg.15` (8bpp DAC screendump oracle).**
A fresh session is the right home for these (esp. `bcg.12`'s delicate ATA
command-sequence change).

### 4.1 Gates that must stay green
`make test` = **55 host + 21 emu gates** (re-verified green this push; was 55+19
before WL-0017). Plus the
separate `make test-boot-bochs` (the Bochs boot leg; env-specific Bochs +
~45 s, NOT in the default `make test`). `make factory` builds; `make` prints
help. The default boot image (`build/tracer_boot.img`) is now the **shell**
kernel; the baked-demo gates (`test-program`/`test-type`/`test-dir`) boot
`build/demo_boot.img`; `test-fs` adds `--disk2 build/fat_data.img`.

### 4.2 See it
`qemu-system-i386 -drive format=raw,file=build/tracer_boot.img -drive
file=build/fat_data.img,format=raw,if=ide,index=1 -serial stdio` → banner + a
`Directory of A:\` listing + the **`A:\>` COMMAND.COM prompt** on the seafoam
desktop. (Under Bochs: `make test-boot-bochs` — same boot via the mode-0x13
fallback, asserted on serial.)

## 5. Branch state + next work (resume here)

**Branches (local-only repo — NO git remote, do NOT try to push):**
- `kernel-hardening` — WL-0014 (hardening/fuzzing) + WL-0015 (`6pj` mode-0x13
  fallback + `564` C Bochs gate). Unmerged.
- `command-com-default` — WL-0016 (`k6x` COMMAND.COM default boot), **stacked on
  `kernel-hardening`**. The current tip. Unmerged.
- Default branch is `master`. Merging these to `master` is a clean next step
  (the operator chose to keep building rather than merge so far).

**M2/M3 internals + the shell are DONE and green** (the stale "M3 in progress"
plan that lived here is superseded — see §4's WL-0008–WL-0016 lines). The DOS
personality boots to an interactive `A:\>` on QEMU and Bochs.

**Ready next work** (`bd ready`; each a distinct direction — pick per operator
steer):
- `26d` — **PS/2 mouse (IRQ12) + the canonical hourglass cursor** (Law 4 canon)
  → toward the interactive desktop / Toolbox (M3/M4 GUI).
- `kg5` — **Chicago + Geneva 9 bitmap strikes + text rendering** (frame fidelity).
- `44m`/`x0i` — **86Box leg** of the tri-emulator gate (lowest priority; its
  Qt-offscreen headless automation is unbuilt — a deep environment task).
- `h58` — cleanup: retire the now-redundant `SHELL_IMG`; add a shell-prompt
  screendump gate.
- `75r` — specs-as-data scaffold (foundational).

The controlled spec-data in `spec/` is the contract; the harness
(`build/qemu_harness`, `build/bochs_harness`) is the oracle.

## 6. Where things live

```
CLAUDE.md            how to work (Laws/Rules)
InitechOS-PRD.md     what to build
docs/adr/            ADR-0003 (DOS, authoritative), CDR-0001 (toolchain deviation)
docs/worklog/        WL-0001..0007 (foundations -> FAT mount); WL-0008+ file
                     handles/SFT, WL-0009 SYSINIT, WL-0010 multi-tenant IO,
                     WL-0011 reentrancy fuzzer, WL-0012 message catalogue,
                     WL-0013 vector cluster, WL-0014 kernel hardening + Bochs
                     diagnosis, WL-0015 Bochs standard-VGA fallback + C Bochs
                     gate, WL-0016 COMMAND.COM default boot (latest)
docs/research/       ground-truth briefs (fat12, boot-to-text, internals/int21h,
                     psp-loader, fs-mount-sft) -- the per-milestone evidence base
docs/HANDOFF.md      this briefing
harness/emu/         qemu.{c,h}+qemu_main.c (QEMU), bochs.{c,h}+bochs_main.c
                     (Bochs, initech-564), rfb_unblock.py (diagnosis only)
spec/                LOCKED spec-as-data: int21h_register.json, dos_structs.h,
                     dos_messages.json, dos_{banner,config_sys,autoexec_bat}.txt,
                     chrome_metrics.json, assets/ (palette/glyph work, deferred)
seed/                C seed Pascal->x86 compiler (= Turbo Initech genesis)
harness/             C factory: emu/ (QEMU oracle harness), factory_smoke.c
os/boot/             C+asm boot chain (MBR -> protected -> LFB -> C-kernel handoff)
os/milton/           THE KERNEL (C+asm): kstart/kernel.ld/kmain, console, idt/isr/
                     pic/panic, int21, psp, loader, fat12, ata, blockdev, boot_info,
                     test_*.c host oracles, test_program.asm (baked .COM)
os/{flair,samir,tps,apps}  the rest of the OS (C; tps/ will hold Turbo Initech)
Makefile             factory + gates; CC interim = host gcc -m32 -ffreestanding
build/               artifacts (gitignored)
```

Beads conventions: issues are `initech-*`; epics carry `m0`..`m8`/`m0.5`/`stretch` + `adr-0003` labels; M2 children are `509.x`. Vestigial-but-required structures carry the `vestigial` label and are implemented **in full** (design stance).

## 7. Gotchas (learned the hard way)

- **Oracle is truth, not the agent's report.** Re-run the gate yourself; verify subagent claims. Mutation-prove goldens (perturb → must go red → restore).
- **Stub honesty.** Gate/oracle Makefile targets exit non-zero when unimplemented; only action targets (image/run) exit 0 when stubbed.
- **Banner/message bytes are controlled vocabulary** (ADR-0003 DEC-13/App D): exact spacing (`InitechDOS  Version 3.30` — double space) is load-bearing and enforced by `test-spec`.
- **Triple-fault detection** keys on QEMU `-d` log strings, NOT `cpu_reset` count (SeaBIOS resets ~2×/boot).
- **Screendump needs a live guest** (race if the guest clean-exits fast; bead `initech-xcg`). The kernel guest hlt-loops, so it's fine.
- **Look at the screendump, don't just trust a green gate (Law 4).** A green `test-fs` once hid a directory listing that never rendered: a **dangling console pointer** (console declared in a nested block, used after scope by the proto-DIR). The screendump check false-passed on the banner alone. Fixed (hoist to function scope) + the oracle was strengthened (`ppm_text_check` now takes an optional `[y0 y1 min_fg]` band; `test-fs` asserts the DIR band). Lesson: in `kernel_main`, anything whose address escapes into a global (`g_int21_con`, `g_dir_con`, the panic console) must outlive every later use — keep it at function scope.
- **The DEC-04a vector map is load-bearing:** `int 0x21` is a TRAP gate at vector 0x21; the 8259 PIC is remapped to **master 0x28 / slave 0x30** (NOT the conventional 0x20/0x28) precisely so 0x21 stays free for the DOS syscall (else IRQ1/keyboard would collide). `int 0x20` (legacy terminate) lives at the now-free vector 0x20. See `docs/adr/ADR-0003-AMENDMENT-DEC-04a-*.md`.
- **`ata.c` first-run guards:** floating-bus (0xFF) = no-drive must return an error, never spin; BSY/DRQ polls are bounded (timeout). A missing `--disk2` makes mount fail-loud-and-continue (boots without a data disk still pass).
- **A review committee earns its keep:** the DEC-04a ARB review caught a real `do_getver` BH-mask bug the unit oracle had missed (then the oracle was made to bite it). Independent perspectives + mutation-proving > a single green pass.
- The reference frame still (`spec/assets/preview.webp`) is a **local-only reference fixture** (gitignored); derive palette/metrics from it, never embed it in committed source.
- Open follow-up beads worth knowing: `509.3`/`509.5` (next work, §5), `saw` (FAT-sourced load), `n62`/`3rs` (keyboard/CON input), `we2`/`xk2` (DEC-04a forward obligations: ring-3 DPL, INT 21h reentrancy), `dao` (fat12 on-stack chain buffer), `x0i` (tri-emulator), `6pm` (i686-elf), `79s` (ADR-0007), `xcg` (screendump race), `ta2` (M1 boot robustness).

---

*— End of Briefing —*

<!-- Tedium certified compliant with NFR-7. If you have received this briefing in error, please shred it and notify the Help Desk (ext. 2504). -->
