#!/bin/sh
# Run after building dos: source purity must still see ATTRIB's own messages.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir -p "$work/dos" "$work/build/kernel" "$work/build/dos/host"
for dir in "$root"/dos/*; do
 [ -d "$dir" ] || continue
 case ${dir##*/} in common) cp -R "$dir" "$work/dos/common";; *) ln -s "$dir" "$work/dos/${dir##*/}";; esac
done
ln -s "$root/kernel" "$work/kernel"
cp "$root/build/kernel/dos_messages.h" "$work/build/kernel/dos_messages.h"
cp "$root"/build/dos/*.COM "$work/build/dos/"
cc "$root/kernel/tests/dosmsg_scan.c" -o "$work/build/dosmsg_scan"
export SOURCE_ROOT=$work HOST_TEST_OUTPUT=$work/build/dos/host
cd "$work"
sh "$root/dos/tests/gates/test-dosmsg.sh" > "$work/green.log" 2>&1
patch -s -p1 < "$root/factory/mutants/interfaces/attrib-catalogue.patch"
# Compile the literal into ATTRIB's real core variant, not just an unused source.
gcc -m32 -Os -ffreestanding -fno-pic -DDOSUTIL_ATTRIB -Idos/common -Ikernel/include -Ibuild/kernel \
 -c dos/common/dosutil.c -o "$work/literal-attrib.o"
rc=0
sh "$root/dos/tests/gates/test-dosmsg.sh" > "$work/red.log" 2>&1 || rc=$?
[ "$rc" -eq 1 ] && grep -q 'RED dosmsg-source:.*inline MSG_DOS_0038' "$work/red.log" || { cat "$work/red.log"; echo 'FAIL ATTRIB catalogue: literal bypass accepted or wrong failure'; exit 1; }
cat "$work/green.log" "$work/red.log"
echo 'ok ATTRIB catalogue: own literal message rejected after compiling the ATTRIB variant; LABEL independently scanned'
