#!/bin/sh
# Run tests/console-symbols.c against an i386 ELF (tier 0: the fixture in
# tests/console-symbols-fixture/; make image: build/system/system.elf) with
# nm -S's independent answer: the g_console_page of size 0xfa0 and
# g_int21_con. The ELF must also hold the second, 4-byte g_console_page, or
# it cannot tell a reader that takes the last match from a correct one.
set -eu
elf=$1 bin=$2
[ -f "$elf" ] || { echo "FAIL console symbols: $elf missing"; exit 1; }
page=$(nm -S "$elf" | awk '$4=="g_console_page" && $2=="00000fa0" {print $1}')
con=$(nm -S "$elf" | awk '$4=="g_int21_con" && $2=="00000004" {print $1}')
[ -n "$page" ] && [ -n "$con" ] || { echo "FAIL console symbols: nm found no page/console"; exit 1; }
decoy=$(nm -S "$elf" | awk '$4=="g_console_page" && $2=="00000004"' | wc -l)
[ "$decoy" -ge 1 ] || { echo "FAIL console symbols: $elf has no 4-byte g_console_page decoy"; exit 1; }
"$bin" "$elf" "$page" "$con"
