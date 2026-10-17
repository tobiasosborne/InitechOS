#!/bin/sh
# Reference pp.7-8,127: relative source is resolved from the working directory.
set -efu
export INITECH_QMP_STDIO=1 LC_ALL=C TZ=UTC
mode=$1 runner=$2 out=build/xcopy-cwd-$1-${3:-green}
mkdir -p "$out"
fail() { echo "RED xcopy-cwd: $*";exit 1; }
dd if=/dev/zero of="$out/data.img" bs=512 count=2880 status=none
mformat -i "$out/data.img" -f 1440 -N 102 ::
mmd -i "$out/data.img" ::SUB ::SUB/SRC ::DST ::SRC
printf 'CWD-SOURCE\r\n' > "$out/sub"
printf 'ROOT-DECOY\r\n' > "$out/root"
mcopy -i "$out/data.img" "$out/sub" ::SUB/SRC/ONE.TXT
mcopy -i "$out/data.img" "$out/root" ::SRC/ONE.TXT
cp -f "$out/data.img" "$out/reference.img"
mcopy -i "$out/reference.img" ::SUB/SRC/ONE.TXT ::DST
if [ "$mode" = host ]; then
    ZUVO_CWD='\SUB' "$runner" --run "$out/data.img" XCOPY 'SRC \DST' > "$out/text" 2> "$out/report" || fail 'host returned error'
else
    mcopy -i "$out/data.img" "${XCOPY_CWD_PROGRAM:-build/xcopy_program.bin}" ::XCOPY.COM
    printf '@echo off\ncd sub\n\\xcopy src \\dst\necho XCOPY-CWD-DONE\n' > "$out/autoexec"
    mcopy -i "$out/data.img" "$out/autoexec" ::AUTOEXEC.BAT
    build/qemu_harness --disk "$runner" --disk2 "$out/data.img" --out "$out" --name run \
     --timeout-ms 25000 --keys-after SHELL-READY --keys 'e,x,i,t,ret' --quit-after SHELL-DONE \
     --rtc-base 1987-07-06T13:30:58 --rtc-deterministic 2> "$out/report" || true
    grep -q quit_after_found=1 "$out/report" || fail 'injected EXIT did not finish'
    grep -q XCOPY-CWD-DONE "$out/run.serial" || fail 'batch did not finish'
fi
mtype -i "$out/data.img" ::DST/ONE.TXT > "$out/actual" || fail 'relative source missing'
mtype -i "$out/reference.img" ::DST/ONE.TXT > "$out/expected"
cmp -s "$out/actual" "$out/expected" || fail 'relative source selected root decoy'
fsck.fat -n "$out/data.img" > "$out/fsck" 2>&1 || fail 'dosfstools damage'
echo "GREEN test-xcopy-cwd-$mode: subdirectory source == mcopy; root decoy excluded; fsck.fat"
