#!/bin/sh
# Mutate real linked instructions: unsafe ESP source and an extra transfer
# outside loader_run_plan (the old symbol-scoped check missed such copies).
set -eu
elf=$1
dir=$2
check=harness/proptest/kernel_transfer_check.sh
sh "$check" "$elf" "$dir/transfer.dis"
set -- $(objdump -h "$elf" | awk '$2==".text" {print $4, $6}')
test "$#" -eq 2
vma=$((0x$1))
fileoff=$((0x$2))
addr=$(awk '$2=="mov" && $3 ~ /\(%(esp|ebp)\),%esp$/ {
    sub(/:$/,"",$1); print $1; n++
} END {if(n!=1) exit 1}' "$dir/transfer.dis")
pos=$((0x$addr - vma + fileoff))
cp -f "$elf" "$dir/bad-stack-base.elf"
test "$(od -An -tu1 -j "$pos" -N1 "$elf" | tr -d ' ')" -eq 139 # MOV r32,r/m32
modrm=$(od -An -tu1 -j "$((pos+1))" -N1 "$elf" | tr -d ' ')
case $((modrm & 7)) in
    4) # ESP base: change only the SIB base from ESP to EBX.
        pos=$((pos+2))
        test "$(od -An -tu1 -j "$pos" -N1 "$elf" | tr -d ' ')" -eq 36
        patch=35;;
    5) # EBP base: preserve the ModRM mode/register, change its base to EBX.
        pos=$((pos+1)); patch=$((modrm-2));;
    *) echo '!!! kernel-transfer-mutant FAIL: unexpected ESP-load encoding'; exit 1;;
esac
printf "\\$(printf '%03o' "$patch")" | dd of="$dir/bad-stack-base.elf" bs=1 seek="$pos" conv=notrunc status=none
if sh "$check" "$dir/bad-stack-base.elf" "$dir/bad-stack-base.dis" >"$dir/bad-stack-base.log" 2>&1; then
    echo '!!! kernel-transfer-mutant FAIL: bad_stack_base survived'; exit 1
fi
grep '^!!! kernel-transfer FAIL: save step 9$' "$dir/bad-stack-base.log"

# Copy the actual save/entry bytes into another executable ELF section.
set -- $(awk '$2=="mov" && $3 ~ /^%esp,.*\(/ {start=$1; sub(/:$/,"",start)}
    start && $2=="jmp" && $3=="*%eax" {sub(/:$/,"",$1); print start, $1; exit}' "$dir/transfer.dis")
test "$#" -eq 2
objcopy --dump-section .text="$dir/transfer-text.bin" "$elf"
dd if="$dir/transfer-text.bin" of="$dir/transfer-copy.bin" bs=1 skip="$((0x$1-vma))" count="$((0x$2-0x$1+2))" status=none
objcopy --add-section .text.transfer_copy="$dir/transfer-copy.bin" --set-section-flags .text.transfer_copy=alloc,load,readonly,code,contents "$elf" "$dir/duplicate-transfer.elf" 2>"$dir/duplicate-transfer-objcopy.log"
if sh "$check" "$dir/duplicate-transfer.elf" "$dir/duplicate-transfer.dis" >"$dir/duplicate-transfer.log" 2>&1; then
    echo '!!! kernel-transfer-mutant FAIL: duplicate_transfer survived'; exit 1
fi
grep '^!!! kernel-transfer FAIL: expected exactly one transfer and restore; got 2/1$' "$dir/duplicate-transfer.log"
echo '>>> test-kernel-transfer-mutant: green (real ELF bad_stack_base and duplicate_transfer RED)'
