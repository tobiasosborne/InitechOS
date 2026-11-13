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

command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-zs24 FAIL: mtools `mcopy` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
command -v mdir    >/dev/null 2>&1 || { printf '!!! test-zs24 FAIL: mtools `mdir` not found.\n'; exit 1; }
command -v "${PYTHON_FAT_REFERENCE:-python3}" >/dev/null 2>&1 || { printf '!!! test-zs24 FAIL: "${PYTHON_FAT_REFERENCE:-python3}" not found (independent reference).\n'; exit 1; }
printf ">>> test-zs24: subdir file CREATE/WRITE/LSEEK-WRITE/UNLINK vs mtools + "${PYTHON_FAT_REFERENCE:-python3}" (beads initech-zs24)\n"
sh kernel/tests/disk/verify_fixture_volume.sh harness "build/fat12_nested.img" HELLO.TXT SUB/NESTED.TXT SUB/DEEP/DEEP.TXT BIGDIR/FILE01.TXT BIGDIR/FILE02.TXT BIGDIR/FILE03.TXT BIGDIR/FILE04.TXT BIGDIR/FILE05.TXT BIGDIR/FILE06.TXT BIGDIR/FILE07.TXT BIGDIR/FILE08.TXT BIGDIR/FILE09.TXT BIGDIR/FILE10.TXT BIGDIR/FILE11.TXT BIGDIR/FILE12.TXT BIGDIR/FILE13.TXT BIGDIR/FILE14.TXT BIGDIR/FILE15.TXT BIGDIR/FILE16.TXT BIGDIR/FILE17.TXT BIGDIR/FILE18.TXT BIGDIR/FILE19.TXT BIGDIR/FILE20.TXT BIGDIR/FILE21.TXT BIGDIR/FILE22.TXT BIGDIR/FILE23.TXT BIGDIR/FILE24.TXT BIGDIR/FILE25.TXT BIGDIR/FILE26.TXT BIGDIR/FILE27.TXT BIGDIR/FILE28.TXT BIGDIR/FILE29.TXT BIGDIR/FILE30.TXT BIGDIR/FILE31.TXT BIGDIR/FILE32.TXT BIGDIR/FILE33.TXT BIGDIR/FILE34.TXT BIGDIR/FILE35.TXT BIGDIR/FILE36.TXT BIGDIR/FILE37.TXT BIGDIR/FILE38.TXT BIGDIR/FILE39.TXT BIGDIR/FILE40.TXT
cp build/fat12_nested.img build/fat12_zs24_scratch.img
sh kernel/tests/disk/verify_fixture_volume.sh harness "build/fat12_zs24_scratch.img" HELLO.TXT SUB/NESTED.TXT SUB/DEEP/DEEP.TXT BIGDIR/FILE01.TXT BIGDIR/FILE02.TXT BIGDIR/FILE03.TXT BIGDIR/FILE04.TXT BIGDIR/FILE05.TXT BIGDIR/FILE06.TXT BIGDIR/FILE07.TXT BIGDIR/FILE08.TXT BIGDIR/FILE09.TXT BIGDIR/FILE10.TXT BIGDIR/FILE11.TXT BIGDIR/FILE12.TXT BIGDIR/FILE13.TXT BIGDIR/FILE14.TXT BIGDIR/FILE15.TXT BIGDIR/FILE16.TXT BIGDIR/FILE17.TXT BIGDIR/FILE18.TXT BIGDIR/FILE19.TXT BIGDIR/FILE20.TXT BIGDIR/FILE21.TXT BIGDIR/FILE22.TXT BIGDIR/FILE23.TXT BIGDIR/FILE24.TXT BIGDIR/FILE25.TXT BIGDIR/FILE26.TXT BIGDIR/FILE27.TXT BIGDIR/FILE28.TXT BIGDIR/FILE29.TXT BIGDIR/FILE30.TXT BIGDIR/FILE31.TXT BIGDIR/FILE32.TXT BIGDIR/FILE33.TXT BIGDIR/FILE34.TXT BIGDIR/FILE35.TXT BIGDIR/FILE36.TXT BIGDIR/FILE37.TXT BIGDIR/FILE38.TXT BIGDIR/FILE39.TXT BIGDIR/FILE40.TXT
# (1) CREATE+WRITE '\SUB\NEW.TXT' (700 bytes, spans 2 clusters).
build/test_fileio_subdir "build/fat12_zs24_scratch.img" "kernel/tests/fixtures" --write create-write
mcopy -n -i build/fat12_zs24_scratch.img ::SUB/NEW.TXT build/zs24_mcopy.bin
"${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_zs24_scratch.img --cat-path 'SUB\NEW.TXT' > build/zs24_py.bin
"${PYTHON_FAT_REFERENCE:-python3}" -c 'import sys; exp=bytes(65+(i%26) for i in range(700)); m=open("build/zs24_mcopy.bin","rb").read(); p=open("build/zs24_py.bin","rb").read(); sys.exit(0 if (m==exp and p==exp and m==p) else 1)' \
	|| { printf '!!! test-zs24 FAIL [CREATE+WRITE]: mcopy/python read-back != written bytes\n'; exit 1; }
mdir -i build/fat12_zs24_scratch.img ::SUB 2>/dev/null | grep -q 'NEW *TXT' \
	|| { printf '!!! test-zs24 FAIL [CREATE+WRITE]: mdir ::SUB does not list NEW.TXT\n'; exit 1; }
printf '>>> test-zs24 [CREATE+WRITE]: NEW.TXT (700B, 2 clusters) lands in \\SUB; mcopy==python==written\n'
# (2) Positioned LSEEK+WRITE: splice 50 bytes at offset 600.
build/test_fileio_subdir "build/fat12_zs24_scratch.img" "kernel/tests/fixtures" --write seek-write
mcopy -n -i build/fat12_zs24_scratch.img ::SUB/NEW.TXT build/zs24_mcopy2.bin
"${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_zs24_scratch.img --cat-path 'SUB\NEW.TXT' > build/zs24_py2.bin
"${PYTHON_FAT_REFERENCE:-python3}" -c 'import sys; e=bytearray(65+(i%26) for i in range(700)); e[600:650]=bytes(48+(i%10) for i in range(50)); e=bytes(e); m=open("build/zs24_mcopy2.bin","rb").read(); p=open("build/zs24_py2.bin","rb").read(); sys.exit(0 if (m==e and p==e and m==p) else 1)' \
	|| { printf '!!! test-zs24 FAIL [LSEEK+WRITE]: spliced read-back != expected\n'; exit 1; }
printf '>>> test-zs24 [LSEEK+WRITE]: positioned splice at offset 600 byte-exact (mcopy==python)\n'
# (3) UNLINK '\SUB\NEW.TXT' -> mdir ::SUB no longer lists it; python list == only NESTED.TXT.
build/test_fileio_subdir "build/fat12_zs24_scratch.img" "kernel/tests/fixtures" --write unlink
if mdir -i build/fat12_zs24_scratch.img ::SUB 2>/dev/null | grep -q 'NEW *TXT'; then \
	printf '!!! test-zs24 FAIL [UNLINK]: mdir ::SUB STILL lists NEW.TXT after UNLINK\n'; exit 1; fi
"${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_zs24_scratch.img --list-path 'SUB' | grep -q '^NESTED.TXT ' \
	|| { printf '!!! test-zs24 FAIL [UNLINK]: python --list-path SUB lost the original NESTED.TXT\n'; exit 1; }
if "${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_zs24_scratch.img --list-path 'SUB' | grep -q '^NEW.TXT '; then \
	printf '!!! test-zs24 FAIL [UNLINK]: python --list-path SUB STILL lists NEW.TXT\n'; exit 1; fi
printf '>>> test-zs24 [UNLINK]: NEW.TXT removed from \\SUB; NESTED.TXT intact (mtools + python agree)\n'
# (4) ROOT regression: a root CREATE+WRITE still works under the generalized primitives.
sh kernel/tests/disk/verify_fixture_volume.sh harness "build/fat12_nested.img" HELLO.TXT SUB/NESTED.TXT SUB/DEEP/DEEP.TXT BIGDIR/FILE01.TXT BIGDIR/FILE02.TXT BIGDIR/FILE03.TXT BIGDIR/FILE04.TXT BIGDIR/FILE05.TXT BIGDIR/FILE06.TXT BIGDIR/FILE07.TXT BIGDIR/FILE08.TXT BIGDIR/FILE09.TXT BIGDIR/FILE10.TXT BIGDIR/FILE11.TXT BIGDIR/FILE12.TXT BIGDIR/FILE13.TXT BIGDIR/FILE14.TXT BIGDIR/FILE15.TXT BIGDIR/FILE16.TXT BIGDIR/FILE17.TXT BIGDIR/FILE18.TXT BIGDIR/FILE19.TXT BIGDIR/FILE20.TXT BIGDIR/FILE21.TXT BIGDIR/FILE22.TXT BIGDIR/FILE23.TXT BIGDIR/FILE24.TXT BIGDIR/FILE25.TXT BIGDIR/FILE26.TXT BIGDIR/FILE27.TXT BIGDIR/FILE28.TXT BIGDIR/FILE29.TXT BIGDIR/FILE30.TXT BIGDIR/FILE31.TXT BIGDIR/FILE32.TXT BIGDIR/FILE33.TXT BIGDIR/FILE34.TXT BIGDIR/FILE35.TXT BIGDIR/FILE36.TXT BIGDIR/FILE37.TXT BIGDIR/FILE38.TXT BIGDIR/FILE39.TXT BIGDIR/FILE40.TXT
cp build/fat12_nested.img build/fat12_zs24_scratch.img
sh kernel/tests/disk/verify_fixture_volume.sh harness "build/fat12_zs24_scratch.img" HELLO.TXT SUB/NESTED.TXT SUB/DEEP/DEEP.TXT BIGDIR/FILE01.TXT BIGDIR/FILE02.TXT BIGDIR/FILE03.TXT BIGDIR/FILE04.TXT BIGDIR/FILE05.TXT BIGDIR/FILE06.TXT BIGDIR/FILE07.TXT BIGDIR/FILE08.TXT BIGDIR/FILE09.TXT BIGDIR/FILE10.TXT BIGDIR/FILE11.TXT BIGDIR/FILE12.TXT BIGDIR/FILE13.TXT BIGDIR/FILE14.TXT BIGDIR/FILE15.TXT BIGDIR/FILE16.TXT BIGDIR/FILE17.TXT BIGDIR/FILE18.TXT BIGDIR/FILE19.TXT BIGDIR/FILE20.TXT BIGDIR/FILE21.TXT BIGDIR/FILE22.TXT BIGDIR/FILE23.TXT BIGDIR/FILE24.TXT BIGDIR/FILE25.TXT BIGDIR/FILE26.TXT BIGDIR/FILE27.TXT BIGDIR/FILE28.TXT BIGDIR/FILE29.TXT BIGDIR/FILE30.TXT BIGDIR/FILE31.TXT BIGDIR/FILE32.TXT BIGDIR/FILE33.TXT BIGDIR/FILE34.TXT BIGDIR/FILE35.TXT BIGDIR/FILE36.TXT BIGDIR/FILE37.TXT BIGDIR/FILE38.TXT BIGDIR/FILE39.TXT BIGDIR/FILE40.TXT
build/test_fileio_subdir "build/fat12_zs24_scratch.img" "kernel/tests/fixtures" --write root-regress
mcopy -n -i build/fat12_zs24_scratch.img ::ROOTNEW.TXT build/zs24_root.bin
"${PYTHON_FAT_REFERENCE:-python3}" -c 'import sys; e=bytes(97+(i%26) for i in range(300)); m=open("build/zs24_root.bin","rb").read(); sys.exit(0 if m==e else 1)' \
	|| { printf '!!! test-zs24 FAIL [ROOT-REGRESS]: root CREATE+WRITE read-back != written\n'; exit 1; }
printf '>>> test-zs24 [ROOT-REGRESS]: a ROOT file write still works (dir_start==0 path intact)\n'
# (5) DIRECTORY GROW (Fix 1): the riskiest new function fat12_grow_dir. \SUB
# holds '.'/'..'/NESTED.TXT/DEEP = 4 entries; spc==1 => 16 entries/cluster, so
# GROW00..GROW11 fill slots 4..15 (the first cluster) and GROW12 (slot 16)
# FORCES fat12_grow_dir to append a 2nd cluster. Diff with BOTH independent
# references: EVERY GROW file is listed AND the boundary file (GROW12) reads
# back byte-exact from the GROWN cluster (proving FAT relink + slot map +
# zero-fill). The boundary file content is 40 bytes of 'A'+12 == 'M'.
sh kernel/tests/disk/verify_fixture_volume.sh harness "build/fat12_nested.img" HELLO.TXT SUB/NESTED.TXT SUB/DEEP/DEEP.TXT BIGDIR/FILE01.TXT BIGDIR/FILE02.TXT BIGDIR/FILE03.TXT BIGDIR/FILE04.TXT BIGDIR/FILE05.TXT BIGDIR/FILE06.TXT BIGDIR/FILE07.TXT BIGDIR/FILE08.TXT BIGDIR/FILE09.TXT BIGDIR/FILE10.TXT BIGDIR/FILE11.TXT BIGDIR/FILE12.TXT BIGDIR/FILE13.TXT BIGDIR/FILE14.TXT BIGDIR/FILE15.TXT BIGDIR/FILE16.TXT BIGDIR/FILE17.TXT BIGDIR/FILE18.TXT BIGDIR/FILE19.TXT BIGDIR/FILE20.TXT BIGDIR/FILE21.TXT BIGDIR/FILE22.TXT BIGDIR/FILE23.TXT BIGDIR/FILE24.TXT BIGDIR/FILE25.TXT BIGDIR/FILE26.TXT BIGDIR/FILE27.TXT BIGDIR/FILE28.TXT BIGDIR/FILE29.TXT BIGDIR/FILE30.TXT BIGDIR/FILE31.TXT BIGDIR/FILE32.TXT BIGDIR/FILE33.TXT BIGDIR/FILE34.TXT BIGDIR/FILE35.TXT BIGDIR/FILE36.TXT BIGDIR/FILE37.TXT BIGDIR/FILE38.TXT BIGDIR/FILE39.TXT BIGDIR/FILE40.TXT
cp build/fat12_nested.img build/fat12_zs24_scratch.img
sh kernel/tests/disk/verify_fixture_volume.sh harness "build/fat12_zs24_scratch.img" HELLO.TXT SUB/NESTED.TXT SUB/DEEP/DEEP.TXT BIGDIR/FILE01.TXT BIGDIR/FILE02.TXT BIGDIR/FILE03.TXT BIGDIR/FILE04.TXT BIGDIR/FILE05.TXT BIGDIR/FILE06.TXT BIGDIR/FILE07.TXT BIGDIR/FILE08.TXT BIGDIR/FILE09.TXT BIGDIR/FILE10.TXT BIGDIR/FILE11.TXT BIGDIR/FILE12.TXT BIGDIR/FILE13.TXT BIGDIR/FILE14.TXT BIGDIR/FILE15.TXT BIGDIR/FILE16.TXT BIGDIR/FILE17.TXT BIGDIR/FILE18.TXT BIGDIR/FILE19.TXT BIGDIR/FILE20.TXT BIGDIR/FILE21.TXT BIGDIR/FILE22.TXT BIGDIR/FILE23.TXT BIGDIR/FILE24.TXT BIGDIR/FILE25.TXT BIGDIR/FILE26.TXT BIGDIR/FILE27.TXT BIGDIR/FILE28.TXT BIGDIR/FILE29.TXT BIGDIR/FILE30.TXT BIGDIR/FILE31.TXT BIGDIR/FILE32.TXT BIGDIR/FILE33.TXT BIGDIR/FILE34.TXT BIGDIR/FILE35.TXT BIGDIR/FILE36.TXT BIGDIR/FILE37.TXT BIGDIR/FILE38.TXT BIGDIR/FILE39.TXT BIGDIR/FILE40.TXT
build/test_fileio_subdir "build/fat12_zs24_scratch.img" "kernel/tests/fixtures" --write grow
# mtools: ALL 13 GROW files are listed in ::SUB (rendered 8.3 "GROWnn   TXT").
for n in 00 01 02 03 04 05 06 07 08 09 10 11 12; do \
	mdir -i build/fat12_zs24_scratch.img ::SUB 2>/dev/null | grep -q "GROW$n  *TXT" \
		|| { printf '!!! test-zs24 FAIL [GROW]: mdir ::SUB does not list GROW%s.TXT\n' "$n"; exit 1; }; \
done
# python: ALL 13 GROW files listed (anchored, rendered 8.3 form).
for n in 00 01 02 03 04 05 06 07 08 09 10 11 12; do \
	"${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_zs24_scratch.img --list-path 'SUB' | grep -q "^GROW$n.TXT " \
		|| { printf '!!! test-zs24 FAIL [GROW]: python --list-path SUB does not list GROW%s.TXT\n' "$n"; exit 1; }; \
done
# Boundary file (GROW12, slot 16 -- the FIRST slot of the appended 2nd cluster)
# reads back byte-exact via BOTH references (mtools AND python agree == written).
mcopy -n -i build/fat12_zs24_scratch.img ::SUB/GROW12.TXT build/zs24_grow_mcopy.bin
"${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_zs24_scratch.img --cat-path 'SUB\GROW12.TXT' > build/zs24_grow_py.bin
"${PYTHON_FAT_REFERENCE:-python3}" -c 'import sys; e=bytes([ord("A")+12]*40); m=open("build/zs24_grow_mcopy.bin","rb").read(); p=open("build/zs24_grow_py.bin","rb").read(); sys.exit(0 if (m==e and p==e and m==p) else 1)' \
	|| { printf '!!! test-zs24 FAIL [GROW]: boundary file GROW12 read-back from the grown cluster != written (mcopy/python disagree)\n'; exit 1; }
# Confirm the directory ACTUALLY grew to 2 clusters (the boundary file lives in
# the appended cluster, not an incidental free slot of the first).
"${PYTHON_FAT_REFERENCE:-python3}" -c 'import importlib.util,sys; s=importlib.util.spec_from_file_location("r","kernel/tests/disk/fat12_ref.py"); m=importlib.util.module_from_spec(s); s.loader.exec_module(m); fs=m.Fat12(open("build/fat12_zs24_scratch.img","rb").read()); _,st=fs.resolve_dir("SUB"); ch=fs.walk_chain(st); sys.exit(0 if len(ch)==2 else 1)' \
	|| { printf '!!! test-zs24 FAIL [GROW]: \\SUB did not grow to 2 clusters -- fat12_grow_dir was not exercised\n'; exit 1; }
printf '>>> test-zs24 [GROW]: GROW12 (slot 16) landed in the appended 2nd cluster; all 13 files listed; mcopy==python==written\n'
printf ">>> test-zs24: green\n"
