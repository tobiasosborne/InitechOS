# Common primitives; scripts emit a trace rather than controlling a second boot.
# Both frontends stop injection on missing W progress. The journey Bochs
# frontend adds serial waits over the unchanged common RFB transport.
inject() { printf '%s,' "$1"; }
await_marker() {
    printf '%s\n' "$1" >> "$markers"
    inject "W$1"
}
capture() {
    if [ "$emulator" = qemu ]; then inject "D$out/$1.ppm"; fi
}
# Production geometry from legacy-final flair_exit_dos_traces.mk. Boot pointer
# starts at (320,240); APPS is the first folder in the working volume.
open_disk='m100:-88,m100:-88,m80:0,l1,l0,l1,l0'
open_apps='m-100:8,m-100:8,m-100:8,m-100:8,m-100:8,m-44:-2,l1,l0,l1,l0'
exit_desktop='m92:-92,l1,m0:100,m0:33,l0'
