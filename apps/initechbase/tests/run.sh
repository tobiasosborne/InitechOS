#!/bin/sh
# Private repo-shaped scratch preserves legacy driver paths without sharing writes.
set -eu
name=$1
bins=$2
src=$(pwd)
# The factory's kill contract for shell checks (factory/testlib/README.md).
checks=${INITECH_FACTORY_ROOT:-$src/../..}/factory/testlib/shell-checks.sh
export LC_ALL=C SKIP_DBASE3_DECOMP=0 PYTHONDONTWRITEBYTECODE=1
work=$bins/work/$name
rm -rf "$work"
mkdir -p "$work/build"
ln -s "$src/tests" "$work/tests"
ln -s "$src/spec" "$work/spec"
ln -s "$src/include" "$work/include"
# Legacy drivers store the source root before chdir and spawn independent readers.
cd "$work"
# The safety suite supplies independent raw/captured DBFs itself. Running the
# product writer as unrelated setup would mask the writer fault it grades.
if [ "$name" != test_ibase_dbsafe ]; then
 "$bins/mint_clients_dbf" build/CLIENTS.DBF > build/fixture.log
fi
mkdir -p build/dbsafe build/ibase-audit build/ibase.audit
# Only these drivers consume private copies of the captured family trees.
# Unit suites read their own referenced files through tests/ and CORPUS.
case "$name" in
 test_ibase_fns|test_ibase_w2|pending|captured-hashes)
  mkdir -p build/ibasefns/golden
  cp -R tests/xbase_prog_diff/fns-mint/. build/ibasefns/golden/;;
esac
case "$name" in
 test_ibase_dml|captured-hashes)
  mkdir -p build/ibasedml/golden
  cp -R tests/xbase_prog_diff/mint/. build/ibasedml/;;
esac
case "$name" in
 test_ibase_dml|test_ibase_fns|test_ibase_w2)
  while read -r case_name mode; do "$bins/$name" "$case_name" "$mode"; done < "tests/$name.cases"
  if [ "$name" = test_ibase_dml ]; then
   for f in GOLD.OUT BASE.DBF NAMES.NDX VALUES.MEM; do cmp "build/ibasedml/golden/abbrev/$f" "build/ibasedml/golden/abbrfull/$f"; done
  fi;;
 test_ibase_report)
  rc=0
  "$bins/$name" "$DBASE3_DECOMP" > build/report.log 2>&1 || rc=$?
  cat build/report.log
  test "$rc" -eq 0 || exit "$rc"
  grep -c 'byte-identical to real dBASE' build/report.log | grep -qx 124;;
 test_ibase_ndx_names)
  nm -u "$bins/../host-core/core/fn_builtins.o" > build/ndx-undefined.log
  if grep -Eq '[[:space:]](wa_|ndx_)' build/ndx-undefined.log; then cat build/ndx-undefined.log; exit 1; fi
  "$bins/$name";;
 test_ibase_ovl) "$bins/$name" --pure;;
 mzlink-io) make -C "${INITECH_FACTORY_ROOT:-$src/../..}/factory/mz" test;;
 pending)
  case "${CASE:?set CASE to a pending function or set-/verb- case}" in
   set-*|verb-*) "$bins/test_ibase_w2" "$CASE" host;;
   *) "$bins/test_ibase_fns" "$CASE" host;;
  esac;;
 retained-screens)
  ln -s "$bins/screen_tool" build/screen_tool
  ln -s "$bins/w4_screen_check" build/w4_screen_check
  INITECH_SHELL_CHECKS=$checks sh tests/w4_retained_test.sh;;
 conformance_host)
  mkdir -p "$work/conformance"
  ln -s "$bins/conformance_host" "$work/conformance/host"
  cases=$(cat tests/conformance/accepted.cases)
  CASES="$cases" sh tests/conformance/run-host.sh "$work/conformance"
  CASES="$cases" sh tests/conformance/check.sh "$work/conformance"
  ;;
 test_ibase_datatype)
  for c in blankdate overflow dateindex dateindex-blank boundary blankdtoc numericfit valstar datecontract; do "$bins/$name" "$c"; done;;
 test_ibase_dbsafe)
  # The modes are independent processes that share only build/dbsafe, so
  # each runs in its own copy of the scratch tree, four at a time in the
  # sequential order. Each is awaited in that order; the first failing mode
  # ends the run, its batch-mates are stopped, and the printed logs are
  # exactly those the sequential loop printed.
  for batch in "append lifetime safety writes" "ndxfail rowfail dbtfail retry" "packfail fromfail blankfail"; do
   pids=
   for mode in $batch; do
    mkdir -p "modes/$mode/build/dbsafe" "modes/$mode/build/ibase-audit" "modes/$mode/build/ibase.audit"
    for d in tests spec include; do ln -s "$src/$d" "modes/$mode/$d"; done
    (cd "modes/$mode" && exec "$bins/$name" "$mode") > "modes/$mode.log" 2>&1 &
    pids="$pids $!"
   done
   set -- $pids
   for mode in $batch; do
    pid=$1; shift
    rc=0; wait "$pid" || rc=$?
    cat "modes/$mode.log"
    if [ "$rc" != 0 ]; then
     for other in "$@"; do kill "$other" 2>/dev/null || :; done
     wait 2>/dev/null || :
     exit "$rc"
    fi
   done
  done;;
 test_interp_do_file) sh tests/do_file_test.sh host "$bins/$name";;
 test_ibase_memocells)
  mkdir -p build/memocells/DRIVE build/memocells/DATA
  "$bins/w2_fixture" build/memocells
  cd build/memocells
  "$bins/$name";;
 ibase_numeric_format) python3 tests/numeric_format_ref.py "$bins/$name" "$DBASE3_DECOMP";;
 readers)
  python3 tests/dbf_ref.py --selftest
  python3 tests/ndx_ref.py --selftest
  python3 tests/conformance/read_dbf.py --selftest;;
 captured-hashes)
  . "$checks"
  (cd tests/write-safety/captured && sha256sum --quiet -c SHA256SUMS) || fail "captured-hashes: write-safety captures"; ok
  (cd tests/prompt-tail && sha256sum --quiet -c SHA256SUMS) || fail "captured-hashes: prompt-tail captures"; ok
  for manifest in use_mint005.md5 report_mint004.md5; do
   (cd "$DBASE3_DECOMP/mint/work" && md5sum --quiet -c "$src/tests/$manifest") || fail "captured-hashes: $manifest"; ok
  done
  (cd "$DBASE3_DECOMP/mint/work" && sha256sum --quiet -c "$src/tests/ndx_incremental.sha256") || fail "captured-hashes: ndx_incremental"; ok
  sha256sum --quiet -c tests/list_off_real.sha256 || fail "captured-hashes: list_off_real"; ok
  for manifest in prog-real.sha256 dml-goldens.sha256 abbrev-goldens.sha256 fns-accepted.sha256 inspection-goldens.sha256; do
   sha256sum --quiet -c "tests/xbase_prog_diff/$manifest" || fail "captured-hashes: $manifest"; ok
  done
  (cd tests/xbase_prog_diff/ibasectl-mint && md5sum --quiet -c ibasectl_mint.md5) || fail "captured-hashes: ibasectl-mint"; ok
  for tree in tests/xbase_prog_diff/*-mint/*; do
   if [ -f "$tree/sha256.txt" ]; then (cd "$tree" && sha256sum --quiet -c sha256.txt) || fail "captured-hashes: $tree"; ok; fi
  done
  # Real III+ forms captures: pinned outputs and the inputs that made them.
  n=0
  for tree in tests/xbase_prog_diff/screen-mint/forms-*; do
   (cd "$tree" && sha256sum --quiet -c SHA256SUMS) || fail "captured-hashes: $tree outputs"; ok
   (cd "$tree" && sha256sum --quiet -c INPUTS.sha256) || fail "captured-hashes: $tree inputs"; ok
   grep -q '3f7ba1f5' "$tree/ORACLE.sha256" || fail "captured-hashes: $tree is not the real DBASE.EXE"; ok
   n=$((n+1))
  done
  test "$n" -ge 50 || fail "captured-hashes: only $n forms captures"; ok
  summary;;
 w4-files)
  for kind in frm lbl qry scr fmt config prg vue exact; do ln -s "$bins/${kind}_diff" "build/${kind}_diff"; done
  ln -s "$bins/w4_mutate" build/w4_mutate
  INITECH_SHELL_CHECKS=$checks sh tests/w4_files_paths_test.sh;;
 messages-generated)
  . "$checks"
  "$bins/../tools/ibase_msg_pack" spec/ibase/dbase_msg_codes.tsv > build/messages.h || fail "messages-generated: packer"; ok
  cmp include/ibase/messages_packed.h build/messages.h || fail "messages-generated: messages_packed.h is not the packed catalogue"; ok
  summary;;
 core-live)
  command -v dosbox-x >/dev/null
  root=$work/live
  mkdir -p "$root/build/ibase-core-mint"
  "$bins/test_ibase_core" "$root"
  "$bins/ibase_core_ref" "$root" prepare
  for f in DBASE.EXE DBASE.OVL DBASE.MSG DBASEINL.OVL; do cp "$DBASE3_DECOMP/mint/work/$f" "$root/build/ibase-core-mint/"; done
  cp "$root/build/core-memo.dbf" "$root/build/ibase-core-mint/MEMO.DBF"
  cp "$root/build/core-memo.dbt" "$root/build/ibase-core-mint/MEMO.DBT"
  SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout -k 5 30 dosbox-x -conf "$root/build/ibase-core-mint/ref.conf" -exit > "$root/mint.log" 2>&1
  "$bins/ibase_core_ref" "$root";;
 *)
  # The table is repository-owned data, with CORPUS expanded as one argument.
  line=$(awk -v n="$name" '$1==n {$1=""; print; found=1} END {if(!found) exit 1}' tests/standalone.cases)
  set --
  for arg in $line; do
   case "$arg" in CORPUS) set -- "$@" "$DBASE3_DECOMP";; *) set -- "$@" "$arg";; esac
  done
  "$bins/$name" "$@";;
esac
