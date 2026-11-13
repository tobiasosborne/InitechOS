set -eu
printf ">>> test-80k: DOS 8.3 FCB wildcard match (*.*, A*.*, *.TXT, FOO?.*, ?.BAR) + ?-over-pad quirks\n"
build/test_int21_wildcard
printf ">>> test-80k: green\n"
