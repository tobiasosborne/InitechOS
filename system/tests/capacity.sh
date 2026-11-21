#!/bin/sh
# Independent mtools FAT accounting. Ordinary product volume, no fixture mint.
set -eu
cd "$(dirname "$0")/../.."
export LC_ALL=C
. "${SYSTEM_VOLUME_HELPER:-system/tests/work-volume.sh}"
listing=$(mdir -i "$volume" ::) || {
 echo "ERROR installed capacity: cannot read $volume" >&2; exit 2;
}
printf '%s\n' "$listing"
free=$(printf '%s\n' "$listing" | awk '/^[[:space:][:digit:]]+ bytes free[[:space:]]*$/ {
 sub(/bytes free.*/, ""); gsub(/[[:space:]]/, ""); print
}')
case "$free" in
 ""|*[!0-9]*) echo "ERROR installed capacity: missing or invalid free byte count" >&2; exit 2;;
esac
# Current self source emits a 954720-byte .S and 164263-byte .EXE. Source
# replacement and a retained second .EXE bring the need above 1.46 MB.
# Reserve 2 MiB including FAT cluster rounding, TPS report and save stages.
[ "$free" -ge 2097152 ] || {
 echo "FAIL installed capacity: $free bytes free; need 2097152 for self-compilation and save stages" >&2
 exit 1
}
echo "system: installed capacity green ($free bytes free; 2097152 reserved)"
