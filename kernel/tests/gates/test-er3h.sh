set -eu
printf ">>> test-er3h: AH=33h GET/SET CTRL-BREAK (DL normalize, out-of-scope AL, boot default ON) + CONFIG BREAK= flow (DEC-16)\n"
build/test_int21
printf ">>> test-er3h: green\n"
