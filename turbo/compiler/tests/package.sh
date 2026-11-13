#!/bin/sh
# Independent assembly/link/mzlink route. Relocations come from two link bases,
# not compiler metadata. Reserve includes the gap preceding NOLOAD BSS.
set -eu
source=$1 dest=$2
root=${TOP:-$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)}
mkdir -p "$dest"
dest=$(CDPATH= cd -- "$dest" && pwd)
cp "$source" "$dest/program.s"
(cd "$dest" && nasm -f elf32 program.s -o program.o)
nasm -f elf32 "$root/turbo/seed/rt/start_extended.asm" -o "$dest/start.o"
for variant in zero probe; do
    if [ "$variant" = zero ]; then base=0; else base=0x01010180; fi
    ld -m elf_i386 --build-id=none --defsym TURBO_LINK_BASE=$base \
        -T "$root/turbo/seed/rt/seed_extended.ld" -o "$dest/$variant.elf" \
        "$dest/start.o" "$root/build/turbo/seed/rt/fileio.o" "$dest/program.o"
    objcopy -O binary "$dest/$variant.elf" "$dest/$variant.bin"
done
end=$(nm "$dest/zero.elf" | awk '$3=="__bss_end" {print $1}')
module=$(wc -c < "$dest/zero.bin")
reserve=$((0x$end-module))
"$root/build/factory/mz/mzlink" "$dest/zero.bin" "$dest/program.exe" \
    --extended --reserve "$reserve" --minimum-heap 0 --minalloc 0 \
    --reloc-diff "$dest/probe.bin" 0x01010180
"$root/build/factory/mz/mzcheck" "$dest/program.exe" "$dest/zero.bin" "$reserve"
printf 'extended module=%d reserve=%d resident=%d headroom=%d stack=65536 guard=4096\n' \
    "$module" "$reserve" "$((0x$end))" "$((0x1ef000-((0x$end+15)/16*16)-16))"
