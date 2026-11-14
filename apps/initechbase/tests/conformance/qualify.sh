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
grep -q '^FAIL: C01 meaningful SAVED.DBF bytes' "$q/corrupted-record.log"
echo 'RED changed saved field rejected'
# Tail controls: garbage after the last record, a missing EOF byte and a
# wrong header length must each compare unequal (initech-16nnj). Each starts
# from the pristine host readback.
cp "$out/host-results/C01/SAVED.DBF" "$q/host-results/C01/SAVED.DBF"
printf 'GARBAGE-AFTER-LAST-RECORD' >> "$q/host-results/C01/SAVED.DBF"
if CASES=C01 sh "$src/check.sh" "$q" > "$q/garbage-tail.log" 2>&1; then
 echo 'FAIL: garbage-tail saved file passed';exit 1
fi
grep -q '^FAIL: C01 meaningful SAVED.DBF bytes' "$q/garbage-tail.log"
echo 'RED garbage after last record rejected'
cp "$out/host-results/C01/SAVED.DBF" "$q/host-results/C01/SAVED.DBF"
# C01 geometry: 97-byte header + 3 records of 15 bytes; byte 143 is the EOF.
truncate -s 142 "$q/host-results/C01/SAVED.DBF"
if CASES=C01 sh "$src/check.sh" "$q" > "$q/missing-eof.log" 2>&1; then
 echo 'FAIL: missing-EOF saved file passed';exit 1
fi
grep -q '^FAIL: C01 meaningful SAVED.DBF bytes' "$q/missing-eof.log"
echo 'RED missing EOF byte rejected'
cp "$out/host-results/C01/SAVED.DBF" "$q/host-results/C01/SAVED.DBF"
{ head -c 97 "$q/host-results/C01/SAVED.DBF";printf '\000';
  tail -c +98 "$q/host-results/C01/SAVED.DBF"; } > "$q/padded.dbf"
mv "$q/padded.dbf" "$q/host-results/C01/SAVED.DBF"
printf '\142\000' | dd of="$q/host-results/C01/SAVED.DBF" bs=1 seek=8 conv=notrunc 2>/dev/null
if CASES=C01 sh "$src/check.sh" "$q" > "$q/header-length.log" 2>&1; then
 echo 'FAIL: wrong-header-length saved file passed';exit 1
fi
grep -q '^FAIL: C01 meaningful SAVED.DBF bytes' "$q/header-length.log"
echo 'RED wrong header length rejected'
# An uncompleted real run must stay pending, even if its files are hash-pinned.
mkdir -p "$q/reference/C01"
cp "$src/reference/C01"/* "$q/reference/C01/"
printf 'incomplete capture\r\n' > "$q/reference/C01/RESULT.OUT"
(cd "$q/reference" && find . -type f ! -name SHA256SUMS -print | LC_ALL=C sort | xargs sha256sum > SHA256SUMS)
if CASES=C01 sh "$src/check.sh" "$out" "$q/reference" > "$q/pending.log" 2>&1; then
 echo 'FAIL: incomplete reference passed';exit 1
fi
grep -q '^ERROR C01: completed real III+ capture required' "$q/pending.log"
echo 'RED incomplete real capture rejected'
