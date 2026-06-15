# WL-0027 -- FAT16 milestone (dao streaming walk + z01 decode layer) + DEC-16 / DEC-07a ratifications

Branch: `command-com-default`. The first forward capstone-path tranche after the
WL-0025/26 follow-up debt was discharged. Orchestrated: a serial coding workflow
(dao -> z01) + parallel adversarial verifiers + a parallel ADR-by-committee
drafting workflow, with the orchestrator owning all re-verification (Law 2),
ADR finalization, and commits.

## What changed

### Code (commit 654e48a)
- **`initech-dao` (P2):** replaced the `uint16_t chain[2880]` (~5.6 KB) on-stack
  buffer in `fat12_read_file` with an incremental `fat12_next_cluster` streaming
  walk (mirroring the proven `fat12_read_partial`), preserving the anti-hang
  guards, the RISK-5 last-cluster truncation, and the corruption-fuzzer contract
  (a cyclic chain whose declared size fits one cluster is still walked to EOC and
  rejected -- O(1) stack, identical semantics). New `test-fat-readfile-mutant`
  (3 mutants) + a 700060-byte BIGCHAIN.TXT large-file leg in test-fat/test-fat12-dir.
- **`initech-z01` (P1, DECODE LAYER + HOST ORACLE -- partial vs the bead title):**
  FAT16 read **decode** -- `fat16_next_cluster` (16-bit entry; EOC 0xFFF8..0xFFFF;
  bad 0xFFF7) + `fat_type` dispatch in the cluster/EOC/free/bad helpers; `fat12_mount`
  now ACCEPTS + classifies FAT16 by cluster count (FAT12 path byte-identical; FAT32
  + spc!=1 still fail loud). New `test-fat16` (3-way differential: our decode vs
  mtools vs `fat16_ref.py`, incl. a 2.5 MB volume whose chain crosses cluster 0xFF8)
  + `test-fat16-mutant` (M1-M5). `docs/research/fat16-ground-truth.md` grounds it.
  **Honest scope:** the kernel cannot yet mount a real FAT16 volume in-emulator --
  the whole-FAT `g_fat[12*512]` buffer is far smaller than a FAT16 FAT (fails loud,
  no corruption). The windowed FAT-sector read is filed as **`initech-d27i`** (now a
  `40oq` dependency). FAT16 WRITE deferred (509.11); initrd/tarfs deferred
  (`initech-qywt`); partitioned/MBR is kzfs/DEC-07a (out of z01).

### ADR ratifications (commit <this>)
The operator ratified BOTH committee drafts "as drafted" (2026-06-15):
- **DEC-16 (OEA-ADR-0003-A4)** -- admits INT 21h **AH=33h** (Get/Set CTRL-BREAK) to
  Appendix A. Resolutions: ^C semantics **Fork A (phased)** (4tw lands the CON
  check-point; ON-widening to every INT 21h is a forward obligation); boot default
  **ON** (PRM, flagged for 86Box, erratum-flippable); class Resident, mnemonic BREAK,
  DL normalized. **Unblocks `initech-4tw` + `initech-er3h`.**
- **DEC-07a (OEA-ADR-0003-A5)** -- the on-disk **partition contract** gating kzfs.
  Fork 1 (start-LBA authority on conflict) = **require-match-fail-loud**
  (`FAT12_ERR_PARTITION`), PROVISIONAL pending an 86Box round-trip (likely flips to
  partition-table-authoritative by erratum); Fork 2 (offset layer) = **mount-layer**
  (`base_lba` on the volume; fat12 biases reads; blockdev stays pure); Fork 3
  (detection) = **BIOS-drive-number first + sector-0 byte-sniff cross-validation**,
  fail-loud on irreconcilable. **Unblocks `initech-kzfs`** (the spec/code -- new
  `spec/mbr_partition_contract.json`, `FAT12_ERR_PARTITION`, `fat12_volume_t.base_lba`
  -- land WITH kzfs per Rule 8, not in the ADR).

The amendments' locked-spec edits (the AH=33h register row; the partition spec/code)
are AUTHORIZED but execute **atomically with their implementing beads** (er3h/4tw,
kzfs), not on ratification -- avoiding a listed-but-unimplemented register entry or a
half-state mid-stream.

## Acceptance

`make clean && make test` GREEN: **90 -> 93 host + 27 emu gates** (+test-fat-readfile-mutant,
+test-fat16, +test-fat16-mutant). Re-verified from a CLEAN tree on the main thread
after the lane's tree-race chaos (below). All FAT16 mutants M1-M5 + the dao readfile
mutants RED for the right reason; FAT12 path byte-identical; corrupt-fuzz green
serially. Non-ASCII clean; no stray mutations / backup files in the tree.

## Frictions / lessons (the big one)

- **The shared-tree workflow race got dangerous.** Coding agents + parallel
  verifiers sharing ONE uncommitted working tree produced transient inconsistency:
  the dao verifier reported a P0 "the streaming change is GONE / chain[2880] is back"
  and a P1 "make test-unit has no rule for test-fat-readfile-mutant" -- BOTH were
  artifacts of a concurrent agent's `git checkout`/revert-restore cycle observed
  mid-window. The orchestrator's clean re-run (Law 2) showed the final tree HAD both
  dao and z01, the Makefile rule present, and 93+27 green. Same class as WL-0026's
  8403 false-alarm P0 and the cnvp spec oscillation. **Standing rule for future
  lanes: give mutation-running verifiers an isolated git worktree, OR commit each
  landing before the next agent runs.** Deferring all commits to the orchestrator
  maximizes this hazard; the only reason it was recoverable is that the FINAL state
  was self-consistent and the orchestrator never trusts an agent self-report.
- **z01 honestly under-delivered its bead title and said so.** "FAT16 HDD read" is
  not complete -- only the decode + host differential is. The verifier surfaced the
  g_fat sizing gap; it was filed (`d27i`) and made a 40oq dependency rather than
  papered over. The decode is the load-bearing algorithmic increment; the windowed
  FAT read is mechanical-but-separate.
- **Parallel committee drafts collided on the Document ID** (both grabbed A4). The
  orchestrator assigned DEC-07a = A5 on finalization. Independent parallel agents
  need a coordinated ID space or a post-hoc reconciliation (this).

## Pointers / next

Forward lane continues (still SERIAL, shared fat12.c/int21.c): `80k` (wildcard) +
`x8fs` (cooked CON read; before 4tw) are the next NON-gated items. `4tw`/`er3h`
(break) are now UNBLOCKED (DEC-16 ratified) -- they execute the AH=33h register row
+ handler atomically. `kzfs` is now UNBLOCKED (DEC-07a ratified) -- it creates the
partition spec/code. `d27i` (in-kernel windowed FAT read) is the true completion of
FAT16 and a 40oq dependency. Follow-ups filed: `d27i` (P1), `qywt` (P2 initrd),
`7mjc` (P3 corrupt-fuzz flake). 86Box automation (initech-44m/x0i) is still the
external-evidence blocker for DEC-07a Fork 1's final lock + DEC-16's boot-default
confirmation -- both ship on documented defaults + erratum-on-evidence (DEC-15
precedent).
