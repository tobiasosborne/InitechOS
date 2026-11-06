#!/bin/sh
# revert-merge.sh -- one-command recovery from a bad lane merge. Reverts a
# first-parent merge commit on main (and every newer first-parent merge above
# it, newest first), runs a smoke set, and pushes only if the smoke is green.
# (initech-ror2g, F10 of epic initech-0uphq; policy docs/process/MERGE-REVERT.md.)
#
# USAGE (run in the PM's main checkout)
#   tools/revert-merge.sh [-n] [-y] [-r REASON] [-l GATE_LOG] MERGE_SHA
#
#   -n         dry run: run every refusal check, print the plan, change nothing
#              (a `git fetch` of origin is the only side effect).
#   -y         allow reverting the newer merges above MERGE_SHA first. Without
#              -y, if newer merges exist the plan is printed and nothing is done.
#   -r REASON  why the merge is being reverted (goes into the bead note).
#   -l LOG     path of the red gate log (goes into the bead note).
#
# REFUSES (exit 2, one named reason, nothing changed) when:
#   * the working tree has uncommitted changes to tracked files (dirty),
#   * the current branch is not main,
#   * MERGE_SHA is not a commit / not a merge commit / not on the first-parent
#     history of HEAD / already reverted on main,
#   * origin/main cannot be fetched or is ahead of HEAD (fetch, then rebase or
#     re-merge, then retry).
#
# BEHAVIOUR
#   * Each merge is undone with `git revert -m 1 --no-edit`: main stays linear
#     and every revert is itself one revertible commit. Newer merges go first,
#     newest to oldest, then MERGE_SHA, so each revert applies to the tree it
#     was merged onto.
#   * Smoke set: $SMOKE if set; else `make -s gates-for DIFF=<merge>^1..<newest>`
#     if the Makefile has a gates-for target; else `make test-merge-samir` when
#     the reverted diff touches only os/samir, harness/diff/dbf_diff or
#     pubs/manuals/initechbase-*; else the trio
#     `make test-kernel-repro test-kernel-flair-repro test-ovl-repro`.
#   * Smoke green: `git push origin main` (never forced) and a bead note.
#     Smoke red: stops BEFORE pushing and says how to continue or abandon.
#   * Never force-pushes, never rewrites history. A merge reverted in error is
#     recovered by reverting the revert commit (see the policy).
#
# EXIT: 0 reverted+smoke green+pushed (or -n plan); 1 usage; 2 refused;
#       3 smoke red (not pushed); 4 a revert conflicted (aborted, tree clean);
#       5 push failed.
#
# POSIX sh; needs git (and make for the smoke set).

set -u

dry=0
yes=0
reason=
logpath=
usage() { sed -n '2,/^# POSIX sh/p' "$0" | sed 's/^# \{0,1\}//' >&2; exit 1; }
# Options may come before or after MERGE_SHA, so no getopts (it stops at the
# first operand).
arg=
while [ $# -gt 0 ]; do
  case $1 in
    -n) dry=1 ;;
    -y) yes=1 ;;
    -r) [ $# -ge 2 ] || usage; reason=$2; shift ;;
    -l) [ $# -ge 2 ] || usage; logpath=$2; shift ;;
    -h|--help|-*) usage ;;
    *) [ -z "$arg" ] || usage; arg=$1 ;;
  esac
  shift
done
[ -n "$arg" ] || usage

refuse() { printf 'revert-merge: REFUSED: %s\n' "$*" >&2; exit 2; }
say() { printf 'revert-merge: %s\n' "$*"; }
short() { git rev-parse --short=10 "$1"; }
subject() { git log -1 --format=%s "$1"; }
clip() { # first 100 chars of the subject, for plan lines
  s=$(subject "$1")
  if [ "${#s}" -gt 100 ]; then printf '%.100s...' "$s"; else printf '%s' "$s"; fi
}

top=$(git rev-parse --show-toplevel 2>/dev/null) || refuse 'not inside a git work tree'
cd "$top" || exit 1

# --- refusals ---------------------------------------------------------------
[ -z "$(git status --porcelain --untracked-files=no)" ] \
  || refuse 'working tree is dirty (commit, stash or discard tracked changes first)'
branch=$(git symbolic-ref --short -q HEAD || true)
[ "$branch" = main ] || refuse "current branch is '${branch:-detached HEAD}', not main"
target=$(git rev-parse -q --verify "$arg^{commit}") || refuse "'$arg' is not a commit"
nparents=$(git rev-list --parents -n 1 "$target" | wc -w)
[ "$nparents" -ge 3 ] || refuse "$(short "$target") is not a merge commit"
git rev-list --first-parent HEAD | grep -qx "$target" \
  || refuse "$(short "$target") is a merge but not on the first-parent history of main (reachable only through a side branch)"
if git log --format=%B "$target..HEAD" | grep -qF "This reverts commit $target"; then
  refuse "$(short "$target") is already reverted on main (to undo that revert, revert the revert commit)"
fi
git remote get-url origin >/dev/null 2>&1 || refuse 'no remote named origin'
git fetch -q origin main || refuse 'cannot fetch origin main (fix the network/remote first)'
ahead=$(git rev-list --count HEAD..origin/main)
[ "$ahead" -eq 0 ] || refuse "origin/main is ahead of HEAD by $ahead commit(s): git pull --ff-only (or rebase) first, then retry"  # CHECK:origin-ahead

# --- plan -------------------------------------------------------------------
newer=$(git rev-list --first-parent --merges "$target..HEAD")   # newest first
plan=$newer
plan="${plan:+$plan
}$target"
start=$(git rev-parse HEAD)
newest=$(printf '%s\n' "$plan" | head -n 1)
base=$(git rev-parse "$target^1")
unpushed=$(git rev-list --count origin/main..HEAD)

say "main is at $(short "$start"); plan, newest first:"
n=0
for m in $plan; do
  n=$((n + 1))
  if [ "$m" = "$target" ]; then role='target'; else role='newer merge, reverted first so main stays linear'; fi
  say "  $n. $(short "$m")  $(clip "$m")  [$role]"
done
[ "$unpushed" -eq 0 ] || say "note: $unpushed local commit(s) on main are not on origin yet; the push will publish them too"
if [ "$dry" -eq 1 ]; then say 'dry run (-n): nothing changed'; exit 0; fi
if [ -n "$newer" ] && [ "$yes" -ne 1 ]; then
  refuse "newer merges sit above $(short "$target"); they must be reverted first. Re-run with -y (or -n to only see the plan)"
fi

# --- revert, newest to oldest -------------------------------------------------
reverts=
for m in $plan; do
  if ! git revert -m 1 --no-edit "$m" >/dev/null 2>&1; then
    git revert --abort >/dev/null 2>&1 || true
    say "revert of $(short "$m") FAILED (conflict); aborted, tree clean."
    say "reverts already made stay LOCAL and unpushed; to drop them: git reset --hard $start"
    exit 4
  fi
  r=$(git rev-parse HEAD)
  reverts="${reverts:+$reverts }$r"
  say "reverted $(short "$m") -> $(short "$r")"
done

# --- smoke -------------------------------------------------------------------
if [ -n "${SMOKE:-}" ]; then
  smoke_cmd=$SMOKE
elif make -n gates-for >/dev/null 2>&1; then
  smoke_cmd="make -s gates-for DIFF=$target^1..$newest"
else
  files=$(git diff --name-only "$base" "$newest")
  only_samir=1
  [ -n "$files" ] || only_samir=0
  for f in $files; do
    case $f in
      os/samir/*|harness/diff/dbf_diff/*|pubs/manuals/initechbase-*) ;;
      *) only_samir=0 ;;
    esac
  done
  if [ "$only_samir" -eq 1 ]; then
    smoke_cmd='make test-merge-samir'
  else
    smoke_cmd='make test-kernel-repro test-kernel-flair-repro test-ovl-repro'
  fi
fi
say "smoke: $smoke_cmd"
smoke_rc=0
sh -c "$smoke_cmd" || smoke_rc=$?
smoke_red() {
  say "smoke RED (exit $smoke_rc): the reverts are NOT pushed; origin/main is unchanged."
  say "  keep going: fix the cause, then run the smoke again by hand, then 'git push origin main'"
  say "  abandon:    git reset --hard $start   (drops the local reverts only)"
  say "  attach the red output to the bead; do not push a red main."
  exit 3
}
[ "$smoke_rc" -eq 0 ] || smoke_red  # CHECK:smoke-red
say 'smoke green'

# --- push --------------------------------------------------------------------
if ! git push origin main; then
  say "push FAILED (origin moved or refused). Nothing was forced. Local reverts are in place:"
  say "  git fetch origin && git status   then re-run, or: git reset --hard $start"
  exit 5
fi
say "pushed origin main at $(short HEAD)"

# --- bead note ---------------------------------------------------------------
echo
echo '--- bead note (paste) ---'
echo "REVERTED merge $(short "$target"): $(subject "$target")"
echo "reason: ${reason:-(not given; pass -r)}"
echo "red gate log: ${logpath:-(not given; pass -l)}"
if [ -n "$newer" ]; then
  for m in $newer; do echo "collateral (re-land after the fix): $(short "$m") $(subject "$m")"; done
fi
echo "revert commits on main (newest last): $(for r in $reverts; do printf '%s ' "$(short "$r")"; done)"
echo "smoke: $smoke_cmd -> green. Recover a wrong revert: git revert <revert commit>."
echo '--- end ---'
