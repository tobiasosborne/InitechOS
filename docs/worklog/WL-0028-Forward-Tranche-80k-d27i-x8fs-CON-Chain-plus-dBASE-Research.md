<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0028 -- Forward tranche (80k wildcard, d27i windowed FAT16, x8fs/er3h/4tw CON cluster) + InitechBase (SAMIR) ground-truth research

Branch: `command-com-default`. The forward tranche pre-grounded in WL-0026/27,
plus the M6 InitechBase evidence base. Orchestrated across four workflows --
(1) parallel read-only grounding, (2) a parallel-worktree implementation tranche,
(3) a serial main-tree CON chain, (4) a parallel adversarial-verification pass --
with the orchestrator owning ALL integration, re-verification (Law 2), conflict
resolution, and the bead ledger. A fifth workflow (parallel) produced the dBASE
research brief in parallel with the kernel work.

## What changed

### Code -- the parallel tranche (region-disjoint, coded in isolated worktrees, integrated serially)
- **`initech-d27i` (P1, commit `5a6361a`):** windowed/streaming FAT-sector read so
  a real FAT16 volume mounts in-kernel. `fileio_fat_bind` is now **mode-aware** --
  a FAT12 volume keeps the whole-FAT `g_fat` (the WRITE path needs it), a FAT16
  volume installs a 2-sector windowed reader (READ-only); `fat12_read_fat_sector`
  fetches only the FAT sector(s) for the current cluster, fetching TWO on a FAT12
  12-bit straddle (`off%512==511`), with a 1-2-sector cache. FAT12 byte-identical.
  `test-d27i` + 3 mutants (sector off-by-one / no-straddle / no-window-dispatch).
  Completes the FAT16 read that WL-0027's z01 left partial (the `g_fat[12*512]`
  too-small problem). FAT16 WRITE still deferred (509.11).
- **`initech-80k` (P2, commit `ce76f69`):** the FCB 8.3 wildcard matcher
  (`build_pattern`/`pattern_match`) was found **already correct** on this branch --
  the bead premise was stale. The contribution is the mutation-proven oracle that
  LOCKS it (`test-80k` + 4 mutants: qmark-literal / match-exact / star-bleed /
  no-upcase) + 7 FINDFIRST integration checks. Corrected the grounder's wrong
  `FOO?.*`/`FOO.TXT` guess against ground truth (`?` matches the trailing pad-space;
  Raymond Chen, *The Old New Thing*, 2007).
- **`initech-x8fs` (P2, commit `ddef39a`):** `AH=3Fh` on a CON handle delivers
  cooked line input (line+CR+LF, count inclusive per MS KB Q113058) via a shared
  `conin_cooked_line()` extracted from `do_buffered_input` (0Ah byte-layout
  preserved -- proven by the surviving 0Ah mutants). `test-x8fs` + mutant.
  **Includes a host-hang guard** (see Frictions).

### Code -- the serial CON chain (coded in the MAIN tree, not worktrees -- see Frictions)
- **`initech-er3h` (P2, commit `7c21b0b` + fix `8d3f287`):** `AH=33h` Get/Set
  CTRL-BREAK per ratified **DEC-16** -- `g_break_flag` (default ON),
  `int21_set/get_break_flag` seam, `CONFIG.SYS BREAK=ON|OFF` -> SYSINIT, `BREAK
  [ON|OFF]` shell built-in. Locked-spec edits (Rule 8, atomic per DEC-16): the
  `AH=33h` row in `int21h_register.json` + the calling-convention stanza +
  `test-spec` strengthened. `test-er3h` + **4** mutants.
- **`initech-4tw` (P2, commit `e4b2222`):** CON-input `^C` (0x03) -> INT 23h
  check-point via shared `conin_check_ctrlc`, hooked into `01h/08h/0Ah`. Correctly
  grounded the DOS exceptions: `07h` (DIRECT) and `06h` deliver 0x03 RAW --
  **overriding the bead's file-touch map per Law 1**. Gating per DEC-16 Fork A:
  the CON family is ALWAYS a check-point (ON-widening to other INT 21h calls is
  followup C-6). Bumped `KERNEL_SECTORS 144->160` (kernel grew) -- a boot-geometry
  change, so the Bochs boot leg was re-run (Rule 5).

### Docs / research
- **InitechBase (SAMIR) ground-truth brief** (commit `219f2d6`,
  `docs/research/dbase-ground-truth.md`, ~8 k words): the M6 evidence base --
  `.dbf/.dbt/.ndx/.mdx` on-disk formats, the MEANINGFUL-vs-NORMALIZE byte
  contract, the xBase dot-prompt subset + evaluator + the proposed
  `spec/xbase_coercion.json` schema + SET EXACT semantics, the three mechanical
  oracles (round-trip / differential program-output / coercion fuzzer) mirroring
  `harness/diff/fat_diff`, the runtime model + M2/M4 deps, the deadpan canon, a
  phased build plan, operator questions + a risk register. Load-bearing pins:
  `C+N` is an ERROR in III+/IV (NOT the modern PLUS-doc auto-stringification);
  `.dbt` endianness + `.mdx` node internals are ORACLE-RESOLVE/deferred (never
  inferred); the Y2K/rounding canon bugs are ENFORCED (Law 4).
- **M6 bead hygiene:** re-parented the 7 engine beads (`aul/7az/ahu/gmo/0tl/17n/ax9`)
  under epic `586` (now 0/11); wired `586.1 -> aul+7az`; filed `586.3`
  (`dbf_normalization.json` locked byte-map) + `586.4` (differential corpus +
  golden-minting harness, P2); annotated the coarse engine beads with the
  decomposition guidance.

## Acceptance
- `make test` = **103 host + 27 emu gates GREEN** (was 93+27 at session start;
  +10 host = d27i/80k/x8fs/er3h/4tw x2). Re-verified from clean by the orchestrator
  after EVERY integration step, timeout-guarded.
- **`make test-boot-bochs` PASS** (the `KERNEL_SECTORS 144->160` geometry change is
  a tri-emulator obligation; verified on QEMU via the default gate + Bochs).
- Every new gate mutation-proven (each mutant confirmed RED for the right reason);
  every new gate WIRED into `TEST_UNIT_GATES` (orphaned-gate gaps caught + fixed).
- Adversarial verification (5 read-only skeptics): 80k UPHELD, d27i UPHELD, x8fs
  WEAKNESS (filed `bsy.6`, not reachable), **er3h REFUTED -> fixed** (`8d3f287`),
  4tw UPHELD.

## Frictions / lessons (four real ones)
1. **Workflow worktree-isolation spawns from stale `main` (`t6nc`, P2).** `main`
   is pinned at `e56695a` (WL-0019); `isolation:'worktree'` based the impl
   worktrees there, not the `command-com-default` tip. The `d27i` agent caught it
   and `git reset` onto the tip; `80k`/`x8fs` were on the stale (but linear-ancestor)
   base, so they cherry-picked cleanly. **Mitigations:** keep `main` fast-forwarded
   at session close; the serial CON chain ran in the MAIN tree to sidestep it.
2. **A host oracle HANG -- worse than red (Rule 2).** `x8fs`'s blocking cooked CON
   read spun forever in `conin_cooked_line` on `conin_get()`'s no-source `0`
   sentinel; it only surfaced under the FULL aggregate gate (via `test_fileio`'s
   `kji0` CREATNEW mutants), froze the gate ~9 h, and a per-target check missed it.
   Root-cause fix: a no-source/no-pushback EOF guard at the top of the cooked-line
   reader. **Lessons:** run the full `make test` (not just per-target) at
   integration, ALWAYS timeout-guarded; saved to `bd memories`
   (`host-oracle-hang-pattern-conin-get-returns-0`).
3. **Agents forget to wire new gates into `TEST_UNIT_GATES` (caught x2).** `d27i`
   and `x8fs` added gate RULES but not aggregate membership, so the first `make
   test` silently skipped them (count stayed 93). The orchestrator caught it via
   the gate-count delta. Subsequent agents were told explicitly to wire + verify
   the count; both did.
4. **Adversarial review earns its keep again.** The skeptic pass found a REAL
   DEC-16 deviation: `er3h`'s `AH=33h` SET wrote `AL`, but DEC-16 3.2 (241-242)
   mandates SET changes no register but the flag+CF. **Root cause: the
   orchestrator's own er3h prompt mis-stated the contract** ("return AL=normalized").
   Fixed at root + a new M7 mutant proves the no-output-register contract (the
   SET-OFF case is the one where "AL unchanged" != "AL=flag", so the assertion
   bites). Reinforces the WL-0017 DEC-04a-ARB lesson: independent perspectives +
   mutation-proving > a single green pass.

## Pointers / next
- Beads closed: `80k`, `d27i`, `x8fs`, `er3h`, `4tw`. Epic `bsy` advanced through
  groups F/H of its sequenced order.
- Filed (non-blocking): `t6nc` (infra), `bcg.16` (in-emulator FAT16 mount oracle ->
  closes `40oq` FAT16-green), `bcg.17` (FAT16 free-space perf), `bsy.1` (DIR
  wildcard emu leg), `bsy.2` (x8fs over-long line remainder), `bsy.3` (AUX/PRN
  read), `bsy.4` (DEC-16 C-6 ON-widening), `bsy.5` (DEC-16 C-7 86Box default),
  `bsy.6` (x8fs defensive cooked-line bound). M6: `586.3`, `586.4`.
- Next forward (per `bsy` sequenced order + `bd ready`): `80k`'s DIR wildcard emu
  leg (`bsy.1`), the in-emulator FAT16 mount oracle (`bcg.16`) toward the `40oq`
  capstone, and the remaining MILTON shell built-ins (COPY/DEL/SET/DATE/TIME...).
- M6 InitechBase is grounded but gated behind M2/M4; operator questions captured in
  the brief Sec 9 (real-dBASE/86Box authenticity tier; `.ndx`-first vs `.mdx`;
  host-reference-interpreter as the differential gate).

---

*-- End of Shard WL-0028 --*

<!-- Tedium certified compliant with NFR-7. -->
