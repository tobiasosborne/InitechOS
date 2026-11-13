#!/bin/sh
# Probe both the compiler and runtime before using hosted i386 checks.
set -eu
cc=$1; out=$2
mkdir -p "$out"
test -f "$out/probe.c" || printf 'int main(void){return 0;}\n' > "$out/probe.c"
make --no-print-directory -f tests/host32.mk OUT="$out" HOSTCC="$cc" "$out/probe"
if test -x "$out/probe" && "$out/probe"; then
 test -f "$out/layout-main.c" || printf '#include "layout.c"\nint main(void){return 0;}\n' > "$out/layout-main.c"
 make --no-print-directory -f tests/host32.mk OUT="$out" HOSTCC="$cc" available
 "$out/layout"
 "$out/host"
 echo 'ABI: hosted i386 layouts and shared fake executed'
else
 echo 'ABI: hosted i386 unavailable; target compile-only layouts remain mandatory'
fi
