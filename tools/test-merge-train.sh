#!/bin/sh
# Ref: CLAUDE Rules 2/4/6; the lane that fails must change no destination byte.
set -eu
train=$(CDPATH= cd -- "$(dirname "$0")" && pwd)/merge-train.sh
mkdir -p build
root=$(mktemp -d "$PWD/build/merge-train-test.XXXXXX")
trap 'rm -rf "$root"' EXIT
export GIT_CONFIG_NOSYSTEM=1 GIT_CONFIG_GLOBAL=/dev/null
export GIT_AUTHOR_NAME=Oracle GIT_COMMITTER_NAME=Oracle
export GIT_AUTHOR_EMAIL=oracle@example.invalid GIT_COMMITTER_EMAIL=oracle@example.invalid
fail() { echo "RED merge-train: $*" >&2; cat "$root/run.log" >&2; exit 1; }
run() {
    expected=$1; shift
    rc=0; (cd "$repo" && sh "$train" "$@") > "$root/run.log" 2>&1 || rc=$?
    [ "$rc" -eq "$expected" ] || fail "expected exit $expected, got $rc: $*"
}
repo=$root/repo
lane=$root/lane
git init -q -b main "$repo"
printf 'base\n' > "$repo/f"
printf 'other\n' > "$repo/g"
printf 'test-merge-samir:\n\t@true\n' > "$repo/Makefile"
git -C "$repo" add .
git -C "$repo" -c core.hooksPath=/dev/null commit -qm base
git -C "$repo" worktree add -qb lane "$lane"
printf 'lane\n' > "$lane/f"
printf 'untracked lane\n' > "$lane/new"
git -C "$lane" diff --cached --binary > "$root/lane-index"
run 0 -n -T "$lane"
[ "$(cat "$repo/f")" = base ] && [ ! -e "$repo/new" ] || fail 'dry run changed destination'
run 0 -T "$lane"
cmp "$repo/new" "$lane/new" || fail 'directory lane lost untracked input'
git -C "$lane" diff --cached --binary > "$root/after-index"
cmp "$root/lane-index" "$root/after-index" || fail 'directory lane changed real index'
git -C "$repo" reset -q --hard HEAD
# An untracked collision must be preserved even with the dirty override.
printf 'precious\n' > "$repo/new"
run 1 -T "$lane"
run 1 -D -T "$lane"
run 1 -n -D -T "$lane"
[ "$(cat "$repo/new")" = precious ] && [ "$(cat "$repo/f")" = base ] || fail 'untracked collision changed destination'
rm -f "$repo/new"
# A conflicting lane also adds a clean file: neither may enter the destination.
printf 'destination\n' > "$repo/f"
git -C "$repo" add f
git -C "$repo" -c core.hooksPath=/dev/null commit -qm destination
run 1 -T "$lane"
[ "$(cat "$repo/f")" = destination ] || fail 'failed lane left conflict markers'
[ ! -e "$repo/new" ] || fail 'failed lane staged a partial addition'
[ -z "$(git -C "$repo" status --porcelain)" ] || fail 'failed lane changed index/worktree'
# An earlier successful lane must survive a later rejection.
git -C "$repo" worktree add -qb first "$root/first"
printf 'first\n' > "$root/first/g"
run 1 -T "$root/first" "$lane"
[ "$(cat "$repo/g")" = first ] || fail 'rollback discarded earlier lane'
[ "$(cat "$repo/f")" = destination ] && [ ! -e "$repo/new" ] || fail 'later lane partially applied'
git -C "$repo" reset -q --hard HEAD
# Untracked is dirty, including in a dry run. -D must preserve it.
printf 'local untracked\n' > "$repo/precious"
run 1 -T "$root/first"
run 1 -n -T "$root/first"
run 1 -n -D -T "$root/first"
run 0 -D -T "$root/first"
[ "$(cat "$repo/precious")" = 'local untracked' ] || fail 'dirty override lost untracked file'
git -C "$repo" reset -q --hard HEAD
rm -f "$repo/precious"
# Allowed staged and unstaged changes survive unrelated lanes.
printf 'staged\n' > "$repo/staged"
git -C "$repo" add staged
printf 'unstaged\n' > "$repo/f"
run 1 -n -D -T "$root/first"
run 0 -D -T "$root/first"
[ "$(cat "$repo/f")" = unstaged ] || fail 'dirty override changed unstaged file'
git -C "$repo" diff --cached --name-only | grep -qx staged || fail 'dirty override lost staged file'
git -C "$repo" reset -q --hard HEAD
# Committed branch/ref lanes use the same preflight and staging path.
git -C "$root/first" add g
git -C "$root/first" -c core.hooksPath=/dev/null commit -qm 'committed lane'
run 0 -T first
[ "$(cat "$repo/g")" = first ] || fail 'branch lane lost its committed edit'
git -C "$repo" reset -q --hard HEAD
# Ordinary gate failures map to 2; malformed parallelism is usage, exit 1.
printf 'test-merge-samir:\n\t@exit 1\n' > "$root/first/Makefile"
run 2 "$root/first"
git -C "$repo" reset -q --hard HEAD
run 1 -j 0 -T "$root/first"
run 1 -j bogus -T "$root/first"
echo 'GREEN test-merge-train: dry/dirty/untracked lanes, atomic conflicts, earlier lanes, index preservation, exit codes'
