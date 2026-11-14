#!/bin/sh
# Capture from the corpus's initialized Lotus R2.2 runtime, under real DOS.
set -eu
src=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
out=${1:?output}; base=${2:?runtime image}; boot=${3:?boot image}
cc -std=c11 -O2 -Wall -Wextra -Werror "$src/drive.c" -o "$out/drive"
for scenario in disabled allprotected firstprotected secondprotected terminator unprotected labels; do
 d=$out/$scenario; mkdir -p "$d"; cp "$base" "$d/runtime.img"
 s=$d/input.steps
 cat > "$s" <<'STEPS'
sleep 4
key esc
type 123
key ret
type /rncTOTAL
key ret
type A1
key ret
STEPS
 for cell in A10 B10 A11 B11; do
  printf "key f5\ntype %s\nkey ret\ntype 'KEEP-%s\nkey ret\n" "$cell" "$cell" >> "$s"
 done
 case "$scenario" in
 firstprotected) printf 'type /ruB10..B11\nkey ret\n' >> "$s" ;;
 secondprotected) printf 'type /ruA10..A11\nkey ret\n' >> "$s" ;;
 terminator) printf 'type /ruA10..B10\nkey ret\n' >> "$s" ;;
 unprotected) printf 'type /ruA10..B11\nkey ret\n' >> "$s" ;;
 labels) printf "key f5\ntype D10\nkey ret\ntype 'TOTAL\nkey ret\nkey f5\ntype E10\nkey ret\ntype 777\nkey ret\n" >> "$s" ;;
 esac
 if [ "$scenario" != disabled ]; then printf 'type /wgpe\n' >> "$s"; fi
 cat >> "$s" <<'STEPS'
type /fsBEFORE
key ret
sleep 0.5
shot before
STEPS
 if [ "$scenario" = labels ]; then
  printf 'type /rnl\nshot labels_menu\n' >> "$s"
  printf 'type r\nshot labels_range_prompt\ntype D10\nkey ret\nsleep 0.5\nshot result\n' >> "$s"
 else
  printf 'type /rnt\nshot table_prompt\ntype A10\nkey ret\nsleep 0.5\nshot result\n' >> "$s"
 fi
 cat >> "$s" <<'STEPS'
key esc
type /fsAFTER
key ret
sleep 0.5
shot saved
end
STEPS
 "$out/drive" "$d" "$boot" - "$d/runtime.img" "$s"
 for f in BEFORE AFTER; do mcopy -i "$d/runtime.img@@32256" "::LOTUS/$f.WK1" "$d/$f.WK1"; done
 mdir -i "$d/runtime.img@@32256" ::LOTUS > "$d/after.mdir"
 for f in "$d"/*.ppm; do convert "$f" "${f%.ppm}.png"; done
 python3 /home/tobias/Projects/lotus123-decomp/tools/wk1_ref.py "$d/BEFORE.WK1" > "$d/BEFORE.dump"
 python3 /home/tobias/Projects/lotus123-decomp/tools/wk1_ref.py "$d/AFTER.WK1" > "$d/AFTER.dump"
done
