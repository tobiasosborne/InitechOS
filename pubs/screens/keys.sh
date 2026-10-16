#!/bin/sh
# keys.sh -- turn literal keyboard text into the qemu_harness --keys token list.
# Usage: keys.sh 'DIR\n'   (the argument is read with printf %b, so \n is Enter)
# Uppercase letters and the shifted symbols are sent as shift chords so the
# screen shows exactly what a user would have typed.  Thin shell (Law 3).
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
      2a) t=shift-8 ;;
      3f) t=shift-slash ;;
      2d) t=minus ;;
      3e) t=shift-dot ;;
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
