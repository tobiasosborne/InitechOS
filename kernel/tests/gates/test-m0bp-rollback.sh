set -eu
command -v mformat >/dev/null 2>&1 || { printf '!!! test-m0bp-rollback FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
command -v mmd     >/dev/null 2>&1 || { printf '!!! test-m0bp-rollback FAIL: mtools `mmd` not found.\n'; exit 1; }
printf ">>> test-m0bp-rollback: MKDIR post-grow rollback atomicity (NO_SPACE) -- nothing leaks (beads initech-m0bp rollback fix)\n"
dd if=/dev/zero of=build/fat12_m0bp_rollback.img bs=512 count=2880 status=none
mformat -i build/fat12_m0bp_rollback.img -f 1440 ::
mmd -i build/fat12_m0bp_rollback.img ::SUB
build/test_fat12_mkdir_rollback "build/fat12_m0bp_rollback.img"
printf ">>> test-m0bp-rollback: green\n"
