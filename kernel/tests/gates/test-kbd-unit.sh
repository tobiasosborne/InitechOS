set -eu
printf ">>> test-kbd-unit: ring (enqueue/dequeue/empty/full/wrap) + scancode set 1 -> ASCII (+shift/caps) + PIT divisor math\n"
build/test_kbd
printf ">>> test-kbd-unit: green\n"
