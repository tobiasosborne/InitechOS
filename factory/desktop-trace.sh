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
# Production geometry from legacy-final flair_exit_dos_traces.mk. Boot pointer
# starts at (320,240); APPS is the first folder in the working volume.
open_disk='m100:-98,m100:-98,m80:0,l1,l0,l1,l0'
open_apps='m-100:8,m-100:8,m-100:8,m-100:8,m-100:8,m-44:-2,l1,l0,l1,l0'
exit_desktop='m92:-92,l1,m0:100,m0:33,l0'
# Default APPS grid follows FAT enumeration order, four columns at 68x52.
# Ground truth: finder_windows.c fw_enum_cb/finder_win_grid_origin and its
# default frame. mdir supplies input coordinates, never verification results.
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
    slot=$(printf '%s\n' "$app_entries" | awk -F/ -v app="$1" '$NF==app {print NR-1}')
    [ -n "$slot" ] || fail "APPS has no $1"
    app_dx=$((slot%4*68)) app_dy=$((slot/4*52))
    move_by "$app_dx" "$app_dy"
    # Finder File > Open (finder_menu.c): selecting then invoking the command
    # avoids depending on emulator event processing fitting a double click.
    inject 'l1,l0,kctrl-o'
    # Optional second argument is the app registration name when it differs.
    await_marker "FLAIR-DISPATCH app=${2:-${1%.EXE}}"
}
restore_pointer() { move_by "$((-app_dx))" "$((-app_dy))"; }
