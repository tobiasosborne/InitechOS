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
  done
 done
done
