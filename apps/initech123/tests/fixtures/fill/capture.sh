#!/bin/sh
# Real R2.2 Data Fill: independent reference process, never product output.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../../../.." && pwd)
runtime=${1:?runtime}; out=${2:?absolute new output}; scenario=${3:?scenario}
mkdir -p "$out"
cp "$runtime/runtime.img" "$out/runtime.img"
cc -std=c11 -O2 -Wall -Wextra -Werror "$root/goldens/dataloss/drive.c" -o "$out/drive"
s=$out/input.steps
printf 'sleep 4\nkey esc\n' > "$s"
for col in A B; do for row in 1 2 3; do
 printf "key f5\ntype %s%s\nkey ret\ntype 'KEEP-%s%s\nkey ret\n" "$col" "$row" "$col" "$row" >> "$s"
done; done
case "$scenario" in
 basic) start=10; step=2; stop=16 ;;
 fractional) start=0.5; step=0.25; stop=1.25 ;;
 decimal) start=0.1; step=0.1; stop=0.3 ;;
 descending) start=10; step=-2; stop=4 ;;
 zero) start=10; step=0; stop=20 ;;
 bad_direction) start=10; step=1; stop=0 ;;
 formatted) start=10; step=2; stop=16; printf 'type /rff2\nkey ret\ntype A1..B3\nkey ret\n' >> "$s" ;;
 protected) start=1; step=1; stop=6; printf 'type /ruA1..B3\nkey ret\ntype /rpB2\nkey ret\ntype /wgpe\n' >> "$s" ;;
 expressions) start=BASE; step='2+3'; stop='BASE+20'; printf 'key f5\ntype D1\nkey ret\ntype 10\nkey ret\ntype /rncBASE\nkey ret\ntype D1\nkey ret\n' >> "$s" ;;
 *) exit 2 ;;
esac
printf 'type /dfA1..B3\nkey ret\nshot start\ntype %s\nkey ret\nshot step\ntype %s\nkey ret\nshot stop\ntype %s\nkey ret\nsleep 0.8\nshot result\n' "$start" "$step" "$stop" >> "$s"
printf 'key esc\nkey esc\nkey esc\nkey esc\nkey esc\ntype /fsRESULT\nkey ret\nsleep 2\nshot saved\nend\n' >> "$s"
"$out/drive" "$out" "$runtime/lotus-boot.img" - "$out/runtime.img" "$s"
mcopy -i "$out/runtime.img@@32256" ::LOTUS/RESULT.WK1 "$out/RESULT.WK1"
lotus=${LOTUS123_DECOMP:-"$HOME/Projects/lotus123-decomp"}
python3 "$lotus/tools/wk1_ref.py" "$out/RESULT.WK1" > "$out/RESULT.dump"
