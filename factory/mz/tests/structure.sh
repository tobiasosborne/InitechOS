#!/bin/sh
# Hand-authored tiny module: NOP, RET, pad, absolute pointer to byte zero.
set -eu
link=$1; check=$2; dir=$3
mkdir -p "$dir"
printf '\220\303\000\000\000\000\000\000' > "$dir/base"
printf '\220\303\000\000\021\001\001\001' > "$dir/probe"
"$link" "$dir/base" "$dir/good" --reloc-diff "$dir/probe" 0x01010111 --header-relocs --minalloc 2
"$check" "$dir/good" "$dir/base" 32
# Independently corrupt entry, relocation site, pointer, BSS, signature.
for field in entry site pointer bss tag; do
 cp "$dir/good" "$dir/bad"
 case $field in
 entry) printf '\001' | dd of="$dir/bad" bs=1 seek=20 conv=notrunc 2>/dev/null ;;
 site) printf '\377\377\377\177' | dd of="$dir/bad" bs=1 seek=32 conv=notrunc 2>/dev/null ;;
 pointer) printf '\377\377\377\177' | dd of="$dir/bad" bs=1 seek=52 conv=notrunc 2>/dev/null ;;
 bss) printf '\000' | dd of="$dir/bad" bs=1 seek=10 conv=notrunc 2>/dev/null ;;
 tag) printf '\000' | dd of="$dir/bad" bs=1 seek=28 conv=notrunc 2>/dev/null ;;
 esac
 if "$check" "$dir/bad" "$dir/base" 32 > "$dir/$field.log" 2>&1; then echo "FAIL accepted $field corruption"; exit 1; fi
done
echo 'GREEN MZ structure: independent reader rejected five corruptions'
