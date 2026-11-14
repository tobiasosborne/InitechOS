set -eu
mkdir -p build/commit-outcome
w=build/commit-outcome
printf 'previous named bytes\n' > "$w/old"
printf 'independently minted replacement bytes\n' > "$w/new"
for mode in replace rename; do
 for dir in root sub; do
  base=$w/$mode-$dir.img
  truncate -s 1474560 "$base"
  mformat -i "$base" -f 1440 ::
  mmd -i "$base" ::SUB
  prefix=; test "$dir" = root || prefix=SUB/
  mcopy -i "$base" "$w/new" "::${prefix}STAGE.BIN"
  mcopy -i "$base" "$w/old" "::${prefix}OTHER.BIN"
  if test "$mode" = replace; then mcopy -i "$base" "$w/old" "::${prefix}DEST.BIN"; fi
  for scenario in before after cleanup persistent read1 read2; do
   after=1; cleanup=0; reads=0
   case $scenario in before) after=0;;cleanup) cleanup=1;;persistent) cleanup=2;;read1) reads=1;;read2) reads=2;;esac
   # Rename has no postcommit maintenance to fault.
   case $mode:$scenario in rename:cleanup|rename:persistent) continue;;esac
   out=$w/$mode-$dir-$scenario.img
   cp "$base" "$out"
   build/test_commit_outcome "$out" "$mode" "$dir" "$after" "$cleanup" "$reads"
   if test "$after" = 1; then
    mcopy -o -i "$out" "::${prefix}DEST.BIN" "$w/readback"
    cmp "$w/new" "$w/readback"
   elif test "$mode" = replace; then
    mcopy -o -i "$out" "::${prefix}DEST.BIN" "$w/readback"
    cmp "$w/old" "$w/readback"
   else
    if mdir -i "$out" "::${prefix}DEST.BIN" >/dev/null 2>&1; then exit 1; fi
   fi
   if test "$scenario" != persistent; then
    "$PYTHON_FAT_REFERENCE" "$SOURCE_ROOT/kernel/tests/disk/check_conservation.py" "$out"
   fi
  done
  if test "$mode" = replace; then
   for action in create truncate write grow move resize whole whole-zero replace-source replace-dest; do
    case $dir:$action in sub:whole|sub:whole-zero) continue;; esac
    out=$w/alias-$dir-$action.img
    cp "$base" "$out"
    build/test_commit_outcome "$out" replace "$dir" 1 2 0 "$action"
    # The permitted postcommit cleanup leak already contains an alias and an
    # old allocation. Grade refusal against that exact preimage: no additional
    # cross-link, lost cluster or payload change can hide in a whole-image cmp.
    cmp "$out.before" "$out"
    mcopy -o -i "$out" "::${prefix}DEST.BIN" "$w/readback"
    cmp "$w/new" "$w/readback"
   done
  fi
 done
done
