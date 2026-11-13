set -eu
printf ">>> test-int21: AH-dispatch + console subset (02h/09h/40h/30h/4Ch) + controlled scope\n"
build/test_int21
printf ">>> test-int21: green\n"
