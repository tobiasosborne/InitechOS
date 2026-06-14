# InitechOS

> *codename **STAPLER***
>
> A bootable, period-plausible operating system for emulated 386+ PCs — the
> exact chimera from the *Office Space* "Saving tables to disk…" frame: a
> DOS-3.3 personality fused with a System-7-style Toolbox. It really boots,
> the windows really work, the bundled database really runs, and the north
> star is a Borland-style Pascal compiler that recompiles itself from inside
> the OS.

InitechOS is not a parody of an old operating system. It is an old operating
system — built from scratch, to period-authentic standards, with a straight
face. The blandness is deliberate and rigorous: every canonical name and every
vestigial structure is preserved, in full, because corporate software accretes
and never deletes.

---

## What it is

The product recreates a single frame of early-90s desktop software — a Macintosh
System 7 window arrangement floating over a DOS session — and makes it real:

| Personality | Codename | What it is | Language |
|---|---|---|---|
| **InitechDOS** | `MILTON` | A DOS 3.3-personality kernel: FAT filesystem, INT 21h, program loader, `COMMAND.COM` | C |
| **The Toolbox** | `FLAIR` | A System-7-style GUI: Window/Menu/Control/Event/Dialog managers + a real region engine | C |
| **InitechBase** | `SAMIR` | A dBASE-compatible database that round-trips real `.dbf`/`.mdx` files | C |
| **Turbo Initech** | `TPS` | The resident, self-hosting Pascal compiler + blue IDE (the in-universe finale) | Pascal |

It targets the 386+ in 32-bit flat mode, boots on real-era-plausible hardware
(verified across **QEMU**, **Bochs**, and **86Box**), and is built to *run on
real iron of the era*, not merely to look the part.

## Status

InitechDOS **boots from disk, prints its banner, mounts a real FAT12 volume,
and drops to an interactive `A:\>` `COMMAND.COM` prompt** — on QEMU *and*
Bochs.

- **Boot:** MBR → stage2 → A20/GDT → 32-bit protected-flat → VESA LFB (with a
  mode-0x13 fallback for Bochs). 8×16 framebuffer text console.
- **Kernel:** a fail-loud interrupt foundation (IDT, CPU-exception handlers → a
  `PC LOAD LETTER` panic), the 8259 PIC remapped so `int 0x21` stays free as the
  DOS syscall gate, PS/2 keyboard, PIT tick, MC146818 RTC, `CONFIG.SYS` parsing.
- **INT 21h:** **39 functions** dispatched — console I/O, the file-handle API
  (OPEN/CREAT/READ/WRITE/CLOSE/LSEEK/DUP/DUP2), FINDFIRST/FINDNEXT, EXEC, date/
  time, memory-map free-space, interrupt vectors, extended error.
- **Filesystem:** FAT12 read **and** write over an ATA PIO block layer, a real
  SFT/JFT handle table, multi-open.
- **Shell:** `COMMAND.COM` with `DIR`, `TYPE`, `CD`, `CLS`, `VER`, `ECHO`,
  `EXIT`, and external `.COM` execution.

In progress / planned: hierarchical directories, the rest of the DOS-3.3 INT 21h
surface and core utilities, the FLAIR Toolbox and desktop, InitechBase, and the
Turbo Initech self-host. See **Roadmap** below.

## The governing idea: the oracle is the truth

Every subsystem has a **mechanical oracle**, and nothing ships on "looks right."
The emulator is the fitness signal.

- The FAT driver is diffed against `mtools`/Python on identical images.
- The region engine is a property test: `rasterize(A ∪ B) == rasterize(A) ∪ rasterize(B)`
  over thousands of random regions, with shrinking.
- InitechBase round-trips through real dBASE.
- Turbo Initech diffs clean against Free Pascal on a shared corpus, and the
  self-host certificate is a bit-for-bit fixed point (`K₂ == K₃`).
- Boot milestones run **differentially across three emulators**.

Golden files are *mutation-proven*: a golden that has never caught a regression
is decoration.

And the canon is enforced, not fixed: the InitechCalc pie chart sums to **116 %**
(`40+35+18+14+9`), the cursor is an hourglass (not a wristwatch), and the ledger
shows a `570-` trailing-minus. These are the spec, not bugs.

## Build & run

Requires a Linux host with the emulators and a freestanding toolchain:

```bash
sudo apt install qemu-system-i386 bochs make nasm mtools
# Target toolchain is i686-elf; the interim toolchain is the host
# gcc -m32 -ffreestanding -nostdlib + nasm + ld.
```

```bash
make image        # build a bootable image -> build/initech.img
make run          # boot in QEMU with serial + gdb stub + screendump wired
make run-bochs    # boot in Bochs (real->protected transition checking)
make test         # the whole gate vector (host oracles + emulator harness)
```

See `qemu-system-i386 -drive format=raw,file=build/tracer_boot.img …` in
[`docs/HANDOFF.md`](docs/HANDOFF.md) for the exact incantation that lands you at
the `A:\>` prompt on the seafoam desktop.

## Repository layout

```
CLAUDE.md            how to work (the Laws & Rules)
InitechOS-PRD.md     what to build (the product spec)
docs/adr/            ratified architecture decisions (ADR-0001..0003, CDR-0001)
docs/worklog/        sharded session log
spec/                LOCKED specs-as-data (JSON / C headers): the contracts
seed/                C seed Pascal->x86 compiler (genesis of Turbo Initech)
harness/             C factory: emulator oracle drivers, SSIM, differential diffs
os/                  THE ARTIFACT (C)
  ├── boot/          MBR -> stage2 -> 32-bit protected/flat
  ├── milton/        InitechDOS kernel (FAT, INT 21h, loader, shell)
  ├── flair/         the Toolbox (Window/Menu/Control/Event/Dialog)
  │   └── atkinson/  the region engine — the load-bearing math
  ├── samir/         InitechBase (dBASE-alike)
  ├── apps/          InitechCalc, File Manager, InitechPaint, FILE COPY
  └── tps/           Turbo Initech (resident compiler + blue IDE) — Pascal
fixtures/            the reference frame still(s), golden files
```

**Two systems coexist:** the **artifact** (the OS itself — C, plus Pascal for
Turbo Initech) and the **factory** (everything that builds and *judges* the
artifact — C only). The factory is what makes the artifact trustworthy.

## Architecture

The ratified decisions live in [`docs/adr/`](docs/adr/):

- **ADR-0001** — 386+, 32-bit flat memory model.
- **ADR-0002** — toolchain, implementation language (C), executable format.
- **ADR-0003** — InitechDOS as the base OS (the active milestone), with its
  INT 21h scope (Appendix A) as a locked contract.
- **CDR-0001** — the interim host-toolchain deviation (time-limited).

Reproducible builds are a hard requirement — deterministic codegen, no
timestamps in artifacts — because the self-host fixed point depends on it.

## Roadmap

| Milestone | What |
|---|---|
| M0 / M0.5 / M1 | Foundations, tracer bullet, boot to text ✓ |
| **M2 — `MILTON`** | **The DOS personality (current): FAT, INT 21h, loader, shell** |
| M3 / M4 — `ATKINSON` / `FLAIR` | Region engine + the Toolbox & desktop chrome |
| M5 | Desktop apps + the full reference frame |
| M6 — `SAMIR` | InitechBase: a dBASE-alike that really works |
| M7 / M8 — `TPS` | Turbo Initech, then the resident IDE + self-host finale |

Work is tracked in [beads](https://github.com/steveyegge/beads) (`bd ready`,
`bd show <id>`); design rationale lands in `docs/worklog/`.

## Contributing & house rules

Read [`CLAUDE.md`](CLAUDE.md) top to bottom — it encodes the four Laws
(ground-truth-before-code; the oracle is truth; never blur artifact and factory;
it must look and feel like the frame) and the numbered Rules. The short version:
ground every change in a local source, write the failing oracle first, fix root
causes, mutation-prove your goldens, and keep shipped source ASCII-clean and
period-authentic. There is no remote CI by design — the gates run locally.

## License

InitechOS is licensed under the **GNU Affero General Public License v3.0**
(AGPL-3.0) — see [`LICENSE`](LICENSE). The AGPL's network-use clause means that
if you run a modified version as a network service, you must offer its users the
corresponding source. Fitting, for an operating system about saving tables to
disk.

---

<sub>Built with the help of agentic tooling. *Office Space* and its imagery are
the property of their respective rights holders; InitechOS is an independent,
non-commercial homage.</sub>
