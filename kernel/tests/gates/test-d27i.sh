set -eu
"${PYTHON_FAT_REFERENCE}" -c 'import sys; n=int(sys.argv[1]); sys.stdout.buffer.write(bytes(i & 0xFF for i in range(n)))' 2621440 > build/highclus.bin
command -v mkfs.fat >/dev/null 2>&1 || { printf '!!! fat16: mkfs.fat not found (apt install dosfstools).\n'; exit 1; }
dd if=/dev/zero of=build/fat16_fixture.img bs=512 count=16384 status=none
mkfs.fat -F 16 -s 1 -S 512 -r 512 -R 1 build/fat16_fixture.img >/dev/null
mcopy -i build/fat16_fixture.img kernel/tests/fixtures/hello.txt ::HELLO.TXT
mcopy -i build/fat16_fixture.img kernel/tests/fixtures/chain.txt ::CHAIN.TXT
mcopy -i build/fat16_fixture.img kernel/tests/fixtures/block.bin ::BLOCK.BIN
mcopy -i build/fat16_fixture.img kernel/tests/fixtures/bigchain.txt ::BIGCHAIN.TXT
mcopy -i build/fat16_fixture.img build/highclus.bin ::HIGHCLUS.BIN
printf ">>> fat16: minted %s (8MB FAT16, non-partitioned, fixtures + HIGHCLUS; build intermediate)\n" "build/fat16_fixture.img"
sh kernel/tests/disk/verify_fixture_volume.sh recipe "build/fat16_fixture.img" HELLO.TXT CHAIN.TXT BLOCK.BIN BIGCHAIN.TXT HIGHCLUS.BIN

dd if=/dev/zero of=build/fat12_fixture.img bs=512 count=2880 status=none
mformat -i build/fat12_fixture.img -f 1440 ::
mcopy -i build/fat12_fixture.img kernel/tests/fixtures/hello.txt ::HELLO.TXT
mcopy -i build/fat12_fixture.img kernel/tests/fixtures/second.txt ::SECOND.TXT
mcopy -i build/fat12_fixture.img kernel/tests/fixtures/chain.txt ::CHAIN.TXT
mcopy -i build/fat12_fixture.img kernel/tests/fixtures/empty.txt ::EMPTY.TXT
mcopy -i build/fat12_fixture.img kernel/tests/fixtures/block.bin ::BLOCK.BIN
mcopy -i build/fat12_fixture.img kernel/tests/fixtures/bigchain.txt ::BIGCHAIN.TXT
printf ">>> fat12: minted %s (1.44MB FAT12, fixtures copied; build intermediate)\n" "build/fat12_fixture.img"
sh kernel/tests/disk/verify_fixture_volume.sh recipe "build/fat12_fixture.img" HELLO.TXT SECOND.TXT CHAIN.TXT EMPTY.TXT BLOCK.BIN BIGCHAIN.TXT

printf '======================================================================\n'
printf 'InitechOS (InitechOS) -- make test-d27i : WINDOWED FAT-sector read\n'
printf '  Ref: PRD Sec 6.1 / ADR-0003 DEC-07. beads initech-d27i.\n'
printf '  FAT16 + FAT12 read through a 2-sector FAT window (no whole-FAT slurp).\n'
printf '======================================================================\n'
printf '>>> test-d27i: FAT16 windowed read + FAT12 straddle byte-identical\n'
build/test_fat16_window "build/fat16_fixture.img" "build/fat12_fixture.img" "kernel/tests/fixtures" \
	|| { printf '!!! test-d27i FAIL: windowed FAT-sector read oracle red\n'; exit 1; }
printf '>>> test-d27i: green (FAT16 mounts + reads windowed; FAT12 straddle byte-identical)\n'
