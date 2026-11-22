#!/bin/sh
# B: with IN.TXT = 'H' 'i' CR LF NUL E0h ^Z 'z' (eight bytes).
set -eu
img=$1
export SOURCE_DATE_EPOCH=699525000 TZ=UTC LC_ALL=C
truncate -s 1474560 "$img"
mformat -i "$img" -f 1440 -N 12345678 ::
t=$(mktemp -d)
printf 'Hi\r\n\000\340\032z' > "$t/IN.TXT"
touch -d '@699525000' "$t/IN.TXT"
mcopy -m -i "$img" "$t/IN.TXT" ::
rm -rf "$t"
