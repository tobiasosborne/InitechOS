#!/bin/sh
# Independent installed layout contract: retain the original application
# directory while adding separate sample/compiler folders. Every original
# program remains visible in the default Finder window without scrolling.
set -eu
cd "$(dirname "$0")/.."
actual=$(mktemp)
trap 'rm -f "$actual"' EXIT HUP INT TERM
awk '$1 !~ /^#/ && $2 ~ /^APPS\// {sub(/^APPS\//,"",$2); print $2}' working-volume.list | LC_ALL=C sort > "$actual"
diff -u tests/applist.expected "$actual"
echo 'system: original APPS directory retained; examples and compiler have their own folders'
