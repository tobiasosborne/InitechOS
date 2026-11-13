#!/bin/sh
set -eu
disk=$1 out=$2
mcopy -o -i "$disk" ::CPANELS.DAT "$out.dat"
printf '\103\120\060\061\001\003\001\001\000\002\001\000\000\000\375\000' > "$out.expected"
cmp "$out.expected" "$out.dat"
fsck.fat -n "$disk" > "$out.fsck"
