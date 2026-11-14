#!/bin/sh
# Actual disk bytes, independent authored-format reader and RI subtraction.
set -eu
disk=${1:?booted disk}; out=${2:?output prefix}
top=$(CDPATH= cd -- "$(dirname "$0")/../../../.." && pwd)
menu=$top/apps/initech123/tests/oracles/menu
mcopy -o -i "$disk" ::APPS/123.MNU "$out.MNU"
"$top/build/apps/initech123/host/test_i123_menu_tree" --inspect "$out.MNU" > "$out.menu.actual"
awk -v ids=1 -f "$menu/project.awk" "$menu/omissions.tsv" "$menu/descriptions.tsv" "$menu/menu_tree.golden" > "$out.menu.expected"
diff -u "$out.menu.expected" "$out.menu.actual"
printf 'ok installed menu: %s\n' "$disk"
