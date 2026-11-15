#!/bin/sh
# Keep the single proved source exactly equal to its editable IDE fragments.
# Update is an explicit developer operation; checks never rewrite source.
set -eu
mode=$1 source=$2 fragments=$3
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' 0 HUP INT TERM
for part in screen buffer editor product; do
  start=$(awk -v p="$part" '$0=="{ BEGIN IDE "p" }" {print NR}' "$source")
  end=$(awk -v p="$part" '$0=="{ END IDE "p" }" {print NR}' "$source")
  test -n "$start" && test -n "$end"
  case $mode in
    check)
      sed -n "$((start+1)),$((end-1))p" "$source" > "$tmp/part"
      cmp "$tmp/part" "$fragments/$part.pas"
      ;;
    update)
      head -n "$start" "$source" > "$tmp/source"
      cat "$fragments/$part.pas" >> "$tmp/source"
      tail -n +"$end" "$source" >> "$tmp/source"
      cat "$tmp/source" > "$source"
      ;;
    *) exit 2;;
  esac
done
echo 'IDE fragments equal the single compiler source'
