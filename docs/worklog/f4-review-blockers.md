# F4 review blockers (initech-q8j)

## Context

Review of 556e9d05 found stale SET oracles after table-only changes, silent
duplicate rows after union merges, and stale indexes after the final entry
was removed. Ref: PRD 6.6; CLAUDE.md Rules 2, 6 and 11.

## Change and rationale

Discovered source-tree .def files invalidate the existing shared Makefile
stamp, covering all present and future consumers without passing tables to
the linker. Preserve that stamp in the kernel VESA pattern override. This
conservatively rebuilds other factory outputs when a definition changes.

A C validator, mandatory in test-samir, checks SET row grammar and unique
keys, marker file types/contents and mutual exclusion, and rejects union
paths without a known schema. Its mutation harness reproduces a real union
merge of competing DEMO rows. Symlink case directories are rejected because
Make's wildcard would otherwise follow cases the validator did not inspect.

Publication groups come from directories, with tracked .gitkeep files to
preserve empty groups in clean checkouts. Entry keys are validated before
any TeX is emitted. The lane howto now quotes keywords and explains what
union merging does and what the validator still has to prove.

## Frictions

Bare -MMD on a multi-source cc/link command records only its last source;
the shared stamp avoids that incomplete dependency scheme. The worktree's
local beads database lacked issue_prefix; configured it before tracking
initech-q8j. Git index/commit writes require access to the worktree metadata
outside the filesystem sandbox. User explicitly requested no push or main
changes, so this session commits only on its worktree branch.

## Acceptance

- test-samir, test-interp-set and its mutant, test-samir-set-color and its
  mutant, test-samir-abbrev, and all 24 accepted fns host gates: exit 0.
- test-samir-def-deps: exit 0. Touching the table rebuilds arbitrary future
  and kernel-pattern consumers. COLOR->COLOUR forces an ordinary rebuild;
  the COLOR gate exits 2 with 59 failures, with no forced-build flag.
- test-samir-registries-mutant: exit 0. Union DEMO, malformed rows, embedded
  NUL, invalid markers, symlink cases and unknown union paths all exit 1.
  Disabling the duplicate detector makes this harness fail (exit 1).
- test-pubs-entry-index: exit 0. C order, creation order, add/remove, empty
  groups and bad%name.tex rejection are checked against explicit fixtures.
- test-pubs-examples (including test-pubs-build): exit 0; 287 replays pass.
- Clean publication build: all 38 PDFs compare byte-identical to 556e9d05.
  SAMIR.COM also compares byte-identical. All cmp commands exit 0.
- Shell syntax, ASCII scan and git diff --check: exit 0.

## Pointers

Factory gates: harness/registries.mk, harness/check_registries.c,
harness/registry_mutants.sh, harness/def_dependency_check.sh and
harness/pubs/entry_index_check.sh. Session logs: build/f4-review/.
