#!/bin/sh
# Keep reference/checker fixed; alter only a serialized target record value.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../../.." && pwd)
out=${1:?fresh directory}; product=${2:?intact smoke files}; core=${3:?core fixtures}
mkdir -p "$out/product"
cp "$product"/*.DBF "$product/NAMES.NDX" "$product/STALE.NDX" "$out/product/"
# PARTS has three descriptors: header 32 + 3*32 + 1 = 129; code at 130.
case ${4:-records} in
 records) file=RESULT.DBF;offset=130;rejected=RECORDS-OK;;
 # C21: 97-byte header, 15-byte records; row 2 VALUE's last digit is 126.
 c21) file=REPAIRED.DBF;offset=126;rejected=C21-RECORDS-OK;;
 *) echo 'FAIL: choose records or c21 negative control';exit 2;;
esac
"$root/build/apps/initechbase/host/w4_mutate" --xor "$offset" "$product/$file" "$out/product/$file"
if sh "$root/apps/initechbase/tests/write-safety/reopen.sh" "$out/reference" "$out/product" "$core" > "$out/red.log" 2>&1; then
 echo 'FAIL: real III+ reopen checker accepted wrong serialized record';exit 1
fi
# A missing RECORDS-OK with completed genuine execution is a semantic kill.
grep -F REOPEN-DONE "$out/reference/result.txt"
grep -Fx INDEX-OK "$out/reference/trace.txt"
grep -Fx MEMO-OK "$out/reference/trace.txt"
grep -Fx PRESERVED-OK "$out/reference/trace.txt"
if grep -F REOPEN-ERROR "$out/reference/trace.txt"; then exit 1; fi
case ${4:-records} in
 records)
  grep -Fx C21-RECORDS-OK "$out/reference/trace.txt"
  grep -Fx C21-INDEX-OK "$out/reference/trace.txt";;
 c21)
  grep -Fx RECORDS-OK "$out/reference/trace.txt"
  if grep -Fx C21-INDEX-OK "$out/reference/trace.txt"; then exit 1; fi;;
esac
if grep -Fx "$rejected" "$out/reference/trace.txt"; then exit 1; fi
echo "RED proven: real III+ opens but rejects wrong target record value ($file)"
