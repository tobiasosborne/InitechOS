#!/bin/sh
set -eu
root=$1 out=$2 component=$PWD
. "$root/turbo/tests/checks.sh"
setup mkdir -p "$out"
setup cc -std=c11 -Wall -Wextra -Werror "$component/tests/runtime-table.c" -o "$out/table"
: > "$out/generated"
for mode in dos extended; do
 setup nasm -f elf32 "$root/turbo/seed/rt/start_$mode.asm" -o "$out/start.o"
 setup nasm -f elf32 "$root/turbo/seed/rt/fileio.asm" -o "$out/fileio.o"
 setup ld -m elf_i386 --build-id=none -T "$component/tests/runtime.ld" -o "$out/runtime.elf" "$out/start.o" "$out/fileio.o"
 setup objcopy -O binary --only-section=.text "$out/runtime.elf" "$out/runtime.bin"
 name=NativeRuntime
 if [ "$mode" = extended ]; then name=NativeExtendedRuntime; fi
 setup "$out/table" "$out/runtime.bin" "$name" >> "$out/generated"
done
setup sed -n '/^{ BEGIN NASM RUNTIME TABLE/,/^{ END NASM RUNTIME TABLE }/p' "$component/turbo.pas" > "$out/checked-in"
assert 'runtime:NASM regeneration byte-identical' cmp "$out/generated" "$out/checked-in"
setup nm -t d "$out/runtime.elf" > "$out/nm"
setup awk '$2 == "T" { print $3, $1 + 0 }' "$out/nm" > "$out/unsorted"
setup env LC_ALL=C sort "$out/unsorted" > "$out/symbols"
setup sed -n "s/^ *\(else \)\{0,1\}if Text = '\(\(rtl\|serial\)_[a-z_]*\)' then N := \([0-9]*\)\$/\2 \4/p" "$component/turbo.pas" > "$out/claimed-unsorted"
setup env LC_ALL=C sort "$out/claimed-unsorted" > "$out/claimed"
assert 'runtime:positive offset inventory' test -s "$out/claimed"
setup env LC_ALL=C join "$out/claimed" "$out/symbols" > "$out/joined"
assert 'runtime:compiler offsets equal independent linker' awk '$2 != $3 {print "offset mismatch: " $1; bad=1} END {exit bad}' "$out/joined"
assert 'runtime:every claimed offset independently found' test "$(wc -l < "$out/joined")" -eq "$(wc -l < "$out/claimed")"
finish
