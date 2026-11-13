set -eu
dd if=/dev/zero of=build/fat12_blank_unit.img bs=512 count=2880 status=none
mformat -i build/fat12_blank_unit.img -f 1440 ::
printf ">>> fat12: minted BLANK %s (1.44MB FAT12, no files; WRITE unit target)\n" "build/fat12_blank_unit.img"

printf ">>> test-fat12-write: FAT12 WRITE unit (12-bit encode + alloc + both-FAT sync + round-trip + full-volume fail-loud + unlink)\n"
cp -f "build/fat12_blank_unit.img" "build/fat12_write_unit_run.img"
build/test_fat12_write "build/fat12_write_unit_run.img"
printf ">>> test-fat12-write: green\n"
