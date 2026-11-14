# EXEC feasibility probe

Question (design ruling for GOLDEN-MASTER item 7): can one extended
`TURBO.COM` stay resident while it EXECs the program it has compiled, and
regain control afterwards?

`probe.sh` builds `PARENT.COM` exactly the way `TURBO.COM` is packaged
(extended InitechMZ via `turbo/compiler/tests/package.sh`, loaded at
0x600000) with a C body that EXECs, in order: a position-independent flat
child (`PIC.COM`), the extended output of the hosted Turbo compiler for
`hello.pas` (`EXT.EXE`), `PIC.COM` again, and the fixed-base flat output of
the same compiler (`FLAT.COM`). The shell first runs `PIC.COM` and
`FLAT.COM` at top level as controls. It boots the ordinary product image
with a separate TEST disk and reads the serial log.

Observed on QEMU, 8 MiB, 2026-11-14 (build/turbo/probe/serial.txt):

```
PROBE-CHILD-RAN at=00040100          top-level PIC.COM control
PROBE-PASCAL-CHILD                   top-level FLAT.COM control
PROBE-PARENT-START
PROBE-CHILD-RAN at=00041220
PROBE-EXEC cf=0 ax=0000 status=0007 name=PIC.COM
PROBE-PARENT-RESIDENT
PROBE-EXEC cf=1 ax=0008 status=0000 name=EXT.EXE
PROBE-PARENT-RESIDENT
PROBE-CHILD-RAN at=00041220
PROBE-EXEC cf=0 ax=0000 status=0007 name=PIC.COM
PROBE-PARENT-RESIDENT
PROBE-EXEC cf=0 ax=0000 status=0000 name=FLAT.COM
PROBE-PARENT-RESIDENT
PROBE-PARENT-END
```

Reading:

- Residency and return work: the extended parent survives two nested runs
  and gets the child's exit code through AH=4Dh.
- An extended child is refused with error 8 (`kernel/loader.c`,
  `g_high_live`): Turbo's extended output cannot run under the IDE.
- A nested flat child is placed after the parent's low PSP block
  (0x41220), not at its link base 0x40100. Turbo's flat output is linked
  at 0x40100 and is not position independent, so it ran at the wrong base:
  EXEC reported success with exit code 0 and the program printed nothing.
  The kernel cannot tell, and runs it anyway.

So one `TURBO.COM` cannot run what it compiles without a change outside
`turbo/`; see the handoff and the bd issue for what the kernel must
provide. This probe is evidence, not a regression test; it is not part of
`make test` or `make smoke`.
