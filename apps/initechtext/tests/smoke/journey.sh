# Preserve the original Bochs Save dialog route and its exact TEXT oracle.
journey() {
 launch_app TEXT.EXE
 inject 'ka,kl,kp,kh,ka,kret,kb,kr,ka,kv,ko'
 capture text
 inject 'kctrl-s,w600,kret,w500'
 restore_pointer
 inject kctrl-q
 await_marker 'TENANT-EXIT rc=0 via=exit'
}
