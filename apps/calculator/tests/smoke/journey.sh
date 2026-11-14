# Match the real Mac capture's unobscured desktop behind rounded corners.
# Close/reopen the Finder windows through its ordinary Close Window command;
# original accessory trace starts at the system menu hotspot (20,9).
journey() {
 inject 'kctrl-w,kctrl-w'
 move_by -92 -93
 inject 'l1,w300,m40:41,l0'
 await_marker 'FLAIR-DISPATCH app=CALCULAT'
 capture calculator
 # Authored Edit bindings: Copy 20 with the pointer, then Clear a pending
 # operation. Pointer starts at (60,50); Edit is at (95,9).
 inject 'k2,k0,m35:-41,l1,m0:22,l0'
 inject 'kc,k2'
 # '+' by its captured button center; Return is the captured '=' mapping.
 # These input routes are supported by both factory transports.
 move_by 256 92
 inject 'l1,l0,k3'
 move_by -256 -114
 inject 'l1,m0:38,l0,k4,kret'
 # Exit through File Close, then verify Copy through another real app.
 inject kctrl-w
 await_marker 'TENANT-EXIT rc=0 via=exit'
 move_by 225 193
 inject "$open_disk"
 await_marker FINDER-OPEN-VOLUME
 inject "$open_apps"
 await_marker 'FINDER-OPEN-FOLDER name=APPS'
 inject m22:20
 launch_app NOTEPAD.EXE
 # Save the copied readout on page 2, leaving page 1 for Note Pad's journey.
 move_by "$((455-112-app_dx))" "$((269-102-app_dy))"
 inject 'l1,l0,kctrl-v'
 move_by -285 0
 inject 'l1,l0'
 move_by "$((112+app_dx-170))" "$((102+app_dy-269))"
 restore_pointer
 inject kctrl-q
 await_marker 'TENANT-EXIT rc=0 via=exit'
}
journey_exits=2
