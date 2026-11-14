#!/bin/sh
# Keep reference/checker fixed; alter only a serialized target record value.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../../.." && pwd)
out=${1:?fresh directory}; product=${2:?intact smoke files}; core=${3:?core fixtures}
mkdir -p "$out/product"
cp "$product"/*.DBF "$product/NAMES.NDX" "$out/product/"
# PARTS has three descriptors: header 32 + 3*32 + 1 = 129; code at 130.
"$root/build/apps/initechbase/host/w4_mutate" --xor 130 "$product/RESULT.DBF" "$out/product/RESULT.DBF"
if sh "$root/apps/initechbase/tests/write-safety/reopen.sh" "$out/reference" "$out/product" "$core" > "$out/red.log" 2>&1; then
 echo 'FAIL: real III+ reopen checker accepted wrong serialized record';exit 1
fi
# A missing RECORDS-OK with completed genuine execution is a semantic kill.
grep -F REOPEN-DONE "$out/reference/result.txt"
grep -Fx INDEX-OK "$out/reference/trace.txt"
grep -Fx MEMO-OK "$out/reference/trace.txt"
if grep -Fx RECORDS-OK "$out/reference/trace.txt"; then exit 1; fi
echo 'RED proven: real III+ opens but rejects wrong target record value'
