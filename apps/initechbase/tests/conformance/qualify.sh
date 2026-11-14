#!/bin/sh
# Comparator controls; alters only disposable copies of host disk readback.
set -eu
src=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
repo=$(CDPATH= cd -- "$src/../../../.." && pwd)
out=${1:-"$repo/build/apps/initechbase/conformance"}
q=$out/qualification
rm -rf "$q";mkdir -p "$q/host-results/C01"
cp "$out/host-results/C01"/* "$q/host-results/C01/"
CASES=C01 sh "$src/check.sh" "$q" > "$q/baseline.log"
printf '\001\002\003' | dd of="$q/host-results/C01/SAVED.DBF" bs=1 seek=1 conv=notrunc 2>/dev/null
printf '\377' | dd of="$q/host-results/C01/SAVED.DBF" bs=1 seek=12 conv=notrunc 2>/dev/null
printf '\377' | dd of="$q/host-results/C01/SAVED.DBF" bs=1 seek=44 conv=notrunc 2>/dev/null
CASES=C01 sh "$src/check.sh" "$q" > "$q/normalized.log"
echo 'GREEN date/reserved/RAM-address normalization'
printf Z | dd of="$q/host-results/C01/SAVED.DBF" bs=1 seek=98 conv=notrunc 2>/dev/null
if CASES=C01 sh "$src/check.sh" "$q" > "$q/corrupted-record.log" 2>&1; then
 echo 'FAIL: changed saved field passed';exit 1
fi
grep -q '^RED C01 meaningful SAVED.DBF bytes' "$q/corrupted-record.log"
echo 'RED changed saved field rejected'
# An uncompleted real run must stay pending, even if its files are hash-pinned.
mkdir -p "$q/reference/C01"
cp "$src/reference/C01"/* "$q/reference/C01/"
printf 'incomplete capture\r\n' > "$q/reference/C01/RESULT.OUT"
(cd "$q/reference" && find . -type f ! -name SHA256SUMS -print | LC_ALL=C sort | xargs sha256sum > SHA256SUMS)
if CASES=C01 sh "$src/check.sh" "$out" "$q/reference" > "$q/pending.log" 2>&1; then
 echo 'FAIL: incomplete reference passed';exit 1
fi
grep -q '^PENDING C01' "$q/pending.log"
echo 'RED incomplete real capture rejected'
