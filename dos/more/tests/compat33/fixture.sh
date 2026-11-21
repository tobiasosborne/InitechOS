# Sourced by the reference capture and product replay.
more_fixture() {
    awk 'BEGIN { for (i=1; i<=60; ++i) printf "PAGE-L%02d\r\n", i }' > "$out/PAGES.TXT"
    awk 'BEGIN { for (i=1; i<=60; ++i) printf "STOP-L%02d\r\n", i }' > "$out/STOP.TXT"
    printf 'a short pipe\r\n' > "$out/SHORT.TXT"
    mcopy -i "$out/data.img" "$out/PAGES.TXT" "$out/STOP.TXT" "$out/SHORT.TXT" ::
}
