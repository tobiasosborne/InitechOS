#!/bin/sh
# The image must carry the system its sources describe (initech-ktd5j): a
# parallel build once rebuilt build/flair/desktop/libdesktop.a and then
# skipped the system link, so INITDOS.SYS booted old Finder code. This gate
# reads system.elf's prerequisites from make's own database and fails if any
# file prerequisite is newer than the linked system. Usage: system-fresh.sh
set -eu
cd "$(dirname "$0")/../.."
elf=$PWD/build/system/system.elf
[ -f "$elf" ] || { echo "FAIL system-fresh: $elf missing"; exit 1; }
deps=$(make -pq --no-print-directory -C system 2>/dev/null |
       awk -v t="$elf" 'index($0, t ":") == 1 { sub(/^[^:]*:/, ""); sub(/\|.*/, ""); print; exit }')
[ -n "$deps" ] || { echo "FAIL system-fresh: no prerequisites for $elf in the system make database"; exit 1; }
stale=0 n=0
for dep in $deps; do
    [ "$dep" = FORCE ] && continue
    n=$((n+1))
    [ -e "$dep" ] || { echo "FAIL system-fresh: prerequisite $dep missing"; stale=1; continue; }
    if [ "$dep" -nt "$elf" ]; then echo "FAIL system-fresh: $dep is newer than build/system/system.elf"; stale=1; fi
done
[ "$stale" -eq 0 ] || exit 1
[ -f build/system/INITDOS.SYS ] && ! [ build/system/system.elf -nt build/system/INITDOS.SYS ] ||
    { echo "FAIL system-fresh: INITDOS.SYS older than system.elf"; exit 1; }
echo "ok system-fresh: system.elf newer than its $n prerequisites"
