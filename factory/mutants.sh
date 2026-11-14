#!/bin/sh
# Validate the complete scheduled inventory; pending coverage is never a kill.
set -eu
out=$1
root=$PWD
script_root=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
mkdir -p "$out/mutants"
out=$(CDPATH= cd -- "$out" && pwd)
run=$(mktemp -d "$out/mutants/run.XXXXXX")
# Separate commands preserve discovery errors: a pipeline ending in sort would
# discard a failed owner lookup under POSIX shell (which has no pipefail).
find "$root" -path "$out" -prune -o -type d \( -name build -o -name attic -o -name .git \) -prune -o -type d -name mutants -print > "$run/directories"
: > "$run/owners"
while IFS= read -r dir; do
    component=${dir%/mutants}
    while [ ! -f "$component/Makefile" ]; do
        [ "$component" != "$root" ] || { echo "FAIL no component owns $dir" >&2; exit 1; }
        component=$(dirname "$component")
    done
    find "$dir" -type f -name '*.patch' ! -path "$dir/pending/*" > "$run/active"
    active=$(wc -l < "$run/active")
    find "$dir" -type f -path "$dir/pending/*.patch" > "$run/pending"
    pending=$(wc -l < "$run/pending")
    if [ -f "$dir/extra.list" ]; then
        while read -r patch_path rest; do
            case $patch_path in ''|\#*) continue;; esac
            case $patch_path in /*|../*|*/../*|*/..) echo "FAIL invalid external mutation $patch_path" >&2; exit 1;; esac
            [ -z "$rest" ] && [ -f "$root/$patch_path" ] || { echo "FAIL missing external mutation $patch_path" >&2; exit 1; }
            active=$((active+1))
        done < "$dir/extra.list"
    fi
    printf '%s %s %s\n' "${component#"$root"/}" "$active" "$pending" >> "$run/owners"
done < "$run/directories"
sort "$run/owners" > "$run/sorted"
awk '{active[$1]+=$2;pending[$1]+=$3} END {for(c in active) print c,active[c],pending[c]}' "$run/sorted" > "$run/combined"
sort "$run/combined" > "$run/inventory"
[ -s "$run/inventory" ] || { echo 'FAIL no mutation components discovered' >&2; exit 1; }
awk '{total+=$2} END {exit (total <= 0)}' "$run/inventory" || {
    echo 'FAIL no active mutation patches discovered' >&2; exit 1;
}
cut -d ' ' -f 1 "$run/inventory" > "$out/mutants/components"
cp "$run/inventory" "$out/mutants/inventory"
bad=0 pids= count=0
wait_batch() {
    for pid in $pids; do
        if wait "$pid"; then :; else
            echo "FAIL mutation worker $pid exited unsuccessfully" >&2; bad=1
        fi
    done
    pids= count=0
}
case ${CERT_MUTANT_JOBS:-4} in ''|*[!0-9]*|0) echo 'FAIL invalid CERT_MUTANT_JOBS' >&2; exit 1;; esac
while read -r component active pending; do
    key=$(printf '%s' "$component" | tr / _)
    rm -f "$out/mutants/$key.log" "$out/mutants/$key.rc" "$out/mutants/$key.counts"
    (
        rc=0
        # Always run the declared target, including pending-only owners. An
        # unimplemented target must fail, never acquire a synthetic success.
        make --no-print-directory -C "$component" mutants > "$run/$key.log" 2>&1 || rc=$?
        printf '%s\n' "$rc" > "$run/$key.rc.tmp"
        mv "$run/$key.rc.tmp" "$run/$key.rc"
        exit "$rc"
    ) &
    pids="$pids $!"; count=$((count+1))
    [ "$count" -lt "${CERT_MUTANT_JOBS:-4}" ] || wait_batch
done < "$run/inventory"
wait_batch
pending_total=0
while read -r component active pending; do
    key=$(printf '%s' "$component" | tr / _)
    pending_total=$((pending_total+pending))
    if [ ! -f "$run/$key.rc" ] || [ ! -f "$run/$key.log" ]; then
        echo "FAIL $component: missing worker completion record or log" >&2; bad=1
    else
        rc=$(cat "$run/$key.rc")
        case $rc in 0) ;; *) echo "FAIL $component: worker result [$rc]" >&2; bad=1;; esac
        if ! awk -v component="$component" -v expected="$active" -f "$script_root/testlib/mutation-summary.awk" "$run/$key.log" > "$run/$key.counts"; then
            bad=1; tail -n 5 "$run/$key.log"
        fi
        sed -n '/mutants, .*killed, .*survived/p' "$run/$key.log"
    fi
    echo "$component: $pending pending patches (unproved)"
    for suffix in log rc counts; do
        [ ! -f "$run/$key.$suffix" ] || cp "$run/$key.$suffix" "$out/mutants/$key.$suffix"
    done
done < "$run/inventory"
printf '%s\n' "$pending_total" > "$out/mutants/pending-count"
if [ "$pending_total" -gt 0 ]; then echo "INCOMPLETE: $pending_total pending patches remain unproved"; fi
if [ "$bad" -ne 0 ]; then echo "FAIL mutation sweep; evidence: $run" >&2; exit 1; fi
echo 'ACTIVE MUTATIONS GREEN: every scheduled worker completed and every active entry was killed'
rm -rf "$run"
