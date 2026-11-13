set -eu
printf ">>> test-x8fs: AH=3Fh on CON (handle 0) cooked line-read (line+CR+LF, inclusive count)\n"
build/test_conin
printf ">>> test-x8fs: green\n"
