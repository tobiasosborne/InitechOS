#!/bin/sh
set -eu
check=$1 fixture=$2 dir=$3
mkdir -p "$dir"
for kind in good maximum-good; do
 "$fixture" "$kind" "$dir/$kind"
 "$check" "$dir/$kind"
done
while read -r kind reason; do
 "$fixture" "$kind" "$dir/$kind"
 if "$check" "$dir/$kind" > "$dir/$kind.log" 2>&1; then
  echo "FAIL accepted $kind"; exit 1
 fi
 grep -Fx "mzcheck: $reason" "$dir/$kind.log"
done <<'CONTROLS'
version extended version
length extended descriptor length
stack extended stack
guard extended guard
descriptor extended descriptor extent
table-low extended relocation table extent
table-high extended relocation table extent
site relocation site outside module
pointer relocated pointer outside image+BSS
duplicate overlapping relocation sites
entry extended entry outside module
reserve extended reserve/window
header-room extended heap header
heap extended minimum heap
heap-round extended minimum heap
minalloc extended minalloc
pages page encoding
truncated module extent
maximum extended module maximum
CONTROLS
echo 'GREEN extended MZ: 2 hand-encoded valid controls, 19 specific rejections'
