journey() {
 launch_app PAINT.EXE
 capture canvas
 restore_pointer
 inject kctrl-q
 await_marker 'TENANT-EXIT rc=0 via=exit'
}
