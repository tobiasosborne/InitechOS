set -eu
command -v mformat >/dev/null 2>&1 || { printf '!!! test-fat-fault-rollback FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
printf ">>> test-fat-fault-rollback: FAT12 write-fault rollback atomicity -- nothing leaks (beads initech-lpf3)\n"
dd if=/dev/zero of=build/fat12_lpf3_fault.img bs=512 count=2880 status=none
mformat -i build/fat12_lpf3_fault.img -f 1440 ::
build/test_fat12_fault_rollback "build/fat12_lpf3_fault.img"
printf ">>> test-fat-fault-rollback: green\n"
