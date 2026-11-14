# Same chooser/Pattern cells as the original app trace, absolute pointer
# positions reconstructed from its (20,9) menu anchor and relative steps.
journey() {
 launch_app CPANELS.EXE
 restore_pointer
 move_by 128 98
 inject 'l1,l0'
 # cp_panels paint() finishes its nine PAT# samples at this final pixel.
 # Wait for repaint completion before changing live mouse/key state.
 await_marker 'TENANT-GATE ax=0x0031 win=1 l=271 t=159 r=272 b=160'
 inject 'm-34:12,l1,l0'
 await_marker 'TENANT-GATE ax=0x0031 win=1 l=271 t=159 r=272 b=160'
 capture patterns
 inject kret
 await_marker 'CPANELS-APPLY pattern=3'
 move_by -94 -110
 inject kctrl-q
 await_marker 'TENANT-EXIT rc=0 via=exit'
}
