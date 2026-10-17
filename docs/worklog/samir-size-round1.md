# SAMIR size round 1: 8452 bytes recovered; acceptance blocked

This is a size-round result, not acceptance of the SET/functions/errors lane.
The tree was retained, work was performed alone, and no Git metadata, beads,
CLAUDE.md or HANDOFF.md was edited. No commit, push, full make test or Bochs.
Every QEMU gate exported INITECH_QMP_STDIO=1. RAM/disk were checked before the
heavy runs (17 GB available RAM / 113 GB free disk at the later check).

## Sources and arithmetic

Read CLAUDE.md top to bottom, PRD 5/6.6/8, ADR-0009, WL-0127, WL-0128,
WL-0152, WL-0148 and samirdml-acceptance.md. The live bd database does not
contain initech-ipu5.8.1; the supplied brief and checked-in JSONL were read.
No tracker records were changed.

The binding samir.ld assertion remains byte-identical to arrival:

    0x6f000 - 0x40100 - 0x10000 - 0x4f70 - 0x20 = 106352 loaded bytes

ADR-0009 DEC-03/04/05 and spec/memory_map.h reserve the BSS, owned PAL heap,
MCB/alignment and environment. No reserve, loader address, stack, heap or
memory-map constant was reduced or moved. BSS remains 48504 bytes.

Arrival was already compiled with -Oz, LTO, function/data sections and GC.
The initial diagnostic link used --noinhibit-exec ONLY to measure the failing
image, never for a shipped build. Its log still records the real guard failure.
Neither the production linker script nor any oracle was relaxed.

| Cumulative change | Loaded B | Recovery B |
| --- | ---: | ---: |
| Arrival diagnostic image | 106367 | 0 |
| Frame-pointer omission, crossjumping, no called-once inlining | 99819 | 6548 |
| Also disable partial inlining | 99755 | 6612 |
| Deterministic byte-pair catalogue storage | 98299 | 8068 |
| Also disable jump tables | 97883 | 8484 |
| Restore normal-path numeric memvar presentation | 97915 | 8452 |

SAMIR_SIZE_CFLAGS is the shared compile/link policy for production and mutants.
The core-emulator mutants now use SAMIR_LINK too, so the GCC driver loads the
LTO plugin. The other recipes already used it. No host-engine optimization
policy changed. Compiler: Ubuntu GCC 13.3.0-6ubuntu2~24.04.1.

Final text 93699 B, rodata 4187 B, loaded alignment 29 B, BSS 48504 B.
Hard-budget headroom is 8437 B; the required 4096 B floor is exceeded by
4341 B. The size gate additionally enforces the requested 8192 B recovery
against the fixed arrival measurement (98175 B maximum).

Two clean builds compare byte for byte, with zero x87 instructions:

    a26b69dc6c1e6ce803ad62feb7470fcf19b94e60fd23b2f7b7b745ff74fd68d4

25 production/behavior-mutant COM files built successfully. Their largest is
SAMIR_SCREEN_SAMIR_SIZE_MUTATE_MSG_PAIR.COM, 97947 B. The additional 4096-byte
file-pad mutant is 102011 B, also below the unchanged 106352 B linker budget.
The full size list is build/samir-size/mutant-sizes.txt.

## Implementation and independent evidence

tools/samir_msg_pack.c deterministically packs the locked DBASE.MSG extraction
using 128 ordered byte pairs. The committed numeric C initializer is ASCII.
messages.h expands into bounded local storage; no extra BSS/PAL allocation.
Every one of the 151 ordinals, including empty messages, is compared with
spec/samir/dbase_msg_codes.tsv, independently of the generated table. The
publication consumers now call the decoder. The inventory's 146 nonempty
catalogue rows also compared byte for byte with the TSV extraction.

Regression acceptance exposed a retained bug in flow.c: normalization to ten
integer columns was inside SAMIR_CORE_MUTATE_MEMVAR_RECLAIM only. It now runs
after both arena branches. The first host differential failed PROG-04/05;
the first QEMU DML-02 comparison differed at byte 14, got 0x31 / real 0x20.
Neither golden was changed. A new real III+ mint also proved RECNO() == 1 and
STORE 7 display with nine leading spaces. Two stale REPL unit assertions were
strengthened to include those independently measured spaces.

Law-1 manual scans inspected at 110 dpi:

- Ashton-Tate Using dBASE III PLUS: printed U5-15 / PDF 127 (?/?? output),
  U7-5 / PDF 493 (catalogue errors 76/77), U5-185 / PDF 297 (RUN really
  executes a DOS command/program and returns; its separate memory requirement).
- IBM DOS 3.30 Technical Reference: printed 6-200 / PDF 323 (AH=4Ch termination,
  return to the invoking process, close files). ADR-0003 Appendix A remains
  the unchanged EXIT/EXEC/memory API authority.

Manual roots are /home/tobias/Projects/initech-manuals-ref/dbase3plus/ and
/home/tobias/Projects/initech-manuals-ref/dos33/. The new packing is a storage
transformation, not a new period diagnostic or a claim about historical BPE.

Mint ledger:

| Row | Evidence | Capture SHA-256 |
| --- | --- | --- |
| SF-memwidth (new) | build/samirfns/golden/memwidth/GOLD.OUT; both markers, DOS EOF, no trapped error; RECNO() and stored 7 | 8f4079fb326ed8d1e559c96deed2994c514ba84eac1db4c2fc6e47534df9e171 |
| PROG-04/05 (reused) | Independent flow/proc outputs from samirdml-mint.md; pinned by prog-real.sha256 | Existing ledger unchanged |
| DML-02 (reused) | Independent macro output; pinned by dml-goldens.sha256 | Existing ledger unchanged |

The new PRG SHA-256 is
0bfe99819d9ecf45bb1933c6b017565610592251403c9d8f7681dbac15832eaa.
samir-size-real.sha256 pins it and GOLD.OUT. `make mint-samir-memwidth`
successfully ran the real III PLUS 1.1 binaries copied read-only from
/home/tobias/Projects/dbase3-decomp/mint/work. fns_mint.sh gained a single-case
filter and a required DOS EOF check. No reference repository was written.
Older DML/PROG captures were restored from the samirdml sibling and verified
against their existing manifests, not synthesized or edited.

## Exact new targets and GREEN tails

All fourteen gates below are in TEST_UNIT_GATES / TEST_EMU_GATES through
harness/diff/dbf_diff/samir_size.mk. The mint action is also new:
`mint-samir-memwidth` ended with `MINT SF-memwidth:` and the GOLD.OUT path above.

| Target | Exact tail (leading >>> omitted) |
| --- | --- |
| test-samir-size | test-samir-size: green (>=8192 B recovery; >=4096 B headroom) |
| test-samir-size-mutant | test-samir-size-mutant: green (SAMIR_SIZE_PAD_4096 RED; image remains below linker budget) |
| test-samir-size-repro | test-samir-size-repro: green (two clean builds byte-identical): a26b69dc6c1e6ce803ad62feb7470fcf19b94e60fd23b2f7b7b745ff74fd68d4  build/SAMIR.COM |
| test-samir-size-emu | test-samir-size-emu: green (boot, persisted writes, canon replay) |
| test-samir-size-emu-mutant | test-samir-size-emu-mutant: green (PAL short-read, dropped write, DO-file truncation RED) |
| test-samir-messages | test-samir-messages: green (deterministic packing; independent catalogue decode) |
| test-samir-messages-mutant | test-samir-messages-mutant: green (SAMIR_SIZE_MUTATE_MSG_PAIR RED) |
| test-samir-messages-emu | test-samir-messages-emu: green (exact catalogue messages 76/77 in QEMU) |
| test-samir-messages-emu-mutant | test-samir-messages-emu-mutant: green |
| test-samir-memwidth | test-samir-memwidth: green (PROG-04/05, DML-02 numeric memvar columns) |
| test-samir-memwidth-mutant | test-samir-memwidth-mutant: green |
| test-samir-memwidth-emu | test-samir-memwidth-emu: green (real III+ DML-02 in QEMU) |
| test-samir-memwidth-emu-mutant | test-samir-memwidth-emu-mutant: green |
| samir-size-goldens | samir-size-goldens: green (SF-memwidth real III+ capture pinned) |

Initial size RED:

    RED SAMIR_SIZE: 106367 B > 98175 B reclaim cap / 102256 B headroom cap

Named new mutant witnesses (the negative processes returned 1, not an
infrastructure error; the recipes returned success after verifying the RED):

    SAMIR_SIZE_PAD_4096:
    RED SAMIR_SIZE: 102011 B > 98175 B reclaim cap / 102256 B headroom cap

    SAMIR_SIZE_MUTATE_MSG_PAIR (host):
    RED SAMIR_MSG_PAIR: catalogue ordinal 2 differs from DBASE.MSG

    SAMIR_SIZE_MUTATE_MSG_PAIR (QEMU):
    FAIL harness/diff/dbf_diff/test_samir_core_emu.c:53: CONCAT_PLUS: guest exact manual error 77

    SAMIR_SIZE_MUTATE_MEMVAR_WIDTH (host):
    FAIL: flow: stdout matches real III+ golden

    SAMIR_SIZE_MUTATE_MEMVAR_WIDTH (QEMU):
    FAIL harness/diff/dbf_diff/test_samir_dml.c:172: DML transcript byte-identical to real III+

The size-emulator wrapper also ran the inherited PAL_MILTON_MUTATE_SHORT_READ,
PAL_MILTON_MUTATE_DROP_WRITE and REPL_MUTATE_DO_TRUNC images: LIST rows absent,
persisted DBF still 3 records/original balances, and canon report body absent,
respectively; all returned to the shell without a crash.

## Regression result and stop

GREEN: test-samir; test-dbase and all its roundtrip/differential mutants;
test-samir-core (286 checks), core-mutants (11), core-diff (104),
core-diff-mutant, core-emu (18), core-emu-mutants (3); the fifteen implemented
DML verbs and all fifteen host AND QEMU mutants; CLEAR/MACRO/CLOSE state gates
and mutants; test-samir-boot/write/canon-y2k and their mutants;
test-interp-flow/proc and mutants; test-samir-repl (47) and mutant;
test-samir-softfp (23858) and implementation mutant; test-pubs-samir-programs;
test-makefile-vars; make image; test-flair-app-launch (all 10 legs);
test-flair-finder-service (both volume legs). All counts have zero failures.

Three required gates remain RED. No exception or acceptance is claimed:

1. RUN in test-samir-dml / -mutants / -emu / -emu-mutants. Both actual
   transcripts differ at byte 11 (0x31 versus real 0x0d, lengths 48/47) and
   VERSION.TXT is absent. PAL has no EXEC slot; loader.c:1214/1215 explicitly
   refuses a nested load to avoid clobbering the loader context/program region.
   The prior samirdml acceptance already records this. A real RUN mint exists;
   this is not a mint-impossible exemption. Nested process execution is outside
   the supplied SET/function/error scope and cannot be replaced with VER text.
2. test-pubs-commands stops in its test-pubs-build prerequisite:
   `LaTeX Error: Command \MakeBackCover already defined.`
   initech-forms.sty:178 and initech-xbase.sty:77 both use newcommand.
   No shared style was edited. test-pubs-examples and test-pubs-mutant therefore
   did not execute their replay bodies. The standalone InitechBase program
   checks and the decoded publication catalogue comparison passed.
3. test-flair-locked-purge:
   `ppm_flair_trash_check: FAIL leg locked-full -- FULL Trash (TRASH_FULL): 617/1024 cells differ from the mint-008 capture`.
   The serial trace reaches FINDER-LOCKED-ALERT after purged=17/refused=2,
   and all 6496 alert-band pixels match; some button details and the Trash
   pixels differ. Thus this is not a file-slot re-key failure. No FLAIR source,
   injected trace, capture or pixel tolerance was changed to force GREEN.

The requested all-existing-gates-green prerequisite is unmet. The remaining
SET/functions/error items and the twelve InitechBase defects are not finished.
Retained earlier fns scaffolding/patches were preserved, not accepted anew.
No book DRAFT markers were removed and no printed examples were changed or
re-minted with pubs-examples. There is no full-lane completion claim.

Evidence: build/samir-size/{final-gates,memwidth-repl-final,new-emu-final,
size-acceptance,host-final,dml-host-mutants,emu-family,core-diff,
state-pubs-programs,desktop-acceptance,pubs,image}.log and memwidth-mint.log.
New/touched source passed ASCII checks; new sources passed whitespace checks.

## Files changed in this launch

- Makefile.
- os/samir/samir_main.c, cmd/flow.c, include/samir/messages.h,
  include/samir/messages_packed.h.
- tools/samir_msg_pack.c.
- harness/diff/dbf_diff/samir_size.mk, test_samir_size.c,
  test_samir_messages.c, test_samir_repl.c, fns_mint.sh.
- harness/diff/dbf_diff/xbase_prog_diff/corpus/fns/memwidth.prg,
  xbase_prog_diff/samir-size-real.sha256.
- harness/pubs/samir_inventory.c, samir_tables.c.
- docs/worklog/samir-size-round1.md.
