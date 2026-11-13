set -eu
printf ">>> test-mzload: parse+dispatch + flat-reloc (trailer + header-resident) + place@PROGRAM_IMAGE + entry + arena/minalloc + foreign-fail-loud + .COM-untouched\n"
build/test_mzload
printf ">>> test-mzload: green\n"
