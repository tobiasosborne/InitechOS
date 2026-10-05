# WL-0098 -- SAMIR data safety: audit L001, L003, L007

## Context

Lane lane-dbsafe, base 1b4a409. Beads: initech-r5ig (L001), initech-0uui
(L003), initech-zk6y (L007); the DBF write portion of B02/L079 also fits here.
The operator required oracle-first work, mutation proofs, the entire host
vector, relevant QEMU families, no bd or push, a writable metadata copy and a
verified bundle. These instructions override the ordinary session-close
workflow. No spec/ data or goldens were changed.

## What changed and why

L001: dbf_append_blank_commit in fs/dbf.c writes only the new record and EOF at
its original offset, then publishes the count. It preserves the original +1/+2
header geometry and never rewrites earlier records. Failed writes/seeks restore
the staged count; a short count write also restores the committed header bytes.
An unsuccessful restoration fails loud and inhibits further writes. cmd/mutate.c
uses this commit path for APPEND rather than the whole-image serializer. The
existing staged codec APIs still serve table construction; dbf_flush is not a
general transaction mechanism. dbf.h exposes the commit API and opened name.

B02: DBF writes preserve no-space, inaccessible and device-write failures.
The REPL renders real catalog #56 with the filename instead of unconditionally
claiming read-only. fileio_fat.c reports rolled-back no-space writes as DOS
AH=40h zero short writes and device failures distinctly. This covers the table
write stack; broader create/memo/index diagnostic work remains outside this
slice.

L003: PAL acquire/release storage is separate from alloc/reset scratch. A fixed
first-fit, coalescing heap lives in the existing disjoint DOS allocation; no
libc enters the OS. DBF, DBT, NDX and work-area caches release only their own
storage. Closing an older area, closing areas in either order, failed opens,
memo/index closes and repeated opens cannot rewind another owner. Duplicate
USE is rejected before closing/allocating the selected area, with catalog #3.
DOS filename comparison folds case, separators and dot components. The linker
now checks BSS against the loader reserve and leaves space for the PAL block.
Milton requests 36 KiB: 16 KiB scratch, 20 KiB owned storage.

L007: cmd/mutate.c consults SET SAFETY before ZAP. The cooked PAL reads one
answer line; only Y/y commits. N, Escape, blank input, EOF and input failure
leave DBF bytes/count unchanged. Default ON and explicit ON/OFF are covered.
The filename is the opened path; the next redirected command remains framed.
set.c/set.h document the implemented consumer. The pre-existing mutation oracle
selects SAFETY OFF explicitly before testing its destructive ZAP path; every
record/count/index assertion remains.

## Grounding

- PRD 6.6 and ../dbase3-decomp/specs/file-formats/dbf.md ss2/4/6/8:
  record count, offsets, both terminator geometries, minimum file length.
- Ashton-Tate Using dBase III Plus.pdf, U7-7: "File is already open", error 3.
  The corpus archive/golden-mined/DBASE_MSG_codes.tsv independently gives #3.
- Using dBase III Plus.pdf, U5-284: ZAP filename confirmation when SAFETY ON;
  U5-257: SAFETY defaults ON and OFF removes the warning.
- Real DBASE.MSG / spec/samir/dbase_msg_codes.tsv: #56 disk full, #29 inaccessible.
  The actual mined catalog is authoritative over inconsistent prose ordinals.
- IBM DOS 3.30 Technical Reference, 6-139/6-140: AH=40h short count on full disk;
  6-42: write-fault error code.
- spec/memory_map.h: PROGRAM_BSS_RESERVE, ENV_BLOCK, program/heap disjointness.

Failure restoration, device-failure wording, allocator/sizing policy, filename normalization/presentation,
line-framed Y/y acceptance and abort edge rules are authored, no local reference
for those implementation choices or exact edge transcripts. They are not claims
of modern transactions or a newly minted real-dBASE transcript.

## Frictions and corrections

The old COM failed all three new target drives: count/length corruption on a
zero-free-cluster FAT disk, no return to the desktop with two areas, and ZAP
before confirmation. Host append/header/record and strict reset oracles also
went RED before production fixes.

An initial static owned heap exceeded the loader's BSS reserve and the target
failed; moving it into the disjoint DOS block plus linker assertions fixed the
actual budget, without changing locked memory-map data.

Poisoned reclamation exposed two old fixture ownership bugs. test_dbt_roundtrip
used buffers after closing their source memo; it now keeps that source live
through all unchanged byte assertions. test_interp_replace closed an adopted
index again after interpreter teardown; it now honors the documented transfer
of ownership. The per-area filter oracle formerly reopened the same file twice;
it now uses a byte-identical distinct copy and retains all its assertions, in
accordance with the manual's duplicate-open refusal. Capture PALs forward both
new storage slots, including the program differential driver.

Parallel experiments exposed shared test-file races. New host and emulator
oracles now use private directories under build/dbsafe; deterministic DBF bytes
and shipped binaries do not contain temporary paths. Only final isolated runs
are acceptance evidence.

Corpus files were read-only. CNAMES.NDX was opened read/write only on a
byte-identical local copy. Existing corpus RW oracles likewise use copies.

## Acceptance actually run

The final complete vector reports:

```
test-unit: ALL GREEN (390 host gates)
```

It includes the fault-injecting FAT rollback/partial-write families, the SAMIR
codec/interpreter/view/USE/PAL families, their mutants, and test-makefile-vars.
The new host results are:

```
append: 502 checks, 0 failures
>>> test-samir-append-safe: green
>>> test-samir-append-safe-mutant: memory correctly RED (L001 in-memory committed count)
>>> test-samir-append-safe-mutant: count correctly RED (L001 original header geometry and committed count)
>>> test-samir-append-safe-mutant: rewrite correctly RED (L001 prior record bytes untouched)
>>> test-samir-append-safe-mutant: green
lifetime: 745 checks, 0 failures
>>> test-samir-lifetime: green
>>> test-samir-lifetime-mutant: reset correctly RED (L003 reset cannot cross a live owner)
>>> test-samir-lifetime-mutant: duplicate correctly RED (L003 exact duplicate refused)
>>> test-samir-lifetime-mutant: heap correctly RED (L003 reclaimed open)
>>> test-samir-lifetime-mutant: green
safety: 67 checks, 0 failures
>>> test-samir-safety: green
>>> test-samir-safety-mutant: green (missing confirmation correctly RED)
```

APPEND injects failures at every normal seek/write boundary and zero/one/half/
last-byte short writes at every write boundary, for +1/+2 headers and the
255-to-256 count carry. It checks all prior bytes, the in-memory count/cursor,
and dbf_ref.py schema AND every committed record after close. Access/device
controls distinguish errors. The heap/lifetime oracle includes two open orders,
repeated reclaim, failed opens, independent memos and an index corpus copy.

New target gates (INITECH_QMP_STDIO=1):

```
full: 2725 checks, 0 failures
>>> test-samir-full-emu: green
>>> test-samir-full-emu-mutant: green (header-first rewrite correctly RED)
lifetime: 38 checks, 0 failures
>>> test-samir-lifetime-emu: green
>>> test-samir-lifetime-emu-mutant: green (duplicate USE correctly RED)
safety: 41 checks, 0 failures
>>> test-samir-safety-emu: green
>>> test-samir-safety-emu-mutant: green (missing confirmation correctly RED)
lifetime-desktop: 241 checks, 0 failures
>>> test-samir-lifetime-desktop: green
>>> test-samir-lifetime-desktop-mutant: green (duplicate USE correctly RED)
```

mtools independently reported zero free bytes on the stopped full-disk image;
fsck.fat reported 6 files, 2847/2847 clusters, with no errors. The extracted DBF
has 12 records, header length 129, record length 31, and size 502; the original
three records and geometry are intact. The guest's committed and reopened
counts agree. The actual desktop gate returns twice to FLAIR and launches SAMIR
again via the system hotkey. These are observed QEMU results, not an inference
from the host PAL.

All 16 existing QEMU gates also passed: test-samir-audit-emu/input-emu and both
mutants; test-samir-boot/write and both mutants; test-samir-canon-y2k/salami and
both mutants; test-flair-samir-suspend and its mutant; test-fatwrite and
test-multiopen. The complete copied verdict lines are in the lane acceptance
report under build/dbsafe, with raw logs beside it.

SAMIR.COM measured 84928 bytes before and 87488 after (+2560), x87=0. Final BSS
is 48704 <= the locked 65536-byte reserve. The linker also reserves the entire
36 KiB PAL block plus overhead. A rebuild was byte-identical; final SHA-256 is
787071843de3ec87f5a7dea7f87d0081998c3c5a12b60a65b981c3f644664dd2.
Source additions are ASCII; git diff --check and test-makefile-vars are green.

## Limits and handoff

Bochs cannot run in this sandbox: test-flair-samir-suspend-bochs is left for the
project manager. No 86Box leg or new real-DBASE.EXE reopen/transcript mint was
run; the existing differential gate loudly skips its real-runtime Tier 2.
Acceptance is independent-reader/FAT/QEMU plus the cited manual/catalog.

Repeated media failure during header restoration cannot be made transactional
by ordinary DOS writes: the implementation fails loud and inhibits further
writes if restoration fails. The injected refusal/short-write and genuine
full-FAT cases are proven. ZAP file truncation/space reclamation and associated
memo/index rebuild parity remain the triage's broader follow-up, as do the
remaining B02 create/memo/index diagnostics. No bead status or remote was changed.

Commits are one per finding in build/lane-meta/git: L001 0a2d4ed, L003 eaf82ae,
and the L007 commit containing this worklog. build/lane.bundle advertises
lane-dbsafe with prerequisite base 1b4a409; verify/import that bundle in the
manager's writable repository. The original .git was not changed.

Pointers: harness/diff/dbf_diff/test_samir_dbsafe{,_emu}.c; new TEST_UNIT_GATES /
TEST_EMU_GATES registrations; build/dbsafe/test-unit.log, new-emu-final.log,
emu-families.log, ownership-fixtures-final.log, repro-final.log. The full-disk
acceptance image is build/dbsafe/emu-n4ZuLw/build/dbsafe/emu.img and its extracted
DBF/schema/records/fsck results are beside it. Source audit: docs/audits/
2026-10-05-astra-full-drive/REPORT.md and TRIAGE.md.
