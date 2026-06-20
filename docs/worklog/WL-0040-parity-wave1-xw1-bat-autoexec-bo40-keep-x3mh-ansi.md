<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# WL-0040 -- DOS-3.3 parity Wave 1: xw1 (.BAT/AUTOEXEC) + bo40 (AH=31h KEEP) + x3mh (ANSI.SYS FSM), orchestrated

**Type:** Feature push (orchestrated, 3 disjoint lanes + orchestrator integration). **Date:** 2026-06-20. **Branch:** command-com-default.
**Operator steer:** orchestrate the next-most-consequential InitechDOS work; delegate each coding step to a subagent (<=6 sonnet / <=2 opus parallel); orchestrator grades/integrates/commits + raises beads + convenes the committee for serious decisions; keep working. Continues WL-0038 (the DOS-3.3 parity push).

## Context

The DOS-3.3 parity milestone (epic `initech-bsy`) capstones at `initech-40oq` (Appendix-A coverage certificate), which still has open deps. The highest-value, cleanly-disjoint set for one wave: `xw1` (the landed `.BAT` parser's REPL integration -- the handoff's explicit NEXT and the biggest remaining shell feature), `bo40` (AH=31h KEEP -- a `40oq` dep), and `x3mh` (ANSI.SYS -- a `40oq` dep, parallelizable as a pure FSM). Ground-truth established disjoint file ownership: xw1 -> command.c (AH=4Dh ERRORLEVEL source already exists, zero int21 edits); bo40 -> int21.c + mcb.c (AH=31h already in the locked spec, mcb arena exists from 509.6); x3mh -> new ansi.c (wiring deferred to avoid an int21.c conflict).

## What changed (3 lanes, each RED->GREEN->mutation-proven; orchestrator owns Makefile + grading + commit)

- **`initech-xw1` -- .BAT/AUTOEXEC wired into COMMAND.COM.** `command_repl`'s inline switch refactored to a shared `dispatch_line`; new `run_batch` driver: per-line classify/expand/dispatch with ECHO state, `@`-suppression, IF [NOT] (ERRORLEVEL n / EXIST / "a"=="b"), FOR %%v IN (set) DO, SHIFT (argv view), GOTO (top rescan), CALL (depth-capped nest), PAUSE (MSG-DOS-0016). ERRORLEVEL latched from AH=4Dh after every external EXEC. AUTOEXEC.BAT runs at REPL entry; `.BAT` candidates in `run_external` now dispatch to `run_batch`. Pure decision helpers (`batch_eval_if`, `batch_for_parse/next_token/subst`) live in batch.c, host-tested. Host gate `test-batch-exec` (63 checks, 2 mutants). EMU gate `test-autoexec` (+mutant): AUTOEXEC.BAT runs end-to-end on QEMU -- @ECHO OFF, IF str + IF [NOT] ERRORLEVEL vs a rc=7 EXEC (GREET.COM), FOR x3, CALL SUB.BAT, then a clean interactive EXIT; 10 serial markers asserted. The mutant (`CMD_MUTATE_NO_AUTOEXEC`) skips the hook -> markers absent -> RED.
- **`initech-bo40` -- INT 21h AH=31h KEEP (TSR) + resident MCB model.** `mcb_keep_resident(arena, psp, paras)` shrinks the terminating program's block (reusing the `mcb_setblock` split) and re-owns it `MCB_OWNER_SYSTEM` so `mcb_alloc` won't reuse it and `mcb_free_owner` skips it on terminate; `do_keep` records AL + termination-type-3 and routes through the existing `do_terminate` return path; AH=4Dh now reports exit-type 3 for a KEEP child. Host gate `test-keep` (43 checks + mutant); int21/mcb regressions green.
- **`initech-x3mh` (pure half) -- ANSI.SYS escape-sequence FSM.** `ansi.c`/`ansi.h`: a GROUND/ESC/CSI FSM emitting an action stream (PUT_CHAR/MOVE_CURSOR/ERASE/SET_ATTR/DSR/KEY_REMAP) -- cursor moves, ED/EL erase, SGR (incl. the grounded ANSI->CGA colour swap: red=4/blue=1/yellow=6/cyan=3), save/restore. Pure + I/O-free. Host gate `test-ansi` (94 checks, 2 mutants). CON wiring deferred -> `initech-p96i`.

## Frictions (the load-bearing lessons -- both caught by the orchestrator at the clean-build + emu re-grade, NOT by the host oracles)

1. **The shell kernel `.bss` overran PROGRAM_BASE.** xw1 allocated `g_batch_bufs[BATCH_NEST_MAX=8][BATCH_FILE_MAX=4096]` = 32 KiB of static BSS, pushing `_kernel_end` to 0x35800 -- ~23 KiB past the 0x2ff00 kernel-window guard. The host oracle cannot see this (it tests pure helpers, not the kernel link); only `make clean && make test-unit` (the shell-image link) caught it. **Deep fix (Rule 3):** one shared `g_batch_buf` + authentic re-read of the parent `.BAT` on CALL return (the parent's pos/len survive on the C stack; DOS likewise re-reads batch files). `_kernel_end` 0x35800 -> **0x2e820 < 0x2ff00 OK**. Proven end-to-end by `test-autoexec`'s CALL SUB.BAT leg (SUB-RAN-WORLD + AUTOEXEC-DONE markers).
2. **A mutant that did not compile under `-Werror`.** `BATCH_MUTATE_FOR_NO_TOKENS` early-returned, leaving `i`/`n`/`bat_is_for_sep` unused -> `-Werror`. A mutant that fails to BUILD is not a valid mutation-proof. Restructured into a genuine one-branch RUNTIME perturbation (skip separators with the live code, then report exhausted). All 5 new mutants now compile under the full CFLAGS AND go RED. **Lesson: re-grade mutants with the REAL `$(CFLAGS)` (-Werror), not a relaxed gcc line.**
3. **HARD LESSON honored (WL-0038):** EXEC-path edits re-run on the emulator. xw1 refactored command.c and bo40 changed do_keep/do_terminate, so the existing emu EXEC/terminate gates (test-exec/program/shell/zs24-exec/ut6d/mzexec/mcb-emu/samir-boot) were re-run -- the AUTOEXEC probe is existence-gated so the other gates' data disks (no AUTOEXEC.BAT) are unaffected.

## Acceptance / state

- `make test-unit` = **230 host gates** (was 224): + test-batch-exec(+mut), test-ansi(+mut), test-keep(+mut). New EMU gate **test-autoexec(+mutant)** in TEST_EMU_GATES. Emu EXEC/terminate regression re-run green.
- batch.o now links into the shell kernel (command.o references it); KERNEL_SHELL_OBJS + the command.o prereq updated.
- All ASCII-clean; reproducible (no timestamps).

## Pointers / next work

- **Follow-ups filed:** `initech-p96i` (wire ANSI.SYS into the CON path + CONFIG.SYS DEVICE=ANSI.SYS gate -- x3mh's deferred half), `initech-2cn6` (in-emulator KEEP-survival gate -- bo40's deeper emu proof; the disjoint-arena bind nuance), AH=34h InDOS (KEEP's TSR companion).
- **Remaining `40oq` deps (Wave 2 candidates):** `mvg` (INT 24h wiring), `509.7` (device chain CON/PRN/AUX/CLOCK$/NUL + INT 2Fh), `slvd` (multi-volume A:/B:/C:), `kzfs` (MBR partition parse, DEC-07a ratified). mvg/509.7/slvd/x3mh-wiring all contend on int21.c (SERIAL); kzfs is the disjoint boot/blockdev lane. Then `bo40` is done, and the `40oq` capstone closes once the rest land (certifies with FCB stubbed per the operator ruling).
- **Remaining xw1 deepening (deferred, filed in the bead notes):** wildcard FOR (glob the set), the `PATH` built-in (so the baseline AUTOEXEC's `PATH X` line works), larger `.BAT`/deeper nest caps if real scripts exceed 4 KiB / 8 levels.
