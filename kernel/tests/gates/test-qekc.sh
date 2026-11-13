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

command -v mdir    >/dev/null 2>&1 || { printf '!!! test-qekc FAIL: mtools `mdir` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
command -v "${PYTHON_FAT_REFERENCE:-python3}" >/dev/null 2>&1 || { printf '!!! test-qekc FAIL: "${PYTHON_FAT_REFERENCE:-python3}" not found (independent reference).\n'; exit 1; }
printf ">>> test-qekc: AH=57h SET FILE DATE/TIME persistence vs python --stat-path-time + mtools mdir (beads initech-qekc)\n"
sh kernel/tests/disk/verify_fixture_volume.sh harness "build/fat12_nested.img" HELLO.TXT SUB/NESTED.TXT SUB/DEEP/DEEP.TXT BIGDIR/FILE01.TXT BIGDIR/FILE02.TXT BIGDIR/FILE03.TXT BIGDIR/FILE04.TXT BIGDIR/FILE05.TXT BIGDIR/FILE06.TXT BIGDIR/FILE07.TXT BIGDIR/FILE08.TXT BIGDIR/FILE09.TXT BIGDIR/FILE10.TXT BIGDIR/FILE11.TXT BIGDIR/FILE12.TXT BIGDIR/FILE13.TXT BIGDIR/FILE14.TXT BIGDIR/FILE15.TXT BIGDIR/FILE16.TXT BIGDIR/FILE17.TXT BIGDIR/FILE18.TXT BIGDIR/FILE19.TXT BIGDIR/FILE20.TXT BIGDIR/FILE21.TXT BIGDIR/FILE22.TXT BIGDIR/FILE23.TXT BIGDIR/FILE24.TXT BIGDIR/FILE25.TXT BIGDIR/FILE26.TXT BIGDIR/FILE27.TXT BIGDIR/FILE28.TXT BIGDIR/FILE29.TXT BIGDIR/FILE30.TXT BIGDIR/FILE31.TXT BIGDIR/FILE32.TXT BIGDIR/FILE33.TXT BIGDIR/FILE34.TXT BIGDIR/FILE35.TXT BIGDIR/FILE36.TXT BIGDIR/FILE37.TXT BIGDIR/FILE38.TXT BIGDIR/FILE39.TXT BIGDIR/FILE40.TXT
cp build/fat12_nested.img build/fat12_qekc_scratch.img
sh kernel/tests/disk/verify_fixture_volume.sh harness "build/fat12_qekc_scratch.img" HELLO.TXT SUB/NESTED.TXT SUB/DEEP/DEEP.TXT BIGDIR/FILE01.TXT BIGDIR/FILE02.TXT BIGDIR/FILE03.TXT BIGDIR/FILE04.TXT BIGDIR/FILE05.TXT BIGDIR/FILE06.TXT BIGDIR/FILE07.TXT BIGDIR/FILE08.TXT BIGDIR/FILE09.TXT BIGDIR/FILE10.TXT BIGDIR/FILE11.TXT BIGDIR/FILE12.TXT BIGDIR/FILE13.TXT BIGDIR/FILE14.TXT BIGDIR/FILE15.TXT BIGDIR/FILE16.TXT BIGDIR/FILE17.TXT BIGDIR/FILE18.TXT BIGDIR/FILE19.TXT BIGDIR/FILE20.TXT BIGDIR/FILE21.TXT BIGDIR/FILE22.TXT BIGDIR/FILE23.TXT BIGDIR/FILE24.TXT BIGDIR/FILE25.TXT BIGDIR/FILE26.TXT BIGDIR/FILE27.TXT BIGDIR/FILE28.TXT BIGDIR/FILE29.TXT BIGDIR/FILE30.TXT BIGDIR/FILE31.TXT BIGDIR/FILE32.TXT BIGDIR/FILE33.TXT BIGDIR/FILE34.TXT BIGDIR/FILE35.TXT BIGDIR/FILE36.TXT BIGDIR/FILE37.TXT BIGDIR/FILE38.TXT BIGDIR/FILE39.TXT BIGDIR/FILE40.TXT
# (1) CREATE '\SUB\TSTAMP.TXT' (0/0 baseline) + AH=57h SET mtime/mdate + FLUSH.
build/test_fileio_subdir "build/fat12_qekc_scratch.img" "kernel/tests/fixtures" --write filetime-set
# python: the on-disk 0x16/0x18 packed words EXACTLY equal what SET wrote
# (0x4A6B 0x2C8D == 19051 11405). NO normalization -- the INVERSE oracle.
test "$("${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_qekc_scratch.img --stat-path-time 'SUB\TSTAMP.TXT')" = "19051 11405" \
	|| { printf '!!! test-qekc FAIL [SET]: python --stat-path-time != the written 19051 11405 (0x4A6B 0x2C8D)\n'; \
	     printf '    got: [%s]\n' "$("${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_qekc_scratch.img --stat-path-time 'SUB\TSTAMP.TXT')"; exit 1; }
# mtools: the SAME packed words rendered as the date 2002-04-13 + time 9:19
# (an INDEPENDENT decoder agreeing with python -- the repository contract two-reference rule).
mdir -i build/fat12_qekc_scratch.img ::SUB 2>/dev/null | grep -q 'TSTAMP  *TXT.*2002-04-13  *9:19' \
	|| { printf '!!! test-qekc FAIL [SET]: mtools mdir does not render TSTAMP.TXT as 2002-04-13 9:19 (the written 0x4A6B/0x2C8D)\n'; \
	     mdir -i build/fat12_qekc_scratch.img ::SUB 2>/dev/null | grep -i tstamp; exit 1; }
printf '>>> test-qekc [SET]: \\SUB\\TSTAMP.TXT on-disk mtime/mdate == 0x4A6B/0x2C8D (python==mtools==written)\n'
# (2) CONTENT intact: the time-set flush did NOT clobber the file body.
"${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_qekc_scratch.img --cat-path 'SUB\TSTAMP.TXT' > build/qekc_body.bin
"${PYTHON_FAT_REFERENCE:-python3}" -c 'import sys; e=bytes(97+(i%26) for i in range(64)); sys.exit(0 if open("build/qekc_body.bin","rb").read()==e else 1)' \
	|| { printf '!!! test-qekc FAIL [SET]: the file body changed after the time-set flush (data clobbered)\n'; exit 1; }
printf '>>> test-qekc [SET]: TSTAMP.TXT body reads back byte-exact (the time-set flush left data untouched)\n'
# (3) PERSISTENCE: a subsequent WRITE must PRESERVE the FILETIME-set stamp
# (fat12_write_partial patches only size/start_cluster, NOT mtime/mdate).
build/test_fileio_subdir "build/fat12_qekc_scratch.img" "kernel/tests/fixtures" --write filetime-persist >/dev/null 2>&1 || true
test "$("${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_qekc_scratch.img --stat-path-time 'SUB\TSTAMP.TXT')" = "19051 11405" \
	|| { printf '!!! test-qekc FAIL [PERSIST]: a later WRITE clobbered the FILETIME-set stamp (fat12_write_partial did not preserve mtime/mdate)\n'; exit 1; }
printf '>>> test-qekc [PERSIST]: a later WRITE preserved the FILETIME-set stamp (write_partial leaves 0x16/0x18 alone)\n'
printf ">>> test-qekc: green\n"
