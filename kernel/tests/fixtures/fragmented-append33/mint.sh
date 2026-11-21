#!/bin/sh
# Phase 2 mtools fixture and a second case with free clusters on both sides.
set -eu
image=${1:?image path required}
kind=${2:?case required}
case $kind in
    tail) count=2846;;
    middle|blocked) count=2;;
    *) exit 2;;
esac
scratch=$(mktemp -d)
trap 'rm -r "$scratch"' EXIT
truncate -s 1474560 "$image"
mformat -i "$image" -f 1440 ::
dd if=/dev/zero of="$scratch/filler" bs=512 count="$count" status=none
dd if=/dev/zero of="$scratch/target" bs=512 count=1 status=none
mcopy -i "$image" "$scratch/filler" ::FILL.BIN
mcopy -i "$image" "$scratch/target" ::TARGET.BIN
if [ "$kind" = blocked ]; then
    printf 'neighboring file\n' > "$scratch/guard"
    mcopy -i "$image" "$scratch/guard" ::GUARD.BIN
fi
mdel -i "$image" ::FILL.BIN
