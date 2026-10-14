#!/bin/sh
# K12 oracle: real PS/2 -> cooked CON -> COMMAND.COM, independent mcopy.
# Ref: 80X0945_DOS_3.30_Technical_Reference_Apr87.pdf pp. 6-14, 6-138, 6-149;
# MS-DOS_3.3_Users_Guide_198707.pdf pp. 132, 158-159.
set -eu
runner=$1
boot=$2
out=$3
name=$4
data=$out/$name-data.img
serial=$out/$name.serial
report=$out/$name.report
dd if=/dev/zero of="$data" bs=512 count=2880 status=none
mformat -i "$data" -f 1440 -N 102 ::
# Echo sentinels have to appear AFTER returned prompts, not just in queued
# keystrokes. EXIT completes a secondary shell; the primary uses a final ECHO.
keys='c,o,p,y,spc,c,o,n,spc,t,dot,t,x,t,ret,h,e,l,l,o,ret,ctrl-z,ret,e,c,h,o,spc,e,o,f,o,k,ret,t,y,p,e,spc,c,o,n,ret,ctrl-z,ret,e,c,h,o,spc,t,y,p,e,o,k,ret,e,c,h,o,spc,h,e,l,l,o,spc,shift-backslash,spc,t,y,p,e,spc,c,o,n,ret,ctrl-c,e,c,h,o,spc,a,b,o,r,t,o,k,ret,c,o,p,y,spc,c,o,n,spc,u,dot,t,x,t,ret,m,o,r,e,ret,ctrl-c,e,c,h,o,spc,c,o,p,y,a,b,o,r,t,o,k,ret,t,y,p,e,spc,c,o,n,spc,shift-dot,spc,r,e,d,i,r,dot,t,x,t,ret,ctrl-c,e,c,h,o,spc,r,e,s,t,o,r,e,d,ret,e,x,i,t,ret'
"$runner" --disk "$boot" --disk2 "$data" --name "$name" --out "$out" \
    --keys "$keys" --keys-after SHELL-READY --keys-after-ms 15000 \
    --quit-after SHELL-DONE --timeout-ms 45000 2>"$report" || true
test -s "$serial"
grep -Fq SHELL-READY "$serial"
if grep -q 'triple_fault=1' "$report"; then
    printf '!!! test-j1lu-control-emu FAIL: triple fault\n'; exit 1
fi
for mark in eofok typeok abortok copyabortok restored; do
    grep -Fq "A:\\>echo $mark" "$serial" || {
        printf '!!! test-j1lu-control-emu FAIL: prompt did not return before %s\n' "$mark"; exit 1;
    }
    grep -qx "$mark$(printf '\r')" "$serial" || {
        printf '!!! test-j1lu-control-emu FAIL: ECHO %s did not run\n' "$mark"; exit 1;
    }
done
grep -Fq SHELL-DONE "$serial"
grep -q 'quit_after_found=1' "$report"
printf 'hello\r\n' >"$out/$name-t.expected"
printf 'more\r\n' >"$out/$name-u.expected"
for f in t u; do
    mcopy -o -i "$data" "::$f.txt" "$out/$name-$f.got"
    cmp "$out/$name-$f.expected" "$out/$name-$f.got"
done
if mdir -i "$data" '::PIPE1.$$$' >/dev/null 2>&1; then
    printf '!!! test-j1lu-control-emu FAIL: pipe temporary leaked\n'; exit 1
fi
printf '>>> test-j1lu-control-emu: green (EOF + abort prompts, COPY bytes=mcopy, redirection + pipe cleanup)\n'
