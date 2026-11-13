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

printf '======================================================================\n'
printf 'InitechOS (InitechOS) -- make test-fat-subdir : FAT12 subdir/path oracle\n'
printf '  Ref: PRD Sec 6.1 / ADR-0003 DEC-07. beads initech-ti8.\n'
printf '  Our C reader vs TWO independent refs (mtools + "${PYTHON_FAT_REFERENCE:-python3}") on a nested tree.\n'
printf '======================================================================\n'
# ---- (D) Tool presence: fail loud, NEVER silently skip (the repository contract). ----
command -v mdir   >/dev/null 2>&1 || { printf '!!! test-fat-subdir FAIL: mtools `mdir` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
command -v "${PYTHON_FAT_REFERENCE:-python3}" >/dev/null 2>&1 || { printf '!!! test-fat-subdir FAIL: "${PYTHON_FAT_REFERENCE:-python3}" not found (needed for the independent reference reader).\n'; exit 1; }
printf '>>> test-fat-subdir [D]: reference tools present (mtools mdir + "${PYTHON_FAT_REFERENCE:-python3}")\n'
# ---- (C) The subdir unit oracle (enumerate + resolve + multi-cluster read). ----
printf '>>> test-fat-subdir [C]: unit oracle (subdir enumerate + path resolve + FILE20 multi-cluster read)\n'
build/test_fat12_subdir "build/fat12_nested.img" "kernel/tests/fixtures" \
	|| { printf '!!! test-fat-subdir FAIL: test_fat12_subdir red\n'; exit 1; }
# ---- (A) NAMES+SIZES per directory: triple agreement (our == python == mdir). ----
printf '>>> test-fat-subdir [A]: names+sizes per dir -- fat_dump == python ref == normalized mdir\n'
for d in \\ \\SUB \\SUB\\DEEP \\BIGDIR; do \
	build/fat_dump "build/fat12_nested.img" --list-path "$d" | sort > "build/fat12_sub_our.list" \
		|| { printf '!!! test-fat-subdir FAIL [A]: fat_dump --list-path %s errored\n' "$d"; exit 1; }; \
	"${PYTHON_FAT_REFERENCE:-python3}" "kernel/tests/disk/fat12_ref.py" "build/fat12_nested.img" --list-path "$d" | sort > "build/fat12_sub_py.list" \
		|| { printf '!!! test-fat-subdir FAIL [A]: fat12_ref.py --list-path %s errored\n' "$d"; exit 1; }; \
	mp=$(printf '%s' "$d" | sed -e 's#^\\##' -e 's#\\#/#g'); \
	mdir -i "build/fat12_nested.img" "::$mp" | awk '{ \
		hd=0; di=0; \
		for(i=1;i<=NF;i++){ if($i ~ /^[0-9]{4}-[0-9]{2}-[0-9]{2}$/){hd=1;di=i;break} } \
		if(!hd) next; \
		name=$1; ext=$2; sz=""; \
		for(i=3;i<di;i++){ sz=sz $i } \
		if(sz !~ /^[0-9]+$/) next; \
		fn=(ext!="")?name"."ext:name; \
		print fn, sz \
	}' | sort > "build/fat12_sub_mtools.list"; \
	diff -u "build/fat12_sub_our.list" "build/fat12_sub_py.list" \
		|| { printf '!!! test-fat-subdir FAIL [A]: %s -- our listing != python reference listing\n' "$d"; exit 1; }; \
	diff -u "build/fat12_sub_our.list" "build/fat12_sub_mtools.list" \
		|| { printf '!!! test-fat-subdir FAIL [A]: %s -- our listing != normalized mdir listing\n' "$d"; exit 1; }; \
	printf '    %-14s %s files : our == python == mdir\n' "$d" "$(wc -l < "build/fat12_sub_our.list" | tr -d ' ')"; \
done
# ---- (B) MULTI-CLUSTER proof: BIGDIR has > 16 files (3-cluster chain walked). ----
printf '>>> test-fat-subdir [B]: BIGDIR multi-cluster -- > 16 files (single-sector reader would lose FILE17+)\n'
bign=$(build/fat_dump "build/fat12_nested.img" --list-path '\BIGDIR' | wc -l | tr -d ' '); \
	if [ "$bign" -le 16 ]; then \
		printf '!!! test-fat-subdir FAIL [B]: BIGDIR listed only %s files (<=16) -- multi-cluster walk broken\n' "$bign"; exit 1; \
	fi; \
	printf '    BIGDIR: %s files across a 3-cluster chain (multi-cluster walk confirmed)\n' "$bign"
printf '%s\n' '----------------------------------------------------------------------'
printf 'VERDICT   : PASS -- our subdir reader agrees with mtools AND an independent\n'
printf '            "${PYTHON_FAT_REFERENCE:-python3}" reader on the nested tree (incl. the multi-cluster BIGDIR).\n'
printf '======================================================================\n'
