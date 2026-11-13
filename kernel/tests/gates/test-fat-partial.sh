set -eu
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
printf 'InitechOS (InitechOS) -- make test-fat-partial : FAT12 positioned-read oracle\n'
printf '======================================================================\n'
command -v "${PYTHON_FAT_REFERENCE:-python3}" >/dev/null 2>&1 || { printf '!!! test-fat-partial FAIL: "${PYTHON_FAT_REFERENCE:-python3}" not found (needed for the independent --cat-range reference).\n'; exit 1; }
command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-fat-partial FAIL: mtools `mcopy` not found (apt install mtools).\n'; exit 1; }
printf '>>> test-fat-partial [A]: host matrix -- fat12_read_partial == fixture slice (byte-for-byte)\n'
build/test_fat12_partial "build/fat12_fixture.img" "kernel/tests/fixtures" \
	|| { printf '!!! test-fat-partial FAIL [A]: host matrix oracle red\n'; exit 1; }
printf '>>> test-fat-partial [A]: green\n'
printf '>>> test-fat-partial [B]: "${PYTHON_FAT_REFERENCE:-python3}" --cat-range == mcopy+dd of the SAME range (reference proven on real FAT12)\n'
set -e; \
for spec in "CHAIN.TXT 0 64" "CHAIN.TXT 500 100" "CHAIN.TXT 600 500" \
            "CHAIN.TXT 1024 512" "CHAIN.TXT 1550 200" "CHAIN.TXT 1 1599" \
            "HELLO.TXT 4 10" "BLOCK.BIN 512 512" "BLOCK.BIN 1000 4096" \
            "SECOND.TXT 50 200"; do \
	set -- $spec; nm="$1"; off="$2"; ln="$3"; \
	"${PYTHON_FAT_REFERENCE:-python3}" "kernel/tests/disk/fat12_ref.py" "build/fat12_fixture.img" --cat-range "$nm" "$off" "$ln" \
		> "build/fatp_py_${nm}_${off}_${ln}.bin" \
		|| { printf '!!! test-fat-partial FAIL [B]: python --cat-range %s %s %s errored\n' "$nm" "$off" "$ln"; exit 1; }; \
	mcopy -n -i "build/fat12_fixture.img" "::$nm" "build/fatp_whole_$nm.bin" \
		|| { printf '!!! test-fat-partial FAIL [B]: mcopy ::%s errored\n' "$nm"; exit 1; }; \
	dd if="build/fatp_whole_$nm.bin" of="build/fatp_dd_${nm}_${off}_${ln}.bin" \
		bs=1 skip="$off" count="$ln" status=none 2>/dev/null || true; \
	cmp -s "build/fatp_py_${nm}_${off}_${ln}.bin" "build/fatp_dd_${nm}_${off}_${ln}.bin" \
		|| { printf '!!! test-fat-partial FAIL [B]: %s [%s,+%s) -- python ref bytes != dd-of-mcopy bytes\n' "$nm" "$off" "$ln"; \
		     cmp "build/fatp_py_${nm}_${off}_${ln}.bin" "build/fatp_dd_${nm}_${off}_${ln}.bin"; exit 1; }; \
done
printf '>>> test-fat-partial [B]: green (python --cat-range agrees with mcopy+dd on every range)\n'
printf '>>> test-fat-partial: green\n'
