# WL-0024 -- ut6d + zs24: the subdirectory chain becomes usable end-to-end

Branch: `command-com-default`. Continues the ti8 subdir chain
(`ti8 L1 -> mzxa L2 -> u6wa -> ut6d -> zs24`). Orchestrated: the main thread
drove a serial, oracle-gated chain (Rule 3 / WL-0023 directive: do NOT
parallelize the shared-file kernel edits), delegating each implementation to a
single Opus subagent and gating every landing on a self-run `make test` PLUS an
independent adversarial-verify fan-out before commit. Sonnet was reserved for
summarization; the parallelism budget went to grounding + adversarial review,
never to concurrent edits.

## Context

After u6wa (CHDIR + root MKDIR/RMDIR dispatch), two links remained: the shell
still had no MD/RD and a fail-loud CD stub (`initech-ut6d`), and the FAT12 write
primitives + EXEC were ROOT-only (`initech-zs24`). A read-only grounding fan-out
(5 agents) scoped both beads + the oracle strategy against the actual repo
before any code (Law 1), and surfaced the one operator-gated fork.

## What changed (each RED-first, mutation-proven, adversarially verified, committed)

- **`initech-mc7r` -- DOS message catalogue amendment** (`9e2238b`). Faithful
  MD/RD/CD diagnostics needed directory-failure strings the DEC-13-locked
  `spec/dos_messages.json` lacked. Operator ratified amending the catalogue
  (Law 4). Record-first (ADR-0003 Appendix C + an amendment note), then the
  verbatim JSON, then the coupled gate bounds moved 16->19 in lockstep
  (generator + test-spec[3/5] + test-dosmsg TOOTH 1/3 + the mutant text list).
  MSG-DOS-0017 "Unable to create directory" / 0018 "Invalid directory" / 0019
  "Invalid path, not directory, or directory not empty" -- one message per
  command, DOS 3.3 canon.

- **`initech-ut6d` -- shell MD/RD/CD** (`ea4b47d`). command.c/.h: CMD_MD/CMD_RD
  + MD/MKDIR/RD/RMDIR classify rows; four INT 21h wrappers (AH=39/3A/3B/47h on
  the dos_open template); `cwd_display()` ("A:\"+GETCWD) shared by the prompt
  and CD-no-arg; builtin_md/builtin_rd (silent success) + rewritten builtin_cd;
  the $P$G prompt is now GETCWD-composed (root byte-identical "A:\>", subdir
  "A:\SUB>"). Oracle: HOST classifier gate (+CMD_MUTATE_NO_MDRD) + EMU test-ut6d
  (md sub/cd sub/dir/cd../rd sub/cd sub) asserting the subdir prompt, the DIR
  dot-dirs, AND RD-removal (post-RD re-`cd sub` fails "Invalid directory");
  2-leg mutation-proof, each confirming the mutant BOOTED before trusting RED.

- **`initech-zs24` L1 -- subdir file WRITE** (`610c656`). fat12.c grew the
  parent-aware core: `fat12_scan_dir` (root region OR subdir cluster-walk),
  `fat12_write_dirent_in_dir`/`_read_dirent_in_dir`, `fat12_subdir_slot_lba`,
  and `fat12_grow_dir` (append a zero-filled cluster when a subdir fills;
  allocate-then-commit + rollback). The historical root fns are now
  byte-identical is_root=1 wrappers. fileio_fat.c guards lifted; the SFT carries
  `dir_start`. A fail-loud mount guard rejects `sectors_per_cluster != 1` (the
  write path's invariant; Rule 2). Oracle (test-zs24, real backend over
  fat12_nested.img, differential vs mtools mcopy/mdir AND python fat12_ref.py
  `--cat-path`): CREATE+WRITE (2-cluster), LSEEK+WRITE, UNLINK, root-regress,
  and a GROW step that fills \SUB past its 16-slot cluster. FIVE mutants, each
  confirming it ran then RED for the right reason (write-back, CREATE gate,
  UNLINK, grow relink/EOC, grow-refuse).

- **`initech-zs24` L2 -- subdir EXEC** (`b67028b`). do_exec drops the
  root-only reject and resolves via `resolve_dir_path` -- the EXACT seam do_open
  uses (one path grammar). The resolved (leaf, dir_start) threads into
  load_program_from_fat, which branches dir_start==0 -> byte-identical
  `fat12_find` (root EXEC unchanged) / dir_start!=0 -> `fat12_find_slot_in`. A
  fail-loud dir-attr guard refuses loading a directory as a .COM (closing a
  latent pre-zs24 hole). Oracle (test-zs24-exec): GREET.COM minted ONLY into
  ::SUB runs absolute (\SUB\GREET) AND CWD-relative (CD SUB; greet), proving
  the CWD is not corrupted; 2 mutants (do_exec reject / loader root-only).

## Acceptance

`make test` GREEN end-to-end: **70 host + 26 emu gates** (was 68+22 at WL-0023;
+test-zs24/-exec/-ut6d & their mutants). Every landing re-verified on the MAIN
thread from a clean tree (Law 2), the FAT/shell/EXEC root-regression sweep
re-run per landing (Rule 3 byte-identity), and each independently
adversarially verified (ABI/safety, regression/fidelity, oracle-rigor lenses).
The adversarial pass EARNED ITS KEEP: it caught that `fat12_grow_dir` shipped
UNTESTED despite a green suite (the agent claimed a one-off grow check it never
committed) -- closed with a grow oracle + 2 grow mutants before commit.

## Frictions / lessons

- **Clock-skew gotcha (env, now in `bd memories`).** build/ artifacts can
  acquire FUTURE mtimes -> GNU make skips ALL rebuilds -> false-green/false-red
  oracles (symptom: "Clock skew detected" + a mutation-proof that won't bite
  because the intermediate `make <bin>` is a silent no-op). `make clean` before
  trusting an incremental oracle run. Every authoritative re-verify this session
  was `make clean && make test`.
- **"Green suite" != "verified".** A subsystem can be green while its riskiest
  new function is never executed by any gate (zs24's grow path). The adversarial
  oracle-rigor lens is what catches this; trust the oracle, but make the oracle
  actually run the code (Law 2 / Rule 6).
- **Match the existing seam, don't invent.** Both ut6d (prompt == CD-no-arg via
  one cwd_display) and zs24 L2 (EXEC resolves == OPEN resolves) avoided a second
  path grammar by reusing the established seam -- fewer divergence bugs.

## Pointers / next

The subdirectory personality is now usable end-to-end: CHDIR/MKDIR/RMDIR +
shell CD/MD/RD + subdir file CREATE/WRITE/UNLINK + subdir EXEC (absolute &
CWD-relative). Remaining subdir follow-ups (all filed, none blocking):
- **`initech-m0bp`** -- nested MKDIR/RMDIR (non-root parent); the L1 infra
  (scan_dir/write_dirent_in_dir/grow) makes this a focused follow-on.
- dir-attr-guard mutation-proof + tighten the EXEC mutant leg-A (`n==1`).
- CWD-aware DIR header ("Directory of A:\SUB"); AH=39/3A/3B/47 rows in
  int21h_calling_convention.json; the FAT free-cluster-hint fragmentation quirk.
Then resume the DOS-3.3 parity push (epic `initech-bsy`): `bd ready`.
