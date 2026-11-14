#!/bin/sh
# The gate never mints. Every expected byte comes from real III+ GOLD.OUT.
set -eu
mode=${1:?host or emu}
runner=${2:-build/test_interp_do_file}
base=tests/xbase_prog_diff/do-file-mint
case "$mode" in host|emu) ;; *) exit 2;; esac
case "$runner" in *[!a-zA-Z0-9_./-]*) exit 2;; esac
mkdir -p build/do-file-test
exec 9> "build/do-file-test/$mode-${runner##*/}.lock"
flock 9
fail=0;checks=0
for source in "$base"/*; do
    test -d "$source" || continue
    name=${source##*/}
    (cd "$source" && sha256sum --quiet -c sha256.txt)
    work="build/do-file-test/$mode-${runner##*/}-$name"
    rm -rf "$work"
    mkdir -p "$work"
    cp -f "$source"/*.PRG "$work/"
    if [ "$mode" = host ]; then
        "$runner" "$work" > "$work/console.out" 2>&1 || { cat "$work/console.out"; exit 2; }
    else
        truncate -s 1474560 "$work/data.img"
        mformat -i "$work/data.img" -f 1440 ::
        mcopy -o -i "$work/data.img" build/INITBASE.EXE build/INITBASE.OVL "$work"/*.PRG ::
        printf 'DO MINT\r\nQUIT\r\n' > "$work/DOFILE.IN"
        printf '@echo off\r\nibase < dofile.in > console.out\r\n' > "$work/DOFILE.BAT"
        mcopy -o -i "$work/data.img" "$work/DOFILE.IN" "$work/DOFILE.BAT" ::
        INITECH_QMP_STDIO=1 build/qemu_harness --disk build/tracer_secondary.img \
            --disk2 "$work/data.img" --name "dofile_$name" --out "$work" \
            --timeout-ms 60000 --keys 'd,o,f,i,l,e,ret,e,x,i,t,ret' \
            --keys-after SHELL-READY --quit-after SHELL-DONE > "$work/qemu.out" 2> "$work/qemu.report"
        grep -q 'triple_fault=0' "$work/qemu.report"
        grep -q 'quit_after_found=1' "$work/qemu.report"
        grep -q 'keys_sent=12' "$work/qemu.report"
        mcopy -o -i "$work/data.img" ::GOLD.OUT "$work/GOLD.OUT"
        fsck.fat -n "$work/data.img" > "$work/fsck.log"
    fi
    checks=$((checks+1))
    if ! cmp "$source/GOLD.OUT" "$work/GOLD.OUT"; then
        printf 'FAIL: DO-FILE %s %s: alternate differs from real III+\n' "$mode" "$name"
        cat "$work/console.out" 2>/dev/null || :
        fail=$((fail+1))
    else
        printf 'GREEN DO-FILE %s %s: byte-identical real III+ alternate\n' "$mode" "$name"
    fi
done
test "$checks" -gt 0 || exit 2
printf '\nINITECH_CHECKS %s failures=%s\n' "$checks" "$fail"
test "$fail" -eq 0
