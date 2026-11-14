set -eu
capture=kernel/tests/fixtures/raw-name33
(cd "$capture" && sha256sum -c SHA256SUMS)
gzip -dc "$capture/data.img.gz" > build/native.img
truncate -s 1474560 build/blank.img
mformat -i build/blank.img -f 1440 ::
mmd -i build/blank.img ::SUBDIREC
index=0
while IFS="$(printf '\t')" read -r operand canonical; do
 cp build/blank.img "build/case-$index.img"
 build/test_raw_names33 "build/case-$index.img" "$capture/result.bin" "$operand" "$index"
 if [ "$canonical" = - ]; then
  cmp build/blank.img "build/case-$index.img"
 else
  mcopy -o -i build/native.img "::$canonical" build/native.bin
  mcopy -o -i "build/case-$index.img" "::$canonical" build/actual.bin
  cmp build/native.bin build/actual.bin
  printf '::/SUBDIREC/\n::/%s\n' "$canonical" | sort > build/expected.list
  mdir -b -i "build/case-$index.img" ::/ ::SUBDIREC/ | sort > build/actual.list
  cmp build/expected.list build/actual.list
 fi
 index=$((index+1))
done < "$capture/cases.tsv"
test "$index" -eq 20
cp build/blank.img build/namespace.img
build/test_raw_names33 build/namespace.img "$capture/namespace.bin" namespace 20
mdir -b -i build/namespace.img ::/ > build/namespace.list
printf '::/SUBDIREC/\n' > build/expected.list
cmp build/namespace.list build/expected.list
${PYTHON_FAT_REFERENCE:-python3} kernel/tests/disk/check_conservation.py build/case-*.img build/namespace.img
