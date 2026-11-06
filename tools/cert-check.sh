#!/bin/sh
# Ref: PRD sections 8/9; Merge workflow v2 F9 (initech-xses3).
set -eu
LC_ALL=C
export LC_ALL
refuse() { printf 'cert-check: %s\n' "$1" >&2; exit 2; }
[ "$#" -eq 1 ] || refuse 'usage: cert-check.sh <cert>'
[ -f "$1" ] || refuse 'certificate missing'
cert=$1
case $cert in /*) ;; *) cert=$PWD/$cert;; esac
root=$(git rev-parse --show-toplevel 2>/dev/null) || refuse 'not a git worktree'
cd "$root"
if grep -q '[^ -~]' "$cert"; then refuse 'non-ASCII certificate'; fi
field() {
    [ "$(grep -c "^$1: " "$cert")" -eq 1 ] || refuse "missing or duplicate $1"
    sed -n "s/^$1: //p" "$cert"
}
base=$(field base_sha)
head=$(field head_sha)
count=$(field gate_count)
case $base in ''|*[!0-9a-f]*) refuse 'invalid base_sha';; esac
case $head in ''|*[!0-9a-f]*) refuse 'invalid head_sha';; esac
[ "${#base}" -eq 40 ] && [ "${#head}" -eq 40 ] || refuse 'invalid SHA length'
main=$(git rev-parse --verify refs/heads/main^{commit} 2>/dev/null) || refuse 'main missing'
[ "$base" = "$main" ] || refuse "stale base_sha: $base; main is $main"
case $count in ''|*[!0-9]*) refuse 'invalid gate_count';; esac
[ "$count" -gt 0 ] || refuse 'empty gate list'
mkdir -p build/cert
scratch=$(mktemp -d build/cert/.cert-check-XXXXXX)
trap 'rm -rf "$scratch"' 0
trap 'exit 2' HUP INT TERM
: > "$scratch/gates"
while IFS= read -r line; do
    case $line in 'gate '*)
        rest=${line#gate }
        gate=${rest%% *}
        final=${rest#* }
        case $gate in ''|[!A-Za-z0-9]*|*[!A-Za-z0-9_.-]*) refuse "invalid gate: $gate";; esac
        case $final in ">>> $gate: green"|">>> $gate: green "*) ;; *) refuse "non-green gate: $gate";; esac
        grep -qxF "$gate" "$scratch/gates" && refuse "duplicate gate: $gate"
        printf '%s\n' "$gate" >> "$scratch/gates";;
    esac
done < "$cert"
[ "$(wc -l < "$scratch/gates")" -eq "$count" ] || refuse 'gate_count mismatch'
make_cmd=${MAKE:-make}
if "$make_cmd" -n gates-for >/dev/null 2>&1; then
    required=$("$make_cmd" -s --no-print-directory gates-for "DIFF=$base..$head") || refuse 'gate selection failed'
    missing=no
    set -f
    for gate in $required; do
        if ! grep -qxF -- "$gate" "$scratch/gates"; then
            printf 'cert-check: missing gate: %s\n' "$gate" >&2
            missing=yes
        fi
    done
    [ "$missing" = no ] || exit 2
else
    printf 'cert-check: gates-for absent; coverage not checked\n'
fi
printf 'cert-check: PASS (%s gates; base == main)\n' "$count"
