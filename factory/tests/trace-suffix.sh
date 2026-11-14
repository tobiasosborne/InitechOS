#!/bin/sh
# Bisect reproducer: a caller-authored impossible mandatory assertion must fail
# with or without 4100 separators. Uses only a private working-disk clone.
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/factory/trace-suffix
out=$(mktemp -d "$PWD/build/factory/trace-suffix/run.XXXXXX")
driver=${TRACE_DRIVER:-build/factory/qemu_harness}
failed=0
for kind in short padded; do
 mkdir -p "$out/$kind"
 cp build/image/data.img "$out/$kind/data.img"
 spec=w1,WUNREACHABLE-SUFFIX-ASSERTION
 if [ "$kind" = padded ]; then
  padding=$(printf '%04100d' 0 | tr 0 ,)
  spec="w1,$padding,WUNREACHABLE-SUFFIX-ASSERTION"
 fi
 rc=0
 INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 "$driver" \
  --disk build/image/boot.img --disk2 "$out/$kind/data.img" \
  --keys-after FLAIR-LIVE-READY --expect FLAIR-LIVE-READY \
  --quit-after FLAIR-LIVE-READY --mouse "$spec" --timeout-ms 60000 \
  --out "$out/$kind" --name run > "$out/$kind/stdout" 2> "$out/$kind/report" || rc=$?
 if [ "$rc" -eq 1 ] && grep -F 'W marker '\''UNREACHABLE-SUFFIX-ASSERTION' "$out/$kind/report" >/dev/null &&
    grep -Fx '[harness] OK=0' "$out/$kind/report" >/dev/null; then
  echo "ok QEMU required suffix: $kind rejected"
 else echo "FAIL QEMU required suffix: $kind exit=$rc";failed=1;fi
done
echo "QEMU required suffix evidence: $out"
test "$failed" -eq 0
