#!/bin/sh
# One host entry plus its independent post-checks. Never reuse scratch files.
set -eu
bin=$1
name=${bin##*/}
dir=$bin.scratch
rm -rf "$dir"
mkdir -p "$dir"
root=$WORDPERFECT51_DECOMP
ref=$root/tools/wp5_ref.py
read_text() {
    python3 -I "$ref" "$1" > "$1.ref"
    grep -qxF "  TEXT: '$2'" "$1.ref"
    grep -q 'END offset=\([0-9]*\) filesize=\1 trailer-errors=0$' "$1.ref"
}
case "$name" in
 test_iword_commands)
    (cd tests/fixtures/status-units; sha256sum -c SHA256SUMS --quiet)
    (cd tests/fixtures/name-search; sha256sum -c SHA256SUMS --quiet)
    "$bin" ;;
 test_iword_move)
    capture=$IWORD_GOLDENS/dataloss/wordperfect/collision
    (cd "$capture"; printf '%s\n' \
      'b2b9df5cd68aee170538d21c36a405ea03dc7aa0d374fc9a26dba6d3002a2ab4  before.NAME.WP5' \
      '1de63f570187add4bdcc1310d90e8db617095525fbec8dbb6e2e299e773220dd  before.NAME.BK!' | sha256sum -c --quiet -)
    "$bin" "$dir" "$capture/before.NAME.WP5" "$capture/before.NAME.BK!"
    find "$dir" -type f ! -name readback -size +0c -exec python3 -I "$ref" {} + > "$dir/readback"
    ! grep -qE '!!|trailer-errors=[1-9]' "$dir/readback"
    test "$(grep -c '  END offset=' "$dir/readback")" -gt 0
    ;;
 test_iword_retrieve)
    fixtures=tests/conformance/fixtures
    (cd "$fixtures"; sha256sum -c ../SHA256SUMS --quiet)
    (cd tests/conformance; grep '  captures/W06-save/AFTER.WP5$' CAPTURES.sha256 | sha256sum -c --quiet -)
    "$bin" "$dir" "$fixtures/W06.WP5" "$fixtures/W07.WP5" "$fixtures/W11.WP5"
    python3 -I "$ref" tests/conformance/captures/W06-save/AFTER.WP5 > "$dir/reference"
    python3 -I "$ref" "$dir/RECOVER.WP5" > "$dir/recovered"
    for dump in reference recovered; do
      grep -qxF "  TEXT: 'ReadableZ'" "$dir/$dump"
      grep -qxF "  CODES: T'Readable' [AttrOn Bold] T'Z'" "$dir/$dump"
      grep -q 'END offset=\([0-9]*\) filesize=\1 trailer-errors=0$' "$dir/$dump"
    done
    (cd tests/conformance; grep '  captures/W11-save/AFTER.WP5$' CAPTURES.sha256 | sha256sum -c --quiet -)
    python3 -I "$ref" tests/conformance/captures/W11-save/AFTER.WP5 > "$dir/large-reference"
    python3 -I "$ref" "$dir/LARGE.WP5" > "$dir/large"
    python3 -I "$ref" "$dir/FULL.WP5" > "$dir/full"
    for dump in large-reference large full; do
      awk -v full="$dump" '
        /^  TEXT: / {
          text=substr($0,10,length($0)-10); seen++
          if(full=="full") { if(length(text)!=69305 || text !~ /^x+$/) exit 1 }
          else if(length(text)!=65211 || substr(text,1,65210) !~ /^x+$/ || substr(text,65211)!="Z") exit 1
        }
        END { if(seen!=1) exit 1 }
      ' "$dir/$dump"
      grep -q 'END offset=\([0-9]*\) filesize=\1 trailer-errors=0$' "$dir/$dump"
    done
    ;;
 test_iword_save)
    capture=$IWORD_GOLDENS/dataloss/wordperfect/collision
    (cd "$capture"; printf '%s\n' \
      'b2b9df5cd68aee170538d21c36a405ea03dc7aa0d374fc9a26dba6d3002a2ab4  before.NAME.WP5' \
      '1de63f570187add4bdcc1310d90e8db617095525fbec8dbb6e2e299e773220dd  before.NAME.BK!' \
      | sha256sum -c --quiet -)
    "$bin" "$dir" "$capture/before.NAME.WP5" "$capture/before.NAME.BK!"
    # One independent invocation reads every disk state at the error prompt.
    find "$dir" -type f \( -name 'NAME.WP5' -o -name 'NAME.BK!' -o -name 'NAME.W00' -o -name 'NAME.N00' \) -size +0c -exec python3 -I "$ref" {} + > "$dir/readback"
    ! grep -qE '!!|trailer-errors=[1-9]' "$dir/readback"
    test "$(grep -c '  END offset=' "$dir/readback")" -gt 0
    ;;
 test_iword_core|test_iword_codes) "$bin" ;;
 test_iword_wp5_roundtrip) "$bin" "$root" "$dir" ;;
 test_iword_reveal_codes) "$bin" "$root" ;;
 test_iword_face)
    "$bin" "$dir"
    a='Initech word processing is here. The quick brown fox jumps over'
    b='the lazy dog.'
    head -c 327 "$root/goldens/minted/MINT02.WP5" > "$dir/MEMO.expected"
    printf '%s\r%s' "$a" "$b" >> "$dir/MEMO.expected"
    cmp "$dir/MEMO.expected" "$dir/MEMO"
    read_text "$dir/MEMO" "$a$b"
    grep -qF "T'$a' [SRt] T'$b'" "$dir/MEMO.ref"
    read_text "$dir/COPY" "X$a$b"
    read_text "$dir/GUARD" 'Keep me'
    read_text "$dir/CODES" 'Plain bold under slant xy'
    grep -qF "T'Plain ' [AttrOn Bold] T'bold' [AttrOff Bold] T' ' [AttrOn Underline] T'under' [AttrOff Underline] T' ' [AttrOn Italic] T'slant' [AttrOff Italic] T' ' [AttrOn Bold] T'x' [AttrOff Bold] T'y'" "$dir/CODES.ref"
    mint09=$root/goldens/minted/MINT09.WP5
    test -f "$mint09" || mint09=$root/mint/work/WP51/MINT09
    tail -c +328 "$dir/TABS" > "$dir/TABS.body"
    tail -c +328 "$mint09" > "$dir/TABS.expected"
    cmp "$dir/TABS.expected" "$dir/TABS.body"
    python3 -I "$ref" "$dir/TABS" > "$dir/TABS.ref"
    grep -q 'END offset=\([0-9]*\) filesize=\1 trailer-errors=0$' "$dir/TABS.ref"
    ;;
 test_iword_face_small)
    "$bin" "$dir"
    python3 -I "$ref" "$dir/BIG" > "$dir/BIG.ref"
    grep -q 'END offset=4096 filesize=4096 trailer-errors=0$' "$dir/BIG.ref"
    ;;
 test_iword_fkeys)
    "$bin" fkeys
    "${bin%/*}/iword_fkeys_gen" include/iword/fkeys.json > "$dir/fkeys.expected.h"
    cmp include/iword/fkeys.h "$dir/fkeys.expected.h"
    ;;
 test_iword_search)
    "$bin" search
    sh tests/diff/search.sh "$root" "$dir" "${bin%/*}/test_iword_search_corpus"
    ;;
 test_iword_replace|test_iword_undo)
    mode=${name#test_iword_}
    "$bin" "$mode"
    sh tests/diff/replace.sh "$root" "$dir" "$bin" "$mode-corpus"
    ;;
 test_iword_setup)
    "$bin" setup "$dir"
    sh tests/diff/settings.sh "$dir"
    ;;
 test_iword_listfiles)
    mkdir -p "$dir/DOCS"
    for f in MEMO LETTER.WP5 TABS.DOC README.TXT ZERO.TXT BIG.DAT HUGE.BIN HIDDEN.SYS; do
        mcopy -n -i "$root/mint/work/LISTF/lf005.img" "::DOCS/$f" "$dir/DOCS/$f"
    done
    "$bin" "$root/mint/work/LISTF/grids" "$dir"
    ;;
 test_iword_listfiles_ops) "$bin" "$dir" tests/fixtures/iword-listfiles-ops/grids ;;
 *) echo "FAIL: unregistered host entry $name" >&2; exit 2 ;;
esac
