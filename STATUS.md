# Status

Rewritten, never appended to. Last rewritten 2026-11-13.

## Where we are

The first vertical slice works on branch slice/desktop. The product boots
InitechDOS, AUTOEXEC.BAT starts FLAIR, the Finder opens INITECH then APPS and
launches the real 123.EXE. The application accepts two numbers and a formula,
saves through its own /File Save, and quits. Special > Exit to InitechDOS
returns to A:\>; typing FLAIR starts the desktop again. The same product
kernel runs the DOS smoke on a private working disk without AUTOEXEC.BAT.

QEMU (486, 8 MiB, acpi=off) and Bochs (installed pentium, 8 MiB, VBE 0101)
both save SLICE.WK1. mtools extracts it and the independent Lotus corpus
reader verifies A1=123, B1=1.5, C1=@SUM(A1..B1), cached=124.5. The QEMU
session also verifies exit/restart and no CPU exception, guest error, triple
fault or extra reset. factory/smoke-desktop.sh is part of make smoke.

The original kmain quarry is 335 lines, down from 8207. Only unbuilt film
scene, hidden-mat activation and text-application handoff fragments remain.
There are no reference tenants, kernel variants, overlays or COMMAND transient
in the product link. The remaining hidden-mat/film work belongs to the desktop
and real InitechCalc integration, not to a new boot personality.

## The vertical slice comes first

S1-S6 are complete. Final make test is ALL GREEN (65.35 s); make smoke
passes its four DOS checks and five desktop checks (61.56 s).
Factory desktop acceptance and red evidence: factory/DESKTOP-ACCEPTANCE.md.
The two required patch mutants both fail and are restored: empty save fails
the host WK1 readback; old loader cap fails the missing-launch session.

flair/desktop implements all 15 flair_platform services using public kernel
headers. kernel/ keeps a weak optional environment entry, console restoration
and the ordinary DOS services; dos/command adds FLAIR. system/ owns the final
link. The file loader's 64 KiB policy had no physical buffer behind it: the
complete PSP/file/BSS allocation now has checked 32-bit arithmetic and is
limited by the supplied Toolbox heap carve, with its normal NOMEM result.
The existing e_minalloc format and 512 KiB BSS policy remain.

Product INITDOS.SYS is 268670 bytes; standalone DOS kernel is 106116. The
boot disk is 1507328 and working disk 1474560 bytes. Two final clean builds
match (53.55 s wall):

- boot: 3e3d92aa5e1eb8d37b4cccc6797e1fe89568f7c302260c7579ac5a640443126b
- data: c8f046cba02b8f0a669e7a26c479896f78b2ed61ba96ce4995f60b5a5ee95b70
- product: 74709462e8adb65539d49fad452c13865d4bb4741723e1a7a5a420040b1c0d4b
- standalone: 6828c24d98ef745b4c7d83f9aae4c6fd05f9d2f40461c215385fc0b163686d12

Desktop host tests run 17 retained suites plus a target/import audit. system/
checks both links, symbol ownership and factory input transport. Both are
registered in top-level COMPONENTS. The live legacy comparison reports zero
pixel differences for the system menu bar, disk icon, Trash icon and exposed
desktop background when both first clock paints have completed. Band 2 is
now the Finder and the HELLO/NOTES specimens are absent, as intended.

## Import board

| Component | State |
|---|---|
| boot/ | Builds and boots; oversized-kernel proof remains. |
| kernel/, dos/ | Product DOS smoke green on both emulators. Host/mutation import owned by import/kernel-tests; existing disabled hooks are not part of this slice. |
| flair/region | Host-green, target builds; eight patch mutants imported. |
| flair/ | Host-green, target Toolbox linked into product. Its remaining mutation conversion is separate. |
| flair/desktop | Target and 17 host suites green; product session on QEMU/Bochs. Additional parked tests and mutation patches are inventoried in its README and tests/MUTANTS-TODO.md. |
| system/ | Product composition and standalone-boundary checks. |
| sdk/ | Host-green and exercised through real INT81 by 123.EXE. |
| apps/initech123 | Host-green and real Finder launch/save/readback on both emulators. |
| apps/initechword | Host-green; app target packaging remains with its owner. |
| apps/initechbase | Host-green, target objects build. Its own reserve declaration blocks packaging; separate from the removed file cap. |
| turbo/ | Host-green; native/self-hosting work remains with its owner. |
| other apps | Source moved; resume imports using this vertical-slice session as the template. |
| factory/ | Product image, DOS+desktop smoke, red checks and clean-build reproducibility. |
| goldens/ | Corpus pinning belongs to conventions/hardening. |

## In flight and remaining ownership

- conventions/hardening owns shared mk/, top-level conventions, test budgets
  and corpus pinning. The slice edits no mk/*.mk file.
- import/kernel-tests owns kernel/DOS host tests and disabled-hook conversion.
  Hardware headers were moved unchanged to kernel/include/ by this slice;
  rebase tests' private-header paths accordingly. Existing resident interpreter
  coupling and utility INT13/25/26 access remain documented kernel/DOS debts.
- Desktop owner claims remaining attic GUI traces/checkers, film/InitechCalc
  harness, hidden-mat data packer and extra icon/clock/menu tests. The quarry
  and README name what is parked. Removed mutation knobs with existing tests
  are listed for conversion; make mutants still fails explicitly.
- Legacy-final remains read-only at /home/tobiasosborne/Projects/InitechOS-legacy.
  Its two desktop disk images are built locally for the live pixel comparison.
- Existing independent WK1 oracle uses Python from the Lotus corpus, as do
  Initech 123 host tests. No new Python factory implementation was introduced.
- Disk layout remains IDE master boot C:, IDE slave working A:. A single
  installable disk and the manuals' install instructions remain delivery work.
- No bd operations, delegation, remote automation or pushes in this slice.
