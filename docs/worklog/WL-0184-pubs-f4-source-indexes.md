# WL-0184: Source lockstep follows F4's per-entry files

## Context

Main 9baae0e1 contains F4's conflict-free manual registries. Merging the
fail-closed source gate from 28060d68 exposed the new generated entry index
as an unsupported input. Bead: initech-nd3. Ref: PUB-REF-0001 GC-01/03;
pubs/entry-index.sh; docs/plans/INITECHBASE-lane-howto.md; PRD Sec 6.6/8;
CLAUDE.md Laws 1/2, Rules 2/3/6/11/12.

## What changed and why

Merged main into this branch in 9a41b02e. Makefile and pubs/pubs.mk merged
without conflicts, retaining both F4's registries and the full session
vector. No changes were made to main or pushed.

The source gate derives each book's entry list by executing the audited
entry-index.sh with separate process arguments, without shell interpolation.
Its safe-key validation and byte order are reused; there is no second sort
implementation. Generator failures and invalid output propagate as RED.
The generated index must match the directory-derived bytes exactly. The
walker then follows the derived list, accepting entries/<group>/<key>
only from that verified index. Missing, repeated, unused or shadowed indexes,
unknown groups, symlinks, nested entry inputs and hidden entry files fail
loudly. Each file must contain one primary entry in its appropriate group;
the historical ampersand compound remains in fn and Volume One's SET entries
remain in cmd, as F4 specifies.

The source target now depends on its five generated entry indexes. None of
these prerequisites invokes LaTeX. Existing mutants target the new per-entry
paths; missing-entry mutants refresh the index to preserve the original
coverage-failure diagnostic instead of failing merely for a stale index.

## Acceptance

The merged old gate reproduced SRC-INPUT on entries-initechbase-volume-one-cmd
(make exit 2). The fixed source baseline passes with 142 entries in each
reference, 286 DotExamples and one Transcript. All original 43 named faults
and their expected reasons remain; five additional faults reject an omitted
file, a phantom index line, a wrong-group move with both indexes refreshed,
reordered index lines and a hidden entry file. All 48 reject as expected;
comment/restored controls pass, including with nonexistent LaTeX commands.

test-pubs-entry-index and test-samir-registries pass. A real
make test-pubs-commands typesets all 38 documents and passes. The source
entry order equals the real LaTeX main.cmds order in both InitechBase
reference books, and each real index passes commands against samir.shipped.
ASan/UBSan exercises the complete mutant suite (LeakSanitizer disabled only
because the environment uses ptrace). ASCII, shell syntax and diff checks
pass. Validation logs are under build/pubs/f4-fix/ (untracked).
