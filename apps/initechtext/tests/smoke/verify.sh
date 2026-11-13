#!/bin/sh
set -eu
disk=$1 out=$2
mcopy -o -i "$disk" ::UNTITLED "$out.txt"
printf 'alpha\rbravo' > "$out.expected"
cmp "$out.expected" "$out.txt"
fsck.fat -n "$disk" > "$out.fsck"
