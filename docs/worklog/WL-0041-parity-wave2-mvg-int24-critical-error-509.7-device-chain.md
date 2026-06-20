<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# WL-0041 -- DOS-3.3 parity Wave 2: mvg (INT 24h critical-error wiring) + 509.7-core (device-header chain), orchestrated

**Type:** Feature push (orchestrated, 2 disjoint lanes + orchestrator integration). **Date:** 2026-06-20. **Branch:** command-com-default.
**Operator steer:** keep orchestrating the next-most-consequential InitechDOS work toward the DOS-3.3 parity capstone (initech-40oq); delegate coding, orchestrator grades/integrates/commits + raises beads. Continues WL-0040 (Wave 1).

## Context

After Wave 1 (xw1/bo40/x3mh), the remaining `40oq` deps are mvg (INT 24h wiring), 509.7 (device chain), slvd (multi-volume), kzfs (MBR partition -- operator-deferred). Most contend on int21.c, so only one int21 lane runs at a time. Wave 2 = mvg (the load-bearing critical-error path; int21.c/fat12.c/ata.c) + 509.7-CORE as a new disjoint module (the device-header chain; new devices.c). A SAMIR transcendentals lane (7az.13) was considered but DROPPED -- it is explicitly GATED (committee-worthy x87-vs-poly strategy + a MINT dependency), off the parity critical path.

## What changed (2 lanes, each RED->GREEN->mutation-proven; orchestrator owns Makefile + kmain wiring + grading + commit)

- **`initech-mvg` -- INT 24h critical-error raised by the disk layer + A/R/F honored.** The 509.8 handler (`int24_dispatch`, MSG-DOS-0001) existed but nothing raised it. Approach (subagent, adversarially graded): a **wrapper blockdev** `crit_blockdev_t` (ata.c) sits between fat12 and the real backend; on a hard inner error it calls a process-wide hook = `int21_run_critical_error` (int21.c, the new choke point), which runs the REAL `int24_dispatch` and maps the result -- **Retry** re-issues the sector op (bounded, 5x), **Abort** routes through the real `do_terminate` (code 0x23), **Fail** propagates CF=1. fat12.c is UNCHANGED (every ~20 call site inherits A/R/F transitively; every ata.c error source -- floating-bus/BSY-DRQ-timeout/status/write-protect -- surfaces uniformly). Host oracle `test-int24-wired` (19 checks, 2 mutants) drives it via the lpf3 fault-injection backend (extended with a symmetric read-fault arm) against the REAL int24_dispatch.
- **`initech-509.7` (CORE) -- the heritage DOS device-driver chain.** New `os/milton/devices.{c,h}`: the device-header model (attribute word w/ cited bits, Strategy/Interrupt entry, 8-char name, chain link) + the request-packet protocol (13-byte header, command codes INIT/READ/WRITE/NDREAD/IN+OUT STATUS/FLUSH, status DONE/ERROR bits) + the five resident char devices NUL/CON/AUX/PRN/CLOCK$ with handlers, all I/O via a `devices_io_t` callback seam (pure + host-testable; CLOCK$ takes an injected time source -- no rtc.c dep). Host oracle `test-devices` (150 checks, 2 mutants). INT 2Fh redirector (amendment-gated) and the int21 OPEN-by-name + sysinit DEVICE= install are deferred (the wiring follow-up).

## Frictions (orchestrator-caught integration design flaw)

1. **My own kmain wiring would have hung every no-disk boot.** mvg's mechanism is sound, but the integration choice -- where to arm the hook -- was load-bearing. My first wiring armed `crit_blockdev_set_hook` BEFORE `fat12_mount`, so a boot WITHOUT a data disk would hit the INT 24h prompt during the boot-time mount probe (sector-0 read fails on no-drive) and BLOCK on A/R/F -- changing the graceful "mount failed, continue" path and breaking every emu gate that boots without --disk2. **Real DOS raises the critical-error prompt for PROGRAM disk I/O, not boot-time drive probing.** Fix: the wrapper stays TRANSPARENT (hook unset) through the mount probe; the hook is armed only AFTER a successful mount, so it covers shell/program file I/O. Verified by the emu regression (no-disk2 boots degrade gracefully; mounted file I/O stays transparent).
2. **Mutant-under-`-Werror` discipline held.** Both mvg mutants (NO_RAISE, RETRY_UNBOUNDED) and both devices mutants (NO_DONE_BIT, NUL_READ_BYTE) were re-graded under the FULL `$(CFLAGS)` (-Werror) -- all compile clean and go RED (the WL-0040 lesson, applied up front in the briefs).

## Acceptance / state

- `make test-unit` = **234 host gates** (was 230): + test-devices(+mut), test-int24-wired(+mut). The crit_blockdev wrapper + int21_run_critical_error + the device chain all link into the freestanding kernel; kernel-end guard OK (_kernel_end=0x2eaa0 < 0x2ff00). mvg wired into kmain (mount through the wrapper; hook armed post-mount).
- Emu regression (no-disk2 graceful boot + mounted file I/O transparent + EXEC paths) re-run green.
- All ASCII-clean; reproducible.

## Pointers / next work

- **Follow-ups filed:** the mvg in-emulator INT-24h-trigger gate (needs a QEMU mid-file disk-fault mechanism; host oracle is authoritative meanwhile), the 509.7 WIRING (int21 AH=3Dh OPEN-by-name -> devices, sysinit DEVICE= install, CLOCK$<-rtc.c bridge), `initech-p96i` (x3mh ANSI CON wiring).
- **Remaining `40oq` deps:** 509.7-wiring + p96i (both touch int21.c CON/open path -- a SERIAL int21 lane, candidate for Wave 3), slvd (multi-volume, depends on kzfs), kzfs (MBR partition -- OPERATOR-DEFERRED). The kzfs/slvd cluster is the capstone's open question: 40oq lists them as deps but the operator deferred kzfs; surface to the operator whether the capstone certifies without partitions+multivol (as it does with FCB stubbed).
