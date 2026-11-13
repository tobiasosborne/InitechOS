set -eu
printf ">>> test-ansi: CSI parser + cursor + erase + SGR + save/restore + edge-clamp + mid-seq garbage\n"
build/test_ansi
printf ">>> test-ansi: green\n"
