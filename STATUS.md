# Status

Rewritten 2026-11-13 for the factory wave. Use `make status` for the live board.

## Product

`make image` builds the boot and working disks. The 486/8 MiB product boots
InitechDOS, starts FLAIR and launches Initech 123 through the Finder. The
factory DOS, test-program, desktop and Turbo Initech smoke sessions run on
QEMU and Bochs. Test programs live on separate disposable volumes.
Turbo Initech compiles itself in the OS; selfhost checks K2 == K3 and DDC
compares the independent Free Pascal bootstrap against that fixed point.

## Factory wave

The factory owner is wave2/factory: mk/, top-level Makefile, factory/ except
the three desktop smoke/readback scripts, and the authorized component build
wiring. Application and Toolbox source owners work in separate worktrees.
This wave does not push and does not use the tracker, per operator instruction.

- Resident COMMAND.COM uses command-resident.o, separate from other variants.
  Kernel entry objects, ELF links and SYS artifacts have separate big-kernel
  names. The system link consumes the kernel's explicit object list, rather
  than every object left in its output directory. Command dependencies no
  longer include generated .d files as if they were make configuration.
- Import auditing uses all-reachable sources plus public headers, strips
  C/NASM/Pascal comments and literals and retains real hook/include failures.
  Tiny red/green fixtures run in factory's test target. Kernel's two resident
  interpreter includes still violate the required include spelling; the
  desktop still has a live POWER_MUT_SILENT_CANCEL hook. Those source fixes
  belong to their component owners. Their import audits stay disabled.
- Target artifacts are declared for kernel, dos, turbo, system and desktop.
  Import checks are enabled where all four conditions pass: region, dos, SDK,
  Turbo Initech and system. Remaining app/Toolbox imports belong to their owners.
- SDK's three save mutants and the seed's 17 mutants are patch files and killed.
  Seed wrong-code catchers execute the compiled programs on the real DOS image
  and compare stdout/files with Free Pascal. The two wrong file-I/O runtime
  paths are removed; the checked-in native runtime table is regenerated.
  The SDK runtime hash is unchanged; all 37 unchanged corpus/example programs
  emit identical seed assembly. Runtime and compiler fixed-point bytes change
  intentionally when the dead runtime faults are removed.
- mk/dependencies.mk is the authoritative build dependency graph. The same
  declarations drive parallel test order, `make dependencies` and an audit of
  references to other components' build outputs. README prose describes the
  interfaces, without maintaining a second scheduling table.
- `make cert` now runs the complete local tier 2, keeps each stage's wall time
  and log, continues after failures and exits nonzero if any stage failed.
  CERT_STRICT=1 additionally requires every registered component imported.
  Pending mutation patches are reported, without being called killed or failed.
  Results are under build/cert/<UTC date/time>-<short sha>. The first full run
  is next; its result will replace this note before session close.

## Remaining work and boundaries

The vertical slice is complete. Application imports, remaining app mutation
conversion, the film scene/FILE COPY work and Turbo Initech's IDE remain on the
closed golden-master list. Unclaimed legacy tests/assets remain in attic/;
the future owner is the corresponding component import, not the factory.
No new attic content was imported by this wave.

The inherited Python readers remain an operator language-policy question.
The loader's current native arena/heap/stack limits and DOS process-boundary
debt remain documented in the component READMEs. Bochs uses its installed
Pentium model; QEMU uses 486 with ACPI off. Both smoke at 8 MiB.

The frozen legacy tree is at /home/tobiasosborne/Projects/InitechOS-legacy.
External period-software captures are hash-pinned by goldens/CORPORA.lock;
the root overrides and defaults are in mk/config.mk.
