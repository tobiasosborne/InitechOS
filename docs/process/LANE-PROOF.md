# Lane proof pack

Ref: PRD sections 8, 9 and 14; CLAUDE.md Rules 1, 6 and 11; Merge workflow
v2 F9 (initech-xses3). The cert and replay logs are build artifacts. Commit
the tests and implementation, never host metadata or generated proof files.

Run `tools/lane-cert.sh` in a clean, committed lane descended from the
current `origin/main`. Fetch before certification when the remote may have
advanced. The script checks the local tracking ref and does not fetch.
It selects gates with `make -s gates-for DIFF=origin/main..HEAD` when the
selector exists. A failed `make -n gates-for` probe means absent ONLY for
exit 2 with make's exact missing-target diagnostic for `gates-for`. Broken
prerequisites, makefile errors and failed selector recipes refuse and print
the selector's stderr. Only a genuinely absent selector permits the
space-separated `GATES` fallback. That fallback also requires nonempty
`REQUIRED_GATES` from the PM (or `--require '<gates>'`); `GATES` must cover
every required gate. Missing coverage refuses with `coverage unchecked`.
An empty selection always refuses.

`MAKE` may name the make executable. Error-ignoring, dry-run, question,
touch, old-file and what-if modes in `MAKEFLAGS` or `GNUMAKEFLAGS` refuse
before gate selection, including short-option clusters. Every selector
probe, selector run and gate invocation clears both flag variables. Gates
run serially with the remaining build environment. Each must exit zero and
its final `>>> <gate>: ...` line must say `green`. A nonzero exit refuses
even if the recipe printed a green summary. Missing summaries refuse.

The ASCII certificate is `build/cert/<branch-slug>.cert`; the slug replaces
each character outside letters, digits, dot, underscore and hyphen with a
hyphen. Failed attempts revoke an older cert at that path. It records
`base_sha`, `head_sha`, `branch`, `selector`, `gate_count`, `toolchain`, `host`, `utc`
as `key: value` lines, plus `gate <name> <final summary>` per unique gate.
Tool versions, machine architecture and UTC are diagnostic metadata only.
`selector` is exactly `absent` or `gates-for`. No environment dump, home
path or non-ASCII data is accepted in a cert.
Slug collisions overwrite the earlier cert, so retain the branch/head
fields when copying certificates into a review pack.

In the PM checkout, run `tools/cert-check.sh <cert> <lane-ref-or-sha>`.
The second argument is required: it resolves to a commit and must equal
`head_sha` exactly. `base_sha` must equal local `refs/heads/main` and be an
ancestor of that head. The checker then requires coverage of
`make -s gates-for DIFF=<base_sha>..<head_sha>` when the selector exists.
The same strict absent-versus-broken distinction applies here. With an
absent selector, the PM must supply nonempty `REQUIRED_GATES` or append
`--require '<gates>'`; otherwise the checker exits 2, `coverage unchecked`.
The certificate must contain every required gate. It reports each missing
gate, and records the coverage source and certificate selector before PASS.
For example, when neither checkout has a selector:

```sh
GATES='test-one test-two' REQUIRED_GATES='test-one test-two' tools/lane-cert.sh
tools/cert-check.sh build/cert/lane-proof.cert lane/proof --require 'test-one test-two'
```

This checks a lane's recorded evidence, not a signature or a substitute for
replay. A lane advancing after certification invalidates the certificate.

Create `build/proof/manifest` as ASCII with one record per line:

```text
KIND|SHA|COMMAND|EXPECT
```

There is no header, comment syntax, blank record, or field escaping.
`KIND` is `RED`, `GREEN`, or `MUTANT`. `SHA` is a full 40-character commit
SHA available in this repository. `COMMAND` is a nonempty POSIX shell
command (usually `make <gate>`); `EXPECT` is a nonempty literal substring
required in its combined stdout/stderr. Neither field can contain `|` or
a newline. Shell quoting, `&&`, and redirection work normally in COMMAND.
GREEN requires exit zero; RED and MUTANT require a nonzero exit as well as
the expected failure text. Name the specific regression in EXPECT so an
unrelated build failure does not count as evidence.

Run `tools/proof-replay.sh` from the lane or PM worktree. It creates a
detached checkout for each SHA at `build/proof/wt-<sha>`, reusing an existing
clean checkout at that exact commit. The command runs there with the
caller's environment (ASCII locale), not the caller's working directory.
Command stdin is empty so a command cannot consume later manifest records.
Ignored build outputs are retained on reuse; tracked or untracked input
changes refuse reuse. Commands should clean their own scratch and avoid
editing tracked files. Relative sibling-repository paths will differ in
these worktrees: export the usual absolute corpus environment variables.
Only run trusted manifests: COMMAND is intentionally executable shell.

It prints PASS/FAIL for each record and saves combined output in
`build/proof/record-<number>.log`. Exit zero means every record passed;
exit one means an output/status mismatch; exit two means invalid input or
setup. Worktrees remain for inspection. Remove them with
`git worktree remove --force build/proof/wt-<sha>` when no longer needed.

Every lane report must contain this brief template, filled with evidence:

```text
BRANCH/COMMITS: branch; base, test-first and implementation SHAs
RED FIRST: base/test-first SHA, command, nonzero exit, specific final line
GREEN: head SHA, each gate command and its final summary
MUTANTS: each actual mutation, command and specific RED observation
CERT: path and contents; cert-check command with lane ref/SHA and coverage source; output
NOT PROVEN: omissions, external dependencies and remaining limitations
```

The PM or verifier replays the manifest and checks the cert before a MERGE
verdict. A reviewer checks that these checks actually occurred and that
the certified head is the proposed lane head. Advancing main makes the
old cert stale: rebase, rerun gates, regenerate the pack.

## Explicit PM subset and batch execution

When the operator explicitly authorizes a smaller list, set
`CERT_SCOPE=required REQUIRED_GATES='<the authorized gates>'` for both
lane-cert.sh and cert-check.sh. Selection is still probed and executed;
broken selectors refuse, and every required gate must be in the selection.
The certificate records `scope: required` and `derived_gate_count`, so it
cannot be mistaken for coverage of the full selector. The checker refuses
this scope without the explicit environment and verifies the supplied list.
Default scope remains `derived`, including on older certificates.

`LANE_CERT_JOBS=1..8` runs one make for all selected gates. Every gate must
still have its own final green summary, and any nonzero batch exit refuses.
The combined log is retained at `build/cert/<branch-slug>.log`. One make
owns dependencies and gate-pair ordering, so shared prerequisites run once.
Unset LANE_CERT_JOBS retains serial gate execution.
