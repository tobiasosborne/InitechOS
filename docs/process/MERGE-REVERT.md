# Merge revert policy (initech-ror2g; epic initech-0uphq, Merge workflow v2)

Purpose: when a merged lane turns main red, the PM restores a green main with
ONE command, without rewriting history, and without losing the lane's work.

## 1. Rules for main

1. One lane = one `--no-ff` merge commit on main. Nothing else lands between
   merges: no fast-forwarded lane commits, no squashes, no stacked lanes.
2. PM housekeeping commits (beads export, worklog shard, HANDOFF) are allowed.
   Each must stand alone: it must revert cleanly on its own and must never
   fold a code change in.
3. Oracle and code travel together. A merge that changes any golden, locked
   `spec/` row, fixture image or sha256 manifest carries that change IN THE
   SAME merge commit as the code that needs it, and the merge subject says so
   (for example `[golden]`, `[spec]`, `[fixture]`, `[manifest]`). A revert then
   restores code and oracle together; main never holds new code with an old
   golden, or the reverse.
4. Lane branches stay on origin as `lane/<name>` until the next green sharded
   certificate (`tools/cert-sharded.sh`) covers the merge. A wrong revert is
   then recoverable from the branch as well as from history.

## 2. The command

Run in the PM's main checkout, on main, clean tree, after `git fetch`:

    tools/revert-merge.sh <merge-sha> -n                       # plan only
    tools/revert-merge.sh <merge-sha> -y -r "<why>" -l <red-gate-log>

The tool refuses (exit 2, named reason, nothing changed) when the tree is
dirty, the branch is not main, the sha is not a first-parent merge of main,
the merge is currently undone, or origin/main is ahead of HEAD. Revert state
uses first-parent history only: active reverts toggle their targets, and an
undone revert has no effect. A revert of a revert restores the merge, so it
can be reverted again; side-branch revert messages do not change this state.

Already-undone merges are excluded from the plan. If active newer
first-parent merges sit above the target, they are listed; with `-y`
they are reverted FIRST, newest to oldest, each by `git revert -m 1 --no-edit`,
then the target. Main stays linear and every revert is one revertible commit.
Newer merges are collateral: re-land them after the fix.

Smoke set (first that applies): `$SMOKE`; `make test-for DIFF=<target>^1..<newest>`
when the `gates-for` selector exists. `test-for` executes the selected gates
with `-k` and fails if any gate is red; printing the names is not a smoke run.
Otherwise, `make test-merge-samir` when the diff is only
`os/samir`, `harness/diff/dbf_diff` or `pubs/manuals/initechbase-*`; otherwise
`make test-kernel-repro test-kernel-flair-repro test-ovl-repro`.
The fallback disables rename detection and reads pathnames without shell
word splitting: a move out of another subsystem into SAMIR selects the trio.
Git-quoted unusual pathnames also conservatively select the trio.

Green: the tool pushes origin main (never forced) and prints a bead note with
the reverted subject, the reason and the red-gate log path. Red: it stops
before pushing, leaves the reverts local, and prints how to continue
(fix, re-smoke, `git push origin main`) or abandon (`git reset --hard <start>`,
which drops only unpushed local reverts). Exit codes: 0 ok, 2 refused, 3 smoke
red, 4 revert conflict or abort failure, 5 push failed. On conflict the tool
attempts `git revert --abort`, preserves abort errors, and reports a clean
tree only after a successful abort and a clean tracked-file status check.
If abort fails, inspect `git status` before continuing recovery.

## 3. Recovering a wrong revert

A revert is an ordinary commit, so undo it the same way: `git revert <revert
sha>` (the "revert of the revert") re-applies the merge's changes in one
commit. Do this, not a re-merge of `lane/<name>`, while the original merge is
still in history (git would otherwise skip the already-merged commits). After a
fix lands on the lane branch, re-merge as a new `--no-ff` merge. Run the
smoke set again before pushing.

## 4. What the sentinel hands the PM

`tools/sentinel.sh` (F11) watches main between certificates. On a red gate it
hands the PM: the culprit merge sha (from `tools/culprit.sh`), the red gate
name, the path of its log, and the ready-to-run line above with `-l` filled
in. The PM checks the culprit claim against the log (Rule 4), runs `-n`, then
`-y`. The sentinel never reverts by itself.

## 5. Not covered

Reverting a non-merge commit, or a merge not on the first-parent history, is a
manual decision and the tool refuses it. Untracked files do not count as a
dirty tree.
