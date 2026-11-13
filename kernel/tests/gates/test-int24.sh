set -eu
printf ">>> test-int24: INT 22/23/24 + SETVECT/GETVECT (25h/35h) + PSP-vector save/restore (DEC-10)\n"
build/test_int24
printf ">>> test-int24: green\n"
