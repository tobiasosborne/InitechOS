set -eu
printf ">>> test-psp: PSP 256-byte construction (int20/segs/jft/int21-entry/cmd-tail) + clamp + no-overflow\n"
build/test_psp
printf ">>> test-psp: green\n"
