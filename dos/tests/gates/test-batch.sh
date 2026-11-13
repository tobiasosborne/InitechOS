set -eu
printf ">>> test-batch: expand (%%0-9/%%VAR%%/%%%%) + classify + label_matches\n"
build/test_batch
printf ">>> test-batch: green\n"
