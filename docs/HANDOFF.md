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

The Programme is well past foundations: **InitechDOS boots from disk, prints its
banner, runs a program through `int 0x21`, and mounts a real FAT12 filesystem.**
Everything below is verified by a mechanical gate (re-run any time; see §4.1).

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
- `saw` (partial) — **FAT12 mount over ATA + proto-DIR** (a directory listing on
  screen). See WL-0007.

### 4.1 Gates that must stay green
`make test-spec test-fat test-idt test-int21 test-psp test-loader test-console
test-fs test-boot test-program test-panic test-tracer-boot test-harness test-seed
test-seed-codegen` (plus the `*-mutant` checks). `make factory` builds; `make`
prints help. `test-fs`/`test-program`/`test-boot` boot the kernel image via the
harness; `test-fs` adds `--disk2 build/fat_data.img`.

### 4.2 See it
`qemu-system-i386 -drive format=raw,file=build/tracer_boot.img -drive
file=build/fat_data.img,format=raw,if=ide,index=1 -serial stdio` → banner + a
loaded-program line + a `Directory of A:\` listing on the seafoam desktop.

## 5. Next work — Milestone 3 is IN PROGRESS (resume here)

The internals roadmap (operator order: internals → shell → rest). Milestone 3 =
the file-system handle layer, 5 steps; **Steps 1–2 done, resume at Step 3** (full
detail + the plan in `WL-0007` §5 and `docs/research/fs-mount-sft-ground-truth.md`):

1. **`509.3` — SFT + JFT + standard handles + DUP/DUP2 (NEXT).** System File Table
   + the per-process Job File Table indirection (DEC-06); pre-open handles 0–4 to
   CON/AUX/PRN; DUP (45h)/DUP2 (46h); route 40h WRITE through JFT→SFT.
2. **`509.5` read-side — file-handle INT 21h functions.** 3Dh OPEN / 3Fh READ /
   3Eh CLOSE / 42h LSEEK over the mounted FAT12 via the SFT; 4Eh/4Fh FINDFIRST/
   FINDNEXT into the DTA. A baked program OPENs + READs + WRITEs a file (a `TYPE`)
   through `int 0x21`. *Read the file into a static buffer at OPEN — not the stack
   (`dao`).*
3. Verify + reconcile + WL-0008.

After M3: `7pc` COMMAND.COM (gated on CON input `n62` / keyboard `3rs`); `saw`
(load a `.COM` from the FAT volume vs the baked blob); then the `f8v.4` keystone
(boot → banner → COMMAND.COM → DIR → TYPE). The deferred two-file kernel partition
`509.2` (IO.SYS/INITDOS.SYS + SYSINIT) and `509.1` (message catalogue) remain M2 work.

The controlled spec-data in `spec/` is the contract; the harness
(`build/qemu_harness`) is the oracle.

## 6. Where things live

```
CLAUDE.md            how to work (Laws/Rules)
InitechOS-PRD.md     what to build
docs/adr/            ADR-0003 (DOS, authoritative), CDR-0001 (toolchain deviation)
docs/worklog/        WL-0001 foundations, WL-0002 ADR-0003 reconciliation,
                     WL-0003 FAT12 read+oracle, WL-0004 boot-to-text banner,
                     WL-0005 interrupt foundation+INT21h, WL-0006 PSP+loader+DEC-04a,
                     WL-0007 FAT mount over ATA+proto-DIR (M3 in progress)
docs/research/       ground-truth briefs (fat12, boot-to-text, internals/int21h,
                     psp-loader, fs-mount-sft) -- the per-milestone evidence base
docs/HANDOFF.md      this briefing
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
