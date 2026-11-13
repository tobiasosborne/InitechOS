set -eu
printf ">>> test-devwire: AH=3Dh OPEN-by-name CON/NUL/PRN/CLOCK$ + READ/WRITE routing + legacy-CON preserved\n"
build/test_devwire
printf ">>> test-devwire: green\n"
