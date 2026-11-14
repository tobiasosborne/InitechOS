set -eu
fail() { printf 'FAIL replace: %s\nINITECH_CHECKS 1 failures=1\n' "$*"; exit 1; }
for tool in mformat mcopy mmd mdel fsck.fat; do command -v "$tool" >/dev/null; done
mkdir -p build/replace
image=build/replace/original.img
dd if=/dev/zero of="$image" bs=512 count=2880 status=none
mformat -i "$image" -f 1440 ::
printf 'the previous document\n' > build/replace/old
# Different sizes/chains, including empty source and destination below.
dd if=kernel/tests/fixtures/chain.txt of=build/replace/new bs=1 count=1537 status=none
touch -t 199103010800.00 build/replace/old
touch -t 199203020930.00 build/replace/new
mcopy -m -i "$image" build/replace/old ::DEST.BIN
mcopy -m -i "$image" build/replace/new ::STAGE.BIN
mmd -i "$image" ::SUB
mcopy -m -i "$image" build/replace/old ::SUB/DEST.BIN
# Put staging in a different directory sector (also crosses a subdir cluster).
for n in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16; do
 mcopy -m -i "$image" build/replace/old ::F$n
 mcopy -m -i "$image" build/replace/old ::SUB/F$n
done
mcopy -m -i "$image" build/replace/new ::FAR.BIN
mcopy -m -i "$image" build/replace/new ::SUB/STAGE.BIN
for kind in root same-sector sub empty-stage empty-dest; do
 input=build/replace/input-$kind.img
 cp "$image" "$input"
 stage=FAR.BIN; dest=DEST.BIN
 case $kind in
 same-sector) stage=STAGE.BIN;;
 sub) stage='SUB\STAGE.BIN'; dest='SUB\DEST.BIN';;
 empty-stage) : > build/replace/empty; mcopy -o -i "$input" build/replace/empty ::FAR.BIN;;
 empty-dest) : > build/replace/empty; mcopy -o -i "$input" build/replace/empty ::DEST.BIN;;
 esac
 mstage=$(printf %s "$stage" | tr "\\\\" /); mdest=$(printf %s "$dest" | tr "\\\\" /)
 mcopy -i "$input" "::$mstage" build/replace/expected-$kind
 control=build/replace/$kind-0-0.img
 cp "$input" "$control"
 build/test_replace "$control" "$stage" "$dest" 0 0 0 > build/replace/control
 cat build/replace/control
 total=$(sed -n 's/.*writes=\([0-9]*\).*/\1/p' build/replace/control)
 test "$total" -ge 2
 for nth in $(seq 1 "$total"); do
  for after in 0 1; do
   out=build/replace/$kind-$nth-$after.img
   cp "$input" "$out"
   rc=0; if test "$nth" -eq 1 && test "$after" -eq 0; then rc=31; fi
   build/test_replace "$out" "$stage" "$dest" "$nth" "$after" "$rc"
   if test "$nth" -eq 1 && test "$after" -eq 0; then cmp "$input" "$out"; fi
  done
 done
 set --
 for out in build/replace/$kind-*.img; do
  mcopy -o -i "$out" "::$mdest" build/replace/readback || { fail "destination chain is independently readable"; }
  case $out in *-1-0.img) mcopy -o -i "$input" "::$mdest" build/replace/old-expected; cmp build/replace/old-expected build/replace/readback;;
   *) cmp build/replace/expected-$kind build/replace/readback
      set -- "$@" "$out"
      if mcopy -o -i "$out" "::$mstage" build/replace/readback 2>/dev/null; then echo 'FAIL staging entry retained';exit 1;fi;;
  esac
  fsck.fat -n "$out" > build/replace/fsck.log
 done
 "$PYTHON_FAT_REFERENCE" kernel/tests/disk/check_replace.py "$input" "$stage" "$dest" "$@"
done
for pair in 'STAGE.BIN SUB\DEST.BIN' 'STAGE.BIN C:\DEST.BIN' 'STAGE.BIN STAGE.BIN' 'MISSING.BIN DEST.BIN' 'SUB DEST.BIN'; do
 set -- $pair
 cp "$image" build/replace/refuse.img
 case $1:$2 in MISSING*) expected=2;; STAGE.BIN:SUB*|STAGE.BIN:C:*) expected=17;; *) expected=5;; esac
 build/test_replace build/replace/refuse.img "$1" "$2" 0 0 "$expected"
 cmp "$image" build/replace/refuse.img
done
"$PYTHON_FAT_REFERENCE" kernel/tests/disk/check_conservation.py build/replace/*.img
printf 'PASS replace: one-sector commit; mtools/fsck/allocation fault sweep\n'
