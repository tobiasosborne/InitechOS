#!/bin/sh
# harness/diff/dbf_diff/corpus_pristine.sh -- the dBASE III PLUS sample corpus
# the SAMIR oracles read is byte-identical to the PRISTINE Ashton-Tate disk
# (bead initech-586.4.1).
#
# usage: corpus_pristine.sh <Sample_Programs_and_Utilities.img> <files-dir> <workdir>
#
# WHY (Law 2): the SAMIR index oracles (test-ndx-parse/keys/seek/build) grade
# against <DBASE3_DECOMP>/goldens/dbase-iii-plus-1.1-pristine/files/
# Sample_Programs_and_Utilities/. In wave 9 (initech-ahu.5) a test opened
# CNAMES.NDX read-write and corrupted it; it was replaced by a SAMIR ndx_build
# rebuild, which made the CNAMES leg of test-ndx-build self-referential (our
# writer graded against our writer). The authentic file was RECOVERED on
# 2026-10-17 from the pristine disk image (mtools, 1986-07-24 12:00 directory
# stamp; ../dbase3-decomp re/mint-results-006.md). This gate keeps it that way:
# every file in the corpus directory must equal, byte for byte, the file of the
# same name on the real disk image, and the image itself must be the pinned one.
#
# ORACLE: the pristine SuperCard Pro dump (dBaseIIIPlus1.1-IMG.7z ->
# IMG/Sample_Programs_and_Utilities.img, 368640 bytes, sha256 pinned below).
# Never our artifact. Read-only: files are extracted into <workdir>.
#
# ASCII-clean (Rule 12). Exit 0 green, 1 on a mismatch, 2 on a missing reference.
set -eu
img=$1
files=$2
work=$3
here=$(dirname "$0")
IMG_SHA256=f86d54f751d063ddeaf6d31a630f4ce8c2254ebc017cddeb78a26cb7b3b5152b
CNAMES_SHA256=6505d4666073bf84bc5c2d1a4dd2937ad430870b156756cfc6d5c7b59ccd6f8a

if [ ! -f "$img" ] || [ ! -d "$files" ]; then
    sh "$here/../../missing_reference.sh" DBASE3_DECOMP "pristine dBASE disk image or corpus dir absent ($img, $files)"
    exit 0
fi
if ! command -v mcopy >/dev/null 2>&1 || ! command -v sha256sum >/dev/null 2>&1; then
    sh "$here/../../missing_reference.sh" REFERENCE_TOOLS "mtools mcopy / sha256sum absent"
    exit 0
fi

got=$(sha256sum "$img" | cut -d' ' -f1)
if [ "$got" != "$IMG_SHA256" ]; then
    echo "!!! test-dbase-corpus-pristine FAIL: disk image sha256 $got is not the pinned pristine dump $IMG_SHA256" >&2
    exit 1
fi

rm -rf "$work"
mkdir -p "$work/disk"
MTOOLS_SKIP_CHECK=1 mcopy -n -m -s -i "$img" '::*' "$work/disk/"

ls "$work/disk" | LC_ALL=C sort > "$work/disk.list"
ls "$files" | LC_ALL=C sort > "$work/files.list"
if ! cmp -s "$work/disk.list" "$work/files.list"; then
    echo "!!! test-dbase-corpus-pristine FAIL: corpus file set differs from the disk directory:" >&2
    diff "$work/disk.list" "$work/files.list" >&2 || true
    exit 1
fi

n=0
bad=0
while read -r name; do
    n=$((n + 1))
    if ! cmp -s "$work/disk/$name" "$files/$name"; then
        first=$(cmp "$work/disk/$name" "$files/$name" 2>&1 | head -1 || true)
        echo "!!! test-dbase-corpus-pristine FAIL: $name differs from the pristine disk image ($first)" >&2
        bad=$((bad + 1))
    fi
done < "$work/disk.list"

got=$(sha256sum "$files/CNAMES.NDX" | cut -d' ' -f1)
if [ "$got" != "$CNAMES_SHA256" ]; then
    echo "!!! test-dbase-corpus-pristine FAIL: CNAMES.NDX sha256 $got is not the recovered authentic $CNAMES_SHA256" >&2
    bad=$((bad + 1))
fi

if [ "$bad" -ne 0 ]; then
    echo "test-dbase-corpus-pristine: $n files, $bad mismatches" >&2
    exit 1
fi
echo "test-dbase-corpus-pristine: $n files byte-identical to the pristine disk image; CNAMES.NDX = $CNAMES_SHA256"
