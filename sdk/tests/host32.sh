#!/bin/sh
# Probe both the compiler and runtime before using hosted i386 checks.
set -eu
cc=$1; out=$2
mkdir -p "$out"
printf 'int main(void){return 0;}\n' > "$out/probe.c"
if "$cc" -m32 "$out/probe.c" -o "$out/probe" > "$out/probe.log" 2>&1 && "$out/probe"; then
 printf '#include "layout.c"\nint main(void){return 0;}\n' > "$out/layout-main.c"
 "$cc" -m32 -std=c11 -Wall -Wextra -Werror -Iinclude -Itests "$out/layout-main.c" -o "$out/layout"
 "$out/layout"
 "$cc" -m32 -std=c11 -O1 -g -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Iinclude -Ihost tests/host.c host/tbx_host.c -o "$out/host"
 "$out/host"
 echo 'ABI: hosted i386 layouts and shared fake executed'
else
 echo 'ABI: hosted i386 unavailable; target compile-only layouts remain mandatory'
fi
