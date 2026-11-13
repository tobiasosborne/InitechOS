# Note Pad and Scrapbook share the persisted TEXT clipboard contract.
journey() {
 launch_app NOTEPAD.EXE
 inject 'ka,kl,kp,kh,ka,kctrl-a,kctrl-c'
 capture notepad
 restore_pointer
 inject kctrl-w
 await_marker 'TENANT-EXIT rc=0 via=exit'
 launch_app SCRAPBK.EXE
 inject kctrl-v
 capture scrapbook
 restore_pointer
 inject kctrl-w
 await_marker 'TENANT-EXIT rc=0 via=exit'
}
# The common driver counts exits, including this journey's second application.
journey_exits=2
