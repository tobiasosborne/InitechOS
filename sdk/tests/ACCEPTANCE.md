# SDK import acceptance

Environment: this local worktree, GCC 13.3.0 and GNU binutils. No emulator,
remote automation, tracker, push or edits to FLAIR/kernel/InitechWord.
The branch is import/sdk. The legacy build was inspected at the operator's
absolute /home/tobiasosborne/Projects/InitechOS-legacy path with lane-env.sh
and make -n build/tenant/i123.exe; its already-built EXE was read directly.

## A. Clean builds and preserved application checks

After top-level make clean:

| Command | Result | Wall time |
|---|---|---|
| make -C sdk test | 37 active + 5 reserved ABI entries; target layouts; all external bindings link; 80 fake checks on each of native host and hosted i386; MZ I/O and five corruption cases green | 1.42 s |
| make -C apps/initech123 test | 18 entries, 0 failures | 14.97 s |
| make -C apps/initech123 | real 123.EXE + 123.MNU | 2.10 s |
| make test (after another top-level clean) | region 2 + SDK + Initech 123 18; ALL GREEN | 18.93 s |

The pre-import baseline took 15.56 s. Every one of the 18 resulting suite logs
was cmp-identical to its pre-import log, including oracle output and counts.
Counts remain 1,023,560 assertions, plus the separately counted 8 command
pairs, 4 tiling cases, 12 self-transfers and 82-level/381-item menu comparisons
listed in apps/initech123/tests/IMPORT.md. Face counts are quit 60, range-menu
2092, entry 73, clock 14, keys 62, hit 38, resize 55 and cells 28. Existing
independent saved-file checks still run after explicit fake fixture export.

## B. Red once, restored

make -C sdk red-once copies SDK source under build/sdk/red, mutates each copy,
and checks the specific diagnostic. Product sources are never left mutated.
All three were actually run, then the unchanged SDK tests were green:

- number: TBX_REGISTER 0x0001 -> 0x0003; abi exits nonzero with
  'ABI audit: TBX_REGISTER'.
- layout: tbx_event_t.wherev int32_t -> int64_t; TARGET_CC -m32 compile exits
  nonzero with 'static assertion failed: "tbx_event_t size"'.
- binding: rename only the fake's tbx_memory definition; host compilation
  succeeds, link-check exits nonzero with 'undefined reference to tbx_memory'.

An additional disk-full model regression was shown red before fixing it:
a positive-length write reduced to zero by a write budget must not perform
DOS's explicit zero-count truncate operation. The 80-check fake suite now
proves this distinction on both host architectures.

## C. Independent EXE validation and legacy comparison

The checker shares no constants/helpers with the writer and compiles no
kernel/attic source. It reads the format independently from the legacy kernel
contract and checks tag/header/page encoding, entry zero, table bounds,
non-overlapping in-image sites and every pointed word inside image+BSS.
With raw-module and BSS arguments it also checks payload identity and the
paragraph-rounded linker BSS size. The build separately checks tbx_entry=0
with nm. Actual output:

```
new:    MZ valid: file=73552 header=4032 image=69520 entry=0 relocations=999 BSS=298416
legacy: MZ valid: file=64804 header=32 image=61904 entry=0 relocations=717 BSS=298176
```

New file is 8,748 bytes larger: module +7,616; container/table overhead +1,132.
Legacy has a 32-byte header and 2,868-byte trailer table. New has a 4,032-byte
header containing the 3,996-byte relocation table and padding. Header placement
is required once the module exceeds the u16 trailer offset. Relocations +282
reflect retained absolute references and runtime functions without legacy LTO,
section garbage collection and size tuning. Default C calls replace the old
register/callee-pop convention. All four previously raw-only Control Panels
services now have named wrappers; CLOCK has a mode-capable binding plus the
preserved tbx_get_clock alias. No trap number or OS-side calling convention
changed. apps/initech123/tests/IMPORT.md lists the full dropped flag set:
-Oz, LTO, function/data sections, GC, stack/data alignment, register ABI and
per-pass size tuning are not reintroduced. Both links use shared TARGET flags.

nm -S shows identical BSS object totals (298,158 bytes) and identical object
sizes by normalized symbol name. The 240-byte e_minalloc difference is exactly
alignment padding: legacy 18 bytes, new 258. There is no worksheet capacity
change. The 17,746-byte 123.MNU compares byte-identical with the legacy
build/i123-menu.mnu resource.

A second build with OUT=build/apps/initech123/repro compared byte-identical
for both artifacts. SHA-256:

```
123.EXE e87d3234fbb5e45bc3203dd3dfa820bb71b378c3364af965bbccc331ab607ed8
123.MNU af69c30d09208e3edadcee9b40432b8da7166ac247d3910835425e601d763925
```

The new executable is structurally valid but exceeds loader_load_tenant's
explicit 64 KiB file-size policy. Tier 1 is deferred: lift that OS-side policy,
then launch through the actual booted desktop and run the QEMU/Bochs cases in
apps/initech123/tests/SMOKES.md. make smoke fails explicitly until then.

## D. Self-containment and real target ABI

The SDK test compiles tests/self-contained.c, whose sole include is "tbx.h",
with TARGET_CFLAGS and -Isdk/include alone (shown as -Iinclude from sdk cwd).
All target records, including native pointer-bearing menus/registration and
Control Panels items, are pinned by sizes and offsets in tests/layout.c.
The object is ELF32/i386. Hosted -m32 compile/run was probed successfully;
the layout program and full fake behavior test also execute as 32-bit ELF.
No host-only layout skip stands in for the target proof.

## E. Scope, ancestry and remaining debts

All attic claims used git mv and are recognized as renames (60-100% similarity)
in the SDK import commit. git log --follow on sdk/include/toolbox_gate.h and
factory/mz/mzlink.c traverses their original paths. Existing mk fragments,
FLAIR, kernel, InitechWord and STATUS.md are unchanged. The only top-level
change is COMPONENTS, with sdk before apps. Source text is ASCII and the
claimed SDK/factory code has no retired codenames. git diff --check is green.
The final integration commit leaves a clean working tree and is not pushed.

Header ownership handoff is sdk/NOTES-FOR-FLAIR.md. InitechWord must use the
SDK include/host directories, archive dependency and link, then replace its
local fake with state aliases/fixture boundaries while retaining its own
app-static driver. Known debts: no version negotiation, native i386 menu and
dialog pointers, raw host trap pointer-width limitation, source-audit coupling
to the current OS dispatch idiom, private dialog adapter relocation, and the
OS file-size/tier-1 work above. No ABI expansion was attempted.
