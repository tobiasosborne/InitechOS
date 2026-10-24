#!/bin/sh
# doctor.sh -- is this machine ready to build, test and mint InitechOS?
# Checks every host tool the factory needs, the sibling ground-truth repos and
# the private reference library. Prints one line per check; exits 1 if any
# REQUIRED item is missing. Ref: docs/BOOTSTRAP.md (the install steps).
#
# Usage: tools/bootstrap/doctor.sh            (from the initech-os checkout)
#        SKIP_86BOX=1 tools/bootstrap/doctor.sh (treat 86Box as optional)

here="$(cd "$(dirname "$0")/../.." && pwd)"
proj="$(dirname "$here")"
fail=0

ok()   { printf '  ok      %-28s %s\n' "$1" "$2"; }
miss() { printf '  MISSING %-28s %s\n' "$1" "$2"; fail=1; }
warn() { printf '  warn    %-28s %s\n' "$1" "$2"; }

need() { # need <tool> <why> [apt package]
    if command -v "$1" >/dev/null 2>&1; then ok "$1" "$2"; else miss "$1" "$2 (${3:-see BOOTSTRAP.md})"; fi
}

echo "== host tools (CLAUDE.md Build & test; CDR-0001 interim toolchain)"
need gcc      "C kernel/Toolbox/apps, factory C"          "build-essential"
need nasm     "boot stubs, transient headers"              "nasm"
need ld       "flat-32 links"                              "binutils"
need make     "the factory"                                "make"
need python3  "harness references (FAT, dbf), lints"       "python3"
need qemu-system-i386 "dev-loop oracle"                    "qemu-system-x86"
need bochs    "real->protected accuracy legs (test-*-bochs)" "bochs bochs-x bochsbios"
need mformat  "FAT differential + fixture volumes (mtools)" "mtools"
need mcopy    "mtools"                                     "mtools"
need mkfs.fat "fixture volumes"                            "dosfstools"
need fsck.fat "fixture volumes"                            "dosfstools"
need fpc      "INDEPENDENT golden for the seed/TPS differential (SKIP_FPC=1 is the loud opt-out)" "fpc"
need dosbox-x "real-software mints (dBASE/Lotus/WP/TP goldens)" "dosbox-x"
need pdflatex "pubs manuals (test-pubs*)"                  "texlive-full"
need latexmk  "pubs manuals"                               "texlive-full"
need pdftoppm "pubs screen checks"                         "poppler-utils"
need convert  "clips + mint screenshots (ImageMagick)"     "imagemagick"
need import   "mint screenshots on Xvfb (ImageMagick)"     "imagemagick"
need Xvfb     "headless display for dosbox-x/Basilisk mints" "xvfb"
need ffmpeg   "record-flair clips (gif/mp4)"               "ffmpeg"
need 7z       "WinWorld archives"                          "p7zip-full"
need jq       "PM scripts"                                 "jq"
need git      "obviously"                                  "git"
need gh       "private repos + release assets"             "gh"
need BasiliskII "Mac OS 8.1 / System 7 capture mints (../system7-decomp)" "basilisk2"

if flatpak info net._86box._86Box >/dev/null 2>&1 && flatpak info net._86box._86Box.ROMs >/dev/null 2>&1; then
    ok "86Box (flatpak)" "period-authenticity legs + ROMs"
elif [ "${SKIP_86BOX:-0}" = 1 ]; then warn "86Box" "skipped by SKIP_86BOX=1"
else miss "86Box" "flatpak install flathub net._86box._86Box net._86box._86Box.ROMs"; fi

if printf 'int main(void){return 0;}' | gcc -m32 -ffreestanding -nostdlib -x c -c -o /dev/null - 2>/dev/null; then
    ok "gcc -m32 freestanding" "CDR-0001 interim target toolchain"
else miss "gcc -m32 freestanding" "install gcc-multilib"; fi

echo "== agent/PM tooling"
need bd    "beads issue tracker (CLAUDE.md Rule 9)" "see BOOTSTRAP.md"
need dolt  "bd's embedded database engine"          "see BOOTSTRAP.md"
need claude "Claude Code"                           "see BOOTSTRAP.md"
need codex "Codex CLI (gpt lanes)"                  "see BOOTSTRAP.md"
need quota "quota app (/usr/bin/quota; ../quota-app)" "see BOOTSTRAP.md"

echo "== sibling ground-truth repos (Law 1; Makefile *_DECOMP paths)"
for d in dbase3-decomp lotus123-decomp system7-decomp win31-decomp wordperfect51-decomp turbopascal-decomp; do
    if [ -d "$proj/$d/.git" ]; then
        if [ -f "$proj/$d/.goldens-restored" ]; then ok "../$d" "clone + goldens restored"
        else warn "../$d" "cloned, but gitignored goldens not restored (tools/bootstrap/fetch-siblings.sh)"; fi
    else miss "../$d" "git clone https://github.com/tobiasosborne/$d $proj/$d"; fi
done

# The exact sibling files the Makefile / gate rules read (grep '_DECOMP)/' in
# Makefile + *.mk). A sibling can be cloned with its goldens "restored" and still
# miss one of these if a snapshot was taken while the path was untracked rather
# than ignored (dbase3 mint/work, 2026-10-24, initech-e8s3). Checked only when
# the sibling is cloned; MISSING turns the SAMIR / FLAIR / I123 / IWORD gates red.
echo "== sibling files the gates read"
for f in \
    dbase3-decomp/goldens dbase3-decomp/mint/work/DBASE.EXE dbase3-decomp/mint/work/DBASE.OVL \
    dbase3-decomp/mint/work/DBASEINL.OVL dbase3-decomp/mint/work/DBASE.MSG dbase3-decomp/mint/samirctl/DBASE.EXE \
    dbase3-decomp/re/mint-results-004.md dbase3-decomp/re/mint-results-005.md \
    lotus123-decomp/goldens/lotus-123-2.2-installed-ref/extracted/installed/123.RI lotus123-decomp/goldens/minted/MINT01.WK1 lotus123-decomp/tools/wk1_ref.py \
    system7-decomp/goldens/captures/s8_desktop_menubar.png system7-decomp/goldens/captures/s8_alert_modal.png \
    system7-decomp/goldens/resources/NFNT_5478.bin "system7-decomp/goldens/resources/PAT#_0.bin" system7-decomp/mint/work/geneva12/NFNT_12112.bin \
    win31-decomp/oracles/wine/server/region.c \
    wordperfect51-decomp/goldens/minted/MINT09.WP5 wordperfect51-decomp/mint/work/WP51/MINT09 wordperfect51-decomp/mint/work/LISTF wordperfect51-decomp/tools/wp5_ref.py \
    turbopascal-decomp/goldens/tp55/tp55.zip; do
    sib="${f%%/*}"
    [ -d "$proj/$sib/.git" ] || continue
    if [ -e "$proj/$f" ]; then ok "../$f" ""; else miss "../$f" "gate input absent: re-run tools/bootstrap/fetch-siblings.sh $sib (snapshot may predate it)"; fi
done

if [ -f "$proj/initech-manuals-ref/MANIFEST.sha256" ]; then
    if (cd "$proj/initech-manuals-ref" && sha256sum -c --quiet MANIFEST.sha256 >/dev/null 2>&1); then ok "../initech-manuals-ref" "manifest verifies"
    else warn "../initech-manuals-ref" "cloned, scans not restored: run its ./fetch.sh"; fi
else miss "../initech-manuals-ref" "git clone https://github.com/tobiasosborne/initech-manuals-ref $proj/initech-manuals-ref && ./fetch.sh"; fi

echo "== sanity"
if [ -f "$here/.beads/issues.jsonl" ]; then ok ".beads/issues.jsonl" "$(wc -l < "$here/.beads/issues.jsonl") rows (bd import if bd ready is empty)"; else miss ".beads/issues.jsonl" "bad clone?"; fi
case "$(uname -m)" in x86_64) ok "x86_64 host" "";; *) warn "host arch $(uname -m)" "gcc -m32 / qemu-system-i386 expect an x86-64 host";; esac

if [ $fail = 0 ]; then echo "DOCTOR: ready. First job: make clean && make test (the full certificate), in a separate worktree."; else echo "DOCTOR: NOT ready (see MISSING above; docs/BOOTSTRAP.md)"; exit 1; fi
