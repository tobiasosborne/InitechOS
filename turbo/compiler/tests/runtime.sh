#!/bin/sh
set -eu
root=$1 out=$2
mkdir -p "$out"
cc -std=c11 -Wall -Wextra -Werror "$root/turbo/compiler/tests/runtime-table.c" -o "$out/table"
: > "$out/generated"
for mode in dos extended; do
 nasm -f elf32 "$root/turbo/seed/rt/start_$mode.asm" -o "$out/start.o"
 nasm -f elf32 "$root/turbo/seed/rt/fileio.asm" -o "$out/fileio.o"
 ld -m elf_i386 --build-id=none -T "$root/turbo/compiler/tests/runtime.ld" -o "$out/runtime.elf" "$out/start.o" "$out/fileio.o"
 objcopy -O binary --only-section=.text "$out/runtime.elf" "$out/runtime.bin"
 name=NativeRuntime
 if [ "$mode" = extended ]; then name=NativeExtendedRuntime; fi
 "$out/table" "$out/runtime.bin" "$name" >> "$out/generated"
done
sed -n '/^{ BEGIN NASM RUNTIME TABLE/,/^{ END NASM RUNTIME TABLE }/p' "$root/turbo/compiler/turbo.pas" > "$out/checked-in"
cmp "$out/generated" "$out/checked-in"
echo 'runtime tables: NASM regeneration byte-identical (flat and extended)'
# The compiler's fixed runtime entry offsets must be the linker's.
nm -t d "$out/runtime.elf" | awk '$2 == "T" { print $3, $1 + 0 }' | LC_ALL=C sort > "$out/symbols"
sed -n "s/^ *\(else \)\{0,1\}if Text = '\(\(rtl\|serial\)_[a-z_]*\)' then N := \([0-9]*\)\$/\2 \4/p" "$root/turbo/compiler/turbo.pas" | LC_ALL=C sort > "$out/claimed"
test -s "$out/claimed"
LC_ALL=C join "$out/claimed" "$out/symbols" | awk '$2 != $3 { print "FAIL runtime offset " $1 ": compiler " $2 ", linker " $3; bad = 1 } END { exit bad }'
test "$(LC_ALL=C join "$out/claimed" "$out/symbols" | wc -l)" -eq "$(wc -l < "$out/claimed")"
echo 'runtime entry offsets: compiler table equals the linked runtime'
