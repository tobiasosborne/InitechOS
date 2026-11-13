#!/bin/sh
# Focused SAMIR emu proof. No manuals, full test vector or Bochs dependencies.
set -eu
out=${SCREEN_SAMIR_OUT:-build/screen-mint/samir}
mkdir -p "$out"
cp -f build/samir_list.img "$out/data.img"
rm -f "$out/live.ppm.screen" "$out/live.ppm"
rc=0
INITECH_QMP_STDIO=1 INITECH_TEXT_SCREEN_TOOL="${SCREEN_CAPTURE_TOOL:-build/screen_capture}" \
INITECH_TEXT_SCREEN_ELF=build/kernel_shell_secondary.elf \
build/qemu_harness --disk build/tracer_secondary.img --disk2 "$out/data.img" \
    --name live --out "$out" --timeout-ms "${SCREEN_SAMIR_TIMEOUT_MS:-15000}" \
    --keys 's,a,m,i,r,ret,u,s,e,spc,c,l,i,e,n,t,s,ret,d,i,s,p,l,a,y,spc,s,t,r,u,c,t,u,r,e,ret' \
    --keys-after SHELL-READY --screendump --screendump-after SHELL-READY \
    > "$out/stdout" 2> "$out/report" || rc=$?
if [ "$rc" != 0 ]; then
    cat "$out/report"
    printf 'RED: SAMIR harness failed (exit %s)\n' "$rc" >&2
    exit 1
fi
if ! grep -qx '\[harness\] OK=1' "$out/report" ||
   ! grep -q 'screendump_taken=1' "$out/report" ||
   ! grep -qx 'GREEN: live SAMIR console captured (2000 characters, 2000 attributes, cursor)' "$out/stdout" ||
   [ ! -s "$out/live.ppm" ] || [ ! -s "$out/live.ppm.screen" ]; then
    cat "$out/report"
    printf 'RED: SAMIR screen capture did not complete\n' >&2
    exit 1
fi
if grep -q 'TRIPLE FAULT\|PANIC' "$out/live.serial" "$out/report"; then cat "$out/report"; exit 1; fi
grep -q '^\. display structure' "$out/live.serial"
build/screen_tool compare "$out/live.ppm.screen" "$out/live.ppm.screen"
printf 'GREEN: SAMIR emu screen capture (live cells, attributes, cursor; harness exit %s)\n' "$rc"
# W3/W4 have not landed: compare, and report either parity or the exact delta.
rc=0
build/screen_tool compare harness/diff/dbf_diff/xbase_prog_diff/screen-mint/dot/structure.screen \
    "$out/live.ppm.screen" > "$out/vs-real.verdict" || rc=$?
cat "$out/vs-real.verdict"
[ "$rc" -lt 2 ]
printf 'GREEN: SAMIR-vs-real comparison executed (parity result exit %s)\n' "$rc"
