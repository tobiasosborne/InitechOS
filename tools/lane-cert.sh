#!/bin/sh
# Ref: PRD sections 8/9/14; Merge workflow v2 F9 (initech-xses3).
set -eu
LC_ALL=C
export LC_ALL
refuse() { printf 'lane-cert: %s\n' "$1" >&2; exit 2; }
required=${REQUIRED_GATES:-}
scope=${CERT_SCOPE:-derived}
jobs=${LANE_CERT_JOBS:-}
case $# in
    0) ;;
    2) [ "$1" = --require ] || refuse 'usage: lane-cert.sh [--require <gates>]'; required=$2;;
    *) refuse 'usage: lane-cert.sh [--require <gates>]';;
esac
root=$(git rev-parse --show-toplevel 2>/dev/null) || refuse 'not a git worktree'
cd "$root"
branch=$(git symbolic-ref --quiet --short HEAD) || refuse 'detached HEAD'
slug=$(printf '%s' "$branch" | sed 's/[^A-Za-z0-9._-]/-/g')
cert=build/cert/$slug.cert
# A failed attempt revokes the previous certificate for this branch.
rm -f "$cert"
case $scope in derived|required) ;; *) refuse 'invalid CERT_SCOPE (derived or required)';; esac
case $jobs in ''|[1-8]) ;; *) refuse 'LANE_CERT_JOBS must be 1..8';; esac

dirty=$(git status --porcelain --untracked-files=all) || refuse 'cannot inspect tree'
if [ -n "$dirty" ]; then
    refuse 'dirty tree'
fi
base=$(git rev-parse --verify origin/main^{commit} 2>/dev/null) || refuse 'origin/main missing'
head=$(git rev-parse --verify HEAD)
merge_base=$(git merge-base "$base" "$head") || refuse 'rebase required: unrelated history'
[ "$merge_base" = "$base" ] || refuse 'rebase required: origin/main advanced'
mkdir -p build/cert
scratch=$(mktemp -d build/cert/.lane-cert-XXXXXX)
trap 'rm -rf "$scratch"' 0
trap 'exit 2' HUP INT TERM
make_cmd=${MAKE:-make}
# GNU make encodes inherited short options as a bare cluster (for example
# "ik"), as well as accepting ordinary command-line option spellings.
set -f
check_make_flags() {
    for flag in $2; do
        case $flag in
            --) break;;
            --ignore-errors*|--dry-run*|--just-print*|--recon*|--question*|--touch*|--old-file*|--assume-old*|--what-if*|--new-file*|--assume-new*)
                refuse "unsafe $1: $flag";;
            --*|*=*) ;;
            *) case ${flag#-} in *[inqtoW]*) refuse "unsafe $1: $flag";; esac;;
        esac
    done
}
check_make_flags MAKEFLAGS "${MAKEFLAGS:-}"
check_make_flags GNUMAKEFLAGS "${GNUMAKEFLAGS:-}"
clean_make() { env -u MAKEFLAGS -u GNUMAKEFLAGS "$make_cmd" "$@"; }
require_coverage() {
    case $required in *[![:space:]]*) ;; *) refuse 'coverage unchecked: gates-for absent; supply REQUIRED_GATES or --require';; esac
}
# A missing prerequisite or a makefile error is not an absent selector.
if clean_make -n --no-print-directory gates-for 'DIFF=origin/main..HEAD' > "$scratch/selector.out" 2> "$scratch/selector.err"; then
    selector=gates-for
    gates=$(clean_make -s --no-print-directory gates-for 'DIFF=origin/main..HEAD' 2> "$scratch/selector.err") || {
        cat "$scratch/selector.err" >&2; refuse 'gate selection failed: gates-for';
    }
else
    probe_rc=$?
    if [ "$probe_rc" -ne 2 ] || ! grep -qF "No rule to make target 'gates-for'." "$scratch/selector.err"; then
        cat "$scratch/selector.err" >&2; refuse 'gate selection failed: gates-for probe'
    fi
    selector=absent
    gates=${GATES:-}
    require_coverage
fi
[ -n "$gates" ] || refuse 'no gate list'
derived_count=$(printf '%s\n' $gates | sort -u | wc -l | tr -d ' ')
if [ "$scope" = required ]; then
    case $required in *[![:space:]]*) ;; *) refuse 'coverage unchecked: CERT_SCOPE=required needs REQUIRED_GATES';; esac
    # Still probe/run the selector: a broken selector never permits a subset.
    printf '%s\n' $gates > "$scratch/derived"
    for gate in $required; do
        grep -qxF -- "$gate" "$scratch/derived" || refuse "required gate not selected: $gate"
    done
    gates=$required
fi
: > "$scratch/gates"
count=0
# No glob expansion or make options from a malformed selector/environment.
set -f
for gate in $gates; do
    case $gate in ''|[!A-Za-z0-9]*|*[!A-Za-z0-9_.-]*) refuse "invalid gate: $gate";; esac
    if grep -qxF "$gate" "$scratch/gates"; then continue; fi
    printf '%s\n' "$gate" >> "$scratch/gates"
    count=$((count + 1))
done
[ "$count" -gt 0 ] || refuse 'no gate list'
if [ "$selector" = absent ]; then
    for gate in $required; do
        case $gate in ''|[!A-Za-z0-9]*|*[!A-Za-z0-9_.-]*) refuse "invalid required gate: $gate";; esac
        grep -qxF -- "$gate" "$scratch/gates" || refuse "missing gate: $gate (REQUIRED_GATES)"
    done
fi
printf 'base_sha: %s\nhead_sha: %s\nbranch: %s\nselector: %s\ngate_count: %s\n' \
    "$base" "$head" "$branch" "$selector" "$count" > "$scratch/cert"
printf 'scope: %s\nderived_gate_count: %s\n' "$scope" "$derived_count" >> "$scratch/cert"
# One make owns the whole batch and its dependency graph, including gate_pairs.
# A failed batch refuses the certificate even if every summary says green.
if [ -n "$jobs" ]; then
    set --
    while IFS= read -r gate; do set -- "$@" "$gate"; done < "$scratch/gates"
    batch_rc=0
    clean_make "-j$jobs" "$@" > "$scratch/batch.log" 2>&1 || batch_rc=$?
    cp -f "$scratch/batch.log" "build/cert/$slug.log"
    [ "$batch_rc" -eq 0 ] || refuse "red gate batch (exit $batch_rc; see build/cert/$slug.log)"
fi
while IFS= read -r gate; do
    gate_rc=0
    if [ -n "$jobs" ]; then
        log=$scratch/batch.log
    else
        log=$scratch/gate.log
        clean_make "$gate" > "$log" 2>&1 || gate_rc=$?
    fi
    # Only this target's FINAL summary can certify it; nested gates do not.
    final=$(grep -F ">>> $gate:" "$log" | tail -n 1)
    gate_ok=no
    case $final in ">>> $gate: green"|">>> $gate: green "*)
        [ "$gate_rc" -ne 0 ] || gate_ok=yes;;
    esac
    if [ "$gate_ok" != yes ]; then
        [ -z "$final" ] || printf '%s\n' "$final" >&2
        refuse "red gate: $gate (exit $gate_rc; missing or non-green final line)"
    fi
    printf 'gate %s %s\n' "$gate" "$final" >> "$scratch/cert"
    printf '%s\n' "$final"
done < "$scratch/gates"
version() {
    if command -v "$1" >/dev/null 2>&1; then
        "$@" 2>&1 | sed -n '1p'
    else
        printf 'absent\n'
    fi
}
printf 'toolchain: gcc=%s; nasm=%s; qemu-system-i386=%s; bochs=%s\nhost: %s\nutc: %s\n' \
    "$(version gcc --version)" "$(version nasm -v)" \
    "$(version qemu-system-i386 --version)" "$(version bochs --help)" \
    "$(uname -m)" "$(date -u '+%Y-%m-%dT%H:%M:%SZ')" >> "$scratch/cert"
# Do not certify a gate which edits tracked files or moves the checkout.
[ "$(git rev-parse HEAD)" = "$head" ] || refuse 'HEAD changed during gates'
[ "$(git symbolic-ref --quiet --short HEAD)" = "$branch" ] || refuse 'branch changed during gates'
[ "$(git rev-parse origin/main)" = "$base" ] || refuse 'origin/main changed during gates'
[ -z "$(git status --porcelain --untracked-files=all)" ] || refuse 'dirty tree after gates'
if grep -q '[^ -~]' "$scratch/cert"; then refuse 'non-ASCII certificate data'; fi
if [ -n "${HOME:-}" ] && grep -qF "$HOME" "$scratch/cert"; then refuse 'home path in certificate data'; fi
mv -f "$scratch/cert" "$cert"
printf 'lane-cert: %s\n' "$cert"
