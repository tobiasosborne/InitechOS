#!/bin/sh
# DOS 3.3 manual pp.28-29 / PDF 41-42, p.45 / PDF 130.
# Run in the lane root. QMP stdio is required; no socket transport fallback.
set -eu
img=$1
data=$2
name=$3
label=${4:-labelled}
out=${BUILD:-build}
cp -f "$data" "$out/${name}_data.img"
rm -f "$out/${name}"_frame_*.ppm
INITECH_QMP_STDIO=1 "$out/qemu_harness" --disk "$img" --disk2 "$out/${name}_data.img" \
    --name "$name" --out "$out" --timeout-ms 30000 --keys-after SHELL-READY \
    --keys-after-ms 15000 --record --record-settle-ms 150 \
    --quit-after 'A:\>' --quit-after-count 4 \
    --keys 'c,l,s,ret,d,i,r,ret,c,l,s,ret' \
    --mouse 'w100,w100,w100,w100,w100,w100,w100,w100,w100,w100,w100,w100,w100,w100,w100,w100' \
    2> "$out/$name.report" || true
grep -q '^SHELL-READY$' "$out/$name.serial"
grep -q 'keys_sent=12 mouse_events_sent=16' "$out/$name.report"
grep -q 'frames_taken=30' "$out/$name.report"
grep -q 'quit_after_found=1' "$out/$name.report"
if grep -q 'triple_fault=1' "$out/$name.report"; then exit 1; fi
# Never let pre-shell demos satisfy the serial assertions.
sed -n '/^SHELL-READY$/,$p' "$out/$name.serial" > "$out/$name.repl"
if [ "$label" = none ]; then
    volume=' Volume in drive A has no label'
else
    volume=' Volume in drive A is INITECH'
fi
grep -qF "$volume" "$out/$name.repl" || { echo 'DIR LOOK RED: volume line absent'; exit 1; }
grep -qF ' Directory of A:\' "$out/$name.repl"
grep -qE '^README   TXT +[0-9]+ +[0-9]+-[0-9]+-[0-9]+ +[0-9]+:[0-9]+[ap]' "$out/$name.repl" || { echo 'DIR LOOK RED: columns/date/time absent'; exit 1; }
grep -qE '^[ ]+[0-9]+ File\(s\) +[0-9]+ bytes free' "$out/$name.repl" || { echo 'DIR LOOK RED: File(s)/bytes free footer absent'; exit 1; }
LC_ALL=C TZ=UTC mdir -i "$data" :: > "$out/$name.mdir"
"$out/test_dir_disk" "$data" "$out/$name.mdir" "$out/$name.cells"
"$out/ppm_text_check" --cells "$out/${name}_frame_00008.ppm" "$out/$name.cells"
{
    printf '0 0 A:\\>\n'
    printf '0 5 %75s\n' ''
    row=1
    while [ "$row" -lt 25 ]; do printf '%s 0 %80s\n' "$row" ''; row=$((row+1)); done
} > "$out/$name.cls.cells"
"$out/ppm_text_check" --cells "$out/${name}_frame_00012.ppm" "$out/$name.cls.cells"
set --
for n in 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28; do
    frame=$(printf '%s/%s_frame_%05d.ppm' "$out" "$name" "$n")
    set -- "$@" "$frame"
done
"$out/ppm_text_check" --cursor-cycle 0 4 "$@"
printf 'VERDICT: PASS -- test-shell-dir-look (volume, manual columns, mdir footer, CLS home, cursor on/off)\n'
