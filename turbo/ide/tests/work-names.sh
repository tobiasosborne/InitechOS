#!/bin/sh
set -eu
out=$1 root=${TOP:?}
. "$root/turbo/tests/checks.sh"
scratch=$out/runs/work-names
setup rm -rf "$scratch"
setup mkdir -p "$scratch/before"
for name in 'TURBOWK0.$$$' 'TURBOWK1.$$$' 'TURBO000.$00' 'TURBO000.$01' 'TURBO001.PAS'; do
 printf 'USER FILE: %s\r\nKEEP EVERY BYTE\032' "$name" > "$scratch/$name"
 setup cp "$scratch/$name" "$scratch/before/$name"
done
printf 'end\n' > "$scratch/steps"
setup "$out/keys" "$scratch/steps" "$scratch/IDEKEYS.BIN" "$scratch/shots.list"
setup sh -c 'cd "$1" && "$2" > stdout' sh "$scratch" "$out/idehost"
assert 'private-name allocation completes' grep -Fx 'IDE-HOST-END shots=0' "$scratch/stdout"
for file in "$scratch/before/"*; do
 assert "startup conserves $(basename "$file")" cmp "$file" "$scratch/$(basename "$file")"
done
finish
