set -eu
dd if=/dev/zero of=build/fat12_mkdir_golden.img bs=512 count=2880 status=none
mformat -i build/fat12_mkdir_golden.img -f 1440 ::
mmd -i build/fat12_mkdir_golden.img ::NEWDIR
printf ">>> fat12: minted %s (mmd ::NEWDIR golden; build intermediate)\n" "build/fat12_mkdir_golden.img"

printf ">>> test-fat12-mkdir: MKDIR/RMDIR write side vs mmd golden ('.'/'..'/EOC byte-for-byte)\n"
dd if=/dev/zero of=build/fat12_mkdir_blank.img bs=512 count=2880 status=none
mformat -i build/fat12_mkdir_blank.img -f 1440 ::
build/test_fat12_mkdir "build/fat12_mkdir_blank.img" "build/fat12_mkdir_golden.img"
printf ">>> test-fat12-mkdir: green\n"
