#!/bin/sh
# merge-train.sh -- apply lane diffs onto main, run the per-merge SAMIR gate
# set ONCE for the whole train, print a summary. (initech-2kkqr, F5;
# docs/plans/INITECHBASE-ship-plan.md sec 1 and 5.)
#
# USAGE
#   tools/merge-train.sh [-n] [-j N] [-T] [-B] [-D] LANE...
#
#   LANE   a lane working directory (a git worktree/clone of this repo): its
#          whole change since the merge-base with this repo's HEAD is taken,
#          committed AND uncommitted AND untracked (read through a temporary
#          index; the lane is not touched) -- or a branch/ref name that this
#          repo can resolve: the diff merge-base(HEAD, ref)..ref.
#   -n     dry run: replay the whole train in a throwaway worktree, report
#          conflicts and the files each lane touches, run no gates, leave this
#          checkout untouched.
#   -j N   parallelism for the gate run (default 32; make -jN test-merge-samir).
#   -T     apply only; do not run the gates (plumbing check).
#   -B     allow running on a branch other than main (default: refuse).
#   -D     allow a dirty working tree to start from (default: refuse; the
#          train must be reviewable as exactly the lanes' diffs).
#
# BEHAVIOUR
#   * Each lane is applied with `git apply --3way` in the order given. Any
#     conflict (or a patch that does not apply) STOPS the train at once with a
#     clear message naming the lane and the files; there is NO auto-resolution
#     and no retry. Lanes applied before the failure stay applied (staged,
#     uncommitted); undo with `git reset --hard HEAD` after checking, or fix
#     the lane and re-run. A conflicting lane is checked BEFORE it is applied
#     in a disposable index. Unmerged entries are rejected before touching
#     this checkout; the resulting clean patch is applied atomically.
#   * The gate run is one `make -jN test-merge-samir` for the whole train
#     (harness/merge_samir.mk: SAMIR unit + emu + mutant gates, plus
#     test-pubs-lockstep-src if the Makefile has it; never LaTeX typesetting).
#     DBASE3_DECOMP etc. are taken from the environment/MAKEFLAGS as usual.
#   * It NEVER commits and NEVER pushes. Applied changes are left staged; the
#     PM reads the diff (Rule 4), commits, and pushes.
#   * Exit status: 0 all lanes applied and gates green; 1 conflict/usage/dirty
#     tree; 2 gates red.
#
# POSIX sh; needs git and make.

set -u

jobs=32
dry=0
run_gates=1
any_branch=0
allow_dirty=0
while getopts nj:TBD opt; do
  case $opt in
    n) dry=1 ;;
    j) jobs=$OPTARG ;;
    T) run_gates=0 ;;
    B) any_branch=1 ;;
    D) allow_dirty=1 ;;
    *) sed -n '2,/^# POSIX sh/p' "$0" | sed 's/^# \{0,1\}//' >&2; exit 1 ;;
  esac
done
shift $((OPTIND - 1))
if [ $# -eq 0 ]; then
  sed -n '2,/^# POSIX sh/p' "$0" | sed 's/^# \{0,1\}//' >&2
  exit 1
fi

die() { printf 'merge-train: %s\n' "$*" >&2; exit 1; }
case $jobs in ''|*[!0-9]*|0) die "-j requires a positive integer" ;; esac
[ "$jobs" -gt 0 ] 2>/dev/null || die "-j requires a positive integer"

repo=$(git rev-parse --show-toplevel 2>/dev/null) || die "not inside a git checkout"
cd "$repo" || die "cannot cd to $repo"
branch=$(git rev-parse --abbrev-ref HEAD)
if [ "$any_branch" -eq 0 ] && [ "$branch" != main ]; then
  die "on branch '$branch', not main (use -B to override)"
fi
dirty=$(git status --porcelain --untracked-files=all)
if [ "$allow_dirty" -eq 0 ] && [ -n "$dirty" ]; then
  die "working tree has uncommitted changes (commit/stash them or use -D)"
fi
if [ "$dry" -eq 1 ] && [ -n "$dirty" ]; then
  die "dry run refuses a dirty destination, including with -D; commit/stash first"
fi

tmp=$(mktemp -d "${TMPDIR:-/tmp}/merge-train.XXXXXX") || die "mktemp failed"
trap 'if [ -n "${drywt:-}" ]; then git worktree remove --force "$drywt" >/dev/null 2>&1; fi; rm -rf "$tmp"' EXIT INT TERM

# target = where patches are applied: this checkout, or a throwaway worktree.
target=$repo
if [ "$dry" -eq 1 ]; then
  drywt=$tmp/dry
  git worktree add --detach "$drywt" HEAD >/dev/null 2>&1 || die "cannot create dry-run worktree"
  target=$drywt
fi

# lane_patch LANE OUTFILE -- write the lane's diff relative to our HEAD base.
lane_patch() {
  lane=$1; out=$2
  if [ -d "$lane" ]; then
    ltop=$(git -C "$lane" rev-parse --show-toplevel 2>/dev/null) || return 1
    base=$(git -C "$ltop" merge-base HEAD "$(git -C "$repo" rev-parse HEAD)" 2>/dev/null) || return 1
    idx=$tmp/index.$$
    index=$(git -C "$ltop" rev-parse --path-format=absolute --git-path index) || return 1
    # Preserve index time: a newly dated copy can hide same-size edits made
    # in the source index's timestamp tick (Git's racily-clean safeguard).
    cp -pf "$index" "$idx" || return 1
    GIT_INDEX_FILE=$idx git -C "$ltop" add -A >/dev/null 2>&1 || return 1
    GIT_INDEX_FILE=$idx git -C "$ltop" diff --cached --binary "$base" > "$out" || return 1
    rm -f "$idx"
  else
    git rev-parse --verify --quiet "$lane^{commit}" >/dev/null || return 1
    base=$(git merge-base HEAD "$lane") || return 1
    git diff --binary "$base" "$lane" > "$out" || return 1
  fi
}

n=0
summary=$tmp/summary
: > "$summary"
for lane in "$@"; do
  n=$((n + 1))
  patch=$tmp/lane$n.patch
  lane_patch "$lane" "$patch" || die "lane '$lane': not a git directory or resolvable ref"
  if [ ! -s "$patch" ]; then
    printf 'merge-train: lane %s: empty diff, skipped\n' "$lane" >&2
    printf '  %-40s empty (skipped)\n' "$lane" >> "$summary"
    continue
  fi
  files=$(git apply --numstat "$patch" | wc -l | tr -d ' ')
  # --check --3way can return 0 WITH conflicts. Actually merge in an isolated
  # index (CLAUDE Rules 2/4), then inspect its stages before any destination
  # write. Diff the before/after trees so the final apply needs no three-way
  # conflict machinery and preserves unrelated staged/unstaged changes (-D).
  index=$(git -C "$target" rev-parse --path-format=absolute --git-path index) || die "cannot locate destination index"
  preview=$tmp/preview.index
  cp -f "$index" "$preview" || die "cannot snapshot destination index"
  before=$(GIT_INDEX_FILE=$preview git -C "$target" write-tree) || die "destination has unmerged entries"
  if ! out=$(GIT_INDEX_FILE=$preview git -C "$target" apply --cached --3way "$patch" 2>&1) ||
     [ -n "$(GIT_INDEX_FILE=$preview git -C "$target" ls-files --unmerged)" ]; then
    printf 'merge-train: CONFLICT: lane %s (%s files) does not apply cleanly onto %s.\n' "$lane" "$files" "$(git -C "$target" rev-parse --short HEAD)" >&2
    printf '%s\n' "$out" | sed 's/^/  | /' >&2
    printf 'merge-train: STOPPED. No auto-resolution. Lanes applied so far stay staged in %s;\n' "$target" >&2
    printf '  inspect with `git diff --cached --stat`, undo with `git reset --hard HEAD`.\n' >&2
    exit 1
  fi
  after=$(GIT_INDEX_FILE=$preview git -C "$target" write-tree) || die "lane '$lane' has unmerged entries"
  clean=$tmp/clean.patch
  git -C "$target" diff --binary "$before" "$after" > "$clean" || die "cannot produce clean lane patch"
  # Applying an already-merged lane is a no-op, not an empty-patch error.
  if [ -s "$clean" ] && ! out=$(git -C "$target" apply --index "$clean" 2>&1); then
    printf 'merge-train: CONFLICT: lane %s does not match the destination working tree.\n' "$lane" >&2
    printf '%s\n' "$out" | sed 's/^/  | /' >&2
    printf 'merge-train: STOPPED. Failed lane was not applied; earlier lanes stay staged in %s.\n' "$target" >&2
    exit 1
  fi
  printf '  %-40s applied, %s files\n' "$lane" "$files" >> "$summary"
  printf 'merge-train: applied lane %s (%s files)\n' "$lane" "$files"
done

printf '\n=== merge-train summary (%s, base %s) ===\n' "$( [ "$dry" -eq 1 ] && echo DRY RUN || echo "branch $branch" )" "$(git rev-parse --short HEAD)"
cat "$summary"
git -C "$target" diff --cached --stat | tail -n 25

if [ "$dry" -eq 1 ]; then
  printf 'dry run: all %s lane(s) apply cleanly; gates not run; this checkout untouched.\n' "$#"
  exit 0
fi
if [ "$run_gates" -eq 0 ]; then
  printf 'applied only (-T); changes are staged, NOT committed.\n'
  exit 0
fi

printf '\n>>> make -j%s test-merge-samir (once for the whole train)\n' "$jobs"
t0=$(date +%s)
if make "-j$jobs" test-merge-samir; then rc=0; else rc=2; fi
t1=$(date +%s)
if [ "$rc" -eq 0 ]; then
  printf '\nmerge-train: GREEN in %ss. Changes are staged, NOT committed or pushed (PM does that).\n' "$((t1 - t0))"
else
  printf '\nmerge-train: gates RED after %ss -- see the failing gate above. Changes left staged.\n' "$((t1 - t0))" >&2
fi
exit "$rc"
