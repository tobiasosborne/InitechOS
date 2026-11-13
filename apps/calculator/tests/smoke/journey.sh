# Match the real Mac capture's unobscured desktop behind rounded corners.
# Close/reopen the Finder windows through its ordinary Close Window command;
# original accessory trace starts at the system menu hotspot (20,9).
journey() {
 inject 'kctrl-w,kctrl-w'
 move_by -58 -93
 inject 'l1,w300,m40:41,l0'
 await_marker 'FLAIR-DISPATCH app=CALCULAT'
 capture calculator
 inject kctrl-q
 await_marker 'TENANT-EXIT rc=0 via=exit'
 move_by 260 190
 inject "$open_disk"
 await_marker FINDER-OPEN-VOLUME
 inject "$open_apps"
 await_marker 'FINDER-OPEN-FOLDER name=APPS'
 inject m22:20
}
