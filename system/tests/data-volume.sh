#!/bin/sh
# Operator delivery ruling: frame data on ATA A:, DESKTOP.DB hidden like a
# Macintosh volume database. mtools is the independent FAT reader.
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
installer=${1:-$root/system/install-data.sh}
manifest=${2:-$root/system/data-volume.list}
checks=0
check() {
 message=$1; shift
 checks=$((checks+1))
 if ! "$@"; then
  echo "FAIL data volume: $message" >&2
  echo "INITECH_CHECKS $checks failures=1"
  exit 1
 fi
}
check 'data-volume installer exists' test -f "$installer"
w=$(mktemp -d)
trap 'rm -rf "$w"' EXIT HUP INT TERM
truncate -s 1474560 "$w/data.img"
mformat -i "$w/data.img" -f 1440 -N 0x494e4941 -v DATA ::
(cd "$root" && sh "$installer" "$w/data.img" "$manifest")
# A failed directory read is an infrastructure error, never absence evidence.
mdir -b -i "$w/data.img" ::/ > "$w/visible"
mdir -a -b -i "$w/data.img" ::/ > "$w/all"
printf '%s\n' DESKTOP.DB LEDGER.TXT MICHAEL.CFG | LC_ALL=C sort > "$w/expected"
sed 's|^::/||' "$w/all" | LC_ALL=C sort > "$w/actual"
check 'data image contains exactly the three frame inputs' cmp "$w/expected" "$w/actual"
check 'normal listing omits DESKTOP.DB' test "$(awk '/DESKTOP[.]DB$/ {n++} END {print n+0}' "$w/visible")" = 0
check 'hidden listing includes exactly one DESKTOP.DB' test "$(awk '/DESKTOP[.]DB$/ {n++} END {print n+0}' "$w/all")" = 1
mattrib -i "$w/data.img" ::DESKTOP.DB > "$w/attributes"
check 'DESKTOP.DB has the hidden attribute' awk '{for(i=1;i<NF;i++)if($i=="H")found=1} END {exit !found}' "$w/attributes"
for name in DESKTOP.DB LEDGER.TXT MICHAEL.CFG; do
 check "$name is installed" mcopy -i "$w/data.img" "::$name" "$w/$name"
 case $name in
  DESKTOP.DB) source=$root/flair/desktop/data/DESKTOP.DB;;
  *) source=$root/apps/initechcalc/data/$name;;
 esac
 check "$name matches its owner source bytes" cmp "$source" "$w/$name"
done
echo "INITECH_CHECKS $checks failures=0"
