#!/bin/sh
# Re-mint into build/ only. Never overwrite the checked-in independent goldens.
set -eu
src=harness/diff/fat_diff/batch_break_mint
out=$(pwd)/build/batch-break-remint
media=${DOS33_BOOT:-/home/tobias/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img}
mkdir -p "$out"
cc -std=c11 -O2 -Wall -Wextra -Werror "$src/mint.c" -o "$out/drive"
sha256sum "$media" > "$out/media.sha256"
reference_rc=0; fsck.fat -n "$media" > "$out/reference.fsck" 2>&1 || reference_rc=$?
[ "$reference_rc" -le 1 ]
# This DOS 3.30 disk already lacks the later extended-BPB volume label.
# Require exactly its existing diagnostics, excluding the final file counts.
sed '$d' "$out/reference.fsck" > "$out/reference.diagnostics"
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
for answer in y n; do
    dir=$out/exec-$answer
    mkdir -p "$dir"
    cp -f "$media" "$dir/boot.img"
    # The reference utility disk is full; reclaim space only on this copy.
    mdel -i "$dir/boot.img" ::BACKUP.COM
    nasm -DREAL_DOS -f bin "$src/exec_child.asm" -o "$dir/BRKCHILD.COM"
    printf '@ECHO OFF\r\nVER > VERSION.TXT\r\nECHO EXEC-BEGIN\r\nBRKCHILD < INPUT.TXT > OUT.TXT\r\nECHO EXEC_AFTER\r\nECHO EXEC_AFTER > EXEC.OUT\r\nPAUSE\r\n' > "$dir/AUTOEXEC.BAT"
    printf 'INPUT-SENTINEL\r\n' > "$dir/INPUT.TXT"
    for file in AUTOEXEC.BAT INPUT.TXT BRKCHILD.COM; do
        mcopy -o -i "$dir/boot.img" "$dir/$file" "::$file"
    done
    # The child's READY is redirected. Its bytes on disk independently prove
    # it reached AH=08h after the unredirected batch boundary marker.
    "$out/drive" "$dir" "$dir/boot.img" "$answer" 'EXEC-BEGIN'
    mcopy -o -i "$dir/boot.img" ::OUT.TXT "$dir/OUT.TXT"
    # Real DOS writes the child's ^C to its redirected stdout; the batch
    # question itself is on CON. Keep these captured bytes exact.
    printf 'EXEC-CHILD-READY\r\n^C\r\n' > "$dir/expected"
    cmp "$dir/expected" "$dir/OUT.TXT"
    mcopy -o -i "$dir/boot.img" ::INPUT.TXT "$dir/actual"
    cmp "$dir/INPUT.TXT" "$dir/actual"
    if [ "$answer" = n ]; then
        mcopy -o -i "$dir/boot.img" ::EXEC.OUT "$dir/EXEC.OUT"
        # DOS ECHO retains the space immediately before the redirect token.
        printf 'EXEC_AFTER \r\n' > "$dir/expected"
        cmp "$dir/expected" "$dir/EXEC.OUT"
    elif mdir -i "$dir/boot.img" ::EXEC.OUT >/dev/null 2>&1; then
        echo 'RED real DOS mint: Y unexpectedly ran EXEC_AFTER' >&2;exit 1
    fi
    rc=0; fsck.fat -n "$dir/boot.img" > "$dir/fsck" 2>&1 || rc=$?
    [ "$rc" = "$reference_rc" ]
    sed '$d' "$dir/fsck" > "$dir/diagnostics"
    cmp "$out/reference.diagnostics" "$dir/diagnostics"
done
echo "Real DOS 3.30 capture written to $out; compare with checked-in VGA goldens."
