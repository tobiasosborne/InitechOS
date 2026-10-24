# One-pass pre-merge review of a fix lane (InitechOS)

You review ONE branch of InitechOS before the PM merges it to main. This
is the only review round; there is no second pass, so be concrete and
decisive. Read /home/tobias/Projects/initech-os/CLAUDE.md (Laws 1-4, the
Rules: oracle-first, Red->Green, mutation-proven goldens, no weakened
oracles, no silent locked-spec edits, ASCII-clean source, no 2026-isms in
the artifact, reproducible builds). Then in your working directory (the
lane's worktree, read-only) run `git log --oneline main..HEAD` and
`git diff main...HEAD` and read every changed file in full where the diff
is not self-explanatory. The bead id and the audit findings this lane
claims to fix are in ./LANE-CONTEXT.txt together with the worker's own
report; verify the report against the diff, not the other way round.

Judge only things that would make the PM regret merging:
1. Correctness: does the change fix the root cause of the cited findings,
   or a symptom? Any new bug, regression, undefined behaviour, buffer or
   bound error, lost error path, data-loss window?
2. Oracle honesty: is there a RED gate that reproduces the finding, does
   its mutant bite, was any existing gate weakened (tolerance raised,
   assertion removed, golden regenerated from the implementation, tag
   class dropped, SKIP introduced)? A golden "minted" from our own output
   is not a golden (Law 2).
3. Ground truth: are the cited references real and do they say what the
   code claims? Any locked spec/ change without a stated decision?
4. Hygiene: non-ASCII bytes in source, host paths, timestamps,
   nondeterminism, debug leftovers, kernel-size impact unreported.
5. Scope: anything changed outside the lane's job that could collide with
   a parallel lane (the list is in LANE-CONTEXT.txt).

Do not comment on style, naming, or things you would merely have done
differently. Do not ask for more tests than the ones that prove the fix
and bite. You may run read-only commands (grep, make -n, git); do not
build or run gates (the worker's gate log is in LANE-CONTEXT.txt and the
PM re-runs the family on main).

Output, ASCII, at most 60 lines, in this exact shape:
VERDICT: MERGE | FIX-THEN-MERGE | REJECT
BLOCKING: numbered list (empty for MERGE) -- each item: file:line, what
  is wrong, the evidence, the smallest fix. Only items that justify the
  verdict.
NOTES: at most five non-blocking observations the PM may file as beads.
