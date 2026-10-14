<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->
<!-- Minted 2026-10-15 by lane-opms-kernelsize (bead initech-opms). Round 2 of
     the kernel size policy; round 1 is initech-8z9j (WL-0091, Makefile
     "KERNEL SIZE POLICY"). Base: 9cc1848 (after tdnl.78 + fvnj). -->

# Kernel Size Policy, Round 2

**Bead:** initech-opms | **Date:** 2026-10-15 | **Base:** 9cc1848 |
**Status:** DECIDED and implemented on lane-opms-kernelsize (levers A and B);
lever C parked with its measurement.

---

## 1. The problem

The kernel's loaded bytes are capped by the stage2 real-mode bounce, not by the
high runway: `KERNEL_SECTORS=384` (196,608 B) is AT the ratified bounce ceiling
(`KERNEL_BOUNCE_CAP=0x30000`, docs/design/kernel-runway-relocation.md K2; the
Makefile parse-time guard fails 385 loud). Growing the window means the
bounce-policy redesign (staged copy-up), which is a boot-geometry change and a
Rule-5 tri-emulator obligation. Round 1 (initech-8z9j) bought 36 KB with
per-object `-Os` on seven shared objects; tdnl.78 (+2,112 B) and fvnj
(+1,248 B) then brought the binding headroom back near the floor.

Measured at 9cc1848 (`make test-kernel-headroom`, 133 kernels):

| kernel | loaded B | headroom B |
|---|---:|---:|
| flagship `kernel_flairtenants` | 191,240 | 5,368 |
| LARGEST (binding) `kernel_flairtenants_mut_no_raise_on_title` | 191,912 | 4,696 |

Floor `KERNEL_HEADROOM_FLOOR` = 4,096 B. Measured kernel slices are 2-11 KB,
so one more kernel-side feature would bust the floor. Acceptance for this
round: binding headroom >= 12 KB, preferably 20 KB+, with no boot-geometry
change.

## 2. Where the bytes are (measured, flagship objects)

The metric is LOADED bytes per object: `.text*` + `.rodata*` + `.data*`, the
PROGBITS kernel.ld keeps. Note for future rounds: `size` (Berkeley) OVERSTATES,
because it counts `.eh_frame`, which kernel.ld discards. A first pass with
`size` predicted about 20 KB for lever A; the real figure is 9 KB.

Optimisation level at 9cc1848: seven round-1 objects plus window_live, tbxgate
and rtc are `-Os`. The ten FLAIRTENANTS-only objects (kmain_flairtenants,
finder_*, process, ref_tenant, desktop_db) are `-O1` (KERNEL_TENANTS_OPT).
**Every other object is `-O0`.** Loaded bytes per level:

| object | now | -O0 | -O1 | -Os | saving to -Os |
|---|---|---:|---:|---:|---:|
| kmain_flairtenants | -O1 | 42,037 | 34,130 | 30,812 * | 3,318 |
| finder_windows | -O1 | 15,643 | 12,543 | 10,322 | 2,221 |
| finder_ops | -O1 | 4,576 | 4,143 | 2,766 | 1,377 |
| finder_desktop | -O1 | 10,098 | 8,109 | 6,887 | 1,222 |
| process | -O1 | 3,652 | 2,988 | 2,528 | 460 |
| desktop_db | -O1 | 1,882 | 1,791 | 1,286 | 505 |
| ref_tenant / finder_cmd / finder_icon / finder_menu | -O1 | | | | 394 |
| loader | -O0 | 5,094 | 4,291 | 3,330 | 1,764 |
| fileio_fat | -O0 | 4,247 | 3,386 | 2,647 | 1,600 |
| devices | -O0 | 3,012 | 1,933 | 1,466 | 1,546 |
| console | -O0 | 3,121 | 2,121 | 1,960 | 1,161 |
| event | -O0 | 2,563 | 1,905 | 1,567 | 996 |
| mcb | -O0 | 2,131 | 1,406 | 1,218 | 913 |
| sysinit | -O0 | 3,278 | 2,766 | 2,437 | 841 |
| config_sys | -O0 | 1,908 | 1,496 | 1,185 | 723 |
| sft | -O0 | 1,415 | 881 | 713 | 702 |
| blitter | -O0 | 1,425 | 1,026 | 778 | 647 |
| shell (flair_shell) | -O0 | 1,738 | 1,531 | 1,205 | 533 |
| cursor | -O0 | 1,579 | 1,189 | 1,047 | 532 |
| flair_look | -O0 | 1,418 | 947 | 931 | 487 |
| heap / psp / desktop / surface | -O0 | | | | 1,389 |
| ata/kbd/mouse/pic/irq/pit/idt/panic (drivers) | -O0 | | | | 3,080 |

\* kmain at `-Os` does not compile under `-Werror` (see lever A); figure with
the fix.

Largest symbols (flagship ELF): `kernel_main` 12.2 KB, `int21_dispatch`
7.5 KB, `tbx_gate_dispatch` 5.7 KB, `flair_live_do_surface` 4.4 KB,
`chicago12_rows` 3 KB (font strike, rodata), `DrawControl` 3 KB. There are no
large misplaced static tables. The only big rodata is the two font strikes,
which the console and Toolbox need before the data volume is mounted. Big BSS
(`g_fat` 6 KB, the Finder dir/cluster buffers) is NOBITS and costs no loaded
bytes. **The kernel is mostly code at `-O0`. The lever is the optimiser, not
the data.**

Dead code (`-ffunction-sections -fdata-sections` plus `ld --gc-sections`,
measured in a scratch build dir): 112 unreferenced sections in the flagship,
**-10,908 B** at 9cc1848 and **-9,084 B** on top of levers A+B. Examples are
the unused Region-Manager entry points (UnionRgn/SectRgn/XorRgn/...),
`sysinit_apply_config`, `loader_prepare`/`load_program` (baked path, DOS
kernels only), and the baked `g_test_prog_image`.

## 3. Decision: what we take

### Lever A: tenants-only objects `-O1` -> `-Os` (KERNEL_TENANTS_OPT)

`KERNEL_TENANTS_OPT := -Os -fno-strict-aliasing --param=min-pagesize=0`

- Round 1 left kmain at `-O1` because `-Os` trips `-Warray-bounds` on the
  fixed-address `boot_info_t` read. The root cause is GCC's heuristic that any
  constant address below `--param=min-pagesize` (default 4096) is a NULL-pointer
  offset; `BOOT_INFO_ADDR` is 0x500. `--param=min-pagesize=0` disables that
  heuristic (the Linux kernel uses the same remedy for GCC 12+). The param only
  affects warnings: all ten objects compile byte-identical at `-Os` with the
  param and with `-Wno-array-bounds`. `-Werror` stays fully on, so we do not
  silence any real array-bounds warning.
- `-fno-strict-aliasing`: at `-O1` GCC does no type-based alias analysis
  (TBAA); `-Os` turns it on. Measured, TBAA changes codegen in kmain,
  finder_windows, finder_ops, finder_desktop, process and ref_tenant. That
  means the certified code would run under alias assumptions it has never been
  tested with. Size effect is within +/-20 B per object, so we keep TBAA off.
- Reclaim: flagship 191,240 -> 181,736 (**-9,504 B**); binding
  191,912 -> 182,856 (**-9,056 B**, headroom 13,752).
- Blast radius: only the 65 `kernel_flairtenants*` kernels change. The other
  68 kernels, mbr.bin and stage2.bin are byte-identical to 9cc1848 (cmp,
  measured).

### Lever B: pure-logic shared `-O0` objects -> `-Os -fno-strict-aliasing`

`KERNEL_SIZE_OPT_R2 := -Os -fno-strict-aliasing`, threaded through
per-object variables (bisectable, the round-1 pattern) into every recipe that
compiles the source for a kernel. That covers the -D mutant objects
(sysinit_fpu_*, sft_mut_noclose, psp_mut_no_jft_inherit,
flair_shell_mut_*, desktop_mut_*) and the test-* freestanding compile checks,
so a mutant still differs from its real object only by its knob.

Objects: fileio_fat, devices, console, mcb, sysinit, config_sys, sft, psp,
blitter, shell, cursor, flair_look, heap, desktop, surface.

Selection rule: an object qualifies only if it has
- no inline asm that switches stacks or resumes at an in-asm label,
- no state shared with an ISR outside `volatile`, and
- no port-I/O timing loops.

All candidates were also checked for empty delay loops, which `-Os` would
delete; none were found. sysinit's x87/CR0 asm is `__volatile__` with
`"memory"` clobbers and volatile operands, so it is safe at any level.

- Reclaim: flagship 181,736 -> 170,600 (**-11,136 B**); binding
  182,856 -> 171,720 (**-11,136 B**).
- Blast radius: every kernel that links these objects (the DOS shell kernels
  included) changes bytes. This is the same Rule-5 class as round 1.

### Net

| kernel | 9cc1848 | after A | after A+B |
|---|---:|---:|---:|
| flagship `kernel_flairtenants` | 191,240 (5,368) | 181,736 (14,872) | 170,600 (**26,008**) |
| binding `..._mut_no_raise_on_title` | 191,912 (4,696) | 182,856 (13,752) | 171,720 (**24,888**) |

(loaded bytes, headroom in parentheses; window 196,608 B)

Binding headroom went from 4,696 B to **24,888 B (+20,192 B)**.
`KERNEL_SECTORS`, `IMG_SECTORS`, mbr.bin and stage2.bin are unchanged. The
`KERNEL_HEADROOM_FLOOR` stays 4,096 B: the floor is "a slice must fit", and
round 2 raises the room above the floor, not the floor itself.

## 4. What stays parked, and why

- **loader.c (-1,764 B).** `loader_run_plan`'s program-entry asm saves only
  ESP/EBP/EBX into the context and resumes at the in-asm label `1:` after
  `loader_exit_hook` jumps back. At `-O0` nothing lives in ESI/EDI across that
  asm. An optimised caller may keep a live value there, and the program is free
  to clobber it. Taking this byte saving needs `"esi","edi"` clobbers on that
  asm (or a full callee-saved save/restore), plus EXEC-family tri-emulator
  proof. That is a separate change to the load-bearing transfer, not a flag
  flip.
- **event.c (-996 B).** The ISR->pump SPSC ring orders the non-volatile slot
  copy against the `volatile` head/tail purely by `-O0` codegen. Optimising it
  needs an explicit compiler barrier in `drain_one`/`flair_raw_post` first.
- **Hardware drivers (ata, kbd, mouse, pic, irq, pit, idt, panic; ~3 KB).**
  These do port I/O with bounded spins and run in IRQ/exception context. The
  saving is small for the risk, so they stay `-O0`.
- **Lever C, `--gc-sections` (~9 KB more on top of A+B).** This is the largest
  remaining lever. It is parked for two reasons:
  1. `-ffunction-sections` must not ride `KERNEL_CFLAGS`, because
     `TENANT_CFLAGS` (disk applications) derives from it. It needs its own
     kernel-only variable, threaded through the ~180 kernel compile recipes.
  2. GC may drop symbols the harness reads by name from a kernel ELF.
     `kstart.o`/`_start` are roots, but every `nm`/gdb symbol lookup in the
     harness and Makefile needs auditing, and any data reached only from asm
     or by a gdb probe needs `KEEP()` in kernel.ld.

  Take it in round 3, with that audit, when binding headroom next drops below
  about 12 KB.
- **The bounce-policy redesign** (K2 staged copy-up beyond 384 sectors) stays
  parked. Levers A+B (and C later) keep it unnecessary for now, and it is the
  only lever with a boot-geometry / Rule-5 tri-emulator cost by construction.
- **Round-1 objects stay as certified**, `-Os` with TBAA on. Re-flagging them
  `-fno-strict-aliasing` would rewrite certified bytes for no measured problem.

## 5. Rules honoured

- Rule 11: `-Os` and the params are deterministic. `make test-kernel-repro`
  (clean double build, byte-identical) is part of acceptance.
- Rule 5: every changed kernel image must pass the QEMU gates on this lane.
  The Bochs legs (`test-boot-bochs`, `test-flair-desktop-bochs`,
  `test-flair-appswitch-bochs`, `test-flair-desktop-icons-bochs`) run on main
  by the PM after merge. Boot geometry did not change.
- `-march=i386` stays pinned in `KERNEL_CFLAGS`. `-O` levels change scheduling
  and inlining, never the ISA (the WL-0087 cmov catch).
- Rule 6: `test-kernel-headroom-mutant` still bites: the floor-1 pad kernel
  goes RED and the floor pad kernel stays GREEN.

## 6. Acceptance (lane-opms-kernelsize, at af2a7b2)

- `test-kernel-headroom`: 133 kernels measured. Flagship
  `kernel_flairtenants` loads 170,600 B (headroom 26,008 B). The LARGEST,
  `kernel_flairtenants_mut_no_raise_on_title`, loads 171,720 B (headroom
  **24,888 B**, binding). Green.
- `test-kernel-headroom-mutant`: the floor-1 pad kernel (4,095 B) goes RED
  for the named reason, and the floor pad kernel (4,096 B) stays GREEN.
- Rule 11:
  - `test-kernel-repro` is green.
  - A clean double build of all 133 kernel `.bin`s (every kernel `.o`/`.elf`/
    `.bin` removed, then rebuilt) is byte-identical for all 133.
  - mbr.bin and stage2.bin are byte-identical to 9cc1848.
- `test-spec` and `test-makefile-vars` are green.
- QEMU emulator gates: 76 run one at a time, and all are green. That covers:
  - the DOS family (boot, tracer-boot, program, fs, type, dir, exec, mzexec,
    mcb-emu, fatwrite, multiopen, exit-handles+mutant, sysinit,
    sysinit-oversize, shell, panic, spurious, datetime, fpu+mutant,
    fpu-absent+mutant, kbd, conin, vect, absdisk-emu, int21-irqstorm,
    autoexec, bsy8-pipe, shell-permanent);
  - SAMIR/TPS (samir-boot, tps-gen-os, compiler-os, seed-fileio-os,
    samir-lifetime-desktop);
  - the FLAIR family (desktop+mutant, menu, menu-crossdrag, live, key, mouse,
    drag, dc4v, cursor, appswitch+mutant, desktop-icons, disk-windows,
    app-launch, tenant-menu, ctenant, files, i123, i123-quitguard,
    file-ops, trash-window, empty-trash, scroll, finder-cmds, held-gestures,
    box-track, zoom-toggle, grow, collapse, data-volume, close-terminate,
    modal-block, samir-suspend, locked-trash, shutdown+mutant, restart,
    and solid+mutant, the timing-sensitive repaint/drag gate).

  `test-flair-app-launch` went RED once mid-sequence. Leg [5/10] CYCLE failed:
  the second launch's clicks did not land while the host load average was
  about 15 on 12 cores. It was green on 4 of 4 runs alone. This is the
  injected-trace-under-oversubscription class (cf. initech-7r2), not a kernel
  defect.
- Not run on the lane: the Bochs legs (the PM runs them on main; this is the
  Rule-5 obligation every changed image carries) and `record-flair`.
