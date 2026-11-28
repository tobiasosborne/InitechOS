# Match the real Mac capture's unobscured desktop behind rounded corners.
# Close/reopen the Finder windows through its ordinary Close Window command;
# original accessory trace starts at the system menu hotspot (20,9).
journey() {
 inject 'kctrl-w,kctrl-w'
 move_by -92 -93
 inject 'l1,w300,m40:41,l0'
 await_marker 'FLAIR-DISPATCH app=CALCULAT'
 capture calculator
 # initech-8l4kp: the held key, as minted. 12+3= reads 15 (s8_calc_12plus3eq;
 # '+' by its captured centre (351,123), as below); key 7 is pressed with the pointer where the capture pair puts it, screen
 # (284,101) in our frame (the capture's (284,175): our window meets the menu
 # bar 74 px higher); it is captured held, then released. Clear returns to 0.
 # FLAIR consumes keyboard events inside a mouse tracker. Wait for the
 # Calculator to receive each arithmetic key before starting the next track.
 # Under load the old trace delivered 2, 3 and Return inside the held 7.
 inject k1
 await_marker 'TENANT-EVT what=3'
 inject k2
 await_marker 'TENANT-EVT what=3'
 move_by 291 73
 await_marker 'FLAIR-CURSOR x=351 y=123'
 inject l1
 await_marker 'TENANT-GATE ax=0x00c0 -> 0'
 inject l0
 await_marker 'TENANT-TRACK up h=351 v=123'
 inject k3
 await_marker 'TENANT-EVT what=3'
 inject kret
 await_marker 'TENANT-EVT what=3'
 move_by -67 -22
 await_marker 'FLAIR-CURSOR x=284 y=101'
 inject l1
 await_marker 'TENANT-GATE ax=0x00c0 -> 0'
 if [ "$emulator" = qemu ]; then
  # A one-pixel move within the key and back is acknowledged inside the
  # tracker, after the pressed face has been presented.
  inject m1:0
  await_marker 'FLAIR-CURSOR x=285 y=101'
  inject m-1:0
  await_marker 'FLAIR-CURSOR x=284 y=101'
  capture calculator-held7
 fi
 inject l0
 await_marker 'TENANT-TRACK up h=284 v=101'
 if [ "$emulator" = qemu ]; then
  # The next desktop clock poll follows release handling and repaint.
  await_marker FLAIR-CLOCK-POLL
  capture calculator-after7
 fi
 inject kc
 move_by -224 -51
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
