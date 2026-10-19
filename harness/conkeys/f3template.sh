#!/bin/sh
# Ref: MS-DOS 3.3 User's Guide pp.154-155,157. Assert command OUTPUT,
# not the echoed input (which also contains REPEAT), after SHELL-READY.
set -eu
export INITECH_QMP_STDIO=1
build=$1; boot=$2; data=$3; name=$4
out=$build/$name
mkdir -p "$out"
cp -f "$data" "$out/data.img"
fail() { printf 'RED F3TEMPLATE %s\n' "$1"; exit 1; }
"$build/qemu_harness" --disk "$boot" --disk2 "$out/data.img" \
 --name "$name" --out "$build" --timeout-ms 12000 \
 --keys 'shift-e,shift-c,shift-h,shift-o,spc,shift-r,shift-e,shift-p,shift-e,shift-a,shift-t,ret,f3,ret,shift-e,shift-c,shift-h,shift-o,spc,shift-d,f3,ret,e,x,i,t,ret' \
 --keys-after SHELL-READY --quit-after SHELL-DONE 2>"$out/report" \
 || { cat "$out/report"; fail boot; }
grep -q 'OK=1' "$out/report" || fail completion
if grep -Eq 'triple_fault=1|unknown token|truncating' "$out/report"; then fail injection; fi
tr -d '\r' <"$build/$name.serial" | sed -n '/^SHELL-READY$/,$p' >"$out/transcript"
grep -Fxq 'SHELL-DONE' "$out/transcript" || fail shell-done
repeats=$(grep -Fxc REPEAT "$out/transcript" || true)
[ "$repeats" = 2 ] || fail "repeat-count: expected 2, got $repeats"
grep -Fxq DEPEAT "$out/transcript" || fail partial-column
grep -Fxq 'A:\>ECHO REPEAT' "$out/transcript" || fail command-echo
grep -Fxq 'A:\>ECHO DEPEAT' "$out/transcript" || fail partial-echo
printf '>>> test-f3template: green (QEMU REPEAT twice; partial ECHO D + F3 produces DEPEAT)\n'
