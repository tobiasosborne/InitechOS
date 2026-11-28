#!/bin/sh
# B: with IN.TXT = 'H' 'i' CR LF NUL E0h ^Z 'z' (eight bytes) and IN0A.TXT
# for buffered input: an edited line, an overlong line, a ^Z line.
set -eu
img=$1
export SOURCE_DATE_EPOCH=699525000 TZ=UTC LC_ALL=C
truncate -s 1474560 "$img"
mformat -i "$img" -f 1440 -N 12345678 ::
t=$(mktemp -d)
printf 'Hi\r\n\000\340\032z' > "$t/IN.TXT"
printf 'ab\010c\r\n123456789AB\r\nx\032y\rp\nq\r' > "$t/IN0A.TXT"
printf 'ABCline\r\n' > "$t/IN0C.TXT"
touch -d '@699525000' "$t/IN.TXT" "$t/IN0A.TXT" "$t/IN0C.TXT"
mcopy -m -i "$img" "$t/IN.TXT" "$t/IN0A.TXT" "$t/IN0C.TXT" ::
rm -rf "$t"
