#!/bin/sh
# Certificate evidence grammar, PRD 8/9; initech-0spyf/fzsvo.
# Keep the actual named verdict. Never infer success from exit zero alone.
cert_pick_summary() {
    awk -v gate="$1" '
        BEGIN {unit=gate; gsub(/-/,"_",unit)}
        function named(prefix) {return $0==prefix || index($0,prefix ":")==1 || index($0,prefix " ")==1}
        index($0,">>> " gate ": ")==1 || named("GREEN " gate) || named("RED " gate) ||
        index($0,"[SKIP] " gate ": ")==1 || index($0,gate ": ")==1 || index($0,unit ": ")==1 {last=$0}
        END {if(length(last)) print last}
    ' "$2"
}
cert_summary_kind() {
    cert_gate=$1
    cert_line=$2
    case $cert_line in
        ">>> $cert_gate: green"|">>> $cert_gate: green "*|"GREEN $cert_gate"|"GREEN $cert_gate:"*|"GREEN $cert_gate "*)
            printf 'green\n'; return 0;;
    esac
    cert_unit=$(printf '%s' "$cert_gate" | tr '-' '_')
    case $cert_line in "$cert_gate: "*|"$cert_unit: "*)
        cert_counts=${cert_line#*: }
        if printf '%s\n' "$cert_counts" | grep -Eq '^[1-9][0-9]* checks, 0 failures$'; then
            printf 'green\n'; return 0
        fi;;
    esac
    # Operator exception, 2026-11-06: retained in coverage and NOT PROVEN.
    # No other gate, skip reason or local reference is eligible.
    if [ "$cert_gate" = test-assets ] &&
       [ "$cert_line" = '[SKIP] test-assets: local-only fixture spec/assets/preview.webp absent (gitignored copyrighted frame); palette-honesty check requires it' ]; then
        printf 'skip\n'; return 0
    fi
    return 1
}
