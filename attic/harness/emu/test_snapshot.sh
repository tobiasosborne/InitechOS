#!/bin/sh
# Rule 6: a real guest writes sector 1; independent cmp must see RW only.
set -eu
harness=$1; sector=$2; debug_sector=$3
mkdir -p build
w=$(mktemp -d build/snapshot-test.XXXXXX)
trap 'rm -rf "$w"' EXIT
export INITECH_QMP_STDIO=1
# A writable boot disk is ALWAYS a private copy, including this negative proof.
cp -f "$sector" "$w/pristine.img"
truncate -s 1474560 "$w/pristine.img"
for leg in overlay rw; do
 cp -f "$w/pristine.img" "$w/$leg.img"
 set --
 [ "$leg" != rw ] || set -- --boot-rw
 "$harness" "$@" --disk "$w/$leg.img" --out "$w" --name "$leg" --screendump \
   --screendump-after BOOT-WRITE-DONE --expect BOOT-WRITE-DONE --timeout-ms 5000 > "$w/$leg.out" 2> "$w/$leg.report"
 grep -qx '\[harness\] OK=1' "$w/$leg.report"
done
cmp "$w/pristine.img" "$w/overlay.img"
if cmp -s "$w/pristine.img" "$w/rw.img"; then echo 'RED RW disk write hidden'; exit 1; fi
cp -f "$debug_sector" "$w/debug.img"; truncate -s 1474560 "$w/debug.img"
"$harness" --disk "$w/debug.img" --out "$w" --name debug --expect BOOT-WRITE-DONE --timeout-ms 5000 > "$w/debug.out" 2> "$w/debug.report"
grep -qx '\[harness\] launched=1 timed_out=0 exit=1 signal=0' "$w/debug.report"
grep -qx '\[harness\] OK=1' "$w/debug.report"
# The harness must remove its per-run overlay, including on a creation failure.
[ -z "$(find "$w/.qemu-tmp" -mindepth 1 -print)" ] || { echo 'RED leaked overlay'; exit 1; }
rc=0; "$harness" --disk "$w/absent.img" --out "$w" --name absent > "$w/absent.out" 2> "$w/absent.report" || rc=$?
[ "$rc" = 2 ] || { echo 'RED missing image accepted'; exit 1; }
[ -z "$(find "$w/.qemu-tmp" -mindepth 1 -print)" ] || { echo 'RED failed overlay leaked'; exit 1; }
echo 'GREEN test-qemu-snapshot: guest writes discarded only in private overlay; RW persists; writable per-run TMPDIR, cleanup and missing-image failure'
