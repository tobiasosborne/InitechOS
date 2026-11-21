#!/bin/sh
set -eu
top=$1 component=$2
shift 2
failed=0
for patch_file do
    case $component/$patch_file in
        # factory/testlib/tests/host-mutants.sh copies this into its generated
        # component. The runner's baseline make test checks it there.
        factory/mutants/host/assertion.patch) continue ;;
        # The factory gate and emulator runners apply to a whole-tree copy.
        factory/*) patch_root=$top ;;
        # Component runners apply to component copies. Use the current
        # directory so the same check works in an isolated baseline.
        *) patch_root=. ;;
    esac
    if ! result=$(patch --dry-run --batch --forward --fuzz=0 -p1 -d "$patch_root" < "$patch_file" 2>&1); then
        printf 'FAIL stale mutant patch: %s/%s\n%s\n' "$component" "$patch_file" "$result" >&2
        failed=1
    fi
done
exit "$failed"
