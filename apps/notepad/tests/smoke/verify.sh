#!/bin/sh
set -eu
disk=$1 out=$2
make -s -C apps/notepad "$PWD/build/apps/notepad/host/readback"
for pair in 'NOTEPAD NPAD' 'SCRAPBK SBK1'; do
 set -- $pair
 mcopy -o -i "$disk" "::$1.DAT" "$out-$1.DAT"
 build/apps/notepad/host/readback "$out-$1.DAT" "$2" alpha
done
fsck.fat -n "$disk" > "$out.fsck"
