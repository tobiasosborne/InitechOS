set -eu
build/test_tsr
printf '>>> test-tsr-unit: green (KEEP top-level + MCB-hosted, files/blocks kept, next EXEC above, 31h twice, INT 27h, InDOS, IRQ gate type)\n'
