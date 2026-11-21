set -eu
for tool in mformat mcopy mmd; do command -v "$tool" >/dev/null; done
(cd kernel/tests/fixtures/dos33_env && sha256sum -c SHA256SUMS)
mkdir -p build/loader-live
build/test_loader_live fixtures build/loader-live
truncate -s 1474560 build/loader-live/input.img
mformat -i build/loader-live/input.img -f 1440 ::
mcopy -i build/loader-live/input.img build/loader-live/*.COM build/loader-live/*.EXE ::
mmd -i build/loader-live/input.img ::SUB
mcopy -i build/loader-live/input.img build/loader-live/RAW.COM ::SUB/ONLY.COM
mcopy -i build/loader-live/input.img build/loader-live/RAW.COM ::SUB/DUMP.COM
mcopy -i build/loader-live/input.img build/loader-live/HIGH.EXE ::PARENT.COM
cp build/loader-live/input.img build/loader-live/before.img
for mode in arena high-plan subdir inplace environment cwd high-owner high-reclaim high-reclaim-failure high-canonical low-high nested-fixed nested-pic nested-invalid resident-flat resident-reloc resident-high; do
 printf 'loader live case: %s\n' "$mode"
 build/test_loader_live "$mode" build/loader-live/input.img
done
cmp build/loader-live/before.img build/loader-live/input.img
