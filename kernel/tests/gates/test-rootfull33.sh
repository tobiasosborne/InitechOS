set -eu
printf '%s  %s\n' df02323a04f9094c40e4ae4b5f5310ea0d3432b5633bb4e78c3c0defe4057c1c kernel/tests/fixtures/rootfull33/result.bin | sha256sum -c -
truncate -s 1474560 build/rootfull.img
mformat -i build/rootfull.img -f 1440 ::
: > build/empty
n=0
while [ "$n" -lt 224 ]; do
 name=$(printf 'F%03d' "$n")
 mcopy -i build/rootfull.img build/empty "::$name"
 n=$((n+1))
done
cp build/rootfull.img build/before.img
build/test_metadata_errors build/rootfull.img rootfull 0 kernel/tests/fixtures/rootfull33/result.bin
cmp build/before.img build/rootfull.img
