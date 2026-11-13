#!/bin/sh
# One suite per shared host-test runner entry; all scratch stays under build/.
set -eu
bin=$1
name=${bin##*/}
out=${bin%/*}/$name.work
rm -rf "$out"
mkdir -p "$out"
ref=tests/oracles/wk1_formula_ref.py
case $name in
  test_i123_wk1_roundtrip) "$bin" "$LOTUS123_DECOMP" "$out" tests/oracles/wk1_canon.py ;;
  test_i123_formula|test_i123_formula_x87_037F)
    "$bin" "$LOTUS123_DECOMP" "$out" "$ref" ;;
  test_i123_formula_x87_027F)
    rc=0
    "$bin" "$LOTUS123_DECOMP" "$out" "$ref" > "$out/precision.log" 2>&1 || rc=$?
    cat "$out/precision.log"
    test "$rc" -eq 1
    test "$(grep -c 'FAIL [AB]:' "$out/precision.log")" -eq 3
    if grep -Eq 'RED-REASON (values|recalc-file|property|manual)' "$out/precision.log"; then
      echo 'FAIL: 53-bit precision run has failures beyond the recorded literal diagnostics' >&2
      exit 1
    fi
    grep -q 'checks,' "$out/precision.log"
    echo '53-bit x87 correctly RED: typed/round-trip literal precision'
    ;;
  test_i123_recalc_order) "$bin" "$LOTUS123_DECOMP" "$ref" ;;
  test_i123_transfer_baseline) "$bin" "$LOTUS123_DECOMP" "$out" "$ref" ;;
  test_i123_copy_move) "$bin" ;;
  test_i123_copy_corpus) "$bin" --corpus "$LOTUS123_DECOMP" "$out" "$ref" ;;
  test_i123_copy_tile) "$bin" --tile tests/fixtures/i123-copy "$out" "$ref" ;;
  test_i123_menu_tree)
    "$bin" "$LOTUS123_DECOMP/goldens/lotus-123-2.2-installed-ref/extracted/installed/123.RI" tests/oracles/menu/menu_ri_offsets.tsv > "$out/ri.txt"
    diff -u tests/oracles/menu/menu_tree.golden "$out/ri.txt"
    "$bin" > "$out/app.txt"
    diff -u "$out/ri.txt" "$out/app.txt"
    "$bin" --resource > "$out/123.MNU"
    "$bin" --load "$out/123.MNU" > "$out/loaded.txt"
    diff -u "$out/ri.txt" "$out/loaded.txt"
    echo '82 levels, 381 items: RI, embedded and resource menus agree'
    ;;
  *) "$bin" "$out" ;;
esac
# These independent saved-file checks were part of the legacy host targets.
case $name in
  test_i123_quit)
    for f in Q PE X; do
      python3 "$LOTUS123_DECOMP/tools/wk1_ref.py" "$out/$f.WK1" > "$out/$f.ref"
      grep -q 'consumed=.* OK$' "$out/$f.ref"
    done
    grep -q 'LABEL.*c0 r0 label=.*Hello' "$out/Q.ref"
    grep -Eq 'INTEGER.*c1 r0 int=123$' "$out/Q.ref"
    grep -Eq 'NUMBER.*c2 r0 num=1\.5$' "$out/Q.ref"
    grep -Eq 'INTEGER.*c3 r0 int=-7$' "$out/Q.ref"
    grep -q 'LABEL.*c0 r0 label=.*Pend' "$out/PE.ref"
    grep -q 'LABEL.*c0 r0 label=.*Again' "$out/X.ref"
    test ! -e "$out/Z.WK1" && test ! -e "$out/W.WK1"
    echo 'Q/PE/X saved bytes: independent reader green; failed saves absent'
    ;;
  test_i123_entry)
    python3 "$LOTUS123_DECOMP/tools/wk1_ref.py" "$out/CURRENCY.WK1" > "$out/CURRENCY.ref"
    grep -q 'NUMBER.*Currency2 c0 r0 num=12.34' "$out/CURRENCY.ref"
    grep -q 'FORMULA.*c1 r0 cached=12.34' "$out/CURRENCY.ref"
    grep -q 'consumed=.* OK$' "$out/CURRENCY.ref"
    echo 'CURRENCY saved bytes: independent reader green'
    ;;
esac
