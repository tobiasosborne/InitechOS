#!/bin/sh
# Mutate isolated copies: the source working tree is never edited.
set -eu
top=$1; out=$2; make_cmd=$3
mkdir -p "$out"
for fault in number layout binding; do
 dir="$out/$fault"
 rm -rf "$dir"
 mkdir -p "$dir"
 cp -R "$top/sdk" "$dir/sdk"
 case $fault in
 number) sed 's/TBX_REGISTER      0x0001u/TBX_REGISTER      0x0003u/' "$dir/sdk/include/toolbox_gate.h" > "$dir/changed"; mv "$dir/changed" "$dir/sdk/include/toolbox_gate.h"; target=abi ;;
 layout) sed 's/int32_t wherev;/int64_t wherev;/' "$dir/sdk/include/tbx.h" > "$dir/changed"; mv "$dir/changed" "$dir/sdk/include/tbx.h"; target=layout ;;
 binding) sed 's/int32_t tbx_memory(/int32_t absent_memory(/' "$dir/sdk/host/tbx_host.c" > "$dir/changed"; mv "$dir/changed" "$dir/sdk/host/tbx_host.c"; target=link-check ;;
 esac
 if "$make_cmd" -C "$dir/sdk" TOP="$top" OUT="$dir/build" "$target" > "$dir/red.log" 2>&1; then
  echo "FAIL $fault mutant stayed green"; exit 1
 fi
 case $fault in
 number) grep -q 'ABI audit: TBX_REGISTER' "$dir/red.log" ;;
 layout) grep -q 'static assertion failed.*tbx_event_t' "$dir/red.log" ;;
 binding) grep -q 'undefined reference.*tbx_memory' "$dir/red.log" ;;
 esac
 echo "RED confirmed: $fault ($target); log $dir/red.log"
done
