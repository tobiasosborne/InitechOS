#!/bin/sh
set -eu
bin=$1
out=$2
name=${bin##*/}
root=$(pwd)
mkdir -p "$out/runs/$name/work/build"
ln -sfn "$SYSTEM7_DECOMP" "$out/runs/$name/system7-decomp"
cd "$out/runs/$name/work"
case "$name" in
 test_chicago_pack)
  cmp "$out/generated/chicago12.h" "$root/include/assets/chicago12.h"
  "$bin" ;;
 test_geneva12_pack)
  cmp "$out/generated/geneva12.h" "$root/include/assets/geneva12.h"
  "$bin" ;;
 test_boot_logo)
  cmp "$out/generated/boot_logo.h" "$root/include/assets/boot_logo.h"
  "$bin" ;;
 test_asset_lz)
  cmp "$out/generated/asset_lz_data.h" "$root/tests/goldens/asset_lz_data.h"
  "$bin" ;;
 test_color_canon)
  cmp "$out/generated/color_canon.h" "$root/include/assets/color_canon.h"
  "$bin" ;;
 test_flair_part_map)
  "$bin" > transcript
  diff -u "$root/tests/fixtures/flair_part_map.golden" transcript ;;
 test_cursor) "$bin" "$SYSTEM7_DECOMP/goldens/captures/s8_alert_modal.png" "$SYSTEM7_DECOMP/goldens/captures/s8_desktop_menubar.png" ;;
 test_menu_look) "$bin" "$SYSTEM7_DECOMP/goldens/captures/s8_menu_dropped.png" "$SYSTEM7_DECOMP/goldens/resources/NFNT_5478.bin" ;;
 test_scrollbar_look) "$bin" "$SYSTEM7_DECOMP/goldens/captures/s8_doc_window_inactive.png" ;;
 *) "$bin" ;;
esac
