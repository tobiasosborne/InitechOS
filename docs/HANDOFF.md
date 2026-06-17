<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# HANDOFF — Programme Continuity Briefing (InitechOS / STAPLER)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Continuity Briefing (living document; supersede in place)
**Last Reconciled:** 2026-06-17

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

**The recursive joke (operator, 2026-06-14; see `bd memories`):** at first glance
InitechOS must be indistinguishable from a real early-90s corporate OS — the
presentation layer (README, manuals, box, the UI) NEVER admits the joke. The
reveal is layered: it looks like a vibe-coded AI toy, then it REALLY boots on
386-era hardware, then the software REALLY works — with deadpan absurdities played
straight (the 116% pie chart, the Y2K accounting system, Michael Bolton's
rounding-error virus, the TPS Report Generator that needs the vestigial FCB API).
Only the FINAL build (physical 5.25" floppies + period manuals/box,
`packaging-epic`) must be completely straight; the DEV JOURNEY is intentionally
transparent (the public repo + AI history are part of the reveal) and should
increasingly read as a found-footage CLEAN-ROOM reconstruction. ADR-by-committee
(subagent role-play) is for BIG features only.

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
`make test` = **124 host + 27 emu gates** (WL-0030 added +20 host: the SAMIR `.dbf` codec +
expression-engine + `.ndx`-parse oracles -- `test-dbf-{header,fields,read,roundtrip,mutate}`,
`test-xbase-{lex,parse,eval,coercion}`, `test-ndx-parse`, each x unit+mutant; re-verified
`make clean && make test-unit` = ALL GREEN 124 host; SAMIR is still host-only so the 27 emu gates
are unchanged and `test-dbase` stays a milestone stub_fail until the M6 differential at S6.3/S6.4).
Prior: WL-0029 added +1 host: `test-samir`, the
SAMIR/M6 foundation umbrella -- pal/rt/pal-host/spec/value + the dbf_ref/ndx_ref Tier-1
gates; re-verified `make clean && make test-unit` = ALL GREEN 104 host. SAMIR is host-only,
not yet in the boot image, so the 27 emu gates are unchanged. `test-dbase` stays a milestone
stub_fail -- the M6 differential lands at S6.3/S6.4). Prior: WL-0028 added +10 host: the forward
tranche `80k`/`d27i`/`x8fs`/`er3h`/`4tw` x2 -- DOS 8.3 wildcard oracle,
windowed FAT16 mount, AH=3Fh cooked CON read, AH=33h Get/Set BREAK + ^C/INT 23h
check-point. Adversarial review caught + fixed a DEC-16 deviation (er3h SET
wrote AL); a host-oracle HANG (x8fs cooked read on the no-source sentinel) was
caught only by the FULL aggregate gate and root-fixed -- ALWAYS run
`make clean && timeout 1200 make test` at integration. WL-0027 added the FAT16 milestone
(dao streaming walk + z01 FAT16 decode): +3 host = test-fat-readfile-mutant +
test-fat16 + test-fat16-mutant, re-verified from clean;
WL-0026 discharged the WL-0025 follow-up debt -- +5 host (`test-nmpo`,
`test-fat-fault-rollback`(+mutant), `test-4nbn-mutant`, `test-nmpo-mutant`) +1 emu
(`test-absdisk-emu`), plus the test-gnrc non-root rename leg, the b53d MUTANT 8
dispatch-edge CX-reject, and the test-spec error_codes completeness assertion;
WL-0025 added the
INT 21h parity tranche gates (`test-kji0/qekc/b53d/gnrc-mutant`,
`test-m0bp-rollback`(+mutant), `test-absdisk`(+mutant), `test-spec[6/6]`) for
CREATNEW/FILETIME/CHMOD/RENAME/IOCTL 5Bh/57h/43h/56h/44h + INT 25h/26h; WL-0024 added the
EMU `test-ut6d`(+mutant, 2 legs), `test-zs24-exec`(+mutant) and the host/diff
`test-zs24`(+mutant, 5 legs) for shell MD/RD/CD + subdir file WRITE + subdir
EXEC, and amended the DOS catalogue 16->19; WL-0023 added `test-u6wa-mutant` +
`test-fat12-mkdir`(+mutant) for CHDIR/MKDIR/RMDIR; WL-0022 added
`test-mzxa-integration` + `test-mzxa-mutant` for ti8 L2; WL-0021 added
`test-fat-subdir`(+mutant) + `test-region`(+mutant); was 59+22 after WL-0019).
**Env gotcha (`bd memories`):** if you see "Clock skew detected", `make clean`
before trusting an incremental oracle -- future-dated build/ artifacts make
`make` skip rebuilds (false greens). Authoritative re-verify = `make clean &&
make test`.
**`make test-boot-bochs` PASS** with `KERNEL_SECTORS=160` (4tw grew it 144->160 in
WL-0028 as the kernel grew; qekc grew it 128->144 in WL-0025; `IMG_MIN`=1+16+160=177
<= `IMG_SECTORS=192` = 3 whole cylinders, no IMG change; a boot-geometry change is a
tri-emulator obligation, Rule 5 -- re-verified on Bochs in WL-0028). The boot image is padded to a whole 2x32 cylinder geometry
(`IMG_SECTORS=192`, build-guarded) so the **Bochs boot leg passes**. Plus the
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

**Branches + remote (a remote now exists — this supersedes the old "local-only,
do not push" note):**
- **`origin` = github.com/tobiasosborne/InitechOS** — PUBLIC, AGPL-3.0. The
  default/showcase branch on GitHub is **`main`**; `command-com-default` is the
  active working branch. BOTH are pushed. Session-close now pushes to `origin`.
- `command-com-default` — the active tip: WL-0016 + WL-0018 + WL-0019 (509.6
  wiring + Bochs geometry fix + the kernel-completeness plan). `main` == HEAD.
- `kernel-hardening`, `master` — older local branches, linear ancestors of the
  tip; left as-is.

**M2/M3 internals + the shell are DONE and green** (the stale "M3 in progress"
plan that lived here is superseded — see §4's WL-0008–WL-0016 lines). The DOS
personality boots to an interactive `A:\>` on QEMU and Bochs.

**ACTIVE WORKSTREAM — DOS 3.3 feature-parity push (epic `initech-bsy`, WL-0018).**
The operator directed a sustained push to DOS 3.3-5.0 parity (spirit, not literal,
where it conflicts with the north star), oriented around the existing beads. A
grounded gap-map established that within ADR-0003 Appendix A the real INT 21h gap
is `39h/3Ah/3Bh` MKDIR/RMDIR/CHDIR, `43h` CHMOD, `44h` IOCTL, `48h/49h/4Ah`
ALLOC/FREE/SETBLOCK, `56h` RENAME, `57h` FILETIME, `5Bh` CREATNEW (all recognized
by `ah_is_listed()` but NOT dispatched), plus the shell built-ins + batch + env.
`bd show initech-bsy` carries the full sequenced build order (Tranches A-I).
**Landed (WL-0019):** `initech-509.6` is DONE — AH=48/49/4Ah wired to the MCB
arena (authentic single-big-block over the locked [0x30000,0x70000) window, NO
spec edit) + the Bochs `IMG_SECTORS` geometry fix. The kernel-completeness gap is
now a **40-bead plan**: Phase 1 (kernel feature-complete, children of
`initech-bsy`, capstoned by **`initech-40oq`** the coverage certificate), Phase 2
(shell built-ins + the new `util-epic`), Phase 4 (the parked `dos5-epic`,
ADR-amendment-gated). FCB (`509.9`) backburnered (P4) but REQUIRED; its flagship
consumer is the **TPS Report Generator** (`8479.1`). Canon beads: Y2K accounting
(`586.1`), Michael Bolton's rounding-error virus (`586.2`), the `packaging-epic`
(the final straight build).
**`initech-ti8` Layer 1 is DONE + green (WL-0021)** — the additive fat12 layer
(`fat12_dir_t` + `fat12_read_dir` + `fat12_resolve_path`, READ-side; 4 root
primitives byte-unchanged; `test-fat-subdir` 3-way differential + 2 mutants).
Grounding split ti8 into L1 (fat12, done) and **`initech-mzxa`** = Layer 2, the
Core-tier INT 21h `int21_file_backend_t` vtable cross-cut (root-only today;
threading a resolved dir touches int21.h + fileio_fat.c + 3 host mocks + g_cwd +
5 rejection sites: do_open@858 / do_creat@931 / do_unlink@995 / do_findfirst@1405
/ do_exec@1476). **`initech-mzxa` (Layer 2) is now DONE + green (WL-0022)** —
the vtable threads a `uint16 dir_start_cluster` (0==root, root path byte-identical)
through `int21_file_backend_t`; `do_open`/`creat`/`unlink`/`findfirst` resolve
`\SUB\FILE`; `g_cwd` plumbing established (reset on launch/terminate, save/restore
around the kernel PSP rebinds, `do_getcwd` wired); the strongest oracle binds the
REAL `fileio_fat` backend over `fat12_nested.img` (45 checks). DOS-correct codes
(0x0002 missing leaf vs 0x0003 missing/non-dir component).

**`initech-u6wa` is DONE + green (WL-0023)** — CHDIR (3Bh) + root-level MKDIR
(39h)/RMDIR (3Ah). CHDIR added a `resolve_dir` backend member + fixed a latent
bug (`fat_resolve` ignored `cwd_start`, so relative resolution was root-anchored);
MKDIR/RMDIR added `fat12_mkdir`/`fat12_rmdir` + a dot-dir writer, the `..`=parent/0
convention pinned EMPIRICALLY from `mmd` and gated by an `mmd` differential. DOS
codes: MKDIR-exists/RMDIR-non-empty 0x0005, RMDIR-of-CWD 0x0010 (new
`INT21_ERR_CURRENT_DIR`).

**`initech-ut6d` + `initech-zs24` are DONE + green (WL-0024)** — the
subdirectory personality is now usable END-TO-END. `ut6d` (`ea4b47d`) wired the
COMMAND.COM REPL to AH=39/3A/3B + a GETCWD-composed `$P$G` prompt ("A:\SUB>"),
behind a new DEC-13 catalogue amendment (`mc7r`/`9e2238b`: MSG-DOS-0017/0018/0019,
catalogue 16->19). `zs24` lifted the ROOT-only write/EXEC restriction in two
serial landings: **L1** (`610c656`) subdir file CREATE/WRITE/UNLINK — the
parent-aware fat12 core (`fat12_scan_dir`/`_write_dirent_in_dir`/
`_subdir_slot_lba`/`fat12_grow_dir`), root fns now byte-identical is_root=1
wrappers, SFT carries `dir_start`, fail-loud `spc!=1` mount guard; **L2**
(`b67028b`) subdir EXEC — `do_exec` reuses the `do_open` resolve seam,
`load_program_from_fat` branches dir_start==0 (byte-identical `fat12_find`) /
!=0 (`fat12_find_slot_in`), with a dir-attr guard. Each landing was self-run
`make test`-gated AND independently adversarially verified before commit; the
adversarial pass caught `fat12_grow_dir` shipping untested despite a green suite
(closed with a grow oracle + 2 grow mutants).

**Resume the DOS-3.3 parity push (epic `initech-bsy`, 8/22): `bd ready`.**
**WL-0025 completed the Appendix-A INT 21h handler tranche** — CREATNEW (5Bh),
FILETIME (57h), CHMOD (43h), RENAME (56h), IOCTL (44h AL=00), nested MKDIR/RMDIR
— PLUS **ADR-0003 Amendment DEC-15** ratifying the absolute-disk vectors and
**INT 25h/26h** (`4mq7`) implemented against it. The Appendix-A INT 21h surface
is now fully dispatched. Filed follow-ups (none blocking): `4nbn` (IOCTL minors +
device-info bit-name labels), `5o6o` (CHMOD SET-reject deviation, doc), `isil`
(gnrc non-root same-dir rename leg), `ycb3` (cross-dir RENAME move), `nmpo`/
`glsw` (CREATNEW oracle/robustness), `lpf3` (write-fail fault-injection backend
for the rollback paths), `cnvp` (absdisk spec bad-buffer row), `8403` (in-emu
int 25h/26h self-test), `t1on` (DEC-07 MBR-partition amendment, blocking the
deferred `kzfs`). The natural Phase-1 closer is the capstone `initech-40oq`
(Appendix-A coverage certificate). The SERIAL, oracle-gated discipline for
shared-file kernel edits still stands (Rule 3 / WL-0023-25).

**WL-0026 discharged the WL-0025 follow-up debt** (orchestrated: a serial coding
workflow + parallel adversarial verifiers + a forward-grounding workflow). Landed
green (90 host + 27 emu): `nmpo`+`glsw` (CREATNEW FS-effect oracle + e2e FAT12
differential + fail-loud open==NULL), `4nbn` (IOCTL AH=44h minors AL=01/06/07/08 +
PRM bit-label fix), `lpf3` (fault-injecting blockdev backend driving the fat12
rollback paths RED), `8403` (in-emulator int 25h/26h asm-path self-test), `cnvp`
(DEC-15 bad-buffer spec row + completeness gate), `isil` (non-root RENAME leg),
`5o6o` (CHMOD SET-reject deviation doc). Adversarial review caught + fixed three
Rule-6 "decoration" findings on the green suite (dead completeness branch, a
phantom-mutant ADR citation, an unmutated headline gate). New P3 follow-ups (none
blocking): `fgdz` (8403 emu serial-capture race -- green serially, flaky under
concurrent QEMU relaunch), `jplu` (4nbn AL=08 BL=drive fidelity), `aaan` (lpf3
mkdir-EOC-fail leg coverage), `815i` (nmpo host leg-(e) mock artifact).

**NEXT FORWARD TRANCHE (pre-grounded in WL-0026, dependency-ordered, another
SERIAL kernel lane):** `dao` (streaming cluster walk; kills the on-stack
chain[2880]; unblocks z01) -> `z01` (FAT16 read-only; initrd/tarfs deferred to a
discovery bead) -> `80k` (DOS 8.3 wildcard engine) -> `x8fs` (cooked CON
line-read; MUST precede `4tw`, shared conin_get_pb seam) -> `4tw`+`er3h` (Ctrl-C/
INT 23h + BREAK=) -> `mvg` (wire INT 24h to real ATA/FAT errors; consumes lpf3's
fault backend). **TWO operator gates (ADR-by-committee draft -> ratify, per the
DEC-15 pattern):** (1) **AH=33h** Get/Set-BREAK is NOT in the locked
`spec/int21h_register.json` / Appendix A -- `4tw`/`er3h` need it added (Rule 8
amendment); (2) **DEC-07 / `initech-t1on`** must rule on the `kzfs` MBR-partition
forks (start-LBA authority: partition-table vs BPB hidden_sectors; application
layer: mount vs blockdev; detection heuristic: BIOS-drive-number vs sector-0
byte-sniff) before `kzfs` can be implemented. Full grounding briefs (file-touch
maps, oracle + mutation plans) captured in the WL-0026 orchestration output.

**WL-0027 landed the FAT16 milestone + ratified both ADR gates.** `dao` (streaming
cluster walk -- killed the on-stack `chain[2880]`) and `z01` (FAT16 read DECODE
layer + a 3-way host differential `test-fat16`, FAT12 path byte-identical) are
green (93 host + 27 emu). **z01 is a PARTIAL delivery of its bead** (honest): the
decode is proven on the host, but the kernel cannot mount a real FAT16 volume yet
because the whole-FAT `g_fat[12*512]` buffer is too small (fails loud) -- the
windowed FAT-sector read is filed as **`initech-d27i`** (P1, now a `40oq` dep) and
is the true completion of FAT16. The operator ratified **DEC-16** (AH=33h -> Appendix
A; unblocks `4tw`/`er3h`) and **DEC-07a** (MBR partition contract; unblocks `kzfs`)
as drafted; their locked-spec edits execute atomically with the implementing beads
(er3h/4tw, kzfs), NOT on ratification. Next NON-gated forward items: `80k` (wildcard),
`x8fs` (cooked CON read; before 4tw). New follow-ups: `d27i` (P1 in-kernel FAT16),
`qywt` (P2 initrd discovery), `7mjc` (P3 corrupt-fuzz flake). The shared-tree
workflow race (verifiers reverting/restoring the uncommitted tree) produced two
transient false-alarm P0/P1s this tranche -- standing mitigation for the next lane:
isolated worktrees for mutation-running verifiers, or commit-per-landing.

**WL-0028 landed the forward tranche + the M6 evidence base** (103 host + 27 emu,
Bochs boot leg PASS). DONE + closed: **`d27i`** (windowed FAT16 mount -- the true
completion of FAT16; mode-aware bind keeps FAT12 whole-FAT for WRITE, windows FAT16
for READ; FAT12 straddle byte-identical), **`80k`** (the FCB wildcard matcher was
already correct -- landed the mutation-proven oracle + corrected a ground-truth
guess), **`x8fs`** (AH=3Fh cooked CON read), **`er3h`** (AH=33h Get/Set BREAK +
CONFIG/built-in + DEC-16 spec edits), **`4tw`** (^C -> INT 23h on 01h/08h/0Ah; 07h/06h
RAW; `KERNEL_SECTORS 144->160`). Filed `docs/research/dbase-ground-truth.md` (M6
InitechBase/SAMIR evidence base) + re-parented the 7 engine beads under `586` + filed
`586.3/586.4`. Three lessons (see WL-0028): (1) **`t6nc`** -- Workflow worktree-isolation
spawns from STALE `main` (e56695a), not the branch tip; keep `main` fast-forwarded or
reset worktrees onto the tip. (2) **A host oracle HANG is worse than red** -- x8fs's
cooked CON read spun on `conin_get`'s no-source 0; only the FULL aggregate gate caught
it; ALWAYS `make clean && timeout 1200 make test` at integration (`bd memories`:
host-oracle-hang-pattern). (3) **Adversarial review caught a real DEC-16 deviation**
(er3h SET wrote AL) -- fixed + M7 mutant. Next: `bsy.1` (DIR wildcard emu leg), `bcg.16`
(in-emulator FAT16 mount oracle -> `40oq` FAT16-green), remaining MILTON shell built-ins.

**SAMIR / M6 (InitechBase) LAUNCHED -- architecture ratified + Phase-0 foundation green
(WL-0029).** The operator opened M6 and directed a corpus-grounded, platform-agnostic,
orchestrated build. **`ADR-0008` is RATIFIED** (ADR-by-committee): the **Platform Abstraction
Layer** (`os/samir/include/samir/pal.h` -- the engine's ONLY OS surface, so InitechDOS drift
touches only `pal_milton.c`), the artifact/factory storage split (`os/samir/` engine +
`harness/diff/dbf_diff/` grader + `spec/samir/` locked data), **dBASE III PLUS 1.1-ONLY**
(IV/`.mdx` dropped; `PRD §6.6` reconciled), and a three-tier corpus-backed grader. The full
**~38-step DAG is materialized** under epic `initech-586` (the granular plan is
`docs/plans/SAMIR-implementation-plan.md` -- THE authority). **Phase 0 (portability + harness
foundation) is COMPLETE + green**: `586.5.1..8` -- the PAL contract, freestanding `rt.c`
(JDN, dec_format ties->+inf) + `value.c`, host PAL binding, the LOCKED `spec/samir/` (imported
from the III+-only corpus: no `==`, `0x1C`/`0x1F` NORMALIZE, 151-code msg table), and BOTH
independent python readers `dbf_ref.py`/`ndx_ref.py` (the oracle independence barrier, 147/0 +
116/0 vs real goldens). Gate: `make test-samir` (7 sub-gates) green, wired into the aggregate.
**Standing dependency:** the sister corpus `../dbase3-decomp` (Tier-1 gates resolve
`DBASE3_DECOMP`; absent -> loud-skip; holds gitignored goldens + the dosbox-x mint harness).
**SAMIR PROGRESS -- Phase 1 codec COMPLETE + Phase 3 core (gmo) COMPLETE + Phase 4 OPENED
(WL-0030).** A second orchestrated session drove the engine forward in 5 parallel-lane waves
(disjoint files, orchestrator owns the Makefile + independent re-grading + commit-per-wave;
**104 -> 124 host gates**, all mutation-proven; 27 emu unchanged). **Phase 1 (.dbf codec,
`aul.1..5`) is COMPLETE**: `dbf_open`/`dbf_field`/`dbf_read_rec` (read) + `dbf_create`/`append`/
`flush` (deterministic write, round-trip vs `dbf_ref.py`) + `append_blank`/`replace`/`delete`/
`recall`/`pack`/`zap` (mutation, assignment-coercion). **Phase 3 core (the `gmo` epic,
`gmo.1..4`) is COMPLETE**: `xb_lex` (rejects IV `==`) -> `xb_parse` (corpus-minted `^` left-assoc,
unary>`^`) -> `xb_eval` (every `xbase_coercion.json` cell incl. the C+N=error#9 HAZARD) -> the
`dbf_coerce_fuzz` property-test (`os/samir/core/{lex,parse,eval}.c` + `eval.h`). **Phase 4 (.ndx,
`ahu.1`) is OPENED**: `ndx_open`/node-read parse vs the 11 corpus `.ndx`, verbatim key-expr
(`os/samir/fs/ndx.c`). See WL-0030 for the per-step ledger + boundary decisions.

**NEXT (remaining DAG; pick per operator steer):** **Phase 2 `.dbt` memo** (`aul.6`=S2.1 read,
`aul.7`=S2.2 write -- resolves the S1.3 M-pointer->text boundary); **Phase 4** continuation
(`ahu.2`=S4.2 key decode+collation -> `ahu.3` SEEK -> `ahu.4` bulk INDEX ON -> `ahu.5`
maintenance); **Phase 3 functions** (`7az` S3.5/S3.6 -- needs the parser to parse call args, the
`XBN_CALL` placeholder); then the convergence point **Phase 5 interpreter** (`7az` S5.1-S5.8, the
dot-prompt REPL `samir_main.c`) which needs Phases 1-4; then **Phase 6** oracle assembly (`17n`/
`586.4` -- the M6 `test-dbase` differential goes green at S6.3/S6.4). **Orchestration cadence
(operator-set):** delegate each step to a subagent (sonnet default, opus for load-bearing); the
orchestrator owns DISJOINT file ownership + Makefile integration + independent re-grading +
commit-per-wave + the bead ledger; run `make clean && make test-unit` at every integration;
report at phase boundaries.

**FLAIR GUI groundwork launched (WL-0021 + WL-0020).** An ADR-by-committee
ratified the region-first Toolbox plan; operator decided indexed-8 depth,
640x480, keep seafoam desktop_bg, proceed in parallel with f8v.4. **The ATKINSON
region engine is implemented + green** (`spec/region_algebra.h` locked;
`os/flair/atkinson/`; `make test-region` homomorphism oracle over 16000 pairs +
3 mutants; freestanding-legal). Draft `ADR-0004` (FLAIR) + `ADR-0005` (region
engine) await operator ratification (`initech-k8o5.2`). Epic **`initech-k8o5`**
carries the 26-bead lattice; next FLAIR steps: `k8o5.6` (extract `console.c`
into one surface module) + `initech-i50` (blitter with region clipping, now
unblocked) `→ initech-26d` / `initech-kg5` `→ initech-87a` (M3 window-drag gate).
STILL-OPEN operator questions (in the ADRs): FLAIR heap home; real-Bochs
pixel-capture funding.

**Other ready work** (`bd ready`; distinct directions — pick per operator
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
                     gate, WL-0016 COMMAND.COM default boot, WL-0017..0024
                     hardening + subdir chain, WL-0025 INT 21h/25h/26h parity
                     tranche (CREATNEW/FILETIME/CHMOD/RENAME/IOCTL + DEC-15),
                     WL-0026 the WL-0025 follow-up tranche (CREATNEW oracle/
                     robustness, IOCTL minors, absdisk spec/emu, fault-injection
                     infra, oracle hardening), WL-0027 the FAT16 milestone (dao
                     streaming walk + z01 FAT16 decode layer) + DEC-16/DEC-07a
                     ADR ratifications, WL-0028 the forward tranche (80k wildcard,
                     d27i windowed FAT16 mount, x8fs/er3h/4tw CON cluster) +
                     InitechBase (SAMIR) M6 ground-truth research brief, WL-0029
                     SAMIR/M6 architecture (ADR-0008) + Phase-0 foundation
                     (PAL, rt, value, spec lock, dbf_ref/ndx_ref) orchestrated (latest)
docs/research/       ground-truth briefs (fat12, fat16, boot-to-text, internals/
                     int21h, psp-loader, fs-mount-sft, dbase/SAMIR) -- the
                     per-milestone evidence base
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
