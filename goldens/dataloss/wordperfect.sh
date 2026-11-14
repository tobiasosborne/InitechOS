#!/bin/sh
# Real WordPerfect 5.1 03-09-92, Original Document Backup collision/failure.
set -eu
src=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
corpus=${INITECH_CORPUS:-"$HOME/Projects"}
wordperfect=${WORDPERFECT51_DECOMP:-"$corpus/wordperfect51-decomp"}
out=${1:?output}; base=${2:?runtime}; boot=${3:?WP boot with FILES=40}
mkdir -p "$out"
cc -std=c11 -O2 -Wall -Wextra -Werror "$src/drive.c" -o "$out/drive"
cc -std=c11 -O2 -Wall -Wextra -Werror "$src/fixtures.c" -o "$out/fixtures"
corpus=$wordperfect/goldens/minted
for scenario in collision full readonly; do
 d=$out/$scenario; mkdir -p "$d"; cp "$base" "$d/runtime.img"
 truncate -s 1474560 "$d/data.img"; mformat -i "$d/data.img" -f 1440 -N 19920302 ::
 mcopy -i "$d/data.img" "$corpus/MINT01.WP5" ::NAME.WP5
 mcopy -i "$d/data.img" "$corpus/MINT02.WP5" '::NAME.BK!'
 if [ "$scenario" = full ]; then
  "$out/fixtures" "$d/FILLER.BIN" 1456640 FILLER
  touch -t 199203020930 "$d/FILLER.BIN"
  mcopy -i "$d/data.img" "$d/FILLER.BIN" ::FILLER.BIN
 fi
 if [ "$scenario" = readonly ]; then mattrib -i "$d/data.img" +r '::NAME.BK!'; fi
 mdir -a -i "$d/data.img" :: > "$d/before.mdir"
 cp "$corpus/MINT01.WP5" "$d/before.NAME.WP5"
 cp "$corpus/MINT02.WP5" "$d/before.NAME.BK!"
 cat > "$d/input.steps" <<'STEPS'
sleep 6
shot start
key shift-f1
shot setup
type 3
shot environment
type 1
shot backup_options
type 2
shot backup_toggle
type y
shot backup_yes
key f7
key shift-f10
type B:\NAME.WP5
key ret
sleep 4
shot retrieved
key home
key home
key down
type SAVE-REVISION-ONE-ABCDEFGHIJKLMNOPQRSTUVWXYZ-0123456789-EXTRA-TEXT
sleep 10
shot edited
key f10
wait Document to be saved
sleep 2
shot save_prompt
key ret
sleep 3
shot replace_prompt
type y
sleep 4
shot result
end
STEPS
 "$out/drive" "$d" "$boot" "$d/data.img" "$d/runtime.img" "$d/input.steps"
 mdir -a -i "$d/data.img" :: > "$d/after.mdir"
 for f in NAME.WP5 'NAME.BK!'; do
  if mdir -i "$d/data.img" "::$f" >/dev/null 2>&1; then mcopy -i "$d/data.img" "::$f" "$d/after.$f"; fi
 done
 for f in "$d"/*.ppm; do convert "$f" "${f%.ppm}.png"; done
 for f in "$d"/before.NAME* "$d"/after.NAME*; do
  python3 "$wordperfect/tools/wp5_ref.py" "$f" > "$f.dump"
 done
done
