#!/bin/sh
# ovl_ptr_check.sh -- ADR-0014 DEC-14-6.2 / DEC-14-9(c): "every code pointer
# kept in resident data is a stub address, never an overlay address", checked
# on both sides of the link (harness/proptest/overlay.mk).
#
#   ovl_ptr_check.sh resident <kernel ELF linked with --emit-relocs> <workdir>
#     Every RELOCATED dword in the resident .rodata/.data (a real pointer, not
#     a coincidental table value: the font offsets and geometry words that
#     merely fall in [0x5C0000, 0x5E0000) carry no relocation) must point
#     outside the overlay window.
#   ovl_ptr_check.sh overlay <section object>...
#     No section object takes the ADDRESS of overlay code (an R_386_32 against
#     a .text section or symbol): such a pointer is the only way an overlay
#     address could reach resident data at run time. Calls are R_386_PC32.
# Exit 1 with a named reason on violation. LC_ALL=C.
set -eu
LC_ALL=C; export LC_ALL
mode=$1; shift
case $mode in
resident)
    elf=$1; work=$2; mkdir -p "$work"; bad=0
    for sec in .rodata .data; do
        vma=$(objdump -h "$elf" | awk -v s=$sec '$2==s{print $4}')
        [ -n "$vma" ] || continue
        objcopy -O binary --only-section=$sec "$elf" "$work/sec.bin"
        od -An -v -tu4 -w4 -Ad "$work/sec.bin" | awk '{print $1 + 0, $2}' > "$work/sec.words"
        readelf -rW "$elf" | awk -v s=".rel$sec" '
            /^Relocation section/ { on = (index($0, "\047" s "\047") > 0); next }
            on && $3 == "R_386_32" { print $1 }' | while read -r o; do printf '%d\n' $(( 0x$o - 0x$vma )); done > "$work/sec.relocs"
        n=$(wc -l < "$work/sec.relocs")
        [ "$n" -gt 0 ] || { printf '!!! ovl-ptr FAIL: no relocations in %s -- link the kernel with --emit-relocs\n' "$sec"; exit 1; }
        # Every relocated dword must sit on a 4-byte boundary of the section,
        # or the word table below could silently miss it.
        [ "$(awk '$1 % 4 != 0' "$work/sec.relocs" | wc -l)" -eq 0 ] || { printf '!!! ovl-ptr FAIL: unaligned relocation in %s\n' "$sec"; exit 1; }
        hits=$(awk 'NR==FNR { r[$1] = 1; next } ($1 in r) && $2 >= 6029312 && $2 < 6160384' "$work/sec.relocs" "$work/sec.words" | wc -l)
        printf '    %s: %s relocated dwords checked, %s into the overlay window\n' "$sec" "$n" "$hits"
        bad=$(( bad + hits ))
    done
    [ "$bad" -eq 0 ] || { printf '!!! ovl-ptr FAIL: %s resident relocated pointer(s) into the overlay window (DEC-14-6.2)\n' "$bad"; exit 1; }
    ;;
overlay)
    for o in "$@"; do
        hit=$(objdump -r "$o" | awk '$2 == "R_386_32" && $3 ~ /^\.text/' | head -n 1)
        [ -z "$hit" ] || { printf '!!! ovl-ptr FAIL: %s takes the address of overlay code (%s) (DEC-14-6.2)\n' "$o" "$hit"; exit 1; }
    done
    ;;
*) echo "usage: ovl_ptr_check.sh resident <elf> <work> | overlay <obj>..." >&2; exit 2 ;;
esac
