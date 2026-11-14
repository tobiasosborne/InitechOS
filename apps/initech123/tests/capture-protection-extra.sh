#!/bin/sh
# Additional real R2.2 edge cases, using the BASE worksheet from the first mint.
# Pass its stopped runtime disk; never read a running guest's disk.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
out=${1:?output};base=${2:?stopped runtime disk};boot=${3:?DOS boot disk}
mkdir -p "$out";cp "$base" "$out/runtime.img"
cc -std=c11 -O2 -Wall -Wextra -Werror "$root/goldens/dataloss/drive.c" -o "$out/drive"
s=$out/input.steps
printf 'sleep 4\nkey esc\ntype 123\nkey ret\n' > "$s"
case_steps() {
 id=$1;setup=$2;command=$3
 printf 'type /frBASE\nkey ret\n%s\ntype /fsB%s\nkey ret\nshot B%s\n%s\nshot R%s\nkey esc\ntype /fsA%s\nkey ret\n' "$setup" "$id" "$id" "$command" "$id" "$id" >> "$s"
}
labels='type /wgpd
key f5
type A1
key ret
type '\''alpha
key ret
key f5
type B1
key ret
type '\''bravo
key ret
key f5
type A2
key ret
type '\''charlie
key ret
key f5
type B2
key ret
type '\''delta
key ret
type /wgpe'
case_steps 22 "$labels
type /ruA1
key ret" 'type /rlrA1..A2
key ret'
case_steps 23 'type /rncTOTAL
key ret
type A1
key ret
type /ruA10..B10
key ret
type /ruA11
key ret' 'type /rntA10
key ret'
case_steps 24 'type /rncTOTAL
key ret
type A1
key ret' 'type /rntA10
key ret'
case_steps 25 '' 'type /rntA10
key ret'
case_steps 26 'type /rncZZZ
key ret
type A1..B2
key ret
type /rncAAA
key ret
type B1
key ret
type /ruA10..B12
key ret' 'type /rntA10
key ret'
case_steps 27 'type /rncTOTAL
key ret
type A1
key ret
type /ruIU8192..IV8192
key ret' 'type /rntIU8192
key ret'
formula='type /wgpd
key f5
type G1
key ret
type +A1+A2
key ret
type /rncTOP
key ret
type A1
key ret
type /rncBOTTOM
key ret
type A2
key ret
type /wgpe'
case_steps 28 "$formula
type /ruD1..D2
key ret" 'type /mA1..A2
key ret
type D1
key ret'
case_steps 29 "$formula
type /ruD1
key ret" 'type /mA1..A2
key ret
type D1
key ret'
case_steps 30 'type /ruD1..E1
key ret' 'type /rvA1..B2
key ret
type D1
key ret'
case_steps 31 'type /ruA1..B1
key ret' 'type /reA1..B2
key ret'
case_steps 32 "$labels
type /ruA1..B1
key ret" 'type /rlrA1..B2
key ret'
case_steps 33 'type /rncTOTAL
key ret
type A1
key ret
type /ruIV10..IV11
key ret' 'type /rntIV10
key ret'
printf 'sleep 1\nend\n' >> "$s"
"$out/drive" "$out" "$boot" - "$out/runtime.img" "$s"
for id in 22 23 24 25 26 27 28 29 30 31 32 33; do
 for prefix in B A; do mcopy -i "$out/runtime.img@@32256" "::LOTUS/$prefix$id.WK1" "$out/$prefix$id.WK1"; done
done
for f in "$out"/*.ppm; do convert "$f" "${f%.ppm}.png"; done
