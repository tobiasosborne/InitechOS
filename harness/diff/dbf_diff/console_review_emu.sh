#!/bin/sh
# initech-0spyf/fzsvo; PRD 6.6. Safety invariants, extended-pair regression,
# host attribute contract, and a real III+ literal-control projection.
set -eu
mode=$1
exe=${2:-build/SAMIR.EXE}
tag=$(basename "$exe" .EXE)
out=build/console-review-$mode-$tag
rm -rf "$out"
mkdir -p "$out"
cp -f build/samir_list.img "$out/data.img"
mcopy -o -i "$out/data.img" "$exe" ::SAMIR.EXE
mcopy -o -i "$out/data.img" "${exe%.EXE}.OVL" ::SAMIR.OVL
cat > "$out/PROBE.PRG" <<'PRG'
SET TALK OFF
SET STATUS OFF
CLEAR
PRG
launch='ks,ka,km,ki,kr,kret'
case $mode in
continuation)
    cat >> "$out/PROBE.PRG" <<'PRG'
@ 24,75 SAY "abcdefgh"
?? REPLICATE("X",80)
WAIT
RETURN
PRG
    ;;
color)
    cat >> "$out/PROBE.PRG" <<'PRG'
SET COLOR TO W+/B
@ 2,3 SAY "S"
?? "O"
WAIT
RETURN
PRG
    ;;
controls)
    cat >> "$out/PROBE.PRG" <<'PRG'
@ 23,78 SAY "XYZ"+CHR(13)+"Q"+CHR(10)+"R"
WAIT
RETURN
PRG
    # Spell OUT.TXT with the standard key names (no interpretation by SAMIR).
    launch='ks,ka,km,ki,kr,kspc,kshift-dot,ko,ku,kt,kdot,kt,kx,kt,kret'
    ;;
keys) ;;
*) exit 2;;
esac
mcopy -o -i "$out/data.img" "$out/PROBE.PRG" ::PROBE.PRG
if [ "$mode" = keys ]; then
    spec="$launch,w1500,kleft,w150,D$out/page.ppm"
else
    spec="$launch,w1500,kd,ko,kspc,kp,kr,ko,kb,ke,kret,w1500,D$out/page.ppm"
fi
rc=0
INITECH_QMP_STDIO=1 INITECH_TEXT_SCREEN_TOOL=build/screen_capture \
INITECH_TEXT_SCREEN_ELF=build/kernel_shell_secondary.elf \
    build/qemu_harness --disk build/tracer_secondary.img --disk2 "$out/data.img" \
    --name live --out "$out" --timeout-ms 15000 --mouse "$spec" \
    --keys-after SHELL-READY --legacy-quit > "$out/stdout" 2> "$out/report" || rc=$?
if ! grep -q '^\[harness\] OK=1$' "$out/report"; then
    cat "$out/report"
    echo "RED console-review: $mode incomplete guest ($rc)"
    exit 1
fi
# HALT from a violated native draw bound is a failure even if QMP can dump.
if grep -q 'SAMIR draw bound' "$out/live.serial"; then
    echo 'RED console-review: continuation native draw bound HALT'
    exit 1
fi
build/test_console_review "$mode" "$out/page.ppm.screen"
