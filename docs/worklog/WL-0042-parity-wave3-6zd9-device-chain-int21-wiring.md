<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# WL-0042 -- DOS-3.3 parity Wave 3: 6zd9 (device chain wired into INT 21h OPEN-by-name + sysinit), orchestrated

**Type:** Feature push (orchestrated, single load-bearing lane + heavy orchestrator integration). **Date:** 2026-06-20. **Branch:** command-com-default.
**Operator steer:** keep orchestrating toward the DOS-3.3 parity capstone (initech-40oq). Continues WL-0041 (Wave 2). Single-lane wave: the remaining doable capstone deps (509.7-wiring, p96i) both serialize on int21.c, so Wave 3 is one focused opus lane.

## Context

Wave 2 landed the device-chain MODULE (devices.c, 509.7-core). 6zd9 wires it into the live kernel: AH=3Dh OPEN-by-name -> the device chain, READ/WRITE routing, sysinit install. This touches the MOST load-bearing path in the OS (CON output -- the shell prompt + every diagnostic + every emu gate depend on it), so the brief's non-negotiable constraint was: preserve the existing direct-CON output byte-for-byte; the change is ADDITIVE.

## What changed

- **`initech-6zd9` -- device chain wired into INT 21h.** (subagent, opus, carefully graded.)
  - **OPEN-by-name (AH=3Dh do_open):** before the FAT path, `dev_open_lookup` matches "CON"/"NUL"/"PRN"/"AUX"/"CLOCK$" (up-cased, space-padded, drive/path stripped, ext ignored) -> binds an `SFT_KIND_DEVICE` slot carrying a `device_header_t*`; never touches the FAT.
  - **READ/WRITE routing (AH=3Fh/40h):** a device-bound slot builds a `device_request_t` -> `devices_request`. NUL discards+EOF; CON -> the console sink; PRN/AUX -> the (sysinit) serial sink; CLOCK$ READ -> the injected date/time.
  - **CON OUTPUT PRESERVED (the constraint):** the legacy predefined CON slots 0-3 (device==NULL) keep the EXACT existing fast path; the device legs are gated on device!=NULL (only an OPEN-by-name slot has one). int21.c self-wires the chain's CON+CLOCK$ legs to its own sink/clock seams -> exactly ONE CON output path. Proven by the no-regression assertions + the full emu suite.
  - **sysinit install:** `devices_init(<kernel io>)` + `int21_set_devices(devices_head(), io)` in sysinit_early; `DEVICE=ANSI.SYS` -> a `g_ansi_enabled` flag (for bead p96i; no ANSI behavior implemented); INITNET.SYS stays accepted(deferred) (INT 2Fh redirector is amendment-gated, OUT).
  - **SFT device-slot reclaim:** `sft_close_process` now also reclaims OPEN-by-name DEVICE slots (>= SFT_FIRST_FILE), still skipping the resident slots 0-3 -- prevents a per-process device-handle leak.
  - Host gate `test-devwire` (40 checks, 1 mutant: OPEN-no-device -> RED), driving the REAL int21_dispatch over the REAL devices.c chain, incl. a no-regression CON assertion.

## Frictions (orchestrator integration -- two real issues caught at grading)

1. **devices.o is now a transitive kernel + host-test dependency.** int21.o (OPEN-by-name + routing) and sysinit.o (install) reference devices_* symbols, so EVERY kernel image and EVERY int21-linking host test must also link devices.{o,c} -- ~19 kernel object-lists + ~12 host-test dep vars. Integrated via consistent-pattern replace-all (`$(KERNEL_INT21_OBJ) $(KERNEL_MCB_OBJ)` -> + DEVICES_OBJ; `$(KERNEL_INT21_C) $(KERNEL_MCB_C)` -> + DEVICES_C) + one manual append (TEST_FILEIO_SUBDIR_DEPS). A missed list fails to LINK (loud), so the clean-build aggregate is the safety net -- it passed.
2. **The kernel-window guard tripped AGAIN.** devices.c BSS + the 6zd9 int21/sysinit additions pushed `_kernel_end` to 0x302e0, ~992 bytes past the 0x2ff00 program-load guard. This is the THIRD time the kernel bumped the window this session (Wave 1 batch buffer, Wave 2 mvg, now Wave 3). Tactical fix: trimmed `BATCH_FILE_MAX` 4096->2048 (a .BAT size cut; AUTOEXEC + typical scripts still fit) -> `_kernel_end=0x2fae0`, ~1 KiB margin. **The recurring pressure is an architectural signal:** filed a bead for the principled headroom fix -- raise PROGRAM_BASE (locked-spec, wide blast radius: every .COM org 0x30100 + the MCB arena + PROGRAM_IMAGE) OR share the two 6 KiB whole-FAT buffers (g_fat in fat12.c + g_load_fat in loader.c). Restore BATCH_FILE_MAX to 4096 once headroom exists.

## Acceptance / state

- `make test-unit` = **236 host gates** (was 234): + test-devwire(+mut). devices.o links into the freestanding kernel; _kernel_end=0x2fae0 < 0x2ff00.
- Full emu suite (CON-path regression -- the universal output path) re-run GREEN across all 39 gates: the 11 pre-sysinit gates (boot/program/fs/type/dir/exec/mzexec/mcb-emu/fatwrite/multiopen/exit-handles) + test-sysinit (fixed) + every post-sysinit gate (shell/ut6d/zs24-exec/panic/spurious/datetime/kbd/conin/vect/absdisk/irqstorm/samir-boot/samir-write/canon-y2k/canon-salami/autoexec, each +mutant where applicable). CON output/input preserved (test-conin/datetime/shell/samir/autoexec all PASS).
- **A real CON-path regression caught by the full emu suite (NOT host):** 6zd9 changed sysinit's `DEVICE=ANSI.SYS` from `accepted(deferred)` to `ansi-enabled` (it now sets g_ansi_enabled for p96i) -- correct + more accurate, but it dropped the exact serial line `test-sysinit` asserted -> the gate went RED. Fix: UPDATED the gate to assert `^SYSINIT: DEVICE=ANSI.SYS ansi-enabled$` -- a STRENGTHENING (the gate now verifies ANSI.SYS is specifically ENABLED, not merely recorded-as-deferred), not a relaxation; the "directive recorded, not silently dropped" intent is preserved and the gate's bite was demonstrated (it went RED on the changed behavior). LESSON (re-affirmed): the subagent ran only HOST gates; the emu suite caught the sysinit serial regression -- emu gates catch what host misses (WL-0038 HARD LESSON).
- All ASCII-clean; reproducible.

## Pointers / next work

- **Follow-ups filed:** the kernel-window headroom fix (raise PROGRAM_BASE / share FAT buffers), the in-emulator OPEN-by-name device gate (a .COM that OPENs NUL/CON/PRN -- host oracle is authoritative meanwhile), plus the carried 509.7 wiring refinements (full CON unification onto devices.c; CLOCK$ WRITE date-half; per-class IOCTL device-info words; a real LPT1/COM1 port driver for PRN/AUX).
- **509.7 status:** CORE (Wave 2) + the int21/sysinit WIRING (this wave) are done; 509.7 can close once the in-emu device gate confirms (or be closed now with that gate filed). 40oq's 509.7 dep is effectively satisfied for the Core scope.
- **Remaining `40oq` deps:** p96i (x3mh ANSI CON wiring -- now unblocked: 6zd9 set the DEVICE=ANSI.SYS flag + established the single CON output path ANSI.SYS hooks) is the last doable dep; then 40oq is BLOCKED on the OPERATOR-DEFERRED kzfs (MBR partition) + slvd (multi-volume). The capstone's open question -- does 40oq certify without partitions+multivol, as it does with FCB stubbed? -- is committee/operator-worthy and should be raised when p96i lands.
