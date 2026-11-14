journey() {
 launch_app PAINT.EXE
 capture canvas
 restore_pointer
 # File Close via pointer, then Close W, Quit Q, and the close box each
 # exercise the same documentless lifecycle in separate launches.
 move_by -57 -93
 inject 'l1,m0:22,l0'
 move_by 57 71
 await_marker 'TENANT-EXIT rc=0 via=exit'
 launch_app PAINT.EXE
 restore_pointer
 inject kctrl-w
 await_marker 'TENANT-EXIT rc=0 via=exit'
 launch_app PAINT.EXE
 restore_pointer
 # Canvas frame (330,20): documentProc close box near (340,30).
 move_by 228 -72
 inject 'l1,l0'
 move_by -228 72
 await_marker 'TENANT-EXIT rc=0 via=close'
 launch_app PAINT.EXE
 restore_pointer
 inject kctrl-q
 await_marker 'TENANT-EXIT rc=0 via=exit'
}
# The shared frontend counts the exit-verb marker; close-box is a distinct
# via=close marker checked above, with Quit last for its completion guard.
journey_exits=3
