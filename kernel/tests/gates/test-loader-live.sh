set -eu
for tool in mformat mcopy mmd; do command -v "$tool" >/dev/null; done
mkdir -p build/loader-live
build/test_loader_live fixtures build/loader-live
truncate -s 1474560 build/loader-live/input.img
mformat -i build/loader-live/input.img -f 1440 ::
mcopy -i build/loader-live/input.img build/loader-live/RAW.COM build/loader-live/MZ.EXE build/loader-live/HIGH.EXE ::
mmd -i build/loader-live/input.img ::SUB
mcopy -i build/loader-live/input.img build/loader-live/RAW.COM ::SUB/ONLY.COM
cp build/loader-live/input.img build/loader-live/before.img
for mode in arena high-plan subdir cwd inplace high-owner high-reclaim resident-flat resident-reloc; do
 printf 'loader live case: %s\n' "$mode"
 build/test_loader_live "$mode" build/loader-live/input.img
done
cmp build/loader-live/before.img build/loader-live/input.img
