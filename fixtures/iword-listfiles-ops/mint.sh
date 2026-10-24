#!/bin/sh
# Real WP 5.1 capture, initech-7aou. Corpus is read-only; each case receives
# a fresh work directory and FAT image. Outputs go to the supplied directory,
# never automatically over these pinned goldens. Requires SDL2 development
# headers, DOSBox-X, mtools, ImageMagick and the corpus's exact font decoder.
set -eu
DEC=${WORDPERFECT51_DECOMP:?Set WORDPERFECT51_DECOMP to the corpus root}
OUT=${1:?Usage: mint.sh output-directory}
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
FIX=$ROOT/fixtures/iword-listfiles-ops
mkdir -p "$OUT/shots" "$OUT/grids"
OUT=$(CDPATH= cd -- "$OUT" && pwd)
WORK=$(mktemp -d "${TMPDIR:-/tmp}/iword-listfiles-mint.XXXXXX")
printf 'Scratch work: %s\n' "$WORK"
cc -Wall -Wextra -Werror -shared -fPIC $(pkg-config --cflags sdl2) \
 "$ROOT/harness/diff/wp5_diff/wp51_headless.c" -o "$WORK/drive.so" $(pkg-config --libs sdl2) -ldl
cp -f "$DEC/mint/work/LISTF/lf005.img" "$WORK/base.img"
SOURCE_DATE_EPOCH=699350400 TZ=UTC mmd -i "$WORK/base.img" ::OPS ::OPS/SUB
printf 'ALPHA CONTENT\r\n' > "$WORK/ALPHA.TXT"
printf 'BETA KEEP\r\n' > "$WORK/BETA.TXT"
printf 'LOCK KEEP\r\n' > "$WORK/LOCK.TXT"
for n in ALPHA BETA LOCK; do
 SOURCE_DATE_EPOCH=699350400 TZ=UTC mcopy -i "$WORK/base.img" "$WORK/$n.TXT" "::OPS/$n.TXT"
done
mattrib -i "$WORK/base.img" +r ::OPS/LOCK.TXT
for case in ops collisions refusals protected selection readonly readonly-copy full; do
 mkdir -p "$WORK/$case"
 cp -rf "$DEC/mint/work/WP51" "$WORK/$case/WP51"
 cp -f "$WORK/base.img" "$WORK/$case/disk.img"
 RO=
 case "$case" in readonly*) RO=-ro;; esac
 if [ "$case" = full ]; then
  free=$(mdir -i "$WORK/$case/disk.img" :: | sed -n 's/^ *\([0-9 ]*\) bytes free.*/\1/p' | tr -d ' ')
  test -n "$free"
  truncate -s "$free" "$WORK/$case/filler"
  SOURCE_DATE_EPOCH=699350400 TZ=UTC mcopy -i "$WORK/$case/disk.img" "$WORK/$case/filler" ::FILLER.BIN
 fi
 cat > "$WORK/$case/wp.conf" <<CONF
[sdl]
output=surface
videodriver=dummy
[dosbox]
machine=svga_s3
memsize=8
captures=$WORK/$case
quit warning=false
[cpu]
core=auto
cputype=386
cycles=max
[mixer]
nosound=true
[autoexec]
mount c $WORK/$case
imgmount a $WORK/$case/disk.img -t floppy $RO
date 03/01/1992
time 09:00:00
a:
cd \OPS
C:\WP51\WP
CONF
 SDL_AUDIODRIVER=dummy WP_MINT_STEPS="$FIX/$case.steps" WP_MINT_SHOTS="$OUT/shots" \
 LD_PRELOAD="$WORK/drive.so" dosbox-x -conf "$WORK/$case/wp.conf" -nogui -nomenu > "$WORK/$case/run.log" 2>&1
 fsck.fat -n "$WORK/$case/disk.img" > "$WORK/$case/fsck.txt"
done
for f in "$OUT/shots/"*.bmp; do
 name=$(basename "$f" .bmp)
 convert "$f" "$OUT/shots/$name.png"
 python3 "$DEC/mint/work/LISTF/decode.py" "$OUT/shots/$name.png" --attr > "$OUT/grids/$name.txt"
done
printf 'Captured real WP screens in %s; scratch and raw BMPs retained.\n' "$OUT"
