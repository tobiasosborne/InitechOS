# WL-0148 -- SAMIR core hardening

## Context

Lane samir-core-hardening; read all 13 assigned beads from .beads/issues.jsonl
because the live bd database has no matching IDs. Beads, CLAUDE.md, HANDOFF.md,
and Git metadata were not edited. No commit or push was performed.

## Changes and ground truth

- DBF record-span arithmetic is checked before multiplication and RW allocation.
  Short external date descriptors cannot overwrite the following field; long
  slots are padded. Using dBASE III PLUS U1-1; corpus dbf.md sections 5/8.
- Memo replacement opens the existing companion without truncation, calls
  dbt_append, then publishes the right-justified decimal block pointer. A failed
  companion open/append leaves the slot intact. Existing memo readers refresh
  their high-water mark, invalidating a cached header first. Corpus dbt.md
  sections 2-5/8; Programming with dBASE III PLUS P8-17.
- DBT scans stop at the live block boundary. Capacity and signed seek arithmetic
  are checked before using corrupt block numbers on reads and padded appends.
  These rejection properties
  are authored safety policy (Rules 2/3), not historical corruption recovery.
- NDX distinguishes empty-file structure from I/O errors, rejects unrepresentable
  key/recno/leaf-count workspaces before allocation, and uses a candidate leaf's
  upper (key,recno) pair to choose among equal-key separators, including stale
  high separators after deletion. Corpus ndx.md sections
  1/3.1/5/6; real III+ 1.1 same-key append/split mint.
- Memvar rebinding compacts retained values, including hidden PRIVATE ancestors,
  and preserves aliased inputs. Released/type-changed values stop consuming
  arena bytes. Compaction uses bounded stack scratch rather than a second BSS
  arena; the latter failed the existing linker reserve assertions. Using III+
  U1-2; corpus memory-variables.md section 3.3; memory_map.h stack contract.
- Concatenation enforces 254 characters with error 77 for plus and 76 for minus.
  The beads' suggested SPACE error 59 is incorrect. Using III+ U7-5 (PDF 493),
  visually verified and independently confirmed by real III+ 1.1.
- FILE accepts a PAL binding without a cursor. Removed the oracle assertion
  requiring error 52 for FILE. Using III+ U6-33 (PDF 429).

Manual scans inspected: Using III+ PDF pages 19/20 (printed U1-1/U1-2),
429 (U6-33), 493 (U7-5); Programming PDF page 161 (P8-17).
Manual root: /home/tobias/Projects/initech-manuals-ref/dbase3plus/.
Byte-reference root: /home/tobias/Projects/dbase3-decomp/.

## Acceptance

The first host run was RED: 257 checks, 86 failures. The first QEMU gate was
RED: 18 checks, 3 failures. Finished gate tails:

| New target | GREEN tail |
| --- | --- |
| test-samir-core | samir-core: 286 checks, 0 failures |
| test-samir-core-mutants | GREEN (all named regressions caught) |
| test-samir-core-diff | samir-core-real-diff: 104 checks, 0 failures |
| test-samir-core-diff-mutant | GREEN (NDX_DUP_DESCENT correctly RED: same-key append/split differential vs real III+) |
| test-samir-core-emu | samir-core-emu: 18 checks, 0 failures |
| test-samir-core-emu-mutants | GREEN (all guest regressions caught) |

All six targets are registered in TEST_UNIT_GATES or TEST_EMU_GATES. The mint
stages real III+ binaries into build/samir-core-mint, generates an independent
31-record DBF, appends to 95 same-key records in real dBASE, and compares the
resulting (key,recno) stream with SAMIR using a separately implemented reader.
Real III+ also reads back SAMIR's emitted DBF/DBT memo and confirms FILE and
concatenation behavior. Copyrighted goldens remain under build only.

Named host mutants (SAMIR_CORE_MUTATE_ prefix), each exits 1 with its own FAIL:
DBF_SIZE, DATE_WIDTH, MEMO_POINTER, DBT_SPAN, NDX_SIZE, NDX_BUILD_SIZE,
NDX_DUP_DESCENT, MEMVAR_RECLAIM, CONCAT_PLUS, CONCAT_MINUS, FILE_NEEDS_USE.
CONCAT_PLUS, CONCAT_MINUS and MEMVAR_RECLAIM also fail in real QEMU EXEC;
NDX_DUP_DESCENT additionally fails the independent real-dBASE differential.

Existing test-samir, test-dbase (including all differential mutants and
round-trip), every test-dbf-*, test-dbt-*, test-ndx-* and test-xbase-* target,
test-interp-flow/proc and their mutants passed. The coercion fuzzer reported
34 directed cells + 2000 seeds, 0 divergences. Existing corpus-open SKIPs
(numeric edge cases, procedure by-reference semantics, etc.) remain explicit.

QEMU test-samir-boot, test-samir-write, test-samir-canon-y2k,
test-flair-locked-purge and test-flair-finder-service passed; make image passed.
Every QEMU invocation used INITECH_QMP_STDIO=1. No Bochs or full make test.
Source ASCII scan: zero matches. ASan/UBSan: 286 checks, zero failures with
ASAN_OPTIONS=detect_leaks=0; LeakSanitizer cannot run under sandbox ptrace.

Logs: build/samir-core-initial-red.log, samir-core-emu-initial-red.log,
samir-core-final.log, samir-core-regression-final.log,
samir-core-emu-final.log, samir-core-sanitize.log. Additional focused RED captures:
samir-core-append-red.log (corrupt writable DBT header), samir-core-stale-red.log
(mixed-key leaf with a stale separator). Individual mutant logs retain
the exact RED assertions; the real mint inputs and outputs remain in build.

## Limits

Memo replacement requires an existing DBT companion; it fails without changing
the DBF pointer when one cannot be opened. This preserves dbf_create's existing
separate DBF/DBT creation API. SAMIR remains packaged on its existing test volumes;
the baseline flair_data.img APPS directory does not include SAMIR.COM. No requested
lane item is deferred.
