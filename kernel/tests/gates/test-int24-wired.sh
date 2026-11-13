set -eu
command -v mformat >/dev/null 2>&1 || { printf '!!! test-int24-wired FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
printf ">>> test-int24-wired: INT 24h raised by the disk layer + A/R/F honored (beads initech-mvg)\n"
dd if=/dev/zero of=build/int24_wired_blank.img bs=512 count=2880 status=none
mformat -i build/int24_wired_blank.img -f 1440 ::
build/test_int24_wired "build/int24_wired_blank.img"
printf ">>> test-int24-wired: green\n"
