# Status

Rewritten, never appended to. Last rewritten 2026-11-13.

## Where we are

The restructured tree has its first bootable DOS spine on spine/boot-kernel.
`make image` builds one BIOS boot disk plus its writable A: data disk. The
chunked loader, InitechDOS, resident COMMAND.COM and fifteen external utilities
build from component Makefiles. `make smoke` drives the product in QEMU at
8 MiB with ACPI off and in Bochs with the VGA fallback, checks concrete serial
output and independently reads the modified FAT disk with mtools. No GUI is
built into this image. The legacy tree remains read-only at
/home/tobiasosborne/Projects/InitechOS-legacy, tag legacy-final.

## Import board

An imported tier-0 component still requires its host suite and COMPONENTS
entry. The spine's booted checks do not claim the pending host-test imports.

| Component | State |
|---|---|
| boot/ | sole variable-size loader imported; normal and oversized boots checked |
| kernel/ | production DOS entry and build; host tests parked under tests/ |
| dos/ | resident interpreter and all fifteen external utilities built; host suites pending |
| factory/ | local image builder, QEMU/Bochs drivers, smoke, parity and repro checks |
| flair/desktop/ | desktop/Finder/COLD quarry parked and not built |
| flair/, sdk/, apps/*, turbo/ | remaining imports, including pilots, proceed separately |
| goldens/ | reference captures still to claim from attic and external corpora |

## Verified spine behavior

The complete factory/session.txt interactive transcript matches the frozen
legacy shell. Smoke rejects a deliberately corrupted VER line. Both emulators
load a kernel over the retired 196608-byte limit and verify the deterministic
128 KiB blob. Two clean image builds have identical SHA-256 values. The
specific low-memory refusal is checked independently of kernel entry.
Evidence, limitations and follow-up work are in factory/ACCEPTANCE.md and the
component READMEs. This branch is committed locally, not pushed, as requested.

## Next

- Import kernel tier-0 tests from kernel/tests/ and their attic fixtures; move
  shell tests into dos/tests/ and claim utility differential tests.
- Convert remaining guarded mutation hooks to patches; the exact inventory is
  kernel/tests/MUTANTS-TODO.md. Untouched tests retain old paths/codenames.
- Separate COMMAND.COM from kernel internals and resolve utility raw-disk/MCB
  boundary debt before claiming an INT21-only program boundary.
- Extract the production desktop from flair/desktop/kmain_legacy.c; do not
  revive boot personalities, exact-address transients, kernel overlays or COLD.
- Continue the product master scope: frame ledger and safe FILE COPY, remaining
  app cores and the Pascal compiler/self-host proof. No remote automation.
