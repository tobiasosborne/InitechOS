#!/bin/sh
# Local first-parent regression search (initech-3nc3v); PRD Sec 8/9/14.
# A solo green classifies the original red as FLAKY; it never erases it.
set -eu
export LC_ALL=C
unset MAKEFLAGS MFLAGS MAKELEVEL MAKEOVERRIDES GNUMAKEFLAGS MAKEFILES
# Standalone runs need the same writable-path isolation as the sentinel.
unset BUILD PUBS_BUILD KERNEL_REPRO_DIR OVL_REPRO_B SCREEN_OUT FIXTURE_VOLUMES_SCAN
unset CCACHE_DIR CCACHE_BASEDIR CCACHE_TEMPDIR CCACHE_CONFIGPATH
unset CCACHE_LOGFILE CCACHE_DEBUGDIR CCACHE_STATSLOG
die() { printf 'culprit: %s\n' "$*" >&2; exit 2; }
tree_error() { : > "$run/worktree-error"; exit 2; }
tree_make() (
    cd "$tree" || tree_error
    [ "$(git rev-parse --show-toplevel)" = "$tree" ] || tree_error
    exec make "$@"
)
check_tree() { [ ! -f "$run/worktree-error" ] || die 'build worktree disappeared or was replaced'; }
[ "$#" -ge 1 ] && [ "$#" -le 2 ] || die 'usage: tools/culprit.sh <gate> [<base-sha>]'
gate=$1
case $gate in test-*) ;; *) die 'expected a test-* gate';; esac
case $gate in *[!A-Za-z0-9_.-]*) die 'invalid gate name';; esac
jobs=${SENTINEL_JOBS:-8}
case $jobs in ''|0*|*[!0-9]*) die 'SENTINEL_JOBS must be a positive integer';; esac
repo=$(git rev-parse --show-toplevel)
cd "$repo"
# The sentinel pins the fetched snapshot even if another checkout fetches.
head=$(git rev-parse --verify "${SENTINEL_HEAD:-origin/main}^{commit}")
if [ "$#" -eq 2 ]; then base=$2
elif [ -f build/sentinel/last-green ]; then base=$(cat build/sentinel/last-green)
else base=$head~1; printf 'culprit: last-green absent; base is origin/main~1\n' >&2
fi
base=$(git rev-parse --verify "$base^{commit}")
git merge-base --is-ancestor "$base" "$head" || die 'base is not an ancestor of head'
mkdir -p build/culprit
run=$(mktemp -d "$repo/build/culprit/run.XXXXXX")
tree=$run/tree
cleanup() {
    rc=$?
    trap - EXIT HUP INT TERM
    if [ -e "$tree/.git" ]; then git worktree remove --force "$tree" || rc=2; fi
    exit "$rc"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' HUP TERM
# Last -j wins, including over explicit -j flags in recursive make recipes.
cat > "$run/make-bounded" <<'MAKE'
#!/bin/sh
[ "$(git rev-parse --show-toplevel)" = "$SENTINEL_EXPECTED_TREE" ] || { : > "$SENTINEL_TREE_ERROR"; exit 2; }
exec make "$@" "-j${SENTINEL_MAKE_JOBS}"
MAKE
chmod +x "$run/make-bounded"
SENTINEL_EXPECTED_TREE=$tree SENTINEL_TREE_ERROR=$run/worktree-error
export SENTINEL_EXPECTED_TREE SENTINEL_TREE_ERROR
git log --reverse --first-parent --merges --format=%H "$base..$head" > "$run/merges"
while IFS= read -r merge; do
    git worktree add --quiet --detach "$tree" "$merge"
    subject=$(git log -1 --format=%s "$merge" | tr -c '\11\12\15\40-\176' '?')
    # A gate introduced later cannot indict this older revision. Match only
    # this target's missing-rule diagnostic, never a missing prerequisite.
    probe_rc=0
    SENTINEL_MAKE_JOBS=1 tree_make --no-print-directory -n -j1 \
        "MAKE=$run/make-bounded" "$gate" > "$run/$merge.exists.log" 2>&1 || probe_rc=$?
    check_tree
    if [ "$probe_rc" -ne 0 ]; then
        if grep -Fq "No rule to make target '$gate'." "$run/$merge.exists.log"; then
            printf 'UNTESTABLE %s %s %s\n' "$gate" "$merge" "$subject"
            git worktree remove --force "$tree"
            continue
        fi
    fi
    if SENTINEL_MAKE_JOBS=$jobs tree_make --no-print-directory "-j$jobs" \
        "MAKE=$run/make-bounded" "$gate" > "$run/$merge.log" 2>&1; then
        check_tree
        git worktree remove --force "$tree"
        : # GREEN-CONTINUE
    else
        check_tree
        # Exactly one rerun, alone, in the SAME worktree. Keep both logs.
        if SENTINEL_MAKE_JOBS=1 tree_make --no-print-directory -j1 \
            "MAKE=$run/make-bounded" "$gate" > "$run/$merge.solo.log" 2>&1; then
            printf 'FLAKY %s %s %s\n' "$gate" "$merge" "$subject"
        fi
        check_tree
        printf 'CULPRIT %s %s %s\n' "$gate" "$merge" "$subject"
        printf 'culprit: logs %s\n' "$run" >&2
        exit 0
    fi
done < "$run/merges"
printf 'no culprit in range (flake class)\n'
