# The original dual-emulator quit guard saves through FILE COPY.
# Before Save the pointer goes where the still shows the hourglass: the
# centre of goldens/frame/geometry.tsv COPYHOURGLASS, from the trace's common
# anchor (112,102; factory/smoke-desktop.sh). The busy cursor's hotspot is
# its waist (flair/tests/test_canon.c C-1), so the hourglass is drawn round
# that point while the copy runs; tests/frame-pixels.sh grades it there.
journey() {
 glass=$(awk -F'\t' '$1=="COPYHOURGLASS" { printf "%d %d", ($11+$13)/2-112, ($12+$14)/2-102 }' goldens/frame/geometry.tsv)
 [ -n "$glass" ] || fail 'geometry.tsv has no COPYHOURGLASS'
 launch_app CALC.EXE
 inject 'k7,k3,kret'
 capture calc
 restore_pointer
 # shellcheck disable=SC2086
 set -- $glass
 move_by "$1" "$2"
 inject 'kctrl-q,w1000,kret'
 # On Bochs this is the terminal journey. No more input is needed while
 # FILE COPY runs; let the driver's completion wait observe the final exit.
 if [ "$emulator" = bochs ]; then expect_marker TENANT-FILECOPY-DONE
 else
  await_marker 'TENANT-FILECOPY-MID v='
  capture calc-copy
  # Six more frames 60 ms apart for the hourglass (tests/frame-pixels.sh).
  for k in 1 2 3 4 5 6; do inject w60; capture "calc-glass-$k"; done
  await_marker TENANT-FILECOPY-DONE
  move_by "$((-$1))" "$((-$2))"
 fi
 await_marker 'TENANT-EXIT rc=0 via=exit'
}
