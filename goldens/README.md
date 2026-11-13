# Pinned independent inputs

CORPORA.lock records the SHA-256 of each consumed file and the HEAD of each
external git corpus. ORIGIN records provenance for Wine, now vendored; it does
not require an external checkout. REPO paths are relative to this repository.
`make test` verifies the lock before starting any component. `make corpus`
performs just this preflight; missing files, changed bytes and wrong HEAD fail
and identify the input or root override. A native stat pass keys the
verification cache by expected digest, resolved
path, device/inode, size, nanosecond mtime and ctime. Restoring mtime cannot
hide changed bytes. Hashes are batched through sha256sum after a cache miss;
metadata is checked again before caching. HEAD and LIST are checked every run.
LIST records also pin directory-enumerated filenames (Lotus mint/*.steps).
`sh goldens/verify.sh REPO` checks just vendored inputs; region opts into this
preflight for standalone tests, without requiring unrelated external corpora.

Derivation: Initech 123 tests/diff's SHIPPED and MINTED tables, formula's typed
entry sources, copy_move's scan of mint/*.steps, and tests/run.sh's 123.RI and
wk1_ref.py reader; InitechWord tests/run.sh, tests/corpus.sh, diff/search.sh's
33 mint documents (using the existing fallback paths), wp5_ref.py and every
file read by the LISTF MD5 manifest. Screenshot names in C comments are
transcribed authorities, not files read at runtime. Wine's three source files
and COPYING.LIB are the unchanged four-file test dependency.

To add a corpus, add its root default/export in mk/config.mk and preflight.mk,
a HEAD record if it is a git checkout, and FILE records for every actual input:
`FILE VARIABLE SHA256 relative/path`. Compute each digest with sha256sum from
the independent capture, not product output. Inspect Makefiles, test sources
and reader imports (including fallback and directory enumeration inputs).
Adding inputs or changing a reader requires reviewing and updating this list;
verification never regenerates expected hashes. Overrides on make's command
line propagate to both the preflight and component tests.

No existing component Makefile is required to adopt new strict checks at merge.
Owners should add `corpus: ; @$(MAKE) -C $(TOP) corpus` (or a separate phony
preflight prerequisite) so standalone component tests also verify this lock.
Top-level test/status already do so. Vendored region needs no sibling corpus.

The lock now covers all successful external regular-file reads of the complete
tier-0 selection: 19 System 7/Mac OS 8 inputs, one Windows 3.1 input, 66 Lotus
inputs, 64 WordPerfect inputs and 769 dBASE inputs, plus four vendored Wine
files. The trace was cross-checked against FLAIR REFERENCE-INPUTS.sha256 and
the existing Lotus/WordPerfect lock derivation. dBASE includes its enumerated
sample files, captured transcripts and the inputs checked by retained manifests.
Optional live DOSBox inputs are outside tier 0. Corpus-side Python bytecode is
excluded: Initech 123 redirects its interpreter cache under its private OUT.
Cache/scratch live only under build/goldens. Concurrent scopes can replace the
cache atomically; a different scope merely causes another verification.
