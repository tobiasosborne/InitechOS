#!/bin/sh
# Each worker owns its dependency tree and runs the strict host grader.
set -eu
top=$1 component=$2 out=$3 make_cmd=$4 jobs=$5
shift 5
count=$#
[ "$jobs" -le "$count" ] || jobs=$count
mkdir -p "$out/mutants"
run=$(mktemp -d "$out/mutants/parallel.XXXXXX")
start=$(date +%s)

# Keep equal selections near each other so most baselines are built once.
# Split large groups at worker boundaries so a single selection can use all
# slots when necessary. The baseline and its patches still share one TOP.
: > "$run/selected"
for patch_file do
    selection=$(sed -n 's/^# test: //p' "$patch_file")
    printf '%s\t%s\n' "$selection" "$patch_file" >> "$run/selected"
done
tab=$(printf '\t')
LC_ALL=C sort -s -t "$tab" -k1,1 "$run/selected" > "$run/sorted"
awk -F "$tab" -v run="$run" -v jobs="$jobs" -v count="$count" '
    NF != 2 { exit 1 }
    { slot = int((NR - 1) * jobs / count) + 1; print $2 > (run "/" slot ".list") }
' "$run/sorted"

pids=
slot=1
while [ "$slot" -le "$jobs" ]; do
    (
        set --
        while IFS= read -r patch_file; do
            set -- "$@" "$patch_file"
        done < "$run/$slot.list"
        MUTANT_JOBS=1 sh "$top/factory/testlib/run-mutants.sh" \
            "$top" "$component" "$run/$slot" "$make_cmd" "$@"
    ) > "$run/$slot.log" 2>&1 &
    pids="$pids $!"
    slot=$((slot + 1))
done

failed=0
for pid in $pids; do
    wait "$pid" || failed=$((failed + 1))
done
slot=1
while [ "$slot" -le "$jobs" ]; do
    cat "$run/$slot.log"
    slot=$((slot + 1))
done
echo "$component sweep: $count patches, $jobs workers, $failed failed workers; $(($(date +%s)-start))s wall"
if [ "$failed" -ne 0 ]; then
    echo "scratch retained: $run"
    exit 1
fi
rm -rf "$run"
