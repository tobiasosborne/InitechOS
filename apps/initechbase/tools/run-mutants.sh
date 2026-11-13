#!/bin/sh
# Group by exact selection: each group uses the factory's strict runner and
# shares its single baseline. Bound host-process parallelism, no emulators.
set -eu
top=$1 out=$2 make_cmd=$3 jobs=$4
shift 4
[ "$#" -gt 0 ] || { echo 'FAIL: no mutation patches selected'; exit 1; }
mkdir -p "$out/mutants"
run=$(mktemp -d "$out/mutants/groups.XXXXXX")
start=$(date +%s)
for patch do
 selection=$(sed -n 's/^# test: //p' "$patch")
 key=$(printf '%s' "$selection" | sha256sum | cut -d ' ' -f1)
 printf '%s\n' "$patch" >> "$run/$key.list"
done
# xargs only orchestrates independent local graders; all patch/build/verdict
# decisions stay in factory/testlib/run-mutants.sh.
export IBASE_MUT_TOP="$top" IBASE_MUT_OUT="$run" IBASE_MUT_MAKE="$make_cmd"
find "$run" -name '*.list' | sort | xargs -P "$jobs" -n 1 sh -c '
 list=$1
 key=$(basename "$list" .list)
 set -- $(cat "$list")
 rc=0
 sh "$IBASE_MUT_TOP/factory/testlib/run-mutants.sh" "$IBASE_MUT_TOP" \
  apps/initechbase "$IBASE_MUT_OUT/$key" "$IBASE_MUT_MAKE" "$@" > "$IBASE_MUT_OUT/$key.log" 2>&1 || rc=1
 echo "$rc" > "$IBASE_MUT_OUT/$key.result"
' sh
bad=0
for result in "$run"/*.result; do
 cat "${result%.result}.log"
 bad=$((bad+$(cat "$result")))
done
echo "InitechBase mutations: $# patches, $bad failed groups; $(($(date +%s)-start))s wall ($jobs workers)"
if [ "$bad" -ne 0 ]; then echo "scratch retained: $run"; exit 1; fi
rm -rf "$run"
