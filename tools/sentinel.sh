#!/bin/sh
# PM-local post-push sentinel (initech-3nc3v); PRD Sec 8/9/14, Rule 10.
# Detached builds only. Never push or change the PM's branch/worktree.
set -eu
export LC_ALL=C
unset MAKEFLAGS MFLAGS MAKELEVEL MAKEOVERRIDES GNUMAKEFLAGS MAKEFILES
# Never inherit writable paths from the PM checkout (including cert workers).
unset BUILD PUBS_BUILD KERNEL_REPRO_DIR OVL_REPRO_B SCREEN_OUT FIXTURE_VOLUMES_SCAN
unset CCACHE_DIR CCACHE_BASEDIR CCACHE_TEMPDIR CCACHE_CONFIGPATH
unset CCACHE_LOGFILE CCACHE_DEBUGDIR CCACHE_STATSLOG
jobs=8 once=0 no_cert=0
die() { printf 'sentinel: %s\n' "$*" >&2; exit 2; }
# A make failure is red; a lost/replaced worktree is a setup error. The marker
# carries that distinction out of subshells even inside if/|| (errexit is off).
tree_error() { : > "$run/worktree-error"; exit 2; }
tree_command() (
    cd "$tree" || tree_error
    [ "$(git rev-parse --show-toplevel)" = "$tree" ] || tree_error
    exec "$@"
)
check_tree() { [ ! -f "$run/worktree-error" ] || die 'build worktree disappeared or was replaced'; }
while [ "$#" -gt 0 ]; do
    case $1 in
        --once) once=1;;
        --no-cert) no_cert=1;;
        --jobs) [ "$#" -ge 2 ] || die '--jobs requires N'; jobs=$2; shift;;
        *) die 'usage: tools/sentinel.sh [--once] [--jobs N] [--no-cert]';;
    esac
    shift
done
case $jobs in ''|0*|*[!0-9]*) die '--jobs must be a positive integer';; esac
repo=$(git rev-parse --show-toplevel)
cd "$repo"
state=$repo/build/sentinel
tree=$(dirname "$repo")/initech-os-sentinel
mkdir -p "$state"
# This sibling can be shared by several PM checkouts. Do not race its HEAD.
mkdir "$tree.lock" 2>/dev/null || die "another sentinel owns $tree.lock"
cleanup() { rmdir "$tree.lock"; }
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' HUP TERM
common=$(git rev-parse --path-format=absolute --git-common-dir)
seen=
while :; do
    git fetch --quiet origin '+refs/heads/main:refs/remotes/origin/main' || die 'fetch origin/main failed'
    head=$(git rev-parse --verify 'origin/main^{commit}')
    if [ "$head" = "$seen" ]; then sleep 60; continue; fi
    if [ -f "$state/last-green" ]; then
        base=$(cat "$state/last-green")
        base_source=last-green
    else
        base=$head~1
        base_source='origin/main~1 (last-green absent)'
        printf 'sentinel: last-green absent; base is origin/main~1\n'
    fi
    base=$(git rev-parse --verify "$base^{commit}") || die 'invalid last-green/base commit'
    git merge-base --is-ancestor "$base" "$head" || die 'last-green is not an ancestor of origin/main'
    if [ -e "$tree" ] || [ -L "$tree" ]; then
        [ ! -L "$tree" ] && [ -f "$tree/.git" ] || die 'sentinel path is not a detached worktree'
        [ "$(git -C "$tree" rev-parse --path-format=absolute --git-common-dir)" = "$common" ] || die 'sentinel belongs to another repository'
        [ -z "$(git -C "$tree" symbolic-ref -q HEAD || :)" ] || die 'sentinel worktree is attached to a branch'
        [ -z "$(git -C "$tree" status --porcelain)" ] || die 'sentinel worktree is dirty'
        git -C "$tree" checkout --quiet --detach "$head"
    else
        git worktree add --quiet --detach "$tree" "$head"
    fi
    dir=$state/$head
    mkdir -p "$dir"
    run=$(mktemp -d "$dir/run.XXXXXX")
    log=$dir/gates.log
    summary=$dir/summary
    : > "$log"
    : > "$run/dispatches"
    : > "$run/green"
    : > "$run/stage-1.red"
    : > "$run/stage-2.red"
    : > "$run/flakes"
    # F8's test-for uses -j16 internally. All recursive makes go through this
    # wrapper; the final -j enforces our bound. Observe F8's completion files,
    # including gates blocked by a failed prerequisite (no log-text guessing).
    cat > "$run/make-bounded" <<'MAKE'
#!/bin/sh
[ "$(git rev-parse --show-toplevel)" = "$SENTINEL_EXPECTED_TREE" ] || { : > "$SENTINEL_TREE_ERROR"; exit 2; }
for arg do
    case $arg in */gate-map-run.*/done.mk) printf '%s\n' "${arg%/done.mk}" >> "$SENTINEL_GATE_RUNS";; esac
done
exec make "$@" "-j${SENTINEL_MAKE_JOBS}"
MAKE
    chmod +x "$run/make-bounded"
    SENTINEL_GATE_RUNS=$run/dispatches
    SENTINEL_EXPECTED_TREE=$tree SENTINEL_TREE_ERROR=$run/worktree-error
    export SENTINEL_GATE_RUNS SENTINEL_EXPECTED_TREE SENTINEL_TREE_ERROR
    started=$(date +%s)
    stage1_rc=0
    tree_command env SENTINEL_MAKE_JOBS=$jobs make --no-print-directory -s "-j$jobs" \
        "MAKE=$run/make-bounded" gates-for "DIFF=$base..HEAD" > "$run/gates" 2>> "$log" || stage1_rc=$?
    check_tree
    if [ "$stage1_rc" -eq 0 ]; then
        cat "$run/gates" >> "$log"
        tree_command env SENTINEL_MAKE_JOBS=$jobs make --no-print-directory -k "-j$jobs" \
            "MAKE=$run/make-bounded" test-for "DIFF=$base..HEAD" >> "$log" 2>&1 || stage1_rc=$?
        check_tree
        observed=0
        sort -u "$run/dispatches" > "$run/dispatches.sorted"
        while IFS= read -r dispatch; do
            case $dispatch in /*) ;; *) dispatch=$tree/$dispatch;; esac
            if cmp -s "$run/gates" "$dispatch/gates" && [ -f "$dispatch/green" ]; then
                cat "$dispatch/green" >> "$run/green"
                observed=$((observed + 1))
            fi
        done < "$run/dispatches.sorted"
        if [ "$observed" -ne 1 ]; then
            printf 'sentinel: expected one F8 completion record, found %s\n' "$observed" >> "$log"
            stage1_rc=2
        fi
        awk 'FILENAME==ARGV[1] {green[$0]=1; next} !($0 in green)' "$run/green" "$run/gates" > "$run/stage-1.red"
        # The original list stays red even if an isolated rerun succeeds.
        cp -f "$run/stage-1.red" "$run/solo"
        while IFS= read -r gate; do
            printf '\n[stage-1 solo %s]\n' "$gate" >> "$log"
            if tree_command env SENTINEL_MAKE_JOBS=1 make --no-print-directory -j1 \
                "MAKE=$run/make-bounded" "$gate" >> "$log" 2>&1; then
                printf 'FLAKY %s stage-1\n' "$gate" >> "$run/flakes"
                : # FLAKE-KEEPS-RED
            fi
            check_tree
        done < "$run/solo"
    fi
    stage1_seconds=$(( $(date +%s) - started ))
    stage1=green
    if [ "$stage1_rc" -ne 0 ] || [ -s "$run/stage-1.red" ]; then stage1=red; fi
    stage2='skipped (--no-cert)'
    stage2_seconds=0
    deferred=0
    if [ "$no_cert" -eq 0 ]; then
        stage2_started=$(date +%s)
        cpus=$(nproc)
        retries=0
        while :; do
            load=${SENTINEL_LOADAVG:-$(cut -d ' ' -f 1 /proc/loadavg)}
            case $load in ''|*[!0-9.]*|.*|*.*.*) die 'invalid 1-minute load average';; esac
            if awk -v avg="$load" -v cpus="$cpus" 'BEGIN {exit !(avg <= cpus * 0.75)}'; then break; fi
            printf 'sentinel: load %s > %s * 0.75; defer (%s/10)\n' "$load" "$cpus" "$retries" >> "$log"
            if [ "$retries" -eq 10 ]; then deferred=1; break; fi
            sleep 60
            retries=$((retries + 1))
        done
        if [ "$deferred" -eq 1 ]; then stage2=deferred
        else
            cert_rc=0
            tree_command "${SENTINEL_CERT_CMD:-tools/cert-sharded.sh}" -j "$jobs" --out "$run/cert" >> "$log" 2>&1 || cert_rc=$?
            check_tree
            if [ -f "$run/cert/results.txt" ]; then
                awk '$1=="FAIL" || $1=="FAIL-CONFIRMED" || $1=="FLAKY" {print $3}' "$run/cert/results.txt" | sort -u > "$run/stage-2.red"
                awk '$1=="FLAKY" {print "FLAKY " $3 " stage-2"}' "$run/cert/results.txt" >> "$run/flakes"
            else
                printf 'sentinel: certificate produced no results.txt\n' >> "$log"
                cert_rc=2
            fi
            stage2=green
            if [ "$cert_rc" -ne 0 ] || [ -s "$run/stage-2.red" ]; then stage2=red; fi
        fi
        stage2_seconds=$(( $(date +%s) - stage2_started ))
    fi
    result=green
    [ "$deferred" -eq 0 ] || result=deferred
    if [ "$stage1" = red ] || [ "$stage2" = red ]; then result=red; fi
    {
        printf 'base %s\nbase-source %s\nhead %s\n' "$base" "$base_source" "$head"
        printf 'stage-1 gates %s\nstage-1 result %s\n' "$(wc -l < "$run/gates" | tr -d ' ')" "$stage1"
        while IFS= read -r gate; do printf 'stage-1 red %s\n' "$gate"; done < "$run/stage-1.red"
        printf 'stage-1 wall-seconds %s\nstage-2 result %s\nstage-2 wall-seconds %s\n' "$stage1_seconds" "$stage2" "$stage2_seconds"
        while IFS= read -r gate; do printf 'stage-2 red %s\n' "$gate"; done < "$run/stage-2.red"
        cat "$run/flakes"
        printf 'result %s\n' "$result"
    } > "$summary"
    cat "$run/stage-1.red" "$run/stage-2.red" | sort -u > "$run/red"
    while IFS= read -r gate; do
        culprit_rc=0
        SENTINEL_HEAD=$head SENTINEL_JOBS=$jobs "$repo/tools/culprit.sh" "$gate" "$base" > "$run/culprit-$gate" 2>> "$log" || culprit_rc=$?
        cat "$run/culprit-$gate" >> "$summary"
        if [ "$culprit_rc" -ne 0 ]; then printf 'culprit error %s (see gates.log)\n' "$gate" >> "$summary"; fi
        merge=$(awk '$1=="CULPRIT" {print $3; exit}' "$run/culprit-$gate")
        if [ -n "$merge" ]; then
            # F10's -y permits undoing newer merges before the culprit.
            printf "tools/revert-merge.sh -y -r 'sentinel: %s' -l 'build/sentinel/%s/gates.log' %s\n" "$gate" "$head" "$merge" >> "$summary"
        fi
    done < "$run/red"
    printf 'wall-seconds %s\n' "$(( $(date +%s) - started ))" >> "$summary"
    if [ "$result" = green ]; then
        printf '%s\n' "$head" > "$state/last-green.next"
        mv -f "$state/last-green.next" "$state/last-green"
    fi
    cat "$summary"
    printf 'sentinel: summary build/sentinel/%s/summary\n' "$head"
    case $result in red) exit 1;; deferred) exit 3;; esac
    [ "$once" -eq 0 ] || exit 0
    seen=$head
    sleep 60
done
