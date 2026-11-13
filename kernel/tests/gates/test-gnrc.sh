set -eu
dd if=/dev/zero of=build/fat12_rename_golden.img bs=512 count=2880 status=none
mformat -i build/fat12_rename_golden.img -f 1440 ::
mcopy -i build/fat12_rename_golden.img kernel/tests/fixtures/chain.txt ::OLD.TXT
mren -i build/fat12_rename_golden.img ::OLD.TXT ::NEW.BAK
printf ">>> fat12: minted %s (mren ::OLD.TXT ::NEW.BAK golden; build intermediate)\n" "build/fat12_rename_golden.img"
sh kernel/tests/disk/verify_fixture_volume.sh recipe "build/fat12_rename_golden.img" NEW.BAK

dd if=/dev/zero of=build/fat12_rename_subdir_golden.img bs=512 count=2880 status=none
mformat -i build/fat12_rename_subdir_golden.img -f 1440 ::
mmd -i build/fat12_rename_subdir_golden.img ::SUB
mcopy -i build/fat12_rename_subdir_golden.img kernel/tests/fixtures/chain.txt ::SUB/OLD2.TXT
mren -i build/fat12_rename_subdir_golden.img ::SUB/OLD2.TXT ::SUB/NEW2.BAK
printf ">>> fat12: minted %s (mren ::SUB/OLD2.TXT ::SUB/NEW2.BAK subdir golden; build intermediate)\n" "build/fat12_rename_subdir_golden.img"
sh kernel/tests/disk/verify_fixture_volume.sh recipe "build/fat12_rename_subdir_golden.img" SUB/NEW2.BAK

command -v mren >/dev/null 2>&1 || { printf '!!! test-gnrc FAIL: mtools `mren` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
command -v mmd  >/dev/null 2>&1 || { printf '!!! test-gnrc FAIL: mtools `mmd` not found.\n'; exit 1; }
printf ">>> test-gnrc: AH=56h RENAME write side vs mren golden (name-field byte-for-byte; chain/size/FAT untouched) + NON-ROOT same-dir leg (beads initech-isil)\n"
dd if=/dev/zero of=build/fat12_rename_art.img bs=512 count=2880 status=none
mformat -i build/fat12_rename_art.img -f 1440 ::
mcopy -i build/fat12_rename_art.img kernel/tests/fixtures/chain.txt ::OLD.TXT
sh kernel/tests/disk/verify_fixture_volume.sh recipe "build/fat12_rename_art.img" OLD.TXT
dd if=/dev/zero of=build/fat12_rename_subdir_art.img bs=512 count=2880 status=none
mformat -i build/fat12_rename_subdir_art.img -f 1440 ::
mmd -i build/fat12_rename_subdir_art.img ::SUB
mcopy -i build/fat12_rename_subdir_art.img kernel/tests/fixtures/chain.txt ::SUB/OLD2.TXT
sh kernel/tests/disk/verify_fixture_volume.sh recipe "build/fat12_rename_subdir_art.img" SUB/OLD2.TXT
build/test_fat12_rename "build/fat12_rename_art.img" "build/fat12_rename_golden.img" \
	"build/fat12_rename_subdir_art.img" "build/fat12_rename_subdir_golden.img"
printf ">>> test-gnrc: green\n"
