# WL-0025 -- DOS-3.3 INT 21h/25h/26h parity tranche (CREATNEW, FILETIME, CHMOD, RENAME, IOCTL, absolute-disk) + DEC-15

Branch: `command-com-default`. Continues the DOS-3.3 feature-parity push (epic
`initech-bsy`). Orchestrated from the main thread: parallel read-only GROUNDING
and parallel ADVERSARIAL VERIFICATION, but strictly SERIAL oracle-gated coding
(every landing shares `os/milton/int21.c` -- Rule 3 / WL-0023-24: do NOT
parallelize shared-file kernel edits). One Opus subagent per implementation;
every landing gated on a main-thread `make clean && make test` re-run PLUS an
independent adversarial fan-out before the bead closed.

## Context

After the subdir chain (WL-0024) the remaining Appendix-A INT 21h gap was the
recognized-but-undispatched handlers (CHMOD 43h, IOCTL 44h, RENAME 56h, FILETIME
57h, CREATNEW 5Bh) plus the nested-MKDIR follow-on, and -- by operator decision
-- the out-of-Appendix-A absolute-disk vectors (INT 25h/26h). A grounding
fan-out (8 read-only briefs + a conflict-group synthesis) mapped the serial
order and surfaced six operator-gated forks before any code (Law 1).

## What changed (each RED-first, mutation-proven, adversarially verified, committed)

- **`initech-kji0` -- INT 21h AH=5Bh CREATNEW** (`5a8f78f`). do_creat clone + a
  side-effect-free pre-existence probe (CF=1/AX=0x0050 if exists); shares the
  resolve_dir_path subdir seam. 4 mutants. Adversarial-clean.

- **`initech-m0bp` -- nested MKDIR/RMDIR (non-root parent)** (`adff903`) + a
  **rollback fix** (`de4b538`). Lifted the root-only guards in fat12_mkdir/
  fat12_rmdir onto the parent-aware Layer-1 infra (scan_dir/write_dirent_in_dir/
  grow). Adversarial review found a REAL P2 leak: three post-grow mid-op failures
  (NO_SPACE / set_entry-EOC / flush) returned without `fat12_shrink_dir_tail`, so
  the appended parent cluster leaked while MKDIR reported failure -- the commit's
  "mirrors fat12_create / neither leaks" claim was false. Fixed all three returns
  + a geometry-driven NO_SPACE fault-injection oracle that bites. (The classic
  "green suite != verified" -- the riskiest new function was unexercised.)

- **ADR-0003 Amendment DEC-15 (OEA-ADR-0003-A3)** (`1d0f31e`). An ARB-by-committee
  deliberation (4 role-played reviewers + chair) ratified adding the INT 25h/26h
  absolute-disk vectors. Rulings: the famous leftover-FLAGS-on-stack wart is
  DOCUMENTED-AND-OMITTED (the DEC-14 philosophy; our 0x8F trap-gate IRETD can't
  leave a stray stack word); a SEPARATE AL/AH hardware-error space (not the
  INT21_ERR_* enum); vectors 0x25/0x26 confirmed collision-free (DEC-10
  precedent); a NEW locked `spec/absdisk_int2526.json` (kept OUT of the INT-21h
  test-spec AH-walk). Operator accepted as drafted.

- **`initech-ro6c` -- INT 21h AH=44h IOCTL AL=00 Get-Device-Info** (`03e67ae`).
  Full DX word locked (CON 0x80D3 / FILE 0x0040, PRM-grounded) in
  int21h_calling_convention.json + asserted; AL=01/06/07/08 minors deferred
  (`initech-4nbn`). 5 mutants. Adversarial-clean behavior (the dead bit-name
  macros were mislabeled -> folded into 4nbn).

- **`initech-qekc` -- INT 21h AH=57h FILETIME** (`b0c4e2d`). GET/SET file
  date+time by handle; CX/DX copied verbatim into mtime/mdate (the DOS pack IS
  the FAT pack). New `set_time` vtable seam (all 6 -- now 7 -- positional
  initializers updated). The oracle INVERTS the codebase's mtime-normalization,
  asserting only caller-written bytes (Rule 11 preserved). 6 mutants. Bumped
  KERNEL_SECTORS 128->144 -> `make test-boot-bochs` re-run PASS (Rule 5
  tri-emulator obligation). Adversarial-clean (P3 stale comment fixed `5b275f5`).

- **`initech-b53d` -- INT 21h AH=43h CHMOD** (`76c4c8f`) + a **GET-on-directory
  fidelity fix** (`c7bffac`). Attribute-byte-only RMW; operator-approved SET
  reject set (dir/vol-label target + Directory/VolLabel CX bits -> 0x0005,
  recorded as an intentional deviation in `initech-5o6o`); 3-way FAT differential
  (fat12_get_attr/python/mtools). Adversarial review found a REAL P2 Law-4
  defect: GET (4300h) on a directory returned 0x0005 instead of CX=0x10 (the
  canonical DOS stat-a-path idiom; RBIL); the oracle was green only because the
  test asserted the deviation. Fixed: GET now returns 0x10; SET reject kept.

- **`initech-gnrc` -- INT 21h AH=56h RENAME** (`0869ca0`). Same-directory
  dir-entry rename; first handler reading a SECOND flat pointer (EDI). Name-field
  -only RMW (chain/size/dates/FAT untouched); dest-exists is the load-bearing
  reject; cross-dir -> 0x0011 (move deferred `initech-ycb3`); dir-target ->
  0x0005. mren differential + 6 mutants. Adversarial-CLEAN: the self/case-only
  rename was verified DOS-faithful by running mtools `mren` (rejects identically).

- **`initech-4mq7` -- INT 25h/26h absolute disk R/W** (`952e019`). isr stubs
  byte-identical to the emu-proven int24 template; int21 dispatch + int24-mirrored
  reentrancy guard; a kmain-bound block-device seam (mounted-only, fail-loud
  unbound; int21.c stays free of fat12.h/blockdev.h); AL=0=A:; the omitted
  FLAGS-stack wart; bounds-checked-before-IO. `test-absdisk` (round-trip +
  independent cross-check + boot/FAT/root non-corruption snapshot + error legs,
  safe scratch LBA=total-1 asserted-free) + `test-spec[6/6]` + 8 DEC-15 mutants.
  Adversarial-CLEAN (the untested asm-entry path is provably-correct-by-identity
  to int24, which `test-vect` exercises end-to-end on QEMU).

## Acceptance

`make clean && make test` GREEN end-to-end: **85 host + 26 emu gates** (was 70+26
at the start of the session). `make test-boot-bochs` PASS (re-verified for the
qekc KERNEL_SECTORS 128->144 geometry change; 144 still fits IMG_SECTORS=192's
3-cylinder window). Every landing re-verified on the MAIN thread from a clean
tree (Law 2), and independently adversarially verified before its bead closed.

## Frictions / lessons

- **Adversarial verification earned its keep, twice.** A green, mutation-proven
  suite still hid (a) m0bp's post-grow parent-cluster leak -- the riskiest new
  function had no fault-injection oracle -- and (b) b53d's GET-on-directory
  fidelity bug -- the oracle was *written to assert the deviation* (Law-2
  "oracle asserts the wrong truth"). Both were P2, both invisible to `make test`.
- **Operator-gate the scope, not the mechanics.** Six forks went to the operator
  (DEC-15 amend-now + accept; 4mq7 implement-now; kzfs defer; ro6c full-word
  spec; gnrc/b53d/qekc DOS-faithful defaults). The in-scope DOS-semantics ran on
  documented defaults; only the ADR amendment + the on-disk-contract fork
  (kzfs, deferred to DEC-07 amendment `initech-t1on`) needed ratification.
- **The vtable-initializer landmine is real and grows.** Each of qekc/b53d/gnrc
  appended a backend member; the positional brace-list literals grew 6 -> 7 -> 8
  -> 9, and every one must get the trailing field or the C aggregate-init
  silently misassigns (a miscompile, not a build error). A fresh grep per landing
  was the discipline.
- **Geometry changes are a tri-emulator obligation (Rule 5).** qekc's
  KERNEL_SECTORS bump was QEMU-green but the agent skipped Bochs; the main-thread
  re-verify added `make test-boot-bochs` (PASS). 4mq7 fit the window (no bump).
- **Match the existing seam.** Every handler reused do_unlink / resolve_dir_path /
  the (dir_start,slot) write-back / the int24 stub template rather than inventing
  a second path -- fewer divergence bugs.

## Pointers / next

The Appendix-A INT 21h surface is now fully dispatched (CHMOD/IOCTL/RENAME/
FILETIME/CREATNEW done; MKDIR/RMDIR/CHDIR earlier), plus the DEC-15 INT 25h/26h
vectors. Epic `initech-bsy` is 8/22. Filed follow-ups (none blocking): `4nbn`
(IOCTL minors + bit-name labels), `5o6o` (CHMOD SET-reject deviation, doc),
`isil` (gnrc non-root same-dir rename leg), `ycb3` (cross-dir RENAME move),
`nmpo`/`glsw` (CREATNEW oracle/robustness), `lpf3` (write-fail fault-injection
backend for the rollback paths), `cnvp` (absdisk spec bad-buffer row), `8403`
(in-emu int 25h/26h self-test), `t1on` (DEC-07 MBR-partition amendment, blocking
the deferred `kzfs`). Resume the parity push: `bd ready`. The capstone
`initech-40oq` (Appendix-A coverage certificate) is the natural Phase-1 closer.
