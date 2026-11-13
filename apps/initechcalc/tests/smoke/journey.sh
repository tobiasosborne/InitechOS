# The original dual-emulator quit guard saves through FILE COPY.
journey() {
 launch_app CALC.EXE
 inject 'k7,k3,kret'
 capture calc
 restore_pointer
 inject 'kctrl-q,w1000,kret'
 # On Bochs this is the terminal journey. No more input is needed while
 # FILE COPY runs; let the driver's completion wait observe the final exit.
 if [ "$emulator" = bochs ]; then expect_marker TENANT-FILECOPY-DONE
 else await_marker TENANT-FILECOPY-DONE; fi
 await_marker 'TENANT-EXIT rc=0 via=exit'
}
