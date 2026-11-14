#!/bin/sh
# Pack copies of the existing initialized reference runtimes on a DOS33 FAT16 disk.
set -eu
out=${1:?output directory}; mkdir -p "$out"
corpus=${INITECH_CORPUS:-"$HOME/Projects"}
lotus=${LOTUS123_DECOMP:-"$corpus/lotus123-decomp"}
wordperfect=${WORDPERFECT51_DECOMP:-"$corpus/wordperfect51-decomp"}
media=${DOS33_BOOT:-"$corpus/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img"}
truncate -s 33554432 "$out/runtime.img"
printf 'label: dos\nlabel-id: 0x19920302\nunit: sectors\n\n63,65457,4,*\n' | sfdisk "$out/runtime.img" > "$out/partition.log"
mformat -i "$out/runtime.img@@32256" -T 65457 -h 16 -s 63 -c 4 -r 32 -H 63 -N 19920302 ::
mmd -i "$out/runtime.img@@32256" ::LOTUS ::WP51
mcopy -s -i "$out/runtime.img@@32256" "$lotus"/mint/work_ref/* ::LOTUS/
mcopy -s -i "$out/runtime.img@@32256" "$wordperfect"/mint/work/WP51/* ::WP51/
for app in lotus wp; do
 cp "$media" "$out/$app-boot.img"
 case "$app" in lotus) dir=LOTUS; exe=123;; wp) dir=WP51; exe=WP;; esac
 printf '@ECHO OFF\r\nC:\r\nCD \\%s\r\n%s\r\n' "$dir" "$exe" > "$out/$app-AUTOEXEC.BAT"
 mcopy -o -i "$out/$app-boot.img" "$out/$app-AUTOEXEC.BAT" ::AUTOEXEC.BAT
 if [ "$app" = wp ]; then printf 'FILES=40\r\nBUFFERS=20\r\n' > "$out/CONFIG.SYS"; mcopy -o -i "$out/$app-boot.img" "$out/CONFIG.SYS" ::CONFIG.SYS; fi
done
