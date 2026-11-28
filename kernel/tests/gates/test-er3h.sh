set -eu
printf ">>> test-er3h: AH=33h GET/SET CTRL-BREAK and CONFIG BREAK= flow\n"
build/test_int21
printf ">>> test-er3h: green\n"
