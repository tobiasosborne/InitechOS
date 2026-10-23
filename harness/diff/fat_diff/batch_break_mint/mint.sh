#!/bin/sh
# Re-mint into build/ only. Never overwrite the checked-in independent goldens.
set -eu
src=harness/diff/fat_diff/batch_break_mint
out=$(pwd)/build/batch-break-remint
media=${DOS33_BOOT:-/home/tobias/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img}
mkdir -p "$out"
cc -std=c11 -O2 -Wall -Wextra -Werror "$src/mint.c" -o "$out/drive"
sha256sum "$media" > "$out/media.sha256"
for answer in y n x; do
    dir=$out/$answer
    mkdir -p "$dir"
    cp -f "$media" "$dir/boot.img"
    for file in AUTOEXEC.BAT PARENT.BAT CHILD.BAT; do
        mcopy -o -i "$dir/boot.img" "$src/$file" "::$file"
    done
    "$out/drive" "$dir" "$dir/boot.img" "$answer"
    mcopy -o -i "$dir/boot.img" ::VERSION.TXT "$dir/version.raw"
    for file in CHILD.OUT PARENT.OUT AUTO.OUT; do
        if [ "$answer" = n ]; then
            mcopy -o -i "$dir/boot.img" "::$file" "$dir/$file"
        elif mdir -i "$dir/boot.img" "::$file" >/dev/null 2>&1; then
            echo "RED real DOS mint: $answer unexpectedly ran $file" >&2;exit 1
        fi
    done
done
echo "Real DOS 3.30 capture written to $out; compare with checked-in VGA goldens."
