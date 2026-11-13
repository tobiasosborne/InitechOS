set -eu
command -v mformat >/dev/null 2>&1 || { printf '!!! test-fat-move FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
command -v mmd     >/dev/null 2>&1 || { printf '!!! test-fat-move FAIL: mtools `mmd` not found.\n'; exit 1; }
command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-fat-move FAIL: mtools `mcopy` not found.\n'; exit 1; }
command -v mdir    >/dev/null 2>&1 || { printf '!!! test-fat-move FAIL: mtools `mdir` not found.\n'; exit 1; }
command -v mtype   >/dev/null 2>&1 || { printf '!!! test-fat-move FAIL: mtools `mtype` not found.\n'; exit 1; }
command -v mlabel  >/dev/null 2>&1 || { printf '!!! test-fat-move FAIL: mtools `mlabel` not found.\n'; exit 1; }
command -v "${PYTHON_FAT_REFERENCE:-python3}" >/dev/null 2>&1 || { printf '!!! test-fat-move FAIL: "${PYTHON_FAT_REFERENCE:-python3}" not found (the INDEPENDENT reference reader).\n'; exit 1; }
printf ">>> test-fat-move: cross-directory dir-entry transplant + Trash suffix (beads initech-tdnl.26)\n"
dd if=/dev/zero of=build/fat12_move_diff.img bs=512 count=2880 status=none
SOURCE_DATE_EPOCH=315532800 TZ=UTC mformat -i build/fat12_move_diff.img -f 1440 ::
SOURCE_DATE_EPOCH=315532800 TZ=UTC mmd -i build/fat12_move_diff.img ::SRC
SOURCE_DATE_EPOCH=315532800 TZ=UTC mmd -i build/fat12_move_diff.img ::DST
SOURCE_DATE_EPOCH=315532800 TZ=UTC mmd -i build/fat12_move_diff.img ::TRASH
SOURCE_DATE_EPOCH=315532800 TZ=UTC mmd -i build/fat12_move_diff.img ::SRC/MOVEDIR
SOURCE_DATE_EPOCH=315532800 TZ=UTC mcopy -i build/fat12_move_diff.img kernel/tests/fixtures/chain.txt ::MOVEME.TXT
SOURCE_DATE_EPOCH=315532800 TZ=UTC mcopy -i build/fat12_move_diff.img kernel/tests/fixtures/chain.txt ::SRC/CROSS.TXT
SOURCE_DATE_EPOCH=315532800 TZ=UTC mcopy -i build/fat12_move_diff.img kernel/tests/fixtures/second.txt ::SRC/SUBFILE.TXT
SOURCE_DATE_EPOCH=315532800 TZ=UTC mcopy -i build/fat12_move_diff.img kernel/tests/fixtures/hello.txt ::SRC/REPORT.DBF
SOURCE_DATE_EPOCH=315532800 TZ=UTC mcopy -i build/fat12_move_diff.img kernel/tests/fixtures/second.txt ::TRASH/REPORT.DBF
SOURCE_DATE_EPOCH=315532800 TZ=UTC mcopy -i build/fat12_move_diff.img kernel/tests/fixtures/hello.txt ::SRC/MOVEDIR/INSIDE.TXT
SOURCE_DATE_EPOCH=315532800 TZ=UTC mcopy -i build/fat12_move_diff.img kernel/tests/fixtures/hello.txt ::COLL.TXT
SOURCE_DATE_EPOCH=315532800 TZ=UTC mcopy -i build/fat12_move_diff.img kernel/tests/fixtures/hello.txt ::DST/COLL.TXT
sh kernel/tests/disk/verify_fixture_volume.sh recipe "build/fat12_move_diff.img" MOVEME.TXT SRC/CROSS.TXT SRC/SUBFILE.TXT SRC/REPORT.DBF TRASH/REPORT.DBF SRC/MOVEDIR/INSIDE.TXT COLL.TXT DST/COLL.TXT
dd if=/dev/zero of=build/fat12_move_struct.img bs=512 count=2880 status=none
SOURCE_DATE_EPOCH=315532800 TZ=UTC mformat -i build/fat12_move_struct.img -f 1440 ::
SOURCE_DATE_EPOCH=315532800 TZ=UTC mlabel -i build/fat12_move_struct.img ::INITECH
SOURCE_DATE_EPOCH=315532800 TZ=UTC mmd -i build/fat12_move_struct.img ::SUBA
SOURCE_DATE_EPOCH=315532800 TZ=UTC mmd -i build/fat12_move_struct.img ::SUBB
SOURCE_DATE_EPOCH=315532800 TZ=UTC mmd -i build/fat12_move_struct.img ::SUBC
SOURCE_DATE_EPOCH=315532800 TZ=UTC mmd -i build/fat12_move_struct.img ::HOLDER
SOURCE_DATE_EPOCH=315532800 TZ=UTC mmd -i build/fat12_move_struct.img ::PARENT
SOURCE_DATE_EPOCH=315532800 TZ=UTC mmd -i build/fat12_move_struct.img ::PARENT/CHILD
SOURCE_DATE_EPOCH=315532800 TZ=UTC mcopy -i build/fat12_move_struct.img kernel/tests/fixtures/hello.txt ::FILEA.TXT
SOURCE_DATE_EPOCH=315532800 TZ=UTC mcopy -i build/fat12_move_struct.img kernel/tests/fixtures/hello.txt ::KEEPA.TXT
SOURCE_DATE_EPOCH=315532800 TZ=UTC mcopy -i build/fat12_move_struct.img kernel/tests/fixtures/hello.txt ::LONGBASE.DAT
SOURCE_DATE_EPOCH=315532800 TZ=UTC mcopy -i build/fat12_move_struct.img kernel/tests/fixtures/hello.txt ::GROWA.TXT
SOURCE_DATE_EPOCH=315532800 TZ=UTC mcopy -i build/fat12_move_struct.img kernel/tests/fixtures/hello.txt ::GROWB.TXT
SOURCE_DATE_EPOCH=315532800 TZ=UTC mcopy -i build/fat12_move_struct.img kernel/tests/fixtures/hello.txt ::GROWC.TXT
SOURCE_DATE_EPOCH=315532800 TZ=UTC mcopy -i build/fat12_move_struct.img kernel/tests/fixtures/second.txt ::HOLDER/KEEP.TXT
sh kernel/tests/disk/verify_fixture_volume.sh recipe "build/fat12_move_struct.img" FILEA.TXT KEEPA.TXT LONGBASE.DAT GROWA.TXT GROWB.TXT GROWC.TXT HOLDER/KEEP.TXT
mdir -i build/fat12_move_diff.img :: | grep 'bytes free' > build/fat12_move_free_before.txt
build/test_fat12_move "build/fat12_move_diff.img" "build/fat12_move_struct.img"
printf '>>> test-fat-move [host]: transplant legs green -- chain/size/attr/timestamps, both FAT copies and the data clusters proven unchanged\n'
mdir -i build/fat12_move_diff.img :: | grep -q '^MOVEME' \
	&& { printf '!!! test-fat-move FAIL [mtools]: MOVEME.TXT is STILL in the root -- the source slot was not released\n'; exit 1; } || true
mdir -i build/fat12_move_diff.img :: | grep -q '^SUBFILE' \
	|| { printf '!!! test-fat-move FAIL [mtools]: SUBFILE.TXT is missing from the root -- the subdir->root move did not land\n'; exit 1; }
mdir -i build/fat12_move_diff.img ::DST | grep -q '^MOVEME' \
	|| { printf '!!! test-fat-move FAIL [mtools]: DST lacks MOVEME.TXT -- the root->subdir move did not land\n'; exit 1; }
mdir -i build/fat12_move_diff.img ::DST | grep -q '^CROSS' \
	|| { printf '!!! test-fat-move FAIL [mtools]: DST lacks CROSS.TXT -- the subdir->subdir move did not land\n'; exit 1; }
mdir -i build/fat12_move_diff.img ::DST | grep -q '^MOVEDIR .*<DIR>' \
	|| { printf '!!! test-fat-move FAIL [mtools]: DST lacks the MOVEDIR <DIR> entry -- the directory move did not land\n'; exit 1; }
mdir -i build/fat12_move_diff.img ::SRC | grep -qE '^(CROSS|SUBFILE|REPORT|MOVEDIR)' \
	&& { printf '!!! test-fat-move FAIL [mtools]: SRC still lists a moved entry -- the source slots were not released\n'; exit 1; } || true
mdir -i build/fat12_move_diff.img ::TRASH | grep -q '^REPORT   DBF' \
	|| { printf '!!! test-fat-move FAIL [mtools]: the ORIGINAL TRASH/REPORT.DBF vanished -- staging clobbered it\n'; exit 1; }
mdir -i build/fat12_move_diff.img ::TRASH | grep -q '^REPOR001 DBF' \
	|| { printf '!!! test-fat-move FAIL [mtools]: TRASH lacks the suffixed REPOR001.DBF -- the F1.4 collision ladder did not land\n'; exit 1; }
mdir -i build/fat12_move_diff.img ::DST/MOVEDIR | grep -q '^INSIDE' \
	|| { printf '!!! test-fat-move FAIL [mtools]: the moved folder is not traversable at its new home (INSIDE.TXT unreachable)\n'; exit 1; }
# The '..' fixup needs an INDEPENDENT reader, not mdir: mtools resolves
# '..' TEXTUALLY from the path string, so `mdir ::DST/MOVEDIR/..` prints
# DST's contents even when the on-disk '..' still names the OLD parent
# (verified against the NO_DOTDOT mutant image -- that grep would have been
# decoration, the repository contract). "${PYTHON_FAT_REFERENCE:-python3}" reads the '..' bytes straight out of slot 1
# of the folder's first cluster and we compare against the NEW parent's own
# start cluster, both from first principles.
dd_have=$("${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_move_diff.img --dotdot 'DST\MOVEDIR') || \
	{ printf "!!! test-fat-move FAIL [python]: cannot read the moved folder's '..' entry\n"; exit 1; }; \
dd_want=$("${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_move_diff.img --dirstart 'DST') || \
	{ printf '!!! test-fat-move FAIL [python]: cannot read DST start cluster\n'; exit 1; }; \
if [ "$dd_have" != "$dd_want" ]; then \
	printf "!!! test-fat-move FAIL [python]: the moved folder's '..' says cluster %s but its new parent DST is cluster %s -- the '..' fixup is wrong\n" "$dd_have" "$dd_want"; exit 1; \
fi
"${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_move_diff.img --list-path 'DST\MOVEDIR' | grep -q '^INSIDE.TXT ' \
	|| { printf '!!! test-fat-move FAIL [python]: the independent reader cannot enumerate the moved folder at its new home\n'; exit 1; }
mtype -i build/fat12_move_diff.img ::DST/MOVEME.TXT | cmp -s - kernel/tests/fixtures/chain.txt \
	|| { printf '!!! test-fat-move FAIL [mtools]: the moved file CONTENT differs from the seed body -- a move must not touch data\n'; exit 1; }
mtype -i build/fat12_move_diff.img ::TRASH/REPOR001.DBF | cmp -s - kernel/tests/fixtures/hello.txt \
	|| { printf '!!! test-fat-move FAIL [mtools]: the STAGED item content differs -- the rename-while-move touched data\n'; exit 1; }
mtype -i build/fat12_move_diff.img ::TRASH/REPORT.DBF | cmp -s - kernel/tests/fixtures/second.txt \
	|| { printf '!!! test-fat-move FAIL [mtools]: the ORIGINAL trashed item content changed -- the suffix ladder aliased it\n'; exit 1; }
mdir -i build/fat12_move_diff.img :: | grep 'bytes free' > build/fat12_move_free_after.txt
cmp -s build/fat12_move_free_before.txt build/fat12_move_free_after.txt \
	|| { printf '!!! test-fat-move FAIL [mtools]: the free-space count CHANGED -- a same-volume move must spend no cluster\n'; exit 1; }
printf '>>> test-fat-move [mtools]: entries transplanted, content identical, folder traversable, free space unchanged\n'
printf ">>> test-fat-move: green\n"
