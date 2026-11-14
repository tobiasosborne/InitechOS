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
  test_i123_menu_routes)
    awk -v ids=1 -f tests/oracles/menu/project.awk tests/oracles/menu/omissions.tsv tests/oracles/menu/descriptions.tsv tests/oracles/menu/menu_tree.golden > "$out/routes.tsv"
    "$bin" "$out" "$out/routes.tsv"
    ;;
  test_i123_import)
    (cd tests/fixtures/files && sha256sum -c SHA256SUMS) > "$out/capture-hashes.log"
    rc=0; "$bin" "$out" || rc=1
    i=0
    for c in CR LF CTRLZ MIX LONG PROTECTED EDGE RIGHT FULL PREFIX AFTERLONG MAX240 CONTROLS TABLIMIT OTHER; do
      f=$(printf 'I%02u.WK1' "$i");set --
      if [ "$c" = FULL ]; then set -- --native-import-prefix; fi
      python3 tests/oracles/worksheet_records.py "tests/fixtures/files/import/$c/RESULT.WK1" "$out/$f" "$@" || rc=1
      i=$((i+1))
    done
    test "$rc" -eq 0
    ;;
  test_i123_files)
    (cd tests/fixtures/files && sha256sum -c SHA256SUMS) > "$out/capture-hashes.log"
    "$bin" "$out"
    index=0
    for category in w p g o; do
      actual=$(CDPATH= cd -- "$out/c$index/FILES" && pwd)
      (cd "$actual" && find . -type f | LC_ALL=C sort | xargs sha256sum) > "$out/c$index/files.sha256"
      diff -u "tests/fixtures/files/fat/$category/after.sha256" "$out/c$index/files.sha256"
      (cd "$actual" && find . -type d | LC_ALL=C sort) > "$out/c$index/dirs.txt"
      diff -u "tests/fixtures/files/fat/$category/dirs.txt" "$out/c$index/dirs.txt"
      cmp "$out/c$index/BEFORE.WK1" "$out/c$index/AFTER.WK1"
      index=$((index+1))
    done
    cmp "$out/routing/READBEF.WK1" "$out/routing/FILES/READAFT.WK1"
    ;;
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
  test_i123_zero)
    (cd tests/fixtures/zero && sha256sum -c SHA256SUMS) > "$out/capture-hashes.log"
    "$bin" "$out"
    for n in YES LEFT RIGHT CENTER REPEAT PLAIN EMPTY; do cmp "$out/${n}B.WK1" "$out/${n}A.WK1"; done
    python3 tests/oracles/worksheet_records.py tests/fixtures/zero/protected/PROTECT.WK1 "$out/PROTECT.WK1"
    ;;
  test_i123_fill)
    (cd tests/fixtures/fill && sha256sum -c SHA256SUMS) > "$out/capture-hashes.log"
    rc=0; "$bin" "$out" || rc=1
    for pair in "basic BASIC" "fractional FRAC" "descending DESC" "formatted FMT" "protected PROT" "expressions EXPR" "zero ZERO" "bad_direction BADDIR" "decimal DECIMAL"; do
      set -- $pair
      python3 tests/oracles/worksheet_records.py "tests/fixtures/fill/$1/RESULT.WK1" "$out/$2.WK1" || rc=1
    done
    python3 tests/oracles/worksheet_records.py tests/fixtures/fill/reuse/REUSE.WK1 "$out/REUSE.WK1" || rc=1
    python3 tests/oracles/worksheet_records.py tests/fixtures/fill/reuse/REUSE.WK1 "$out/CANCEL.WK1" || rc=1
    python3 tests/oracles/worksheet_records.py tests/fixtures/fill/memory/FULLREF.WK1 "$out/MEMORY.WK1" --native-fill-prefix || rc=1
    test "$rc" -eq 0
    ;;
  test_i123_time)
    (cd tests/fixtures/time-formats && sha256sum -c SHA256SUMS) > "$out/capture-hashes.log"
    "$bin" "$out"
    python3 tests/oracles/time_records.py tests/fixtures/time-formats/TIMEREF.WK1 "$out/TIMEREF.WK1"
    for f in GLOBAL1 GLOBAL2; do python3 tests/oracles/time_records.py "tests/fixtures/time-formats/global/$f.WK1" "$out/$f.WK1"; done
    python3 tests/oracles/time_records.py tests/fixtures/time-formats/final/NARROW.WK1 "$out/NARROW.WK1"
    python3 tests/oracles/time_records.py tests/fixtures/time-formats/more/MOREREF.WK1 "$out/MOREREF.WK1"
    ;;
  test_i123_widthrange)
    (cd tests/fixtures/width-range && sha256sum -c SHA256SUMS) > "$out/capture-hashes.log"
    "$bin" "$out"
    for f in RANGESET RANGERST RANGEIV RANGECAN; do python3 tests/oracles/width_records.py "tests/fixtures/width-range/$f.WK1" "$out/$f.WK1"; done
    python3 tests/oracles/edit_labels.py "$out/KEEPBEF.WK1" "$out/KEEPAFT.WK1"
    ;;
  test_i123_conformance)
    (cd tests/conformance/fixtures && sha256sum -c ../SHA256SUMS) > "$out/fixture-hashes.log"
    (cd tests/conformance && sha256sum -c CAPTURES.sha256) > "$out/capture-hashes.log"
    (cd tests/conformance && sha256sum -c BOUNDARIES.sha256) > "$out/boundary-hashes.log"
    for c in F124 N124 B123 D119 S249 F125 B124 D125; do cp "tests/conformance/boundaries/$c/CASE.WK1" "$out/$c.WK1"; done
    cp tests/conformance/fixtures/L05.WK1 tests/conformance/fixtures/L10.WK1 tests/conformance/fixtures/L12.WK1 "$out/"
    "$bin" "$out"
    python3 tests/oracles/edit_labels.py tests/conformance/captures/L10-f2/AFTER.WK1 "$out/L10F2.WK1"
    ;;
  test_i123_protection)
    captures=${I123_PROTECTION_ROOT:-../../goldens/dataloss/lotus}
    manifest=$(pwd)/tests/oracles/name-protection.sha256
    (cd "$captures" && sha256sum -c "$manifest") > "$out/name-hashes.log"
    (cd tests/fixtures/protection && sha256sum -c SHA256SUMS) > "$out/write-hashes.log"
    "$bin" "$out" "$captures" tests/fixtures/protection
    ;;
  test_i123_menu_disk) "$bin" "${bin%/*}/../123.MNU" ;;
  test_i123_menu_tree)
    menu_checks=0
    menu_fail() { echo "FAIL menu: $1"; echo "INITECH_CHECKS $menu_checks failures=1"; exit 1; }
    menu_run() {
      label=$1; output=$2; shift 2; menu_checks=$((menu_checks+1)); rc=0
      "$bin" "$@" > "$output" || rc=$?
      case $rc in 0) : ;; 1) menu_fail "$label" ;; *) exit 2 ;; esac
    }
    menu_compare() {
      menu_checks=$((menu_checks+1)); rc=0
      diff -u --label expected --label actual "$2" "$3" || rc=$?
      case $rc in 0) : ;; 1) menu_fail "$1" ;; *) exit 2 ;; esac
    }
    menu_run 'full RI reader' "$out/ri.txt" "$LOTUS123_DECOMP/goldens/lotus-123-2.2-installed-ref/extracted/installed/123.RI" tests/oracles/menu/menu_ri_offsets.tsv
    menu_compare 'full captured RI strings' tests/oracles/menu/menu_tree.golden "$out/ri.txt"
    awk -F '|' '$2 ~ /^ `\// {split($2,p,"`");n=split($3,a,"`");for(i=2;i<n;i+=2)print p[2] "\t" a[i]}' README.md > "$out/readme-omissions.tsv" || exit 2
    menu_compare 'frozen README omissions' tests/oracles/menu/omissions.tsv "$out/readme-omissions.tsv"
    awk -f tests/oracles/menu/project.awk tests/oracles/menu/omissions.tsv tests/oracles/menu/descriptions.tsv "$out/ri.txt" > "$out/shipped.txt" || exit 2
    awk -F '\t' 'NR==FNR{keep[$1]=1;next}!seen[$1]++&&!keep[$1]{print $1}' "$out/shipped.txt" "$out/ri.txt" > "$out/pruned-branches.txt" || exit 2
    menu_compare 'frozen empty branches' tests/oracles/menu/pruned-branches.txt "$out/pruned-branches.txt"
    awk -F '\t' 'NR==FNR{keep[$1]=1;next}$2=="Quit"&&!keep[$1]{print $1 "/" $2}' "$out/shipped.txt" "$out/ri.txt" > "$out/pruned-navigation.txt" || exit 2
    menu_compare 'frozen navigation removals' tests/oracles/menu/pruned-navigation.txt "$out/pruned-navigation.txt"
    menu_run 'embedded hierarchy invariant' "$out/app.txt"
    menu_compare 'shipped hierarchy and descriptions' "$out/shipped.txt" "$out/app.txt"
    menu_run 'resource export' "$out/123.MNU" --resource
    menu_run 'shipped resource accepted' "$out/loaded.txt" --load "$out/123.MNU"
    menu_compare 'loaded hierarchy' "$out/shipped.txt" "$out/loaded.txt"
    awk -v ids=1 -f tests/oracles/menu/project.awk tests/oracles/menu/omissions.tsv tests/oracles/menu/descriptions.tsv "$out/ri.txt" > "$out/actions.txt" || exit 2
    menu_run 'embedded action invariants' "$out/app-actions.txt" --actions
    menu_compare 'stable action identities' "$out/actions.txt" "$out/app-actions.txt"
    menu_run 'independent resource grammar' "$out/resource-actions.txt" --inspect "$out/123.MNU"
    menu_compare 'serialized tree and actions' "$out/actions.txt" "$out/resource-actions.txt"
    menu_run 'loaded action invariants' "$out/loaded-actions.txt" --load-actions "$out/123.MNU"
    menu_compare 'loaded stable actions' "$out/actions.txt" "$out/loaded-actions.txt"
    menu_run 'menu source generation' "$out/menu_data.h" --project tests/oracles/menu/menu_tree.golden tests/oracles/menu/omissions.tsv tests/oracles/menu/descriptions.tsv
    menu_compare 'reproducible menu source' core/menu_data.h "$out/menu_data.h"
    echo 'Full RI unchanged; 25 shipped levels, 105 items: subtraction, embedded and independent resource reader agree including actions'
    echo "INITECH_CHECKS $menu_checks failures=0"
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
if [ "$name" = test_i123_edit ]; then
  (cd tests/fixtures/edit && sha256sum -c SHA256SUMS) > "$out/capture-hashes.log"
  python3 tests/oracles/edit_labels.py tests/fixtures/edit/EDITKEYS.WK1 "$out/EDITKEYS.WK1"
fi
if [ "$name" = test_i123_edit ]; then
  python3 "$LOTUS123_DECOMP/tools/wk1_ref.py" "$out/NUMFORM.WK1" > "$out/NUMFORM.ref"
  grep -q 'INTEGER.*c0 r0 int=1203$' "$out/NUMFORM.ref"
  grep -q 'FORMULA.*c1 r0 cached=55' "$out/NUMFORM.ref"
  grep -q 'LABEL.*c2 r0 label=.*Alpha' "$out/NUMFORM.ref"
  grep -q 'LABEL.*c3 r0 label=.*Old' "$out/NUMFORM.ref"
  grep -q 'INTEGER.*c4 r0 int=234$' "$out/NUMFORM.ref"
fi
