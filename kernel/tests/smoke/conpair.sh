#!/bin/sh
# Read 00h, output, then read 10h/13h from real PS/2 input. Compare literal
# bytes via independent mtools, and require a positive guest verdict. Source:
# MS-DOS Encyclopedia AH=07h; IBM keyboard set-1 Alt-Q/Alt-R, initech-drm4v.
set -eu
top=$1
out=$2
cd "$top"
mkdir -p "$out"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
export SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C
boot=${CONPAIR_BOOT:-build/image/boot.img}
failures=0
for emu in qemu bochs; do
    for scan in 10 13; do
        work=$out/$emu-$scan
        mkdir -p "$work"
        truncate -s 0 "$work/data.img"
        truncate -s 1474560 "$work/data.img"
        mformat -i "$work/data.img" -f 1440 -N 0x434f4e50 ::
        mcopy -i "$work/data.img" "build/kernel/smoke/CONPAIR$scan.COM" ::PAIR.COM
        printf '@ECHO OFF\r\nPAIR\r\n' > "$work/AUTOEXEC.BAT"
        mcopy -i "$work/data.img" "$work/AUTOEXEC.BAT" ::AUTOEXEC.BAT
        if [ "$scan" = 10 ]; then key=q; printf '\000\020' > "$work/expected.bin"
        else key=r; printf '\000\023' > "$work/expected.bin"; fi
        # Fallback bytes make the old Ctrl-P/Ctrl-S interpretation produce an
        # explicit wrong pair instead of relying on a timeout as the red proof.
        # Ctrl-N/Enter then cleans up printer echo in the negative control,
        # after the pair was captured, so even that mutant prints a verdict.
        if ! timeout 40 "build/factory/${emu}_harness" --disk "$boot" --disk2 "$work/data.img" \
            --keys-after CONPAIR-READY --keys "alt-$key,z,y,ctrl-n,ret" \
            --expect CONPAIR-DONE --quit-after CONPAIR-DONE --timeout-ms 20000 \
            --name run --out "$work" > "$work/stdout" 2> "$work/report"; then
            cat "$work/report"; echo "RED CONPAIR $emu 00/$scan transport/completion"; failures=$((failures+1))
        fi
        tr -d '\r' < "$work/run.serial" > "$work/transcript"
        if ! grep -Fx CONPAIR-PASS "$work/transcript" >/dev/null ||
            grep -E 'CONPAIR-(FAIL|IO-FAIL)|CONKEYS-(PAUSE|PRN-ON)|PANIC' "$work/transcript"; then
            cat "$work/transcript"; echo "RED CONPAIR $emu 00/$scan guest verdict"; failures=$((failures+1))
        fi
        if ! mcopy -o -i "$work/data.img" ::PAIR.BIN "$work/actual.bin" ||
            ! cmp "$work/expected.bin" "$work/actual.bin"; then
            od -An -tx1 "$work/actual.bin"
            echo "RED CONPAIR $emu 00/$scan disk bytes vs literal pair"; failures=$((failures+1))
        fi
        if ! grep -F triple_fault=0 "$work/report" >/dev/null; then
            cat "$work/report"; echo "RED CONPAIR $emu CPU fault"; failures=$((failures+1))
        fi
        echo "CONPAIR $emu expected 00 $scan; actual $(od -An -tx1 "$work/actual.bin")"
    done
done
echo "CONPAIR SMOKE: 4 emulator/pair cases, $failures failures"
test "$failures" -eq 0
