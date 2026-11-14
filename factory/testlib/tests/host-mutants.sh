#!/bin/sh
# Wrong product lives only in an external patch; infrastructure never kills it.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir -p "$work/component/mutants" "$work/bin"
cat > "$work/component/Makefile" <<'MAKE'
COMPONENT := fixture
include $(TOP)/mk/config.mk
HOST_TESTS := probe
probe_SRCS := probe.c
include $(TOP)/mk/host-test.mk
MAKE
cat > "$work/component/probe.c" <<'C'
#include "test_assert.h"
TEST_HARNESS();
static int product(void) { return 7; }
int main(void) { CHECK(product()==7, "literal seven"); return TEST_SUMMARY("probe"); }
C
# Explicitly point the fixture at the real, public assertion API.
printf '\nHOST_INCLUDES := -I%s/factory/include\n' "$root" >> "$work/component/Makefile"
cp "$root/factory/mutants/host/assertion.patch" "$work/component/mutants/"
cat > "$work/component/mutants/no-op.patch" <<'PATCH'
--- a/probe.c
+++ b/probe.c
@@ -1,4 +1,4 @@
 #include "test_assert.h"
 TEST_HARNESS();
-static int product(void) { return 7; }
+static int product(void) { return 7; } /* unchanged */
 int main(void) { CHECK(product()==7, "literal seven"); return TEST_SUMMARY("probe"); }
PATCH
cat > "$work/bin/timeout" <<'INJECT'
#!/bin/sh
case $* in *baseline-*) exec /usr/bin/timeout "$@";; esac
# Even stale-looking semantic output cannot convert a transport failure to a kill.
echo '  FAIL probe.c:4: literal seven'
echo 'INITECH_CHECKS 1 failures=1'
exit "$INJECT_EXIT"
INJECT
chmod +x "$work/bin/timeout"
for fault in 1 2 124 126 127 137 139 143; do
 rc=0
 (cd "$work/component" && PATH="$work/bin:$PATH" INJECT_EXIT=$fault \
  sh "$root/factory/testlib/run-mutants.sh" "$root" fixture "$work/out-$fault" make mutants/no-op.patch) > "$work/$fault.log" 2>&1 || rc=$?
 test "$rc" -ne 0
 grep -q '1 mutants, 0 killed, 0 survived, 1 errors' "$work/$fault.log"
 echo "ok infrastructure exit $fault is an error"
done
rc=0
(cd "$work/component" && sh "$root/factory/testlib/run-mutants.sh" "$root" fixture "$work/control" make mutants/no-op.patch) > "$work/control.log" 2>&1 || rc=$?
test "$rc" -ne 0
grep -q '1 mutants, 0 killed, 1 survived, 0 errors' "$work/control.log"
echo 'ok unchanged product survives'
(cd "$work/component" && sh "$root/factory/testlib/run-mutants.sh" "$root" fixture "$work/semantic" make mutants/assertion.patch) > "$work/semantic.log" 2>&1 || { cat "$work/semantic.log"; exit 1; }
grep -q '1 mutants, 1 killed, 0 survived, 0 errors' "$work/semantic.log"
echo 'ok completed assertion killed twice'
echo 'factory: 1 mutants, 1 killed, 0 survived, 0 errors'
