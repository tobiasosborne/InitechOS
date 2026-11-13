set -eu
printf ">>> test-int21-edge: INT 21h error paths (bad/closed handle, double-close, CX=0, short read) + 0Eh/19h/47h/59h/62h via mock backend (initech-xrd/1zk)\n"
build/test_int21_edge
printf ">>> test-int21-edge: green\n"
