#!/bin/sh
# Fresh real III+ retained screens. Same guest SCREEN.COM + screen_tool route
# as screen_mint.sh, with HELP.DBS and a delayed startup-license Enter.
# No -silent, no executable patches, no writes to the sister repository.
# initech-9nzpr, DEC-03a.10; generated-form uses real pristine .FMT/.DBF.
set -eu
export LC_ALL=C TZ=UTC
app=${1:?retained, structure-retained or generated-form}; dest=${2:?output directory}
case $app in retained|create-retained|structure-retained|generated-form) ;; *) exit 2;; esac
root=$(pwd); ref=${DBASE3_DECOMP:?absolute sister repository required}
case $ref in /*) ;; *) exit 2;; esac
src=$root/harness/diff/dbf_diff/xbase_prog_diff/w4/$app
mkdir -p "$dest"; dest=$(cd "$dest" && pwd)
work=$(mktemp -d "$root/build/w4-real.XXXXXX")
trap 'if [ "${W4_MINT_KEEP:-0}" = 1 ]; then echo "work: $work"; else rm -rf "$work"; fi' EXIT HUP INT TERM
mkdir "$work/drive" "$work/output"
drive=$work/drive
build/screen_tool check-setup "$src/SETUP"
for f in DBASE.EXE DBASEINL.OVL DBASE.MSG DBASE.OVL HELP.DBS; do
    cp -f "$ref/mint/work/$f" "$drive/$f"
done
cp -rf "$src/SETUP/." "$drive/"
if [ "$app" = generated-form ]; then
    real=$ref/goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities
    cp -f "$real/CLIENTS.DBF" "$real/CLIENTS.FMT" "$drive/"
fi
cp -f build/SCREEN.COM "$drive/SCREEN.COM"
build/screen_tool compile "$src/KEYS" "$src/DUMPS" "$drive/EVENTS.BIN"
printf 'DO SETUP\r\nRUN SCREEN\r\nRETURN\r\n' > "$drive/MINT.PRG"
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
AUTOTYPE -w 10 -p 1 enter
DBASE MINT
SCREEN /S
exit
EOF
rc=0
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout --foreground -k 2 "${W4_MINT_TIMEOUT:-90}" \
    dosbox-x -defaultconf -defaultmapper -fastlaunch -nopromptfolder -conf "$work/mint.conf" -exit > "$dest/mint.log" 2>&1 || rc=$?
if [ "$rc" != 0 ] || [ ! -s "$drive/CAPTURE.OK" ]; then
    echo "RED: retained real III+ mint $app exit=$rc or missing successful-save marker"; exit 1
fi
build/screen_tool check-save "$src/DUMPS" "$drive/CAPTURE.OK"
i=0
while IFS= read -r name; do
    case $name in ''|'#'*) continue;; esac
    build/screen_tool encode "$drive/DUMP$i.BIN" "$work/output/$name.screen"
    build/w4_screen_check "$work/output/$name.screen" "$src/$name.ASSERTIONS"
    i=$((i+1))
done < "$src/DUMPS"
(cd "$work/output" && sha256sum ./*.screen) > "$work/output/SHA256SUMS"
(cd "$src" && find SETUP -type f -print | sort | xargs sha256sum; sha256sum KEYS DUMPS ./*.ASSERTIONS) > "$work/output/INPUTS.sha256"
(
    for f in DBASE.EXE DBASEINL.OVL DBASE.MSG DBASE.OVL HELP.DBS SCREEN.COM EVENTS.BIN; do
        sha256sum < "$drive/$f" | sed "s/  -$/  $f/"
    done
    for f in screen_tool w4_screen_check; do sha256sum < "$root/build/$f" | sed "s/  -$/  $f/"; done
    sha256sum < "$(command -v dosbox-x)" | sed 's/  -$/  dosbox-x/'
    sha256sum < "$root/harness/diff/dbf_diff/w4_retained_mint.sh" | sed 's/  -$/  w4_retained_mint.sh/'
    if [ "$app" = generated-form ]; then
        for f in CLIENTS.FMT CLIENTS.DBF; do sha256sum < "$real/$f" | sed "s/  -$/  $f/"; done
    fi
) > "$work/output/ORACLE.sha256"
cp -f "$work/output/"* "$dest/"
echo "GREEN: retained real III+ mint $app ($i dumps, fresh marker)"
