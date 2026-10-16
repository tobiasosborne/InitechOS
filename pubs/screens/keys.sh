#!/bin/sh
# keys.sh -- turn literal keyboard text into the qemu_harness --keys token list.
# Usage: keys.sh 'DIR\n'   (the argument is read with printf %b, so \n is Enter)
# Uppercase letters and the shifted symbols are sent as shift chords so the
# screen shows exactly what a user would have typed.  Control characters are
# sent as Ctrl chords: \0032 (Ctrl-Z, end of a COPY CON file), \0003 (Ctrl-C),
# \0010 (Backspace).  Thin shell (Law 3).  Bead initech-ipu5.7 added the
# symbols the Reference examples type (= + , < | $ % @ ( ) ; / _ ").
set -eu
printf '%b' "$1" | od -An -v -tx1 | tr -s ' ' '\n' | sed '/^$/d' | {
  out=
  while read -r h; do
    c=$(printf "\\$(printf '%03o' 0x$h)")
    case $h in
      0a) t=ret ;;
      20) t=spc ;;
      2e) t=dot ;;
      5c) t=backslash ;;
      3a) t=shift-semicolon ;;
      3b) t=semicolon ;;
      2a) t=shift-8 ;;
      3f) t=shift-slash ;;
      2f) t=slash ;;
      2d) t=minus ;;
      5f) t=shift-minus ;;
      3e) t=shift-dot ;;
      3c) t=shift-comma ;;
      2c) t=comma ;;
      3d) t=equal ;;
      2b) t=shift-equal ;;
      7c) t=shift-backslash ;;
      24) t=shift-4 ;;
      25) t=shift-5 ;;
      40) t=shift-2 ;;
      28) t=shift-9 ;;
      29) t=shift-0 ;;
      22) t=shift-apostrophe ;;
      27) t=apostrophe ;;
      1a) t=ctrl-z ;;
      03) t=ctrl-c ;;
      08) t=backspace ;;
      *)
        case $c in
          [a-z0-9]) t=$c ;;
          [A-Z]) t=shift-$(printf '%s' "$c" | tr 'A-Z' 'a-z') ;;
          *) printf 'keys.sh: no key token for byte 0x%s\n' "$h" >&2; exit 1 ;;
        esac ;;
    esac
    out=${out:+$out,}$t
  done
  printf '%s\n' "$out"
}
