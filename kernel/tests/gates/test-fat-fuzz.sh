set -eu
dd if=/dev/zero of=build/fat12_fuzz_blank.img bs=512 count=2880 status=none
mformat -i build/fat12_fuzz_blank.img -f 1440 ::
printf ">>> fat12: minted BLANK %s (1.44MB FAT12, no files; generative fuzzer template)\n" "build/fat12_fuzz_blank.img"

cp -f build/fat12_fuzz_blank.img build/test-fat-fuzz.fuzzblank.img
printf '======================================================================\n'
printf 'InitechOS (InitechOS) -- make test-fat-fuzz : FAT12 generative fuzzer (deterministic seed sweep)\n'
printf '======================================================================\n'
command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-fat-fuzz FAIL: mtools `mcopy` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
command -v "${PYTHON_FAT_REFERENCE:-python3}" >/dev/null 2>&1 || { printf '!!! test-fat-fuzz FAIL: "${PYTHON_FAT_REFERENCE:-python3}" not found (needed for the independent reference).\n'; exit 1; }
printf '>>> test-fat-fuzz [A]: model + remount + FAT-sync over seeds %s..%s (fast, every op)\n' "1" "${FAT_SEEDS}"
build/fat12_fuzz --sweep 1 "${FAT_SEEDS}" --ops 40 --no-external "build/test-fat-fuzz.fuzzblank.img" \
	|| { printf '!!! test-fat-fuzz FAIL [A]: a seed diverged (shrunk reproducer above) -- root-cause it (the repository contract)\n'; exit 1; }
printf '>>> test-fat-fuzz [B]: + mtools/python external cross-check over seeds %s..%s\n' "1" "${FAT_EXTERNAL_SEEDS}"
build/fat12_fuzz --sweep 1 "${FAT_EXTERNAL_SEEDS}" --ops 40 "build/test-fat-fuzz.fuzzblank.img" \
	|| { printf '!!! test-fat-fuzz FAIL [B]: mtools/python disagree with the model (shrunk reproducer above)\n'; exit 1; }
rm -f "build/test-fat-fuzz.fuzzblank.img".seed*.scratch "build/test-fat-fuzz.fuzzblank.img".*.bin
printf '>>> test-fat-fuzz: green (deterministic seed sweep -- model + remount + FAT-sync + mtools/python all agree)\n'
