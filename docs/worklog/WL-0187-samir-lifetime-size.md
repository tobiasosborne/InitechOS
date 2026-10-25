# WL-0187 -- Compact SAMIR lifetime audits

Bead initech-1x6x2.6; lane/samir-shrink, based on 3c194c50. Ref: PRD
Sec 6.6, docs/design/samir-overlays.md Sec 6 rule 4. Operator instruction:
commit locally, no push, no bd.

## Context and measurement

The unchanged baseline loads 87,184 resident bytes, leaving 6,592 bytes
against the 93,776-byte budget (2,496 above the 4 KiB floor). A private
copy of that baseline with only require-resident/validate-PAL/validate-ctx
made inert loads 84,144 bytes: these lifetime guards cost 3,040 bytes.
This ablation is measurement-only and is never used by an acceptance gate.
It leaves the manager's other checks, including live reentry, intact.

`nm -S --size-sort` and section sizes show 2,054 resident text bytes and
992 rodata bytes of guard cost, less 6 bytes of alignment. Beware LTO:
without guards xb_interp_make is inlined into samir_milton_entry. The
combined pair grows from 1,520 to 1,898 bytes (+378), rather than all 665
bytes of the newly separate xb_interp_make being guard overhead. The two
samir_bind_resolver.lto_priv symbols alias the same 54 bytes.

## What changed and why

One resident walker checks the PAL/context/cursor fields in their original
order. Two-byte descriptors hold the offsetof and diagnostic suffix length;
the field lists generate both descriptors and suffixes. Compile-time checks
reject offset truncation, overlong names and incompatible pointer widths.
Function/data pointer representations are copied with fixed-size builtin
memcpy, avoiding type aliasing and emitting a single load on the target.

Only failures assemble a diagnostic from its prefix and suffix on the
stack, immediately before the non-returning panic. This retains every
original spelling, including v-> for binding and c->dbcur-> for boundary
audits. Cursor binding still checks c/v/user, then every nested slot, before
publishing. All other binding, ownership, boundary and reentry checks stay
in place. No interpreter path was moved into an overlay.

## Result

| Symbol / section | Before | After | Recovered |
|---|---:|---:|---:|
| samir_ovl_validate_pal | 414 | 31 | 383 |
| samir_ovl_validate_ctx | 386 | 64 | 322 |
| xb_interp_make + samir_milton_entry | 665 + 1233 | 0 + 1773 | 125 |
| ovl_validate_fields (new) | 0 | 115 | -115 |
| resident .text | 78,281 | 77,566 | 715 |
| resident .rodata (includes new descriptors) | 8,877 | 8,653 | 224 |
| resident alignment | 23 | 2 | 21 |
| resident module | 87,184 | 86,224 | **960** |

Data remains 3 bytes, BSS 47,252 bytes, slot 9,888 bytes, OVL file 27,136
bytes; every overlay's code length and relocation count are unchanged.
Headroom is now 7,552 bytes, 3,456 above the unchanged 4 KiB floor.
The intermediate walker using rt_memcpy loaded 86,256 bytes; fixed-size
builtin memcpy recovered another 32 bytes and avoids an extra call on
each successful field check. Shared diagnostic suffixes also reduce net
rodata despite the added tables. The original range predicate was already
out of line, so merely adding noinline would not remove its callers' code.

## Acceptance and frictions

Source tools/lane-env.sh before every build. The shared ccache directory is
read-only in this lane's sandbox: the first size command exited 2; subsequent
commands use CCACHE_DIR=$PWD/build/shrink-ccache. Reference variables point
to dbase3-decomp, lotus123-decomp, system7-decomp and win31-decomp under
/home/tobiasosborne/Projects.

`make -j64 test-merge-samir` exited 0 on its cold run: all 337 gates green,
including test-samir-size, test-samir-size-repro, all test-samir-ovl gates
and mutants, and the lifetime host/EMU/desktop gates and mutants. All ten
escape/reentry guard mutants and NO_RELOC completed their named diagnostic
kernel-panic traces (14 checks each). No warm retry was needed.

The new test-samir-ovl-lifetime uses the actual manager on the host, with a
captured panic endpoint. It poisons every one of the original retained
fields at both slot endpoints and checks exact names, pointer values,
half-open range edges, audit precedence and no cursor publication on a
failure: 600 checks, no failures. The same test against the original code
also passes. Removing index_name from a private copy of the new field list
exits 1 for missing v->index_name and c->dbcur->index_name refusals.
The first oracle run caught a test-fixture pointer expectation error; a
subsequent fixture edit had an incorrect function pointer cast, rejected by
-Werror. Both were corrected before the full acceptance run.

## Pointers

Full logs, before/after ELF symbol sizes, all per-symbol ablation deltas and
command exit codes: build/shrink-evidence/shrink-o.txt and sibling files.
The ablation source tree and skip-index-name mutant remain confined to that
ignored evidence directory. No specs, budgets, oracle tolerances or golden
files changed.
