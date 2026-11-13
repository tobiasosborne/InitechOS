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

command -v mattrib >/dev/null 2>&1 || { printf '!!! test-b53d FAIL: mtools `mattrib` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
command -v "${PYTHON_FAT_REFERENCE:-python3}" >/dev/null 2>&1 || { printf '!!! test-b53d FAIL: "${PYTHON_FAT_REFERENCE:-python3}" not found (independent reference).\n'; exit 1; }
printf ">>> test-b53d: AH=43h CHMOD GET/SET ATTR persistence vs python --attr + mtools mattrib (beads initech-b53d)\n"
sh kernel/tests/disk/verify_fixture_volume.sh harness "build/fat12_nested.img" HELLO.TXT SUB/NESTED.TXT SUB/DEEP/DEEP.TXT BIGDIR/FILE01.TXT BIGDIR/FILE02.TXT BIGDIR/FILE03.TXT BIGDIR/FILE04.TXT BIGDIR/FILE05.TXT BIGDIR/FILE06.TXT BIGDIR/FILE07.TXT BIGDIR/FILE08.TXT BIGDIR/FILE09.TXT BIGDIR/FILE10.TXT BIGDIR/FILE11.TXT BIGDIR/FILE12.TXT BIGDIR/FILE13.TXT BIGDIR/FILE14.TXT BIGDIR/FILE15.TXT BIGDIR/FILE16.TXT BIGDIR/FILE17.TXT BIGDIR/FILE18.TXT BIGDIR/FILE19.TXT BIGDIR/FILE20.TXT BIGDIR/FILE21.TXT BIGDIR/FILE22.TXT BIGDIR/FILE23.TXT BIGDIR/FILE24.TXT BIGDIR/FILE25.TXT BIGDIR/FILE26.TXT BIGDIR/FILE27.TXT BIGDIR/FILE28.TXT BIGDIR/FILE29.TXT BIGDIR/FILE30.TXT BIGDIR/FILE31.TXT BIGDIR/FILE32.TXT BIGDIR/FILE33.TXT BIGDIR/FILE34.TXT BIGDIR/FILE35.TXT BIGDIR/FILE36.TXT BIGDIR/FILE37.TXT BIGDIR/FILE38.TXT BIGDIR/FILE39.TXT BIGDIR/FILE40.TXT
cp build/fat12_nested.img build/fat12_b53d_scratch.img
sh kernel/tests/disk/verify_fixture_volume.sh harness "build/fat12_b53d_scratch.img" HELLO.TXT SUB/NESTED.TXT SUB/DEEP/DEEP.TXT BIGDIR/FILE01.TXT BIGDIR/FILE02.TXT BIGDIR/FILE03.TXT BIGDIR/FILE04.TXT BIGDIR/FILE05.TXT BIGDIR/FILE06.TXT BIGDIR/FILE07.TXT BIGDIR/FILE08.TXT BIGDIR/FILE09.TXT BIGDIR/FILE10.TXT BIGDIR/FILE11.TXT BIGDIR/FILE12.TXT BIGDIR/FILE13.TXT BIGDIR/FILE14.TXT BIGDIR/FILE15.TXT BIGDIR/FILE16.TXT BIGDIR/FILE17.TXT BIGDIR/FILE18.TXT BIGDIR/FILE19.TXT BIGDIR/FILE20.TXT BIGDIR/FILE21.TXT BIGDIR/FILE22.TXT BIGDIR/FILE23.TXT BIGDIR/FILE24.TXT BIGDIR/FILE25.TXT BIGDIR/FILE26.TXT BIGDIR/FILE27.TXT BIGDIR/FILE28.TXT BIGDIR/FILE29.TXT BIGDIR/FILE30.TXT BIGDIR/FILE31.TXT BIGDIR/FILE32.TXT BIGDIR/FILE33.TXT BIGDIR/FILE34.TXT BIGDIR/FILE35.TXT BIGDIR/FILE36.TXT BIGDIR/FILE37.TXT BIGDIR/FILE38.TXT BIGDIR/FILE39.TXT BIGDIR/FILE40.TXT
# (1) CREATE '\SUB\ATTR.TXT' (0x20 baseline) + AH=43h SET RO|HIDDEN (0x03) +
# FLUSH; the harness self-checks the 3 in-process read-backs + the dir/missing
# reject set (10 CHECKs). A non-zero failure count fails the gate.
build/test_fileio_subdir "build/fat12_b53d_scratch.img" "kernel/tests/fixtures" --write attr-set > build/b53d_attrset.report 2>&1; \
	grep -Eq 'test_fileio_subdir: [0-9]+ checks, 0 failures' build/b53d_attrset.report \
	|| { printf '!!! test-b53d FAIL [SET]: the attr-set harness reported failures (in-process read-back / reject set):\n'; cat build/b53d_attrset.report; exit 1; }
printf '>>> test-b53d [SET]: \\SUB\\ATTR.TXT on-disk attr set to 0x03 (dispatch GET + fat12_get_attr + resolved entry agree; dir/missing rejects fire)\n'
# (2) python: the raw on-disk attribute byte (ent[11]) EXACTLY == 3 (0x03).
test "$("${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_b53d_scratch.img --attr 'SUB\ATTR.TXT')" = "3" \
	|| { printf '!!! test-b53d FAIL [SET]: python --attr != 3 (the written RO|HIDDEN 0x03)\n'; \
	     printf '    got: [%s]\n' "$("${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_b53d_scratch.img --attr 'SUB\ATTR.TXT')"; exit 1; }
# (3) mtools mattrib: the SAME byte rendered as the flag string "HR"
# (Hidden + Read-only) -- an INDEPENDENT decoder agreeing with python.
mattrib -i build/fat12_b53d_scratch.img ::SUB/ATTR.TXT 2>/dev/null | grep -Eq '^ +HR +' \
	|| { printf '!!! test-b53d FAIL [SET]: mtools mattrib does not render ATTR.TXT as HR (Hidden+Read-only == 0x03)\n'; \
	     mattrib -i build/fat12_b53d_scratch.img ::SUB/ATTR.TXT 2>/dev/null; exit 1; }
printf '>>> test-b53d [SET]: \\SUB\\ATTR.TXT on-disk attr == 0x03 (python --attr==3 == mtools mattrib HR == fat12_get_attr)\n'
# (4) the repository contract: the time-set... ahem, the ATTR-set flush touched ONLY the 0x0B
# byte -- the BODY and mtime/mdate are untouched (deterministic 0/0 baseline).
"${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_b53d_scratch.img --cat-path 'SUB\ATTR.TXT' > build/b53d_body.bin
"${PYTHON_FAT_REFERENCE:-python3}" -c 'import sys; e=bytes(65+((i%26)) for i in range(48)); sys.exit(0 if open("build/b53d_body.bin","rb").read()==e else 1)' \
	|| { printf '!!! test-b53d FAIL [SET]: the file body changed after the attr-set flush (data clobbered)\n'; exit 1; }
test "$("${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_b53d_scratch.img --stat-path-time 'SUB\ATTR.TXT')" = "0 0" \
	|| { printf '!!! test-b53d FAIL [SET]: the attr-set flush DISTURBED mtime/mdate (the repository contract violated -- the RMW must touch ONLY 0x0B)\n'; exit 1; }
printf '>>> test-b53d [SET]: ATTR.TXT body + mtime/mdate untouched by CHMOD (attribute-byte-only RMW, the repository contract)\n'
# (5) GET-on-DIRECTORY fidelity (beads initech-b53d fix): real DOS AH=4300h on
# a directory SUCCEEDS with CX=0x10 (RBIL, no directory exclusion). The harness
# attr-set self-check ALREADY proved the in-process legs (AH=43h GET '\SUB\DEEP'
# -> CF=0/CX=0x10 AND fat12_get_attr off the cached FAT -> 0x10). Here the TWO
# INDEPENDENT references must AGREE the DEEP directory reads 0x10 -- the 3-way
# differential (fat12_get_attr / python --attr / mtools mattrib):
#   python --attr: the raw on-disk attribute byte (ent[11]) EXACTLY == 16 (0x10)
test "$("${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_b53d_scratch.img --attr 'SUB\DEEP')" = "16" \
	|| { printf '!!! test-b53d FAIL [GET-dir]: python --attr SUB\\DEEP != 16 (the dir 0x10 byte)\n'; \
	     printf '    got: [%s]\n' "$("${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_b53d_scratch.img --attr 'SUB\DEEP')"; exit 1; }
#   mtools mattrib: a directory carries NO R/H/S/A flag letters (the 0x10
#   Directory bit alone) -- an INDEPENDENT decoder agreeing it is not R/H/S/A.
#   mattrib prints "<flags>   ::/path"; isolate the FLAGS COLUMN (everything
#   BEFORE '::') so the path text 'SUB' is not mistaken for flag letters.
mattrib -i build/fat12_b53d_scratch.img ::SUB/DEEP 2>/dev/null | sed 's/::.*//' | grep -Eq '[RHSA]' \
	&& { printf '!!! test-b53d FAIL [GET-dir]: mtools mattrib shows R/H/S/A flags on the DEEP directory (expected bare 0x10)\n'; \
	     mattrib -i build/fat12_b53d_scratch.img ::SUB/DEEP 2>/dev/null; exit 1; } || true
printf '>>> test-b53d [GET-dir]: \\SUB\\DEEP reads 0x10 (fat12_get_attr == python --attr==16 == mtools mattrib bare; GET-on-dir succeeds, RBIL 4300h)\n'
printf ">>> test-b53d: green\n"
