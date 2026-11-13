set -eu
dd if=/dev/zero of=build/fat12_m0bp_golden.img bs=512 count=2880 status=none
mformat -i build/fat12_m0bp_golden.img -f 1440 ::
mmd -i build/fat12_m0bp_golden.img ::SUB
mmd -i build/fat12_m0bp_golden.img ::SUB/NEWDIR
printf ">>> fat12: minted %s (mmd ::SUB + ::SUB/NEWDIR nested golden; build intermediate)\n" "build/fat12_m0bp_golden.img"

command -v mformat >/dev/null 2>&1 || { printf '!!! test-m0bp FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
command -v mmd     >/dev/null 2>&1 || { printf '!!! test-m0bp FAIL: mtools `mmd` not found.\n'; exit 1; }
command -v mdir    >/dev/null 2>&1 || { printf '!!! test-m0bp FAIL: mtools `mdir` not found.\n'; exit 1; }
command -v "${PYTHON_FAT_REFERENCE:-python3}" >/dev/null 2>&1 || { printf '!!! test-m0bp FAIL: "${PYTHON_FAT_REFERENCE:-python3}" not found (independent reference).\n'; exit 1; }
printf ">>> test-m0bp: NESTED MKDIR/RMDIR (non-root parent) vs mmd golden + grow cross-check (beads initech-m0bp)\n"
dd if=/dev/zero of=build/fat12_m0bp_blank.img bs=512 count=2880 status=none
mformat -i build/fat12_m0bp_blank.img -f 1440 ::
mmd -i build/fat12_m0bp_blank.img ::SUB
build/test_fat12_mkdir_nested "build/fat12_m0bp_blank.img" "build/fat12_m0bp_golden.img"
mdir -i build/fat12_m0bp_blank.img ::SUB 2>/dev/null | grep -qi 'GROWDIR' \
	|| { printf '!!! test-m0bp FAIL [GROW]: mdir ::SUB does not list GROWDIR <DIR> -- the grown boundary dir is unreachable to mtools\n'; exit 1; }
# The python reference lists only REGULAR files, so resolve the nested PATH
# 'SUB\GROWDIR' instead: descending SUB's GROWN chain to find GROWDIR (a dir
# living in the appended 2nd cluster, slot 16) and listing it succeeds (rc 0)
# ONLY if GROWDIR is reachable; a missing dir resolves rc != 0 (the check has
# teeth -- proven against SUB\NOSUCHDIR). Independent of mtools (the repository contract/the repository contract).
"${PYTHON_FAT_REFERENCE:-python3}" kernel/tests/disk/fat12_ref.py build/fat12_m0bp_blank.img --list-path 'SUB\GROWDIR' >/dev/null 2>&1 \
	|| { printf '!!! test-m0bp FAIL [GROW]: python cannot resolve SUB\\GROWDIR -- the grown boundary dir is unreachable to the independent reader\n'; exit 1; }
printf '>>> test-m0bp [GROW]: GROWDIR in the appended 2nd cluster of \\SUB; mtools (mdir <DIR>) + python (path-resolve) agree\n'
printf ">>> test-m0bp: green\n"
