# InitechWord import inventory and verification

Baseline: legacy-final, run in the read-only source checkout at
/home/tobiasosborne/Projects/InitechOS-legacy. Paths below are relative to
apps/initechword unless prefixed attic. No word-processing behavior changed.

## Claimed with git mv

- core/iword.h -> include/iword.h.
- All six attic/spec/iword files -> include/iword/ (five headers + fkeys.json).
- All seven harness/proptest/test_iword*.c -> tests/.
- All three harness/diff/wp5_diff/test_iword*.c, iword_keys.h and
  listfiles_mint005.md5 -> tests/diff/.
- search.sh, replace.sh and settings.sh -> tests/diff/, adapted for the
  component boundary described below.
- tools/iword_fkeys_gen.c -> tests/tools/ (support for the fkeys gate).
- harness/diff/wp5_diff/wp51_headless.c -> tests/tools/ (optional capture support).
- fixtures/iword-goto and fixtures/iword-listfiles-ops -> tests/fixtures/,
  retaining all captures, step files, grids, hash manifests and provenance.

The face suite's recording fake was extracted into tests/fake_tbx/recording.h;
the driver and tests stay in test_iword_face.c. Included-suite architecture
is preserved for p3, p3_part2, listfiles and listfiles_ops. Shared assertions
and corpus policy use factory/testlib, not a copied seed header.

## Classification and every legacy host gate

Ten source suites: seven ORACLE-STRONG, three UNIT. There are thirteen default
runner entries plus the search-corpus helper (32 separate invocations).
ORACLE-STRONG sources: core, codes, wp5_roundtrip, reveal_codes, search_corpus,
listfiles and listfiles_ops. Models, real WP.EXE bytes or captured cell grids
supply the expectations independently of the implementation being graded.
UNIT sources: face, p3 and p3_part2. These mix authored face/state contracts
with literal manual/screen expectations; their independent saved-file and
corpus post-checks are retained as stronger checks, not discarded as units.
The replace model uses existing piece-table/reflow primitives; it is independent
of replace_match, but is not an independent codec or piece-table oracle.

Actual legacy execution (not a dry-run), with its unmodified recipes:

```
. tools/lane-env.sh
make test-iword-core test-iword-codes test-iword-wp5-roundtrip \
 test-iword-reveal-codes test-iword-face test-iword-fkeys test-iword-search \
 test-iword-listfiles test-iword-replace test-iword-undo test-iword-setup
```

The listfiles target also runs test-iword-listfiles-ops and
 test-iword-listfiles-ops-goldens. The face target runs normal and small builds.
The fkeys recipe also regenerates and compares the keyboard header. Thus every
positive legacy host gate is represented below; no slow sweep was reduced.

| Legacy gate / leg | New entry (test_iword_ prefix) | Class | Legacy checks | New checks |
|---|---|---|---:|---:|
| test-iword-core | core | ORACLE-STRONG | 27 | 27 |
| test-iword-codes | codes | ORACLE-STRONG | 6 | 6 |
| test-iword-wp5-roundtrip | wp5_roundtrip | ORACLE-STRONG | 584 | 584 |
| test-iword-reveal-codes | reveal_codes | ORACLE-STRONG | 69 | 69 |
| test-iword-face, normal | face | UNIT + saved-file oracle | 188 | 188 |
| test-iword-face, small | face_small | UNIT + independent reader | 4 | 4 |
| test-iword-fkeys | fkeys (p3.c) | UNIT + generator consistency | 154 | 154 |
| test-iword-search, face | search (p3.c) | UNIT | 17 | 17 |
| test-iword-search, corpus | inside search, search_corpus.c | ORACLE-STRONG | 13944 | 13944 |
| test-iword-listfiles | listfiles | ORACLE-STRONG | 1926 | 1926 |
| test-iword-listfiles-ops | listfiles_ops | ORACLE-STRONG | 138 | 138 |
| test-iword-replace, face | replace (p3_part2.c) | UNIT | 11 | 11 |
| test-iword-replace, corpus | inside replace | UNIT primitive model | 3534 | 3534 |
| test-iword-undo, face | undo (p3_part2.c) | UNIT | 11 | 11 |
| test-iword-undo, corpus | inside undo | UNIT restore properties | 1769 | 1769 |
| test-iword-setup | setup (p3_part2.c) | UNIT + captured settings | 12 | 12 |
| **Total counted checks** | | | **22394** | **22394** |

Direct suite summaries total 3147; corpus summaries add 19247. Independent
shell checks are additional, not included in those C assertion counts. Counts
were read from actual logs, including all 32 M*.log files in each corpus leg.
The 27/6 property checks aggregate large programs: 300 piece-table edit programs,
a 3000-key session, 2000 wrapper paragraphs, 400 typing sessions and 1300
code/geometry/block sessions, with the original seeded generators/shrinking.

Seven strong sources and three unit sources become six predominantly strong
runner entries and seven predominantly unit entries: search_corpus is an extra
helper within the unit search entry. No META or SIZE/LAYOUT suite was imported
as a behavioral test. The fkeys generator is support, not an extra oracle suite.

## Independent inputs and post-checks

- Roundtrip exercises 31 real WP goldens, real-key replay and the independent
  corpus tools/wp5_ref.py reader. Reveal Codes compares 69 captured panes.
- Search, replace and undo each exercise all 32 real documents: MINT01..24,
  MINT24A, MINT25..28, MINT30..32. Existing goldens/minted paths fall back to
  mint/work/WP51 as in the legacy suite. No synthetic input replaces a mint.
- Face MEMO equals real MINT02's 327-byte prefix plus the literal wrapped
  paragraph; MEMO, COPY, GUARD, BIG, CODES and TABS get independent reader checks.
  CODES checks the exact bold/underline/italic token stream. TABS body equals
  real MINT09. The small build's BIG parses to exactly 4096 bytes.
- List Files reads the real LISTF grids and eight files extracted from the
  pinned lf005.img; all legacy MD5 checks run every time. Ops compares the moved
  captured grids and verifies the original SHA256SUMS every time.
- fkeys compares the generator output to the committed header. It is a
  consistency check; the test's literal expected bindings come from the manual,
  not from that generated table. The generator never rewrites source on make.
- Setup checks captured settings through an independent mtools FAT roundtrip.

### InitechDOS differential separated at the boundary

Legacy search.sh compared the kernel's fat_dump output with mtype on identical
FAT images. replace/undo reused it; settings.sh did the same for settings.
Building fat_dump here would import kernel filesystem implementation into an
application test, or depend on an unbuilt legacy tree. It remains with its owner.
The moved scripts retain mformat/mcopy/mtype, original-input byte comparisons,
unchanged-image comparisons, fsck and all 96 document operations plus settings.
They compare the mtools-extracted input directly with the real WP file. The
application check counts are unchanged; **InitechDOS == mtools is not claimed
by this component**. Legacy ran that additional comparison on all 96 document
reads and the settings read; new component runs zero kernel FAT comparisons.
Reattach that cross-component differential in the kernel/factory import.

## Left in attic / excluded gates

- harness/diff/wp5_diff/{p3.mk,p3_part2.mk,p3_listfiles.mk} and
  spec/flair_iword_traces.mk: retired emulator/build wiring, not tier-0 suites.
  All emulator gates, mutant twins and Bochs wrappers are in SMOKES.md.
- tools/ppm_iword_codes_check.c: ORACLE-STRONG emulator pixel consumer; defer
  with the codes smoke. Host face assertions cannot substitute for raster pixels.
- harness/pubs/iword_probe.c, iword_check.sh, iword_features.sh, iword_mutants.sh,
  iword.allow and the manuals: META/publication ownership. test-pubs-iword,
  test-pubs-iword-mutant and pubs-iword-screens inventory/replay the implementation
  and check publication coverage/typeset screens, not independent WP parity.
  These remain for the publications import, including all publication mutants.
- harness/test_gate_honesty.sh, fixture_volumes.mk, sentinel.mk and
  emu/test_bochs_legs.c: META tests of retired skip/accounting/volume/recipe policy.
- Shared full-save, Toolbox STYLE/font/underline, finder launch and tenant gates
  mention Word but grade other owners; retained as cross-component smoke notes.
- The tenant-exe-rules 64 KiB cap, relocation/entry/BSS/headroom checks are
  SIZE/LAYOUT. No such constraint is reintroduced into the objects build.
- All twelve host -mutant families (core, codes, wp5-roundtrip, reveal-codes,
  face, fkeys, search, replace, undo, setup, listfiles, listfiles-ops) are deferred
  to patch conversion. MUTANTS-TODO.md names all 73 faults, sites and catchers.
- test-iword-listfiles-ops-goldens is a support hash gate, now corpus.sh's
  SHA256SUMS check, not an omitted source test. The LISTF md5 checks move too.

No unrelated host source was claimed just because it mentioned WordPerfect.
Optional fixture capture support moved with its script but is never built by
default; requires SDL2/DOSBox-X and the corpus's exact glyph decoder.

## Flags and boundaries

Word had no application-specific 64 KiB optimization block: the enormous
I123_SIZE_FLAGS list is conditional on the spreadsheet source in the legacy
recipe, so it never applied to Word and is not counted as dropped Word flags.
Word's extra common flags dropped: -fno-pie,
-fno-tree-loop-distribute-patterns, -fno-unwind-tables, -fno-common and the old
build-directory include path. -Os and -fno-asynchronous-unwind-tables are already
in shared TARGET_CFLAGS. No -Oz, LTO, section GC, register-argument or stack-size
flags are carried over. The old Word recipe already used default cdecl; it
filtered out the kernel's -mregparm=3/-mrtd/-freg-struct-return. Those were never
Word ABI requirements. There are no extra semantically required target flags.
IWORD_SMALL_BUFFERS is retained only for the host compaction test; _DEFAULT_SOURCE
exposes MAP_ANONYMOUS for the forked host driver.

Core/fs compile with only -Iinclude. The former core List Files include of the
entire shared Toolbox header was replaced by an owned, cited packed input
contract with six compile-time ABI comparisons in every face suite. The face
alone adds SDK and temporary shared spec headers. The SDK event problem and its
Initech 123 bridge, unchanged here, are documented in README. No shared file or
mk fragment changed; top-level Makefile changes only COMPONENTS.

Shared host-test.mk needs a wrapper hook for fresh scratch and independent
multi-command post-checks, and otherwise recompiles library sources per suite.
The local host_test_run override and host-core rules address these exactly as
the spreadsheet import does. Header/face/fake dependencies trigger rebuilds.

## Acceptance evidence

After restoring the red control, from clean component output:

```
make -C apps/initechword clean
/usr/bin/time -f 'wall %e seconds' make -C apps/initechword test
# apps/initechword: 13 tests, 0 failures
# wall 24.94 seconds
make -C apps/initechword
# exit 0; all eight objects: ELF 32-bit LSB relocatable, Intel 80386
make test
# flair/region: 2 tests, 0 failures
# apps/initech123: 18 tests, 0 failures
# apps/initechword: 13 tests, 0 failures
# ALL GREEN
```

Red once: changed the single prefix-copy assignment in fs/wp5.c from
`out[i] = prefix[i]` to `out[i] = prefix[i] ^ 1u`, then ran
`make -C apps/initechword test TESTS=test_iword_wp5_roundtrip`. Exit 2:

```
RED-REASON golden-rewrite: 31 goldens not byte-identical
RED-REASON wp5-ref-disagrees: 32 readings differ from wp5_ref.py
RED-REASON typed: 27 typed documents differ from what WP.EXE wrote
test-iword-wp5-roundtrip: 677 checks, 121 failures
apps/initechword: 1 tests, 1 failures
```

The source was restored byte for byte (cmp), and the clean green run above
followed restoration. The negative run has extra assertions because failed
rewrite comparisons emit byte-level diagnostics; it is not part of parity totals.

`make -C apps/initechword test WORDPERFECT51_DECOMP=/no/such/wordperfect51-corpus`
exited 2 with:

```
FAIL: WordPerfect corpus missing or incomplete at /no/such/wordperfect51-corpus;
set WORDPERFECT51_DECOMP to wordperfect51-decomp (tools/wp5_ref.py)
```

The final wrapper-only refactoring retained the legacy adapter structure for
rename history; focused replace/undo runs then passed (2 tests, 0 failures).
`git diff --check`, staged diff checks, shell syntax checks, ASCII/name scans
and all 75 catalogue locations passed. The event bridge is byte-identical to
Initech 123's. All claimed files are detected as renames; history is checked
with git log --follow after committing. Only the component, claimed attic files
and top-level COMPONENTS are touched. The operator requested a local branch
commit, no push, no tracker and no STATUS edit outside the allowed scope.
