# Note Pad and Scrapbook share the persisted TEXT clipboard contract.
journey() {
 launch_app NOTEPAD.EXE
 inject 'ka,kl,kp,kh,ka,kctrl-z'
 # Menu Undo toggles the typing back. The pointer route is separate from Z.
 move_by "$((95-78-app_dx))" "$((9-102-app_dy))"
 inject 'l1,m0:22,l0'
 move_by "$((78+app_dx-95))" "$((102+app_dy-31))"
 inject 'kctrl-a,kctrl-c'
 capture notepad
 restore_pointer
 inject kctrl-w
 await_marker 'TENANT-EXIT rc=0 via=exit'
 launch_app SCRAPBK.EXE
 inject 'kctrl-v,kctrl-x,kctrl-z'
 # Clear with the pointer, Undo via Z: saved page must still be alpha.
 move_by "$((95-78-app_dx))" "$((9-102-app_dy))"
 inject 'l1,m0:94,l0,kctrl-z,kctrl-a'
 move_by "$((78+app_dx-95))" "$((102+app_dy-103))"
 capture scrapbook
 restore_pointer
 inject kctrl-w
 await_marker 'TENANT-EXIT rc=0 via=exit'
}
# The common driver counts exits, including this journey's second application.
journey_exits=2
