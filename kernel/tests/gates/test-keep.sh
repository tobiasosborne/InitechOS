set -eu
printf ">>> test-keep: AH=31h KEEP (TSR) -- mcb resident model + shrink/mark + ALLOC-avoids-kept + terminate-survives + AH=4Dh type 3\n"
build/test_keep
printf ">>> test-keep: green\n"
