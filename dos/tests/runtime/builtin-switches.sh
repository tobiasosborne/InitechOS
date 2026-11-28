#!/bin/sh
# DOS 3.30 COMMAND audit rows 4–6, 27, 31 and hands-on rows 7, 11.
set -eu
root=${1:?root} boot=${2:?boot} out=${3:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
. dos/tests/runtime/verdict.sh
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
mmd -i "$out/data.img" ::SAVED
printf 'BAD-TYPE-PAYLOAD\r\n' > "$out/ONE.TXT"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
opts='A B C D E F G H I J K L M N O P Q R S T U V W X Y Z ?'
(
    set -f
    printf '%s\r\n' '@echo off' 'ECHO SWITCH-BEGIN'
    for opt in $opts; do
        printf '%s\r\n' "MD CREATED /$opt" "RD SAVED /$opt" \
            "TYPE ONE.TXT /$opt" "CLS /$opt" "VER /$opt"
    done
    printf '%s\r\n' 'BREAK /Z' 'VOL /Z' 'COMMAND /C EXITARG.BAT' 'ECHO SWITCH-END'
) > "$out/AUTOEXEC.BAT"
(
    set -f
    printf '@echo off\r\n'
    for opt in $opts; do printf 'EXIT /%s\r\n' "$opt"; done
    printf 'ECHO CHILD-SURVIVED\r\n'
) > "$out/EXITARG.BAT"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" "$out/EXITARG.BAT" "$out/DONE.COM" "$out/ONE.TXT" ::
sh dos/tests/runtime/execute.sh "$root" "$boot" "$out" d,o,n,e,ret TRANSPORT-DONE ||
    error 'built-in switch audit transport failed'
mdir -a -b -i "$out/data.img" :: > "$out/names" || error 'cannot read disk inventory'
forbid 'MD created a directory with an unsupported switch' grep -E '^::/CREATED/?$' "$out/names"
require 'RD preserved the directory after an unsupported switch' grep -E '^::/SAVED/?$' "$out/names" >/dev/null
sed -n '/^SWITCH-BEGIN$/,/^SWITCH-END$/p' "$out/text" | sed '/^$/d' > "$out/actual"
(
    set -f
    printf 'SWITCH-BEGIN\n'
    for opt in $opts; do
        for verb in MD RD TYPE CLS VER; do printf 'Invalid parameter\n'; done
    done
    printf '%s\n' 'Invalid parameter' 'Invalid parameter'
    for opt in $opts; do printf 'Invalid parameter\n'; done
    printf '%s\n' CHILD-SURVIVED SWITCH-END
) > "$out/expected"
require 'built-ins refuse unsupported switches with DOS 3.30 wording' cmp "$out/expected" "$out/actual"
echo 'GREEN built-in switch sweep and directory retention'
