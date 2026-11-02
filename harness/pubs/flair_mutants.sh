#!/bin/sh
# flair_mutants.sh -- Rule 6 for the FLAIR manuals' gates (bead
# initech-ipu5.11): each NEW rule of test-pubs-flair / test-pubs-flair-
# screens is run against an isolated one-fault fixture and must go RED with
# its own tag.  No tracked publication, capture, inventory or allow-list is
# changed (the harness/pubs/mutants.sh discipline).
#
# Usage: flair_mutants.sh <pubs_gate> <flair_grey> <check dir> <gate build dir>
#                         <flair.allow> <book>...
# Environment (from pubs/flair.mk): CC, MENU_INC, FLAIR_KMAIN_I (the
# preprocessed flagship kmain), FLAIR_FIXTURES, HARNESS, FLAIR_BOOT,
# FLAIR_DATA, CONSOLE_TEXT, VGABIOS.
set -eu
gate=$1; grey=$2; out=$3; built=$4; allow=$5; shift 5
books=$*
m=$out/flair-mutants
rm -rf "$m"; mkdir -p "$m"
red() {
    name=$1; tag=$2; shift 2
    if "$@" > "$m/$name.log" 2>&1; then
        cat "$m/$name.log"; printf '!!! test-pubs-flair-mutant FAIL: %s survived\n' "$name"; exit 1
    fi
    grep -F "test-pubs RED $tag" "$m/$name.log" || {
        cat "$m/$name.log"; printf '!!! test-pubs-flair-mutant FAIL: %s failed for the wrong reason\n' "$name"; exit 1; }
    printf '>>> test-pubs-flair-mutant: %s rejected\n' "$name"
}
cmds=""
for b in $books; do
    grep -v '|operation|' "$built/$b/main.cmds" > "$m/$b.cmds"
    cmds="$cmds $m/$b.cmds"
done
first=flair-users-guide
case " $books " in *" $first "*) ;; *) printf "!!! test-pubs-flair-mutant FAIL: the User's Guide is not among the books\n"; exit 1 ;; esac

# 1. A documented menu item the build does not ship (a renamed command).
sed 's/^FINDER>File>New Folder>N>on|/FINDER>File>New Folders>N>on|/' "$m/$first.cmds" > "$m/renamed.cmds"
grep -q 'New Folders' "$m/renamed.cmds"
red RENAMED_COMMAND DOC-NOT-SHIPPED "$gate" commands "$out/flair.shipped" "$allow" "$m/renamed.cmds"
# 2. A dimmed command documented as available (the State field).
sed 's/^FINDER>File>Get Info>I>off|/FINDER>File>Get Info>I>on|/' "$m/$first.cmds" > "$m/state.cmds"
grep -q 'Get Info>I>on' "$m/state.cmds"
red WRONG_STATE DOC-NOT-SHIPPED "$gate" commands "$out/flair.shipped" "$allow" "$m/state.cmds"
# 3. A keyboard equivalent the build does not bind.
sed 's/^FINDER>Edit>Select All>A>on|/FINDER>Edit>Select All>L>on|/' "$m/$first.cmds" > "$m/key.cmds"
grep -q 'Select All>L' "$m/key.cmds"
red WRONG_KEY DOC-NOT-SHIPPED "$gate" commands "$out/flair.shipped" "$allow" "$m/key.cmds"
# 4. A shipped menu item with no entry (an item added to the build).
{ cat "$out/flair.shipped"; printf 'FINDER>Special>Format Disk>->on|menu\n'; } | LC_ALL=C sort > "$m/added.shipped"
red ADDED_ITEM SHIPPED-NOT-DOCUMENTED "$gate" commands "$m/added.shipped" "$allow" $cmds
# 5. The inventory is the SOURCE: the finder_menu.c mutant that gives Open
#    New Folder's key, inventoried, no longer matches the books.
$CC -std=c11 -w -ffunction-sections -fdata-sections -Wl,--gc-sections -DFINDER_MENU_MUT_CMDCHAR_DUP \
    $MENU_INC -o "$m/inventory_dup" harness/pubs/flair_inventory.c
"$m/inventory_dup" "$FLAIR_KMAIN_I" os/apps/tenantfx.asm $FLAIR_FIXTURES > "$m/dup.part"
grep -q '^FINDER>File>Open>N>cond|menu$' "$m/dup.part"
{ cat "$m/dup.part"; grep -v '^\(SYSTEM\|APPLE\|FINDER\|HELLO\|NOTES\|TENANTFX\)>' "$out/flair.shipped" | grep -v '|file$'; } | LC_ALL=C sort -u > "$m/dup.shipped"
red SOURCE_KEY_DRIFT DOC-NOT-SHIPPED "$gate" commands "$m/dup.shipped" "$allow" $cmds
# 6. An alert quoted with one character changed.
sed 's/^The item is locked\.|alert|/The item is Locked.|alert|/' "$m/$first.cmds" > "$m/alert.cmds"
grep -q 'is Locked' "$m/alert.cmds"
red ALERT_TEXT DOC-NOT-SHIPPED "$gate" commands "$out/flair.shipped" "$allow" "$m/alert.cmds"
# 7. A file of the data disk left out of the volume table.
grep -v '^APPS\\TEXT.EXE|file|' "$m/$first.cmds" > "$m/nofile.cmds"
others=""; for b in $books; do [ "$b" = "$first" ] || others="$others $m/$b.cmds"; done
red MISSING_FILE SHIPPED-NOT-DOCUMENTED "$gate" commands "$out/flair.shipped" "$allow" "$m/nofile.cmds" $others
# 8. A committed capture changed by one byte: the screens leg's comparator.
g=pubs/manuals/$first/screens/desktop.grey
sed '2s/^\(..\)/FF/' "$g" > "$m/desktop.grey"
red CHANGED_CAPTURE SCREEN-DRIFT "$gate" same SCREEN-DRIFT "$m/desktop.grey" "$g"
# 9. A capture in a book that prints nothing from it.
root=$m/manuals; mkdir -p "$root"
for b in $books; do cp -r "pubs/manuals/$b" "$root/"; done
cp "$g" "$root/$first/screens/unprinted.grey"
red UNPRINTED_CAPTURE SCREEN-UNPRINTED env FLAIR_BOOKS_ROOT="$root" sh harness/pubs/check_flair.sh "$built" $books
rm -f "$root/$first/screens/unprinted.grey"
# 10. A printed figure whose trace was deleted.
rm -f "$root/$first/screens/desktop.mouse"
red MISSING_TRACE SCREEN-PAIR env FLAIR_BOOKS_ROOT="$root" sh harness/pubs/check_flair.sh "$built" $books
cp "pubs/manuals/$first/screens/desktop.mouse" "$root/$first/screens/"
# 11. A documented operation no capture replays.
mkdir -p "$m/built/$first"
cp "$built/$first/main.flairscreens" "$m/built/$first/"
{ cat "$built/$first/main.cmds"; printf 'FINDER-EXIT-DOS-CANCEL|operation|GATED|FINDER-EXIT-DOS-CANCEL\n'; } > "$m/built/$first/main.cmds"
for b in $books; do [ "$b" = "$first" ] || { mkdir -p "$m/built/$b"; cp "$built/$b/main.cmds" "$built/$b/main.flairscreens" "$m/built/$b/"; }; done
red UNPROVED_OPERATION OPERATION-UNPROVED env FLAIR_BOOKS_ROOT="$root" sh harness/pubs/check_flair.sh "$m/built" $books
# 12. A documented operation the source never reports.
{ cat "$built/$first/main.cmds"; printf 'FINDER-ERASE-DISK|operation|GATED|FINDER-ERASE-DISK\n'; } > "$m/built/$first/main.cmds"
red UNSHIPPED_OPERATION OPERATION-UNSHIPPED env FLAIR_BOOKS_ROOT="$root" sh harness/pubs/check_flair.sh "$m/built" $books
# 13. A screendump pixel outside the locked palettes.
printf 'P6\n2 1\n255\n\215\334\334\001\002\003' > "$m/offpalette.ppm"
red OFF_PALETTE FLAIR-GREY "$grey" capture spec/assets/clut.json spec/assets/color_canon.json "$m/offpalette.ppm" 0 0 0 0 "$m/offpalette.grey"
# 14. A capture row claiming an operation its trace does not perform: the
#     capture route itself goes RED (one QEMU boot).
mkdir -p "$m/cap"
grep '^desktop|' "pubs/manuals/$first/screens/FLAIR" | sed 's/$/|FINDER-OPEN-VOLUME/' > "$m/cap/FLAIR"
cp "pubs/manuals/$first/screens/desktop.mouse" "$m/cap/"
red UNREPLAYED_OPERATION OPERATION-UNREPLAYED sh pubs/screens/flair_capture.sh "$HARNESS" "$FLAIR_BOOT" "$FLAIR_DATA" "$grey" \
    spec/assets/clut.json spec/assets/color_canon.json "$CONSOLE_TEXT" "$VGABIOS" "$m/cap/FLAIR" "$m/cap/out"
# 15. The Locked-notice figure captured on the volume AS DELIVERED (no file of
#     it is locked): the drag reaches the Trash, the notice never appears,
#     and the row's proof goes RED (one QEMU boot; image field dropped).
grep '^locked|' "pubs/manuals/$first/screens/FLAIR" | \
    sed 's/^locked|FINDER-LOCKED-ALERT|\([^|]*\)|\([^|]*\)|locked$/locked|FLAIR-CURSOR x=560 y=400|\1|\2/' > "$m/cap/FLAIR"
grep -q '^locked|FLAIR-CURSOR x=560 y=400|[^|]*|FINDER-LOCKED-ALERT$' "$m/cap/FLAIR"
cp "pubs/manuals/$first/screens/locked.mouse" "$m/cap/"
red LOCKED_ON_STOCK_VOLUME OPERATION-UNREPLAYED sh pubs/screens/flair_capture.sh "$HARNESS" "$FLAIR_BOOT" "$FLAIR_DATA" "$grey" \
    spec/assets/clut.json spec/assets/color_canon.json "$CONSOLE_TEXT" "$VGABIOS" "$m/cap/FLAIR" "$m/cap/out2"
printf '>>> test-pubs-flair-mutant: green (15 named mutants rejected by their gate legs)\n'
