# Same chooser/Pattern cells as the original app trace, absolute pointer
# positions reconstructed from its (20,9) menu anchor and relative steps.
journey() {
 launch_app CPANELS.EXE
 restore_pointer
 move_by 162 98
 inject 'l1,l0,w200,m-34:12,l1,l0,w200'
 capture patterns
 inject 'kret,w300'
 move_by -128 -110
 inject kctrl-q
 await_marker 'TENANT-EXIT rc=0 via=exit'
}
