#!/bin/sh
# GM8: C is the installed system; legacy A is temporary until C activation.
# The C fixture has >2 MiB free and A has <2 MiB, independently made by mtools.
set -eu
cd "$(dirname "$0")/../.."
work=$(mktemp -d "$PWD/build/system/working-capacity.XXXXXX")
trap 'rm -rf "$work"' 0 HUP INT TERM
export MTOOLS_SKIP_CHECK=1 SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C
truncate -s $((20160*512)) "$work/boot.img"
mformat -i "$work/boot.img@@32256" -T 16384 -h 16 -s 63 -c 8 -H 63 -N 0x43504133 ::
printf 'installed command marker\n' > "$work/command"
mcopy -i "$work/boot.img@@32256" "$work/command" ::COMMAND.COM
truncate -s 1474560 "$work/data.img"
mformat -i "$work/data.img" -f 1440 -N 0x41444133 ::
rc=0
SYSTEM_BOOT_IMAGE="$work/boot.img" SYSTEM_DATA_IMAGE="$work/data.img" sh system/tests/capacity.sh > "$work/result" 2>&1 || rc=$?
if [ "$rc" != 0 ]; then
 cat "$work/result"
 if [ "$rc" != 1 ] || ! grep -q '^FAIL installed capacity:' "$work/result"; then
  echo 'ERROR working capacity: incomplete capacity grader' >&2; exit 2
 fi
 echo 'FAIL working capacity: installed C capacity must pass despite small A' >&2; exit 1
fi
mdel -i "$work/boot.img@@32256" ::COMMAND.COM
rc=0
SYSTEM_BOOT_IMAGE="$work/boot.img" SYSTEM_DATA_IMAGE="$work/data.img" sh system/tests/capacity.sh > "$work/result" 2>&1 || rc=$?
if [ "$rc" = 0 ]; then echo 'FAIL working capacity: legacy small A must refuse capacity' >&2; exit 1; fi
if [ "$rc" != 1 ] || ! grep -q '^FAIL installed capacity:' "$work/result"; then
 cat "$work/result"; echo 'ERROR working capacity: incomplete refusal grader' >&2; exit 2
fi
echo 'INITECH_CHECKS 2 failures=0'
