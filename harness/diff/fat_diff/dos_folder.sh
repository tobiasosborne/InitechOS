#!/bin/sh
# Factory only. Independent roster + mtools/dosfstools on the identical image,
# and the real artifact FAT reader (fat_dump), following harness/diff/fat_diff.
# Law 1: ADR-0003 Appendix A/B.1, DEC-11; manual pp.94,97,135
# (PDF179,182,220): MORE redirection, PATH and AUTOEXEC startup flowchart.
set -eu
export INITECH_QMP_STDIO=1 LC_ALL=C
mode=$1; img=$2; out=$3
mkdir -p "$out"
fail() { echo "RED dos-folder $*"; exit 1; }
names() { mdir -b -i "$img" "::$1" | sed 's#.*/##'; }
if [ "$mode" = host ]; then
    names DOS > "$out/dos.order" 2>/dev/null || fail external-set
    cmp -s spec/dos_folder_externals.txt "$out/dos.order" || fail external-set
    names APPS > "$out/apps.order"
    cmp -s spec/dos_folder_apps.txt "$out/apps.order" || fail apps-order
    for d in DOS APPS; do
        build/fat_dump "$img" --list-path "\\$d" > "$out/$d.artifact" || fail artifact-reader
        : > "$out/$d.mtools"
        for f in $(names "$d"); do
            mcopy -i "$img" "::$d/$f" - > "$out/file"
            printf '%s %s\n' "$f" "$(wc -c < "$out/file")" >> "$out/$d.mtools"
        done
        sort "$out/$d.mtools" -o "$out/$d.mtools"
        cmp -s "$out/$d.artifact" "$out/$d.mtools" || fail fat-reader-differential
    done
    printf '@ECHO OFF\r\nPATH=A:\\DOS\r\n' > "$out/autoexec.want"
    mcopy -i "$img" ::AUTOEXEC.BAT - > "$out/autoexec.got" 2>/dev/null || fail autoexec
    cmp -s "$out/autoexec.want" "$out/autoexec.got" || fail autoexec
    fsck.fat -n "$img" > "$out/fsck" 2>&1 || fail filesystem
    echo 'GREEN test-dos-folder-host: exact DOS/APPS order, AUTOEXEC bytes, artifact FAT reader == mtools, fsck.fat'
    exit 0
fi
serial=$out/guest.serial
tr -d '\r' < "$4" > "$serial"
if [ "$mode" = finder ]; then
    grep -qx 'FINDER-OPEN-VOLUME win=0 n=4' "$serial" || fail root-count
    grep -qx 'FINDER-OPEN-FOLDER name=DOS win=1 singleton=0' "$serial" || fail finder-folder
    echo 'GREEN test-dos-folder: Finder opens DOS as a folder (root n=4)'
    exit 0
fi
if grep -qE 'Enter new (date|time)' "$serial"; then fail startup-prompts; fi
grep -qx 'DOS-FOLDER-DONE' "$serial" || fail completion
# Parse the complete serial listing, including unexpected non-COM leaves.
# DOS DIR's only permitted directories here are the FAT dot entries.
awk '/^ Directory of A:\\DOS$/ {p=1;next}
     p && /File\(s\)/ {p=0}
     p && NF {if ($1=="." || $1=="..") next;
              if ($2=="<DIR>") print $1 "/";
              else if (NF==5) print $1 "." $2; else print $1}' \
    "$serial" > "$out/dir.names"
cmp -s spec/dos_folder_externals.txt "$out/dir.names" || fail serial-external-set
grep -qF 'Bad command or file name' "$serial" && fail 'PATH: Bad command or file name'
grep -qxF 'PATH=A:\DOS' "$serial" || fail path-value
mcopy -i "$img" ::CHK.OUT - > "$out/chkdsk.got" 2>/dev/null || fail chkdsk-output
grep -q 'bytes total disk space' "$out/chkdsk.got" || fail chkdsk-report
mcopy -i "$img" ::MORE.OUT - > "$out/more.got" 2>/dev/null || fail more-output
mcopy -i "$img" ::README.TXT - > "$out/more.want"
cmp -s "$out/more.want" "$out/more.got" || fail more-bytes
fsck.fat -n "$img" > "$out/fsck" 2>&1 || fail filesystem
echo 'GREEN test-dos-folder: serial DIR == fourteen externals; PATH finds CHKDSK and MORE; MORE <README.TXT exact bytes; no startup prompts; fsck.fat'
