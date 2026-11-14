#!/bin/sh
# CLAUDE.md Two interfaces: LABEL may use INT25/26; ATTRIB may not.
# Compile the real adapter twice, then inspect independent machine instructions.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
cc=${1:-gcc}
[ "$#" -eq 0 ] || shift
for utility in attrib label; do
 define=
 [ "$utility" != attrib ] || define=-DDOSUTIL_ATTRIB
 "$cc" "$@" -m32 -Os -ffreestanding -fno-pic -I"$root/dos/common" -I"$root/kernel/include" \
  $define -c "$root/dos/common/dosutil_target.c" -o "$work/$utility.o"
 objdump -d "$work/$utility.o" > "$work/$utility.asm"
 nm --defined-only "$work/$utility.o" > "$work/$utility.symbols"
done
if grep -Eq 'int[[:space:]]+\$0x(25|26)' "$work/attrib.asm" || grep -Eq '[[:space:]]du_sector$' "$work/attrib.symbols"; then
 echo 'FAIL utility vectors: ATTRIB links LABEL absolute-sector helper'; exit 1
fi
for vector in 25 26; do
 grep -Eq 'int[[:space:]]+\$0x'"$vector" "$work/label.asm" || { echo "FAIL utility vectors: LABEL lost INT ${vector}h"; exit 1; }
done
echo 'ok utility vectors: ATTRIB has no absolute-sector helper; LABEL retains INT25/26'
