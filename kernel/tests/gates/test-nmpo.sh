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

dd if=/dev/zero of=build/fat12_nested.img bs=512 count=2880 status=none
mformat -i build/fat12_nested.img -f 1440 ::
mmd -i build/fat12_nested.img ::SUB
mmd -i build/fat12_nested.img ::SUB/DEEP
mmd -i build/fat12_nested.img ::BIGDIR
mcopy -i build/fat12_nested.img kernel/tests/fixtures/hello.txt ::HELLO.TXT
mcopy -i build/fat12_nested.img kernel/tests/fixtures/nested.txt ::SUB/NESTED.TXT
mcopy -i build/fat12_nested.img kernel/tests/fixtures/deep.txt ::SUB/DEEP/DEEP.TXT
n=1; while [ $n -le 40 ]; do  	nn=$(printf '%02d' $n);  	mcopy -i build/fat12_nested.img kernel/tests/fixtures/big_fill.txt "::BIGDIR/FILE$nn.TXT"  		|| exit 1;  	n=$((n+1));  done
printf ">>> fat12: minted %s (nested SUB/DEEP/BIGDIR tree; build intermediate)\n" "build/fat12_nested.img"
sh kernel/tests/disk/verify_fixture_volume.sh recipe "build/fat12_nested.img" HELLO.TXT SUB/NESTED.TXT SUB/DEEP/DEEP.TXT BIGDIR/FILE01.TXT BIGDIR/FILE02.TXT BIGDIR/FILE03.TXT BIGDIR/FILE04.TXT BIGDIR/FILE05.TXT BIGDIR/FILE06.TXT BIGDIR/FILE07.TXT BIGDIR/FILE08.TXT BIGDIR/FILE09.TXT BIGDIR/FILE10.TXT BIGDIR/FILE11.TXT BIGDIR/FILE12.TXT BIGDIR/FILE13.TXT BIGDIR/FILE14.TXT BIGDIR/FILE15.TXT BIGDIR/FILE16.TXT BIGDIR/FILE17.TXT BIGDIR/FILE18.TXT BIGDIR/FILE19.TXT BIGDIR/FILE20.TXT BIGDIR/FILE21.TXT BIGDIR/FILE22.TXT BIGDIR/FILE23.TXT BIGDIR/FILE24.TXT BIGDIR/FILE25.TXT BIGDIR/FILE26.TXT BIGDIR/FILE27.TXT BIGDIR/FILE28.TXT BIGDIR/FILE29.TXT BIGDIR/FILE30.TXT BIGDIR/FILE31.TXT BIGDIR/FILE32.TXT BIGDIR/FILE33.TXT BIGDIR/FILE34.TXT BIGDIR/FILE35.TXT BIGDIR/FILE36.TXT BIGDIR/FILE37.TXT BIGDIR/FILE38.TXT BIGDIR/FILE39.TXT BIGDIR/FILE40.TXT

command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-nmpo FAIL: mtools `mcopy` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
command -v mdir    >/dev/null 2>&1 || { printf '!!! test-nmpo FAIL: mtools `mdir` not found.\n'; exit 1; }
command -v "${PYTHON_FAT_REFERENCE:-python3}" >/dev/null 2>&1 || { printf '!!! test-nmpo FAIL: "${PYTHON_FAT_REFERENCE:-python3}" not found (independent reference).\n'; exit 1; }
printf ">>> test-nmpo: AH=5Bh CREATNEW end-to-end (FRESH materialize + COLLISION reject + no-truncate) vs mtools + "${PYTHON_FAT_REFERENCE:-python3}" (beads initech-nmpo)\n"
sh kernel/tests/disk/verify_fixture_volume.sh harness "build/fat12_nested.img" HELLO.TXT SUB/NESTED.TXT SUB/DEEP/DEEP.TXT BIGDIR/FILE01.TXT BIGDIR/FILE02.TXT BIGDIR/FILE03.TXT BIGDIR/FILE04.TXT BIGDIR/FILE05.TXT BIGDIR/FILE06.TXT BIGDIR/FILE07.TXT BIGDIR/FILE08.TXT BIGDIR/FILE09.TXT BIGDIR/FILE10.TXT BIGDIR/FILE11.TXT BIGDIR/FILE12.TXT BIGDIR/FILE13.TXT BIGDIR/FILE14.TXT BIGDIR/FILE15.TXT BIGDIR/FILE16.TXT BIGDIR/FILE17.TXT BIGDIR/FILE18.TXT BIGDIR/FILE19.TXT BIGDIR/FILE20.TXT BIGDIR/FILE21.TXT BIGDIR/FILE22.TXT BIGDIR/FILE23.TXT BIGDIR/FILE24.TXT BIGDIR/FILE25.TXT BIGDIR/FILE26.TXT BIGDIR/FILE27.TXT BIGDIR/FILE28.TXT BIGDIR/FILE29.TXT BIGDIR/FILE30.TXT BIGDIR/FILE31.TXT BIGDIR/FILE32.TXT BIGDIR/FILE33.TXT BIGDIR/FILE34.TXT BIGDIR/FILE35.TXT BIGDIR/FILE36.TXT BIGDIR/FILE37.TXT BIGDIR/FILE38.TXT BIGDIR/FILE39.TXT BIGDIR/FILE40.TXT
cp build/fat12_nested.img build/fat12_zs24_scratch.img
sh kernel/tests/disk/verify_fixture_volume.sh harness "build/fat12_zs24_scratch.img" HELLO.TXT SUB/NESTED.TXT SUB/DEEP/DEEP.TXT BIGDIR/FILE01.TXT BIGDIR/FILE02.TXT BIGDIR/FILE03.TXT BIGDIR/FILE04.TXT BIGDIR/FILE05.TXT BIGDIR/FILE06.TXT BIGDIR/FILE07.TXT BIGDIR/FILE08.TXT BIGDIR/FILE09.TXT BIGDIR/FILE10.TXT BIGDIR/FILE11.TXT BIGDIR/FILE12.TXT BIGDIR/FILE13.TXT BIGDIR/FILE14.TXT BIGDIR/FILE15.TXT BIGDIR/FILE16.TXT BIGDIR/FILE17.TXT BIGDIR/FILE18.TXT BIGDIR/FILE19.TXT BIGDIR/FILE20.TXT BIGDIR/FILE21.TXT BIGDIR/FILE22.TXT BIGDIR/FILE23.TXT BIGDIR/FILE24.TXT BIGDIR/FILE25.TXT BIGDIR/FILE26.TXT BIGDIR/FILE27.TXT BIGDIR/FILE28.TXT BIGDIR/FILE29.TXT BIGDIR/FILE30.TXT BIGDIR/FILE31.TXT BIGDIR/FILE32.TXT BIGDIR/FILE33.TXT BIGDIR/FILE34.TXT BIGDIR/FILE35.TXT BIGDIR/FILE36.TXT BIGDIR/FILE37.TXT BIGDIR/FILE38.TXT BIGDIR/FILE39.TXT BIGDIR/FILE40.TXT
# CREATNEW '\SUB\NEW5B.TXT' (700B, 2 clusters); re-CREATNEW collides 0x0050; no truncation.
build/test_fileio_subdir "build/fat12_zs24_scratch.img" "kernel/tests/fixtures" --write creatnew
mcopy -n -i build/fat12_zs24_scratch.img ::SUB/NEW5B.TXT build/nmpo_mcopy.bin
"${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_zs24_scratch.img --cat-path 'SUB\NEW5B.TXT' > build/nmpo_py.bin
"${PYTHON_FAT_REFERENCE:-python3}" -c 'import sys; exp=bytes(65+(i%26) for i in range(700)); m=open("build/nmpo_mcopy.bin","rb").read(); p=open("build/nmpo_py.bin","rb").read(); sys.exit(0 if (m==exp and p==exp and m==p) else 1)' \
	|| { printf '!!! test-nmpo FAIL: CREATNEW-materialized NEW5B.TXT read-back != written (mcopy/python disagree -- a phantom or truncated create)\n'; exit 1; }
mdir -i build/fat12_zs24_scratch.img ::SUB 2>/dev/null | grep -q 'NEW5B *TXT' \
	|| { printf '!!! test-nmpo FAIL: mdir ::SUB does not list NEW5B.TXT (CREATNEW did not materialize)\n'; exit 1; }
printf '>>> test-nmpo: green (NEW5B.TXT materialized in \\SUB, 700B byte-exact; re-CREATNEW collided 0x0050; no truncation -- mcopy==python==written)\n'
