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

printf '======================================================================\n'
printf 'InitechOS (InitechOS) -- make test-fat16 : FAT16 READ-ONLY differential\n'
printf '  Ref: PRD Sec 5/6.1 / ADR-0003 DEC-07. beads initech-z01.\n'
printf '  Our C reader vs TWO independent refs (mtools + "${PYTHON_FAT_REFERENCE:-python3}") on a FAT16 vol.\n'
printf '======================================================================\n'
# ---- (D) Tool presence: fail loud, NEVER silently skip (the repository contract). ----
command -v mkfs.fat >/dev/null 2>&1 || { printf '!!! test-fat16 FAIL: mkfs.fat not found (apt install dosfstools). A skipped oracle is worse than a red one.\n'; exit 1; }
command -v mdir   >/dev/null 2>&1 || { printf '!!! test-fat16 FAIL: mtools `mdir` not found (apt install mtools).\n'; exit 1; }
command -v mcopy  >/dev/null 2>&1 || { printf '!!! test-fat16 FAIL: mtools `mcopy` not found.\n'; exit 1; }
command -v "${PYTHON_FAT_REFERENCE:-python3}" >/dev/null 2>&1 || { printf '!!! test-fat16 FAIL: "${PYTHON_FAT_REFERENCE:-python3}" not found (independent reference reader).\n'; exit 1; }
printf '>>> test-fat16 [D]: reference tools present (mkfs.fat + mtools mdir/mcopy + "${PYTHON_FAT_REFERENCE:-python3}")\n'
# ---- (C) The FAT16 unit oracle (mount+classify, enumerate, read). ----
printf '>>> test-fat16 [C]: unit oracle (mount+classify FAT16, enumerate, byte-for-byte read)\n'
build/test_fat16 "build/fat16_fixture.img" "kernel/tests/fixtures" \
	|| { printf '!!! test-fat16 FAIL: test_fat16 unit oracle red\n'; exit 1; }
# ---- (A) NAMES + SIZES: triple agreement (our == python == mdir). ----
printf '>>> test-fat16 [A]: names+sizes -- our fat_dump == python ref == normalized mdir\n'
build/fat_dump "build/fat16_fixture.img" --list > "build/fat16_our.list" \
	|| { printf '!!! test-fat16 FAIL: fat_dump --list errored\n'; exit 1; }
"${PYTHON_FAT_REFERENCE:-python3}" "kernel/tests/disk/fat16_ref.py" "build/fat16_fixture.img" --list > "build/fat16_py.list" \
	|| { printf '!!! test-fat16 FAIL: fat16_ref.py --list errored\n'; exit 1; }
mdir -i "build/fat16_fixture.img" :: | awk '{ \
	hd=0; di=0; \
	for(i=1;i<=NF;i++){ if($i ~ /^[0-9]{4}-[0-9]{2}-[0-9]{2}$/){hd=1;di=i;break} } \
	if(!hd) next; \
	name=$1; ext=$2; sz=""; \
	for(i=3;i<di;i++){ sz=sz $i } \
	if(sz !~ /^[0-9]+$/) next; \
	fn=(ext!="")?name"."ext:name; \
	print fn, sz \
}' | sort > "build/fat16_mtools.list"
diff -u "build/fat16_our.list" "build/fat16_py.list" \
	|| { printf '!!! test-fat16 FAIL [A]: our listing != python reference listing\n'; exit 1; }
diff -u "build/fat16_our.list" "build/fat16_mtools.list" \
	|| { printf '!!! test-fat16 FAIL [A]: our listing != normalized mdir listing\n'; exit 1; }
printf '    triple agreement on names+sizes (%s files):\n' "$(wc -l < "build/fat16_our.list" | tr -d ' ')"
sed 's/^/      /' "build/fat16_our.list"
# ---- (B) CONTENT per file: our == mcopy == python, byte-for-byte. ----
printf '>>> test-fat16 [B]: content -- our fat_dump --cat == mcopy == python ref, per file\n'
for f in HELLO.TXT CHAIN.TXT BLOCK.BIN BIGCHAIN.TXT HIGHCLUS.BIN; do \
	build/fat_dump "build/fat16_fixture.img" --cat "$f" > "build/fat16_our_$f.bin" \
		|| { printf '!!! test-fat16 FAIL [B]: fat_dump --cat %s errored\n' "$f"; exit 1; }; \
	mcopy -i "build/fat16_fixture.img" "::$f" - > "build/fat16_mtools_$f.bin" 2>/dev/null \
		|| { printf '!!! test-fat16 FAIL [B]: mcopy ::%s errored\n' "$f"; exit 1; }; \
	"${PYTHON_FAT_REFERENCE:-python3}" "kernel/tests/disk/fat16_ref.py" "build/fat16_fixture.img" --cat "$f" > "build/fat16_py_$f.bin" \
		|| { printf '!!! test-fat16 FAIL [B]: fat16_ref.py --cat %s errored\n' "$f"; exit 1; }; \
	cmp -s "build/fat16_our_$f.bin" "build/fat16_mtools_$f.bin" \
		|| { printf '!!! test-fat16 FAIL [B]: %s -- our bytes != mcopy bytes\n' "$f"; exit 1; }; \
	cmp -s "build/fat16_our_$f.bin" "build/fat16_py_$f.bin" \
		|| { printf '!!! test-fat16 FAIL [B]: %s -- our bytes != python ref bytes\n' "$f"; exit 1; }; \
	printf '    %-12s %7s bytes : our == mcopy == python\n' "$f" "$(wc -c < "build/fat16_our_$f.bin" | tr -d ' ')"; \
done
printf '%s\n' '----------------------------------------------------------------------'
printf 'VERDICT   : PASS -- our FAT16 reader agrees with mtools AND an independent\n'
printf '            "${PYTHON_FAT_REFERENCE:-python3}" reader on names, sizes, and content (triple oracle).\n'
printf '======================================================================\n'
