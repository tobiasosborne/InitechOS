#!/bin/sh
# Actual target objects and GNU make graphs; independent boundary invariants.
set -eu
repo=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir -p "$work/factory" "$work/apps/calculator" "$work/flair/include" "$work/kernel/include" "$work/build/apps/calculator"
ln -s "$repo/factory/testlib" "$work/factory/testlib"
cat > "$work/flair/README.md" <<'TEXT'
<!-- interface-declarations -->
Interface role: resident
Interface export-pure: flair/include/surface.h surface_fill
Interface export-binding: flair/include/window.h NewWindow
<!-- end-interface-declarations -->
TEXT
printf 'void surface_fill(void);\n' > "$work/flair/include/surface.h"
printf 'void NewWindow(void);\n' > "$work/flair/include/window.h"
cat > "$work/kernel/README.md" <<'TEXT'
<!-- interface-declarations -->
Interface role: startup
Interface export-binding: kernel/include/sft.h sft_close_process
<!-- end-interface-declarations -->
TEXT
printf 'void sft_close_process(void);\n' > "$work/kernel/include/sft.h"
cd "$work/apps/calculator"
cat > README.md <<'TEXT'
<!-- interface-declarations -->
Interface role: desktop
Interface use-pure: flair/include/surface.h
<!-- end-interface-declarations -->
TEXT
cat > Makefile <<'MAKE'
all: ../../build/apps/calculator/product.o
../../build/apps/calculator/product.o: product.c
	gcc -m32 -ffreestanding -fno-pic -I../../flair/include -MMD -MP -c $< -o $@
-include ../../build/apps/calculator/product.d
MAKE
cat > product.c <<'C'
#include "surface.h"
void run(void) { surface_fill(); __asm__ volatile("int $0x81"); }
C
cp product.c healthy.c
cp README.md healthy.readme
check() {
 make -s -B all || return $?
 make -qp all > graph 2>/dev/null || test $? -eq 1
 IMPORT_ROOT=$work IMPORT_COMPONENT=apps/calculator IMPORT_OUT=$work/audit \
 sh "${INTERFACE_IMPORT_DRIVER:-$repo/factory/testlib/import-check.sh}" '_(MUT|MUTATE|MUTANT)_' graph ../../build/apps/calculator/product.o '' '' Makefile > log 2>&1
}
reject() {
 if check; then echo "FAIL interface fixture: accepted $1"; exit 1; fi
 grep -F "$2" log >/dev/null || { cat log; echo "FAIL interface fixture: wrong rejection $1"; exit 1; }
 echo "ok interface: $1 rejected"
 cp healthy.c product.c; cp healthy.readme README.md
}
check
echo 'ok interface: declared pure call and INT81 compiled green'
if [ "${INTERFACE_ONLY:-}" != wrong-vector ]; then
 patch -s -p1 < "$repo/factory/mutants/interfaces/forbidden-direct-call.patch"
 reject direct-call 'undeclared external symbol sft_close_process'
fi
patch -s -p1 < "$repo/factory/mutants/interfaces/forbidden-vector.patch"
reject wrong-vector 'forbidden INT 39h'
printf '\nvoid bad(void) { __asm__ volatile("int3"); }\n' >> product.c
reject trap-vector 'forbidden interrupt opcode'
sed '/end-interface-declarations/i Interface use-pure: kernel/include/sft.h' healthy.readme > README.md
reject caller-pure-laundering 'unpublished use-pure: kernel/include/sft.h'
sed '/end-interface-declarations/i Interface use-binding: kernel/include/sft.h' healthy.readme > README.md
reject caller-resident-laundering 'cannot use resident binding'
printf '#include "sft.h"\nvoid bad(void) {}\n' >> product.c
# A manually copied private include must remain forbidden even with no call.
sed -i 's|-I../../flair/include|-I../../flair/include -I../../kernel/include|' Makefile
reject undeclared-include 'undeclared external header'
# Ordinary strings/comments do not emit interrupts.
printf '\nconst char *text="int $0x39"; /* int $0x13 */\n' >> product.c
check
echo 'ok interface: data/comment vector lookalikes green'
# Compile one independent object per vector/role, exercising the same decoder
# used for production objects. Constants below are the amended allowed list.
vector() {
 component=$1 role=$2 object=$3 artifact=$4 number=$5 expected=$6
 printf 'void entry(void) { __asm__ volatile("int $0x%s"); }\n' "$number" > vector.c
 gcc -m32 -ffreestanding -c vector.c -o vector.o
 objdump -d vector.o > disassembly
 rc=0
 awk -v component="$component" -v role="$role" -v object="$object" -v artifacts="$artifact" \
  -f "$repo/factory/testlib/interface-vectors.awk" disassembly > vector.log || rc=$?
 if [ "$rc" -ne "$expected" ]; then cat vector.log; echo "FAIL interface fixture: vector $number $component/$artifact status $rc expected $expected"; exit 1; fi
}
vector dos dos /out/chkdsk/program.o /out/CHKDSK.COM 25 0
vector dos dos /out/chkdsk/program.o /out/CHKDSK.COM 26 0
vector dos dos /out/common/label.o /out/LABEL.COM 25 0
vector dos dos /out/common/label.o /out/LABEL.COM 26 0
for app in FORMAT FDISK SYS DISKCOPY DISKCOMP; do
 vector dos dos /out/raw.o "/out/$app.COM" 13 0
done
vector dos dos /out/raw.o /out/ATTRIB.COM 25 1
vector dos dos /out/raw.o /out/EDLIN.COM 13 1
vector dos dos /out/raw.o '/out/FORMAT.COM /out/MORE.COM' 13 1
vector dos dos /out/program.o /out/MORE.COM 21 0
vector dos dos /out/program.o /out/MORE.COM 20 0
vector apps/calculator desktop /out/program.o /out/CALCULAT.EXE 81 0
vector apps/calculator desktop /out/program.o /out/CALCULAT.EXE 21 1
vector apps/calculator desktop /out/program.o /out/CALCULAT.EXE 13 1
vector dos dos /out/program.o /out/MORE.COM 81 1
echo 'ok interface: every allowed raw-disk role, DOS termination and desktop gate; wrong roles and mixed consumers red'
# A real resident consumer may use the published Window manager binding.
mkdir -p "$work/flair/desktop" "$work/build/flair/desktop"
cd "$work/flair/desktop"
cat > README.md <<'TEXT'
<!-- interface-declarations -->
Interface role: resident
Interface use-binding: flair/include/window.h
<!-- end-interface-declarations -->
TEXT
printf '#include "window.h"\nvoid run(void) { NewWindow(); }\n' > resident.c
gcc -m32 -ffreestanding -fno-pic -I../include -MMD -MP -c resident.c -o "$work/build/flair/desktop/resident.o"
printf 'all: %s/build/flair/desktop/resident.o\n%s/build/flair/desktop/resident.o: resident.c\n' "$work" "$work" > graph
sh "$repo/factory/testlib/interface-check.sh" "$work" flair/desktop graph "$work/resident-audit"
echo 'ok interface: published resident manager binding compiled green'
# Stale and unknown migration records are failures, not permanent exemptions.
printf 'Interface migration: fdisk-reset initech-q9p80\n' > declarations
if sh "$repo/factory/testlib/interface-migrations.sh" dos declarations > migration.log 2>&1; then
 echo 'FAIL interface fixture: stale migration accepted'; exit 1
fi
grep -q 'stale migration fdisk-reset' migration.log
printf 'Interface migration: imaginary initech-q9p80\n' > declarations
if sh "$repo/factory/testlib/interface-migrations.sh" dos declarations > migration.log 2>&1; then
 echo 'FAIL interface fixture: unknown migration accepted'; exit 1
fi
echo 'ok interface: stale and unknown migration records rejected'
# The private resident header exception covers exactly two kernel callers.
cd "$work"
mkdir -p kernel
printf '#include "../dos/command/entry.h"\n' > kernel/main.c
printf '#include "../dos/command/entry.h"\n' > kernel/other.c
awk -v component=kernel -v here="$work/kernel" -v pattern='a^' -f "$repo/factory/testlib/source-audit.awk" "$work/kernel/main.c"
if awk -v component=kernel -v here="$work/kernel" -v pattern='a^' -f "$repo/factory/testlib/source-audit.awk" "$work/kernel/other.c" > header.log; then
 echo 'FAIL interface fixture: resident header exception broadened'; exit 1
fi
grep -q 'FAIL includes:' header.log
echo 'ok interface: exact resident-command include green; adjacent caller red'
# A parent's output prefix must not make a registered child its own library.
mkdir -p "$work/flair/region/include" "$work/build/flair/region"
printf 'int region_query(void);\n' > "$work/flair/region/include/region.h"
cat > "$work/flair/region/README.md" <<'TEXT'
<!-- interface-declarations -->
Interface role: pure
Interface export-pure: flair/region/include/region.h region_query
<!-- end-interface-declarations -->
TEXT
cd "$work/flair"
printf '#include "region.h"\nint caller(void) { return region_query(); }\n' > caller.c
printf 'int region_query(void) { return 1; }\n' > region/query.c
gcc -m32 -ffreestanding -fno-pic -Iregion/include -MMD -MP -c caller.c -o "$work/build/flair/caller.o"
gcc -m32 -ffreestanding -fno-pic -c region/query.c -o "$work/build/flair/region/query.o"
printf 'all: %s/build/flair/caller.o %s/build/flair/region/query.o\n' "$work" "$work" > graph
if sh "$repo/factory/testlib/interface-check.sh" "$work" flair graph "$work/nested-audit" > nested.log 2>&1; then
 echo 'FAIL interface fixture: nested provider treated as parent-owned'; exit 1
fi
grep -q 'undeclared external symbol region_query' nested.log
sed '/end-interface-declarations/i Interface use-pure: flair/region/include/region.h' README.md > readme.tmp
mv readme.tmp README.md
sh "$repo/factory/testlib/interface-check.sh" "$work" flair graph "$work/nested-audit"
echo 'ok interface: nested provider requires its own declared pure contract'
