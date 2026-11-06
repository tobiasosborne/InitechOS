#!/bin/sh
# Ref: PRD sections 8/9/14; Merge workflow v2 F9 (initech-xses3).
set -eu
LC_ALL=C
export LC_ALL
refuse() { printf 'lane-cert: %s\n' "$1" >&2; exit 2; }
root=$(git rev-parse --show-toplevel 2>/dev/null) || refuse 'not a git worktree'
cd "$root"
branch=$(git symbolic-ref --quiet --short HEAD) || refuse 'detached HEAD'
slug=$(printf '%s' "$branch" | sed 's/[^A-Za-z0-9._-]/-/g')
cert=build/cert/$slug.cert
# A failed attempt revokes the previous certificate for this branch.
rm -f "$cert"
dirty=$(git status --porcelain --untracked-files=all) || refuse 'cannot inspect tree'
if [ -n "$dirty" ]; then
    refuse 'dirty tree'
fi
base=$(git rev-parse --verify origin/main^{commit} 2>/dev/null) || refuse 'origin/main missing'
head=$(git rev-parse --verify HEAD)
merge_base=$(git merge-base "$base" "$head") || refuse 'rebase required: unrelated history'
[ "$merge_base" = "$base" ] || refuse 'rebase required: origin/main advanced'
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
if clean_make -n gates-for >/dev/null 2>&1; then
    gates=$(clean_make -s --no-print-directory gates-for "DIFF=origin/main..HEAD") || refuse 'gate selection failed'
else
    gates=${GATES:-}
fi
[ -n "$gates" ] || refuse 'no gate list'
mkdir -p build/cert
scratch=$(mktemp -d build/cert/.lane-cert-XXXXXX)
trap 'rm -rf "$scratch"' 0
trap 'exit 2' HUP INT TERM
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
printf 'base_sha: %s\nhead_sha: %s\nbranch: %s\ngate_count: %s\n' \
    "$base" "$head" "$branch" "$count" > "$scratch/cert"
while IFS= read -r gate; do
    gate_rc=0
    clean_make "$gate" > "$scratch/gate.log" 2>&1 || gate_rc=$?
    # Only this target's FINAL summary can certify it; nested gates do not.
    final=$(grep -F ">>> $gate:" "$scratch/gate.log" | tail -n 1)
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
