<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# WL-0038 -- DOS-3.3 parity push: Tranche E/F/G + the MZ .EXE end-to-end (orchestrated)

**Type:** Feature push (orchestrated, multi-wave). **Date:** 2026-06-20. **Branch:** command-com-default (pushed: 34843df..a99e451).
**Operator steer:** pause FLAIR (golden masters being minted in sister repos ../system7-decomp + ../win31-decomp); drive MILTON to full DOS 3.3 parity; orchestrate (delegate coding to subagents, orchestrator grades/integrates/commits; <=6 sonnet / <=2 opus parallel; committee for serious decisions; .EXE wanted soon). Continues WL-0037.

## Context

A parity state-of-play audit (5 parallel readers + synth + adversarial verify) mapped the gap: the INT 21h kernel surface was ~80-85% of ADR-0003 Appendix A, but the COMMAND.COM shell was the biggest hole (~25-30%), gated by the absence of an environment store. Two OPERATOR RULINGS were locked (see `bd memories`): (1) parity target = "Appendix-A now, amendments later" (COUNTRY/SHARE/INT-2Fh -> a separate amendment-gated epic); (2) the 40oq capstone certifies with FCB stubbed.

## What changed (waves, each RED->GREEN, mutation-proven, independently re-graded + commit-per-wave)

- **Tranche E (env) COMPLETE -- bead `1i0x` CLOSED.** inc-1 `env.c` master environment store (DOS env-block format, UPSERT, overflow-guarded); inc-2 the `SET` built-in + a file-scope `g_master_env` seeded COMSPEC/PATH/PROMPT, wired into the REPL; inc-3 EXEC env inheritance (`loader_decide_env`: command.c serializes the master env into ENV_BLOCK + passes env_block==ENV_BLOCK; the loader writes the empty block only when none is provided; baked path byte-identical).
- **MZ `.EXE` END-TO-END (ADR-0003 DEC-08a, committee-ratified).** A 3-lens committee (loader-architect/period-purist/north-star) ruled (NO gridlock): **InitechMZ** = a genuine MZ container interpreted FLAT -- 32-bit flat load module, u32 flat-base relocations (add PROGRAM_IMAGE to the dword), real header fields honored (e_minalloc/maxalloc get MCB teeth), content-based .COM/.EXE dispatch, a tag byte (0x4943 at e_res[0]) + a fail-loud panic on an untagged foreign 16-bit MZ (never misexecute 16-bit code in 32-bit mode), and a HARNESS-ONLY parse-only genuine-16-bit reader. Honesty boundary stated. Landed: `mz.{c,h}` unit (`dtw.1`, test-mz), the loader integration (`dtw.2`/wczy, test-mzload -- relocate-BEFORE-move so a header-resident reloc table survives), and **`0kiq` -- a real .EXE PROVABLY RUNS in-emulator** (mzlink factory tool + a reloc-dependent fixture + test-mzexec: MZEXEC-OK only if the reloc resolved at runtime, + a foreign-MZ fail-loud panic gate; test-mzexec-mutant: the no-reloc image stays silent).
- **Tranche F shell verbs -- `hpls`/`fyox`/`uy4l` CLOSED.** COPY, DEL/ERASE (wildcard via FINDFIRST/NEXT), REN/RENAME (AH=56h), DATE, TIME -- on existing oracle-backed primitives. (Deferred: wildcard COPY, DEL *.* prompt, cross-dir REN.)
- **Tranche G start.** `dibc` PROMPT + the pure `$P$G` template engine (12 $-metachars). `atf` PATH-directory search + COMSPEC (cmd_path_candidates wired into run_external). `xw1` the PURE .BAT parser/expander module (`batch.{c,h}`: batch_expand/classify/label_matches) -- the command.c REPL + AUTOEXEC integration is the deferred remaining half.

## Frictions (the load-bearing lessons)

1. **A real regression I introduced + caught via emu (commit 3009091).** inc-3 made do_exec read env_block from the AH=4Bh param block and feed it to loader_decide_env, which fail-loud-rejects any value != {0, ENV_BLOCK}. A caller issuing AH=4Bh with a stale/garbage EBX -> BAD_ENV -> BAD_FORMAT, breaking a valid .COM EXEC. **Caught by the test-exec EMU gate, NOT the host test-exec-unit** -- I had committed inc-3 + dtw.2 on host-green alone. Root fix: do_exec SANITIZES env_block at the INT 21h trust boundary (honor only {0, ENV_BLOCK}, else degrade to inherit-empty -- matching its own Rule-2 intent + the cmd_tail guard). **LESSON (bd remember): EXEC-path / int21 / loader edits MUST re-run the emu EXEC gates (test-exec, test-program, test-zs24-exec, test-ut6d, test-samir-boot), not just `make test-unit`.**
2. **The test-mzexec gate: a serial-file flush race + cold-TCG slowness.** A post-hoc `grep` of the .serial file raced QEMU's flush (the harness reports serial_len early; the file completes after). Fix: assert via the harness `--expect` (LIVE serial read -> marker_found in the report; race-free) and judge on marker_found, NOT the harness exit code (these self-test kernels don't halt after their leg -> they time out -> non-zero exit even on success; same idiom as test-exec's `|| true`). 30s cold-TCG ceiling.
3. **Boot-geometry obligation (Rule 5).** The cumulative kernel growth (env + mz folded into loader.o + SET + Tranche-F verbs) pushed kernel_shell.bin past the 80 KiB window -> KERNEL_SECTORS 160->208 (Makefile + stage2.asm equate), IMG_SECTORS 192->256. Re-verified QEMU + Bochs.
4. **Parallel lanes on DISJOINT files in one tree are safe; shared files (command.c, loader.c) are SERIAL.** Two sonnet PROMPT/PATH lanes applied their own Makefile mutant targets (told not to, but harmless + correct). One agent correctly detected a parallel lane's concurrent loader.c edits.

## Acceptance / state

- `make clean && make test-unit` = **ALL GREEN 224 host gates** (was 216 at session start). New: test-env(+mut), test-mz(+mut), test-mzload(+mut), test-batch(+mut), + command-mutant grew to 11 (SET/COPY/DEL/REN/DATE/TIME/PROMPT/PATH). New EMU gates: test-mzexec(+mutant) in TEST_EMU_GATES. Re-verified emu: test-exec, test-program, test-zs24-exec, test-ut6d, test-samir-boot, test-shell, test-mzexec, test-boot-bochs all PASS.
- ADR-0003 DEC-08a written (`docs/adr/ADR-0003-AMENDMENT-DEC-08a-MZ-EXE-Flat-Loader.md`).
- Pushed: 34843df..a99e451 on origin/command-com-default.

## Pointers / next work

- **Remaining Tranche G:** the `xw1` command.c integration (run .BAT line-by-line, AUTOEXEC.BAT at boot, IF/FOR/SHIFT/CALL/PAUSE/ECHO-state/ERRORLEVEL -- the batch.c parser is ready); `509.10` (mint an Appendix-D boot vol + AUTOEXEC runs).
- **Tranche I:** `mvg` (wire INT 24h to real ATA/FAT errors), `509.7` (device chain: CLOCK$/NUL/AUX/PRN + ANSI.SYS `x3mh`), `bcg.12` (ata error paths), `bcg.13` (msg-catalogue scanner).
- **`bo40`** AH=31h KEEP/TSR -- the last undispatched Appendix-A CORE fn; required before the **`40oq`** capstone (Appendix-A coverage certificate; certifies with FCB stubbed per the ruling).
- **Deferred/filed:** `0kiq`-style Bochs leg blocked on `x0i` (harness no --disk2); MZ overlays (`dtw.4`), harness 16-bit differential reader (`dtw.3`); honor arbitrary caller env blocks (filed); wildcard COPY + DEL *.* prompt (filed); the in-emu env-echo leg (filed).
- **Out of the parity milestone (amendment-gated, separate epic):** COUNTRY/codepage, SHARE/locking, INT 2Fh redirector (`om2a`/`ws3x`/`t1hl`).
