#!/bin/sh
# W4.0 / PRD 6.6: real III+ bytes only. Factory C + thin shell, no Python.
set -eu
export LC_ALL=C TZ=UTC
root=$(pwd)
case_dir=$(cd "${1:?case directory required}" && pwd)
dest=${2:?output directory required}
mkdir -p "$dest"
dest=$(cd "$dest" && pwd)
ref=$(cd "${DBASE3_DECOMP:-../dbase3-decomp}" && pwd)
tool="$root/build/screen_tool"
"$tool" check-setup "$case_dir/SETUP"
work=$(mktemp -d "$root/build/screen-work.XXXXXX")
trap 'if [ "${SCREEN_MINT_KEEP:-0}" = 1 ]; then echo "work: $work"; else rm -rf "$work"; fi' EXIT HUP INT TERM
mkdir "$work/output" "$work/drive"
drive="$work/drive"
for f in DBASE.EXE DBASEINL.OVL DBASE.MSG DBASE.OVL; do
    cp -f "$ref/mint/work/$f" "$drive/$f"
done
cp -rf "$case_dir/SETUP/." "$drive/"
cp -f "$root/build/SCREEN.COM" "$drive/SCREEN.COM"
"$tool" compile "$case_dir/KEYS" "$case_dir/DUMPS" "$drive/EVENTS.BIN"
printf 'DO SETUP\r\nRUN SCREEN\r\nRETURN\r\n' > "$drive/MINT.PRG"
# Defence in depth: no supplied capture or marker may survive into a new boot.
rm -f "$drive"/DUMP*.BIN "$drive/CAPTURE.OK"
# Ref: DOSBox-X's installed reference config freesizecap=fixed and MOUNT
# geometry. Pin metadata independently of host mtimes; host logs stay outside C:.
find "$drive" -exec touch -h -t 198607241200.00 {} +
cat > "$work/mint.conf" <<EOF
[sdl]
output=surface
[dosbox]
machine=svga_s3
memsize=8
freesizecap=fixed
[cpu]
core=normal
cputype=386
cycles=fixed 10000
[autoexec]
mount c "$drive" -size 512,32,2048,1024 -freesize 16 -label SCREEN
c:
DATE 07/24/1986
TIME 12:00:00
DBASE MINT
SCREEN /S
exit
EOF
status=0
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout --foreground -k 2 \
    "${SCREEN_MINT_TIMEOUT:-90}" dosbox-x -defaultconf -fastlaunch -nopromptfolder \
    -conf "$work/mint.conf" -exit > "$dest/mint.log" 2>&1 || status=$?
if [ "$status" != 0 ]; then
    printf 'RED: screen mint emulator exit=%s; see %s/mint.log\n' "$status" "$dest" >&2
    exit 1
fi
if [ ! -f "$drive/CAPTURE.OK" ]; then
    printf 'RED: screen mint has no fresh successful-save marker\n' >&2
    exit 1
fi
"$tool" check-save "$case_dir/DUMPS" "$drive/CAPTURE.OK"
# Decode only declared, freshly captured dumps. Missing/partial capture is RED.
i=0
while IFS= read -r name || [ -n "$name" ]; do
    case $name in ''|'#'*) continue ;; esac
    "$tool" encode "$drive/DUMP$i.BIN" "$work/output/$name.screen"
    i=$((i + 1))
done < "$case_dir/DUMPS"
(cd "$work/output" && sha256sum ./*.screen) > "$work/output/SHA256SUMS"
(
    cd "$case_dir"
    find SETUP -type f -print | sort | xargs sha256sum
    sha256sum KEYS DUMPS
) > "$work/output/INPUTS.sha256"
sh "$root/harness/diff/dbf_diff/screen_mint_oracle.sh" --write "$case_dir" "$work/output/ORACLE.sha256"
# Publish only after every declared point and manifest is complete.
cp -f "$work/output/"* "$dest/"
printf 'GREEN: real III+ mint %s (%s dump points)\n' "$(basename "$case_dir")" "$i"
