# Source after changing to the project root. The caller provides run_emulator,
# its evidence directory (out) and, optionally, a running assertion count.
# Keep standalone and mutation probes in their original fail-fast order.
smoke_emulators() {
    if [ "${INITECH_FULL_SMOKE:-0}" != 1 ]; then
        for emulator do run_emulator "$emulator"; done
        return
    fi
    smoke_pids=
    for emulator do
        (
            checks=0
            run_emulator "$emulator"
            echo "$checks" > "$out/$emulator.checks"
        ) > "$out/$emulator.smoke-log" 2>&1 &
        smoke_pids="$smoke_pids $!:$emulator"
    done
    smoke_rc=0
    for smoke_job in $smoke_pids; do
        emulator=${smoke_job#*:}
        rc=0
        wait "${smoke_job%:*}" || rc=$?
        cat "$out/$emulator.smoke-log"
        if [ "$rc" -ne 0 ]; then
            echo "FAIL smoke: $0 $emulator (exit $rc)" >&2
            if [ "$rc" -gt "$smoke_rc" ]; then smoke_rc=$rc; fi
        else
            smoke_count=$(cat "$out/$emulator.checks")
            checks=$((${checks:-0} + smoke_count))
        fi
    done
    return "$smoke_rc"
}
