#!/bin/sh
# Factory input installer, run from the source root on a fresh FAT data image.
# The factory owns volume creation and final deterministic FAT timestamps.
set -eu
image=${1:?formatted data-volume image required}
manifest=${2:-system/data-volume.list}
awk '
 /^#/ || NF==0 {next}
 NF!=3 || $1 !~ /^build\// || $2 ~ /(^\/|\.\.|[\\\/])/ || $3 !~ /^[AH]$/ {bad=1}
 seen[toupper($2)]++ {bad=1}
 END {exit bad}' "$manifest" || { echo 'Invalid data-volume manifest' >&2; exit 2; }
while read -r source destination attributes; do
 case $source in ''|\#*) continue;; esac
 test -f "$source"
 mcopy -o -i "$image" "$source" "::$destination"
 mattrib -i "$image" +a -h -s -r "::$destination"
 case $attributes in H) mattrib -i "$image" +h "::$destination";; esac
done < "$manifest"
