#!/bin/sh
# A no-op must survive; injected transport/setup failures must be errors.
set -eu
root=${1:?root};out=${2:?output};mkdir -p "$out/fault-bin"
cat > "$out/noop.patch" <<'PATCH'
# session: auto
--- a/command/batch.c
+++ b/command/batch.c
@@ -833,1 +833,1 @@
-    int n = 0;
+    int n = 0; /* negative control: unchanged behavior */
PATCH
cat > "$out/fault-bin/timeout" <<'WRAPPER'
#!/bin/sh
for arg do
 case "$arg" in *.img)
  resolved=$(readlink -f "$arg")
  case "$resolved" in */noop/*)
   if [ "${CONTROL_TRANSPORT_RC:-0}" != 0 ]; then
    echo "CONTROL injected transport failure $CONTROL_TRANSPORT_RC" >&2
    exit "$CONTROL_TRANSPORT_RC"
   fi;;
  esac;;
 esac
done
exec /usr/bin/timeout "$@"
WRAPPER
chmod +x "$out/fault-bin/timeout"
for mode in normal timeout startup; do
 case $mode in normal) rc=0;;timeout) rc=124;;startup) rc=127;;esac
 status=0
 CONTROL_TRANSPORT_RC=$rc PATH="$out/fault-bin:$PATH" sh "$root/dos/tests/runtime/mutants.sh" "$root" "$out/noop.patch" > "$out/$mode.log" 2>&1 || status=$?
 [ "$status" -ne 0 ] || { echo "RED controls: false certificate for $mode" >&2;exit 1; }
 case $mode in
 normal) grep -F '1 mutants, 0 killed, 1 survived, 0 errors;' "$out/$mode.log" >/dev/null;;
 *) grep -F '1 mutants, 0 killed, 0 survived, 1 errors;' "$out/$mode.log" >/dev/null;;
 esac
done
echo 'GREEN runtime controls: no-op survives; timeout and startup failures are errors'
