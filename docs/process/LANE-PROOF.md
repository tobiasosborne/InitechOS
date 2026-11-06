# Lane proof pack

Ref: PRD sections 8, 9 and 14; CLAUDE.md Rules 1, 6 and 11; Merge workflow
v2 F9 (initech-xses3). The cert and replay logs are build artifacts. Commit
the tests and implementation, never host metadata or generated proof files.

Run `tools/lane-cert.sh` in a clean, committed lane descended from the
current `origin/main`. Fetch before certification when the remote may have
advanced. The script checks the local tracking ref and does not fetch.
It selects gates with `make -s gates-for DIFF=origin/main..HEAD` when
`make -n gates-for` succeeds, otherwise with space-separated `GATES`.
`MAKE` may name the make executable. An empty selection refuses. Gates run
serially with the inherited build environment. Each must exit zero and its
final `>>> <gate>: ...` line must say `green`. Missing summaries refuse.

The ASCII certificate is `build/cert/<branch-slug>.cert`; the slug replaces
each character outside letters, digits, dot, underscore and hyphen with a
hyphen. Failed attempts revoke an older cert at that path. It records
`base_sha`, `head_sha`, `branch`, `gate_count`, `toolchain`, `host`, `utc`
as `key: value` lines, plus `gate <name> <final summary>` per unique gate.
Tool versions, machine architecture and UTC are diagnostic metadata only.
No environment dump, home path or non-ASCII data is accepted in a cert.
Slug collisions overwrite the earlier cert, so retain the branch/head
fields when copying certificates into a review pack.

In the PM checkout, run `tools/cert-check.sh <cert>`. It compares the base
with local `refs/heads/main`, then requires coverage of
`make -s gates-for DIFF=<base_sha>..<head_sha>` when the selector exists.
A missing selector is reported explicitly; only base and internal gate
records can then be checked. It reports each missing gate. This checks a
lane's recorded evidence, not a signature or a substitute for replay.

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
CERT: path and contents; base == main; gate coverage result
NOT PROVEN: omissions, external dependencies and remaining limitations
```

The PM or verifier replays the manifest and checks the cert before a MERGE
verdict. A reviewer checks that these checks actually occurred and that
the certified head is the proposed lane head. Advancing main makes the
old cert stale: rebase, rerun gates, regenerate the pack.
