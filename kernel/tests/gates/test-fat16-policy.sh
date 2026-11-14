set -eu
mkdir -p build/fat16-policy
w=build/fat16-policy
printf 'mtools independent data\n' > "$w/input"
for type in 12 16; do
 sectors=2880; test "$type" = 12 || sectors=16384
 truncate -s $(((sectors+71)*512)) "$w/base$type.img"
 printf 'drive z: file="%s/%s/base%s.img" partition=1\n' "$PWD" "$w" "$type" > "$w/mtoolsrc"
 export MTOOLSRC="$PWD/$w/mtoolsrc"
 mpartition -I z:
 ptype=1; test "$type" = 12 || ptype=6
 mpartition -a -c -b 63 -l "$sectors" -T "$ptype" z:
 mformat -i "$w/base$type.img@@32256" -T "$sectors" -h 16 -s 63 -c 1 -H 63 ::
 mcopy -i "$w/base$type.img@@32256" "$w/input" ::CHAIN.BIN
 mmd -i "$w/base$type.img@@32256" ::SUB
 for letter in 0 2 3; do
  cp "$w/base$type.img" "$w/run$type-$letter.img"
  ro=0; test "$type" = 12 || ro=1
  build/test_fat16_policy "$w/run$type-$letter.img" "$ro" "$letter"
  if test "$type" = 16; then
   cmp "$w/base$type.img" "$w/run$type-$letter.img"
   mcopy -i "$w/run$type-$letter.img@@32256" ::CHAIN.BIN "$w/readback-$letter"
   cmp "$w/input" "$w/readback-$letter"
  else
   if cmp -s "$w/base$type.img" "$w/run$type-$letter.img"; then exit 1; fi
  fi
 done
done
