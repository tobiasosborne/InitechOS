#!/bin/sh
# flair_alerts.sh -- the SHIPPED FLAIR alerts and dialogs, for the FLAIR
# manuals' GC-01/GC-02 gate (bead initech-ipu5.11; PUB-REF-0001 NT-16).
#
# Prints one `<key>|alert` line per alert or dialog a FLAIR user can see,
# the key being the alert's FIRST LINE as the source draws it -- or, where
# the source builds the line at run time, its constant leading literal
# ("The Trash contains", "Replace existing").  Every key is READ OUT OF THE
# SOURCE by the patterns below (quoted string literals at the alert's call
# site); nothing here is typed text.  A pattern that stops matching is RED
# (Rule 2): the source changed shape, and the manual must follow it.
#
#   kmain.c         the Locked notice (locked_text[]), the shut-down screen
#                   (finder_shutdown_screen text[]), About This Computer
#                   (BANNER_LINE1), Standard File's Replace question (pre)
#   finder_windows.c  Empty Trash (the fw_cat prefix) and Exit to InitechDOS
#                   (fw_alert_wrap)
#   dialog.h        the FILE COPY box: its title and its message
#   itext.c         InitechText: every say() notice, the Save changes
#                   question (a), the Save As prompt (tbx_sf_putfile)
# Thin shell (Law 3).  ASCII-clean (Rule 12).
set -eu
k=os/milton/kmain.c; w=os/flair/finder_windows.c; d=os/flair/dialog.h; t=os/apps/itext/itext.c
emit() {
    [ -n "$1" ] || { printf '!!! test-pubs RED FLAIR-INVENTORY: alert pattern %s matched nothing\n' "$2" >&2; exit 1; }
    printf '%s\n' "$1" | sed 's/  */ /g; s/ *$//; s/$/|alert/'
}
# (TeX reads a run of spaces as one, so keys are compared with runs of
# spaces collapsed: "InitechDOS  Version 3.30" is keyed with one space.)
# the first string literal on the line after a matched line
after() { sed -n "/$2/{n;p;q}" "$1" | sed -n 's/^[^"]*"\([^"]*\)".*/\1/p'; }
emit "$(after $k 'locked_text\[3\] = {')" locked_text
emit "$(sed -n '/^static int finder_shutdown_screen/,/^}/p' $k | after /dev/stdin 'text\[3\] = {')" shutdown
emit "$(sed -n 's/^#define BANNER_LINE1 "\(.*\)"$/\1/p' $k)" BANNER_LINE1
emit "$(sed -n 's/.*const char \*pre = "\(Replace existing\) .*/\1/p' $k)" replace
emit "$(sed -n 's/.*fw_cat(msg, n, "\(The Trash contains\) ");.*/\1/p' $w)" trash
emit "$(sed -n 's/.*fw_alert_wrap("\([^"]*\)", out);.*/\1/p' $w)" exit_dos
emit "$(sed -n 's/^#define FLAIR_CANON_FILECOPY_TITLE "\(.*\)"$/\1/p' $d)" filecopy_title
emit "$(sed -n 's/^#define FLAIR_CANON_FILECOPY_MSG "\(.*\)"$/\1/p' $d)" filecopy_msg
say=$(sed -n 's/.*say("\([^"]*\)".*/\1/p' $t | LC_ALL=C sort -u)
emit "$say" say
emit "$(sed -n 's/.*const char \*a = "\(Save changes to the document\)";.*/\1/p' $t)" save_changes
emit "$(sed -n 's/.*tbx_sf_putfile("\([^"]*\)".*/\1/p' $t)" sf_prompt
