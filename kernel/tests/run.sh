#!/bin/sh
set -eu
component=$(pwd)
source_root=${HOST_TEST_SOURCE_ROOT:?source root missing}
case $0 in /*) ;; esac
gate=$1
out=${HOST_TEST_OUTPUT:?HOST_TEST_OUTPUT missing}
work=$out/cases/$gate
rm -rf "$work"
mkdir -p "$work/build"
"$out/$gate" --link-tools "$out" "$work/build"
export SOURCE_ROOT=$source_root
export LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=315532800
case ${THOROUGH:-0} in
 1) CONFIG_SEEDS=4000; FAT_SEEDS=200; FAT_EXTERNAL_SEEDS=40; CORRUPT_SEEDS=200;;
 *) CONFIG_SEEDS=64; FAT_SEEDS=12; FAT_EXTERNAL_SEEDS=2; CORRUPT_SEEDS=12;;
esac
export CONFIG_SEEDS FAT_SEEDS FAT_EXTERNAL_SEEDS CORRUPT_SEEDS
for dir in kernel dos factory; do
 if [ "$dir" = "$HOST_TEST_COMPONENT" ]; then
  ln -s "$component" "$work/$dir"
 else
  ln -s "$source_root/$dir" "$work/$dir"
 fi
done
short=$(mktemp -d /tmp/initech-host.XXXXXX)
rmdir "$short"
ln -s "$work" "$short"
trap 'rm -f "$short"' 0 HUP INT TERM
cd "$short"
sh "$component/tests/gates/$gate.sh"
