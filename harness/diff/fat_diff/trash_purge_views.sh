#!/bin/sh
# harness/diff/fat_diff/trash_purge_views.sh -- the emulator gate for bead
# initech-uios (audit L002): Empty Trash retires EVERY view of a removed folder,
# and a New Folder afterwards can only land in a live directory.
#
# Usage: trash_purge_views.sh <boot img> <prepared data img> <trace> <purge|ctrln> [suffix] [quit marker]
#
# The prepared volume (Makefile $(FLAIR_PURGE_VIEWS_DATA), built by mtools, never
# by our FAT code) holds \TRASH\A\CHILD\GRAND\NOTE.TXT and \TRASH\A\KEEP.TXT. The
# locked trace (spec/flair_trash_traces.mk Sec 4) opens the Trash, A, CHILD and
# GRAND windows (slots 0..3), chooses Special > Empty Trash and clicks OK;
# `ctrln` then presses Ctrl-N. RE-KEY (bead initech-vnl4, audit 2026-10-23
# desktop L008): the only window left is the Trash, where New Folder is now
# DARK -- MenuKey hands the chord to nobody, so nothing is written anywhere:
# no FINDER-CMD line, no NEWFOLD, the cluster count is the purge leg's. The
# boot ends on the chord's own release echo (FLAIR-EVT what=4: the keyDown has
# been fully handled by then). The L002-RESTORED mutant still keeps GRAND
# (not the Trash) front, so its Ctrl-N still writes into the freed cluster
# and fsck still finds the orphan; it passes its own quit marker
# FINDER-NEW-FOLDER as the optional 6th argument.
#
# The judges are INDEPENDENT of the Finder's model (Law 2): the serial chain is
# hand-derived (sizes and the \TRASH cluster read with mtools); the disk is
# judged on the STOPPED image by mtools (mdir/mshowfat) and `fsck.fat -n`, whose
# in-use cluster count is hand arithmetic over the pristine image's own count;
# the screen by tools/ppm_flair_disk_windows_check leg purgeviews.
# Ref: Apple_Macintosh_System_Software_Users_Guide_V6.0.pdf printed p. 112
# (PDF p. 132): a discarded folder and all its contents disappear;
# docs/research/fat12-ground-truth.md Sec 3 (0x000 = free cluster).
set -eu
boot=$1
base=$2
trace=$3
mode=$4
name=flair_purge_views_${mode}${5:-}
img=build/$name.data.img
fail() { printf 'FAIL trash-purge-views %s: %s\n' "$mode" "$*" >&2; exit 1; }

cp -f "$base" "$img"

# ---- expectations, read off the pristine image with mtools ------------------
minfo -i "$img" :: | grep -q 'cluster size: 1 sectors' || fail 'fixture: want 1-sector (512-byte) clusters'
used0=$(fsck.fat -n "$img" | tail -1 | sed -n 's/.* \([0-9][0-9]*\)\/[0-9]* clusters$/\1/p')
[ -n "$used0" ] || fail 'fixture: fsck.fat gave no cluster count'
keep=$(mcopy -n -i "$img" ::/TRASH/A/KEEP.TXT - | wc -c)
note=$(mcopy -n -i "$img" ::/TRASH/A/CHILD/GRAND/NOTE.TXT - | wc -c)
[ "$keep" -gt 0 ] && [ "$keep" -le 512 ] && [ "$note" -gt 0 ] && [ "$note" -le 512 ] \
    || fail 'fixture: KEEP.TXT / NOTE.TXT must each fill exactly one cluster'
k=$(( (keep + note + 1023) / 1024 ))
t=$(mshowfat -i "$img" ::/TRASH | sed -n 's/.*<\([0-9]*\).*/\1/p')
[ -n "$t" ] || fail 'fixture: mshowfat found no ::/TRASH'

if [ "$mode" = purge ]; then
    quit=FINDER-TRASH-EMPTIED
    shot="--screendump --screendump-after FINDER-TRASH-EMPTIED"
else
    quit=${6:-'FLAIR-EVT what=4 '}
    shot=
fi
# shellcheck disable=SC2086
build/qemu_harness --disk "$boot" --disk2 "$img" --expect FLAIR-FAT-MOUNT-OK \
    --name "$name" --out build --mouse "$trace" --keys-after FLAIR-LIVE-READY \
    --quit-after "$quit" $shot --timeout-ms 40000 > "build/$name.report" 2>&1 || true
serial=build/$name.serial
if grep -q 'triple_fault=1' "build/$name.report"; then fail 'TRIPLE FAULT'; fi
grep -q "$quit" "$serial" || { grep -E '^(FINDER|DESK|PANIC)' "$serial" >&2 || true; fail "the guest never reached $quit"; }

# ---- the screen (purge): every purged folder's window is gone ---------------
if [ "$mode" = purge ]; then
    [ -s "build/$name.ppm" ] || fail 'no screendump'
    build/ppm_flair_disk_windows_check purgeviews "build/$name.ppm" \
        || fail "a purged folder's window is still drawn (ppm leg purgeviews)"
fi

# ---- the disk, on the STOPPED image ------------------------------------------
fsck_out=$(fsck.fat -n "$img" 2>&1) && rc=0 || rc=$?
printf '    fsck.fat: %s\n' "$(printf '%s' "$fsck_out" | tail -1)"
if [ "$rc" -ne 0 ] || printf '%s' "$fsck_out" | grep -qiE 'reclaim|unused|lost|orphan|bad '; then
    printf '%s\n' "$fsck_out" >&2
    fail 'fsck.fat -n: an allocated cluster is unreachable (orphan) or the volume is damaged'
fi
# in use = pristine + DESKTOP.DB (created at boot, 1 cluster) - 5 purged
# (A, CHILD, GRAND, KEEP.TXT, NOTE.TXT); Ctrl-N in the Trash makes nothing
# (bead initech-vnl4, L008), so both legs count the same.
want=$(( used0 + 1 - 5 ))
used=$(printf '%s' "$fsck_out" | tail -1 | sed -n 's/.* \([0-9][0-9]*\)\/[0-9]* clusters$/\1/p')
[ "$used" = "$want" ] || fail "fsck.fat counts $used clusters in use, want $want (pristine $used0 + DB 1 - 5 purged)"
all=$(mdir -/ -a -b -i "$img" ::)
if printf '%s\n' "$all" | grep -qE '/(A|CHILD|GRAND|KEEP\.TXT|NOTE\.TXT)/?$'; then
    printf '%s\n' "$all" >&2; fail 'a purged item is still on the volume'
fi
[ -z "$(mdir -b -i "$img" ::/TRASH 2>/dev/null)" ] || fail '::/TRASH is not empty after the purge'
! printf '%s\n' "$all" | grep -q NEWFOLD || fail 'a NEWFOLD appeared (New Folder is dark with the Trash front, L008)'

# ---- the serial chain (hand-derived; n=5, k from mtools) ---------------------
{
    printf '%s\n' 'DESK-TRASH-ICON full' 'FINDER-OPEN-TRASH win=0 n=1 singleton=0' \
        'FINDER-OPEN-FOLDER name=A win=1 singleton=0' \
        'FINDER-OPEN-FOLDER name=CHILD win=2 singleton=0' \
        'FINDER-OPEN-FOLDER name=GRAND win=3 singleton=0' \
        'FINDER-CMD id=11 name=EMPTY_TRASH src=mouse sel=0' \
        "FINDER-TRASH-ALERT n=5 k=$k" 'DESK-TRASH-ICON empty' \
        'FINDER-TRASH-EMPTIED purged=5 refused=0' \
        'FINDER-TRASH-VIEW-CLOSED win=1' 'FINDER-TRASH-VIEW-CLOSED win=2' \
        'FINDER-TRASH-VIEW-CLOSED win=3'
} > "build/$name.chain.want"
grep -E '^(FINDER-OPEN|FINDER-CMD|FINDER-TRASH|FINDER-NEW|DESK-TRASH-ICON)' "$serial" > "build/$name.chain.got" || true
cmp -s "build/$name.chain.want" "build/$name.chain.got" \
    || { diff "build/$name.chain.want" "build/$name.chain.got" >&2 || true; fail 'the serial chain differs from the locked one'; }
grep -q 'OK=1' "build/$name.report" || fail 'the emulator run did not complete (OK=1 missing)'

printf 'VERDICT: PASS -- trash-purge-views %s (views of the purged A/CHILD/GRAND closed; mtools + fsck.fat: %s clusters, no orphan%s)\n' \
    "$mode" "$used" "$( [ "$mode" = ctrln ] && printf '; Ctrl-N with the Trash front made nothing (L008)' )"
