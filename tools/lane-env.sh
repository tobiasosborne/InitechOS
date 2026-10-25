# tools/lane-env.sh -- source this in a lane / PM shell before building.
#
#   . tools/lane-env.sh
#
# Routes every host cc/gcc call through ccache via Ubuntu's masquerade links
# (/usr/lib/ccache), with ONE cache shared by all lane clones under
# ~/Projects (CCACHE_BASEDIR makes the clone path irrelevant to the hash).
# No Makefile change: CC/KERNEL_CC stay as they are, PATH picks the wrapper.
#
# Measured 2026-10-25 (bead initech-b9y7b): fresh clone `make -j32 samir-com
# image` 28 s cold -> 18 s with a warm cache (148/148 compile hits; the rest
# is nasm/ld/image assembly). build/SAMIR.COM and build/initech.img are
# byte-identical with and without ccache (Rule 11 unaffected).
#
# PATH is quoted: under WSL it contains Windows directories with spaces.

if [ -d /usr/lib/ccache ] && command -v ccache >/dev/null 2>&1; then
    case ":$PATH:" in
        *:/usr/lib/ccache:*) ;;
        *) PATH="/usr/lib/ccache:$PATH" ;;
    esac
    CCACHE_DIR="${CCACHE_DIR:-$HOME/.cache/initech-ccache}"
    CCACHE_BASEDIR="${CCACHE_BASEDIR:-$HOME/Projects}"
    CCACHE_NOHASHDIR=1
    export PATH CCACHE_DIR CCACHE_BASEDIR CCACHE_NOHASHDIR
    hash -r 2>/dev/null || true
else
    printf 'lane-env: ccache not installed; building without it (sudo apt install ccache)\n' >&2
fi
