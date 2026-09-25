<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0091 -- THE NORTH STAR IS RESTATED, THE APP LAUNCHES FROM DISK, AND THE HARNESS STOPS LYING UNDER LOAD

**Date:** 2026-09-25/26 (operator handed programme ownership to the agent-PM;
"work Saturday and Sunday"; max two lanes, opus for hard work, sonnet for
busywork; stop at 45% weekly/Fable quota)
**Beads:** CLOSED initech-6gkm (M7 blocker -- a HARNESS bug), initech-8z9j
(kernel size policy), initech-qw0t (ADR-0001 + 486 floor; supersedes
initech-fgs1), initech-lmkp (deterministic datetime gate), initech-qed1
(keyed-gate hardening), initech-6m52 + epic initech-rnh (M7 closes),
initech-tdnl.14 (R3.7 app launch V1). FILED: epics initech-68iw (Initech
123) + initech-fdxa (InitechWord) with phase beads; initech-yjlo (ADR-0009
numeric amendment); initech-cnpm/2svz/t5xs/smp0 (R3.7 follow-ups).
**Commits:** `ec05e91` (Rule 14) -> `888a5d2` (6gkm) -> `1fa5191` (8z9j) ->
`99320c4` (ADR-0001) -> `b4c2009` (Makefile stamp) -> `ffa4bea` (hwspec
oracle) -> `b347b7f`/`3473d96` (I123 + IWORD plans) -> `26111d7` (TPS gates)
-> `66d86db` (lmkp) -> `51705fd` (qed1) -> `8f884a7` (tdnl.14).
**Closing certificate:** ALL GREEN 355 host + 116 emu (cert-06, first-person, 2026-09-26 01:37, on 8f884a7; the emu vector grew 105 -> 116 with the tdnl.14, qed1, 6gkm and lmkp gates)

## Context

The operator restated the north star (2026-09-25): a fully period-correct
windowed OS on an era-authentic DOS, with FULL-FEATURED canonical apps --
dBASE III, a word processor, Lotus 1-2-3 -- plus the self-hosting compiler
with a Borland-style GUI; 486-class hardware greenlit, 386 desirable; the
bar is "could have been bought in the 90s and judged competitive". A
four-lane sonnet audit against that bar found: M0-M3 + M6 closed, M4/M7
partial, M5/M8 untouched; dBASE is the only canonical app that exists; no
word processor, no spreadsheet, no IDE, no self-host; the last ten shards
were spent on Finder chrome and the compiler, none on a canonical app.

## What changed

1. **Rule 14 (CLAUDE.md):** window-system features are accepted on an emu
   video clip (`record-flair`, repro-proven), never on host oracles alone.
2. **6gkm was never a MILTON bug.** The harness quit QEMU ~400 ms after the
   last injected key while the guest was still running AUTOEXEC.BAT
   (SHELL-READY prints BEFORE the batch runs). Fix: `--quit-after MARK` +
   `--keys-after-ms`; TPS_OS_COMPLETION waits for SHELL-EXIT; the six M7
   on-OS gates rejoin TEST_EMU_GATES; test-harness-quit-after(+mutant)
   pins it (34-program batch: 2/34 pre-fix, 34/34 post). test-compiler-os
   17/17 on the booted 386; OFFBYONE + VARPARAM RED by execution. **M7
   closes.**
3. **Kernel size policy (8z9j):** per-object -Os on seven measured objects
   (int21 -11,552; fat12 -8,096; control -4,448; chrome -3,808; dialog
   -3,360; menu -2,720; region -2,464; -O1 GROWS fat12.o). Binding kernel
   (the largest mutant) 194,884 -> 158,436 B; headroom 1,724 -> 38,172 B.
   New test-kernel-headroom enumerates all 73 kernel .bins (floor 4096 B)
   + mutant. kmain -O1, -march=i386, KERNEL_SECTORS=384, mbr/stage2
   bit-identical, clean double build identical.
4. **The stamp (Rule 11):** the first certifying run went RED on
   test-kernel-repro with 34,530 differing bytes -- STALE objects (objects
   had no Makefile dependency, so the -Os flag change never rebuilt them).
   Root fix: `build/.makefile.stamp` as `.EXTRA_PREREQS` on every target.
5. **ADR-0001 authored** (it was cited everywhere and absent): DEC-01 the
   386 origin (reconstructed), DEC-02 486-class ratified 2026-09-25,
   DEC-03 what does not change (-march=i386: cmov is P6; the 384-sector
   bounce ceiling is bootloader design), DEC-04 unlocks (x87 zj6w,
   ADR-0009 hw-float, CPUID, heap floor), DEC-05 86Box = 486. PRD Sec 2/5,
   spec/hardware.json, CLAUDE.md header, and the hardware-spec oracle
   (Rule-8 deliberate: new exact value expected, old "386+" is the mutant).
6. **Two canonical-app plans** (sonnet lanes): docs/plans/INITECH-123-plan.md
   (Lotus 1-2-3 R2.2; two NATIVE faces over one recalc/.WK1 core per
   ADR-0013 Sec 4; a populated sheet exceeds the 187 KiB program window, so
   disk-launched only) and docs/plans/INITECH-WORD-plan.md (WordPerfect
   5.1; piece-table core in os/word/core; InitechText kept as the light
   editor; Face B blocked on the absent Font Manager). Both epics filed
   with phase beads; both P0 golden-acquisition beads are `bd human`
   flagged: the operator must supply the copyrighted media.
7. **Under host load the harness lied.** An unrelated benchmark of the
   operator's (bench/seam/run_family.py, 8 workers) pushed load to 13-20 on
   12 cores; test-datetime, test-tps-lex-os and test-seed-fileio-os went
   RED with serial logs ending at the command echo. Three root fixes:
   test-datetime now runs under `-icount shift=4,sleep=on` (virtual time is
   a function of instructions retired; assertion unchanged; 3/3 green at
   load 16-24); the three TPS on-OS gates got typed-ahead exit +
   completion; and initech-qed1 swept all 125 keyed invocations across 79
   targets -- 42 unguarded targets hardened with the guest's OWN markers
   (SHELL-DONE, FLAIR-EVT, KBD-ECHO-END, IRQSTORM-EXIT, ...), five
   documented `--legacy-quit` absence-only mutants, and the harness now
   REFUSES a bare --keys/--mouse invocation (test-harness-bare-keys +
   mutant).
8. **R3.7 APP LAUNCH FROM DISK (tdnl.14) V1 landed** -- the load-bearing
   mechanism every canonical app ships through. Double-click
   APPS\TENANTFX.EXE -> InitechMZ into a FLAIR-heap block (parameterized-
   base relocation, PSP at block) -> REGISTER through the INT 81h Toolbox
   Gate (spec/toolbox_gate.h locks vector 0x81, trap gate 0x8F, args at
   frame+68, codes REGISTER 0x0001 / NEWWINDOW 0x0020 / SETWTITLE 0x0022 /
   TEXTDRAW 0x0030 / FILLRECT 0x0031 / EXIT 0x0070) -> admit, then
   activate on first NEWWINDOW -> push-callback events via a kernel-owned
   vtable -> teardown between pump iterations on EXIT / close box / crash,
   code block freed last; a fault inside the tenant image kills only the
   tenant. Locked 19-line launch trace; test-flair-app-launch 8 legs
   (SHOW pixel grader, RESTORE byte-identical, DOUBLE, CYCLE avail-stable,
   CLOSE, NOREG, CRASH) + four D1.7 mutants RED for their named reasons +
   Bochs leg (16 TENANT-* lines byte-identical). Rule-14 clip
   build/clips/app_launch.{gif,mp4}, 30 frames, repro byte-identical; the
   PM eyeballed frames 12/22 (Finder -> APPS -> TenantFix window). Two
   guest bugs fixed en route: Finder listed . and .. (mutant DOTS_SHOWN);
   chrome.c drew title text unclipped over the front window. Headroom
   after: flagship 31,192 B, binding 30,520 B.

## Frictions

- Two concurrent full vectors OOM-killed both opus lanes (30 GB host).
  Rule now: lanes run targeted gates; the PM certifies once on main.
- `pkill -f` / `for p in $(pgrep -f ...)` with a pattern present in the
  agent's own command line kills the agent's shell (exit 144) -- twice.
  Kill by pid from a prior `pgrep -x`.
- Worktree lanes need absolute DBASE3_DECOMP/WIN31_DECOMP/SYSTEM7_DECOMP.
- The bd embedded backend is single-writer; a lane's `bd show` locks the
  PM out transiently.

## Acceptance

ALL GREEN 355 host + 116 emu (cert-06, first-person, 2026-09-26 01:37, on 8f884a7; the emu vector grew 105 -> 116 with the tdnl.14, qed1, 6gkm and lmkp gates) -- first-person, on main at the pushed HEAD, under the
operator's benchmark load (the hardened harness is the point).

## Pointers -- NEXT AGENT START HERE

1. **Operator rulings queue:** (a) supply Lotus 1-2-3 R2.2 and WordPerfect
   5.1 media for the two golden repos (initech-qh83, initech-rv9y are
   `bd human`); (b) ADR-0009 numeric amendment (initech-yjlo; x87 via
   zj6w vs soft-float); (c) the tdnl.12 stage-2 chimera ruling still
   stands; (d) dictionary placement + Font-Manager sequencing for
   InitechWord.
2. **Next lanes, in order:** initech-cnpm (SETMBAR -- a disk tenant's own
   menu bar; band 2 currently falls back to the Photoshop bar), initech-
   34dh -> 6k12 -> p6st (R3.4 file ops, now unblocked by 38 KiB headroom),
   initech-zj6w (kernel x87 init) then yjlo, then I123 P1 as soon as P0
   media lands. tdnl.13 (menu-bar clock) stays a good small filler.
3. Standing: 86Box driver (initech-x0i) still unbuilt; SSIM still a stub.
