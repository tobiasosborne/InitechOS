# Preserve the original Bochs Save dialog route and its exact TEXT oracle.
journey() {
 launch_app TEXT.EXE
 inject 'ka,kl,kp,kh,ka,kret,kb,kr,ka,kv,ko'
 capture text
 # Each Edit command has a real changing fixture; final TEXT stays literal.
 inject 'kctrl-a,kctrl-c,kctrl-x,kctrl-v,kctrl-z,kctrl-z,kctrl-a'
 move_by "$((95-78-app_dx))" "$((9-102-app_dy))"
 inject 'l1,m0:94,l0,kctrl-z'
 move_by "$((78+app_dx-95))" "$((102+app_dy-103))"
 inject 'kctrl-s,w600,kret,w500'
 # Save As must open a usable dialog; Escape preserves the saved document.
 move_by "$((55-78-app_dx))" "$((9-102-app_dy))"
 inject 'l1,m0:90,l0,w300,kesc'
 move_by "$((78+app_dx-55))" "$((102+app_dy-99))"
 restore_pointer
 # File's pruned menu: Quit is row 8 (two structural dividers).
 move_by -23 -93
 inject 'l1,m0:100,m0:8,l0'
 move_by 23 -15
 await_marker 'TENANT-EXIT rc=0 via=exit'
}
