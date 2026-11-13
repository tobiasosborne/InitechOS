set -eu
printf ">>> test-mcb-int21: AH=48h/49h/4Ah dispatch -- seam binding + segment conversion + owner == PSP + shrink-then-alloc + round-trip\n"
build/test_mcb_int21
printf ">>> test-mcb-int21: green\n"
