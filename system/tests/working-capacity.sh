#!/bin/sh
# GM8: a roomy installed C: passes even with a small A: attached.
# mtools creates the independent FAT12 fixtures and measures free space.
set -eu
cd "$(dirname "$0")/../.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
export MTOOLS_SKIP_CHECK=1 SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C
export SYSTEM_BOOT_IMAGE="$work/boot.img" SYSTEM_DATA_IMAGE="$work/data.img" SYSTEM_WORK_IMAGE=
truncate -s $((20160*512)) "$SYSTEM_BOOT_IMAGE"
c=$SYSTEM_BOOT_IMAGE@@32256
mformat -i "$c" -T 16384 -h 16 -s 63 -c 8 -H 63 -N 0x43504133 ::
printf 'installed command marker\n' > "$work/command"
mcopy -i "$c" "$work/command" ::COMMAND.COM
truncate -s 1474560 "$SYSTEM_DATA_IMAGE"
mformat -i "$SYSTEM_DATA_IMAGE" -f 1440 -N 0x41444133 ::

rc=0
sh system/tests/capacity.sh > "$work/result" 2>&1 || rc=$?
if [ "$rc" != 0 ]; then
 cat "$work/result"
 if [ "$rc" != 1 ] || ! grep -q '^FAIL installed capacity:' "$work/result"; then
  echo 'ERROR working capacity: incomplete capacity grader' >&2; exit 2
 fi
 echo 'FAIL working capacity: installed C capacity must pass despite small A' >&2; exit 1
fi

# The boot-only layout still falls back to legacy A:, which is too small.
mdel -i "$c" ::COMMAND.COM
rc=0
sh system/tests/capacity.sh > "$work/result" 2>&1 || rc=$?
if [ "$rc" = 0 ]; then
 echo 'FAIL working capacity: legacy small A must refuse capacity' >&2; exit 1
fi
if [ "$rc" != 1 ] || ! grep -q '^FAIL installed capacity:' "$work/result"; then
 cat "$work/result"
 echo 'ERROR working capacity: incomplete refusal grader' >&2; exit 2
fi
echo 'INITECH_CHECKS 2 failures=0'
