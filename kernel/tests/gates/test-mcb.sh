set -eu
printf ">>> test-mcb: MCB arena 48h/49h/4Ah -- chain invariants + first-fit + split/coalesce + setblock + data-integrity fuzz\n"
build/test_mcb
printf ">>> test-mcb: green\n"
