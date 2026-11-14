# Common primitives; scripts emit a trace rather than controlling a second boot.
# Both frontends stop injection on missing W progress. The journey Bochs
# frontend adds serial waits over the unchanged common RFB transport.
inject() { printf '%s,' "$1"; }
expect_marker() {
    printf '%s\n' "$1" >> "$markers"
}
await_marker() {
    expect_marker "$1"
    inject "W$1"
}
capture() {
    if [ "$emulator" = qemu ]; then inject "D$out/$1.ppm"; fi
}
# Boot pointer starts at (320,240); APPS is the first folder in the working
# volume. The imported full-name layout uses 128-pixel columns; the reviewed
# ordinary APPS geometry puts its first icon's pointer anchor at (112,102).
open_disk='m100:-98,m100:-98,m80:0,l1,l0,l1,l0'
# Open the folder by name too. The following movements only park the pointer;
# they never choose a filesystem entry or depend on the icon's rectangle.
open_apps='kup,ka,kp,kp,ks,kctrl-o,m-100:8,m-100:8,m-100:8,m-100:8,m-100:8,m-10:-2'
exit_desktop='m58:-92,l1,m0:100,m0:33,l0'
move_by() {
    move_x=$1 move_y=$2
    while [ "$move_x" -ne 0 ] || [ "$move_y" -ne 0 ]; do
        step_x=$move_x step_y=$move_y
        [ "$step_x" -le 100 ] || step_x=100
        [ "$step_x" -ge -100 ] || step_x=-100
        [ "$step_y" -le 100 ] || step_y=100
        [ "$step_y" -ge -100 ] || step_y=-100
        inject "m$step_x:$step_y"
        move_x=$((move_x-step_x)) move_y=$((move_y-step_y))
    done
}
launch_app() {
    count=$(printf '%s\n' "$app_entries" | awk -F/ -v app="$1" '$NF==app {n++} END{print n+0}')
    [ "$count" -eq 1 ] || fail "APPS must contain exactly one $1"
    app_dx=0 app_dy=0
    # fw_key explicitly clears type-ahead on Up, then selects and scrolls
    # a full filename into view. Full spelling distinguishes 123.EXE/123.MNU.
    # Keyboard selection reaches offscreen rows without moving the pointer.
    app_keys=$(printf '%s\n' "$1" | tr 'A-Z' 'a-z' | fold -w1 |
      while IFS= read -r key; do
       case $key in .) key=dot;; [a-z0-9]) ;; *) exit 1;; esac
       printf 'k%s,' "$key"
      done)
    inject "kup,${app_keys}kctrl-o"
    # Optional second argument is the app registration name when it differs.
    await_marker "FLAIR-DISPATCH app=${2:-${1%.EXE}}"
}
restore_pointer() { move_by "$((-app_dx))" "$((-app_dy))"; }
