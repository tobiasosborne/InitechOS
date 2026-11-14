set -eu
ref=dos/command/tests/dos33/reference/wide
gzip -dc "$ref/before-data.img.gz" > build/reference.img
mdir -a -b -i build/reference.img :: | sed 's|^::/||' > build/names
awk 'p && / File\(s\)/ {exit} /^SUB /{p=1} p {print}' "$ref/TRACE.TXT" > build/expected
build/dir_wide < build/names > build/actual
cmp build/expected build/actual || { echo 'RED DIR/W: genuine captured fields, tabs or row endings differ' >&2;exit 1; }
echo 'GREEN DIR/W: real DOS rows against independent mtools names'
