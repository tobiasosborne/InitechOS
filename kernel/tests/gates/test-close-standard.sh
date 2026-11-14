set -eu
(cd kernel/tests/fixtures/close3-dos33 && sha256sum -c SHA256SUMS)
nasm -f bin kernel/tests/fixtures/close3-dos33/CLOSE3.ASM -o build/close-standard-reference.com
cmp kernel/tests/fixtures/close3-dos33/CLOSE3.COM build/close-standard-reference.com
grep -F CLOSE3-OK kernel/tests/fixtures/close3-dos33/result.txt >/dev/null
build/test_close_standard
