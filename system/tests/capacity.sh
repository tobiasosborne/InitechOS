#!/bin/sh
# Independent mtools FAT accounting. Ordinary product volume, no fixture mint.
set -eu
cd "$(dirname "$0")/../.."
volume=${SYSTEM_WORK_IMAGE:-build/image/data.img}
listing=$(mdir -i "$volume" ::)
printf '%s\n' "$listing"
free=$(printf '%s\n' "$listing" | awk '/bytes free/ {gsub(/ /,"",$0); sub(/bytesfree.*/,"",$0); print $0}')
# Current self source emits a 954720-byte .S and 164263-byte .EXE. Source
# replacement and a retained second .EXE bring the need above 1.46 MB.
# Reserve 2 MiB including FAT cluster rounding, TPS report and save stages.
[ "${free:-0}" -ge 2097152 ] || {
 echo "FAIL installed capacity: $free bytes free; need 2097152 for self-compilation and save stages" >&2
 exit 1
}
echo "system: installed capacity green ($free bytes free; 2097152 reserved)"
