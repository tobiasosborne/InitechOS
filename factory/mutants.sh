#!/bin/sh
# Components own their selection; pending patches remain explicitly unproved.
set -eu
out=$1
mkdir -p "$out/mutants"
find . -type d \( -name build -o -name attic -o -name .git \) -prune -o -type d -name mutants -print |
while read -r dir; do
    component=${dir%/mutants}
    while [ ! -f "$component/Makefile" ]; do component=$(dirname "$component"); done
    printf '%s\n' "${component#./}"
done | sort -u > "$out/mutants/components"
# Bounded batches avoid multiple makes owning any component's OUT.
pids= count=0
wait_batch() {
    for pid in $pids; do wait "$pid" || :; done
    pids= count=0
}
while read -r component; do
    key=$(printf '%s' "$component" | tr / _)
    (
      rc=0
      make --no-print-directory -C "$component" mutants > "$out/mutants/$key.log" 2>&1 || rc=$?
      echo "$rc" > "$out/mutants/$key.rc"
    ) &
    pids="$pids $!"; count=$((count+1))
    [ "$count" -lt "${CERT_MUTANT_JOBS:-4}" ] || wait_batch
done < "$out/mutants/components"
wait_batch
bad=0
while read -r component; do
    key=$(printf '%s' "$component" | tr / _)
    pending=$(find "$component" -path '*/mutants/pending/*.patch' -type f | wc -l)
    sed -n '/mutants, .*killed, .*survived/p' "$out/mutants/$key.log"
    echo "$component: $pending pending patches"
    if [ "$(cat "$out/mutants/$key.rc")" -ne 0 ]; then
        bad=1; tail -n 5 "$out/mutants/$key.log"
    fi
done < "$out/mutants/components"
exit "$bad"
