#!/bin/sh
# Only the listed contracts are visible; never expose the whole attic to C.
set -eu
src=$1
out=$2
public=$3
mkdir -p "$out/sdk" "$out/dos"
for h in toolbox_gate event_model find_data cp_dialog cpanels_record; do
 ln -sf "$src/$h.h" "$out/sdk/$h.h"
done
# Real-DOS integration test only. These stay owned by the kernel importer.
for h in dos_structs tsr_contract memory_map initechmz_extended; do
 ln -sf "$src/$h.h" "$out/dos/$h.h"
done
ln -sf "$public/grafport.h" "$out/sdk/grafport.h"
touch "$out/stamp"
