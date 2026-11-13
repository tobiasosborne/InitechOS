set -eu
printf '>>> test-fpu-probe: x87 probe verdict + CR0 arithmetic (kernel/fpu.h; beads initech-zj6w)\n'
build/test_fpu_probe
