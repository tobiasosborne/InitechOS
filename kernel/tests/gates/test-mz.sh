set -eu
printf ">>> test-mz: is_mz + parse (both last-page cases) + foreign-gate + fail-loud + apply_relocs\n"
build/test_mz
printf ">>> test-mz: green\n"
