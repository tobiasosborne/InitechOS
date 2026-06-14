# InitechOS&trade;

### Operating Environment &mdash; Version 3.30

**Initech Systems Corporation**
Part No. 1024-3300-01 &middot; Document Control: STAPLER &middot; Revision G

> *Controlled Document. Printed copies are uncontrolled. Verify revision
> before use. This product is licensed, not sold.*

---

InitechOS&trade; is a single-user, single-tasking 32-bit operating environment
for Intel&reg; 80386&trade; and compatible business workstations. It integrates
the InitechDOS&trade; operating system, the FLAIR&trade; graphical operating
environment, and the InitechBase&trade; data management system into one unified,
fully conformant computing platform suitable for the modern automated office.

The product has been engineered in accordance with Initech Corporate Engineering
Standard PRD-STAPLER and is supplied subject to the Initech Systems Corporation
Software License Agreement (see [`LICENSE`](LICENSE)).

## Product configuration

| Module | Designation | Function |
|---|---|---|
| **InitechDOS&trade;** | `MILTON` | Operating system services: FAT-compatible file management, program execution, the standard INT 21h application interface, and the `COMMAND.COM` command processor. |
| **FLAIR&trade;** | `ATKINSON` / `FLAIR` | Graphical operating environment: window, menu, control, event, and dialog management with the standard desktop presentation. |
| **InitechBase&trade;** | `SAMIR` | Relational data management system. Reads and writes industry-standard `.DBF` data files and `.MDX` index files. |
| **Turbo Initech&reg;** | `TPS` | Integrated Pascal application development system with resident editor and compiler. |
| Bundled applications | &mdash; | InitechCalc&trade; spreadsheet, File Manager, InitechPaint&trade;, and the FILE COPY utility. |

## System requirements

- Intel 80386 or higher microprocessor
- 4 MB of system memory (2 MB minimum)
- Fixed-disk controller and one fixed disk
- One 5.25-inch or 3.5-inch diskette drive
- VGA-compatible display adapter and monitor
- Serial pointing device (recommended)

## Features

- FAT-compatible file system with hierarchical directory organization.
- Full complement of standard operating-system services per the Initech
  Application Binary Interface Specification, Appendix A.
- The `COMMAND.COM` command processor with batch-file and environment support.
- The FLAIR graphical operating environment with overlapping windows, pull-down
  menus, and the standard control set.
- Cooperative application scheduling for responsive multi-application operation.
- InitechBase relational data management with full-screen and command-line
  operation.
- The Turbo Initech integrated development system for in-house application
  development.
- Configurable system startup via `CONFIG.SYS` and `AUTOEXEC.BAT`.
- Installable character-device drivers and the standard resident devices
  (`CON`, `AUX`, `PRN`, `CLOCK$`, `NUL`).

## Installation

1. Insert Distribution Diskette 1 in drive A.
2. At the system prompt, type `A:SETUP` and press ENTER.
3. Follow the on-screen instructions. The Setup program will copy the system
   files to your fixed disk and configure `CONFIG.SYS` and `AUTOEXEC.BAT`.
4. Remove the diskette and restart the workstation.

Upon restart the system displays the InitechDOS banner and the `A:\>` command
prompt. Type `WIN` to start the FLAIR graphical operating environment.

## Conformance

InitechOS conforms to Initech Corporate Engineering Standard PRD-STAPLER and to
the ratified Architecture Decision Records governing the platform (see
[`docs/adr/`](docs/adr/)). The application interface is frozen at the Appendix A
service set; deterministic, reproducible system builds are a controlled
requirement of the platform.

---

### Appendix A &mdash; Operation on contemporary host systems

The distribution may be operated on a present-day host by means of an
80386-compatible system emulator. The build and emulation harness targets, in
order of fidelity, QEMU (development), Bochs (transition accuracy), and 86Box
(period authenticity).

```sh
# One-time host preparation (Debian/Ubuntu)
sudo apt install qemu-system-i386 bochs make nasm mtools

make image        # produce a bootable distribution image
make run          # start the system under QEMU
make run-bochs    # start the system under Bochs
make test         # run the full conformance and verification suite
```

The build is freestanding (`gcc -m32 -ffreestanding -nostdlib` + `nasm` + `ld`
on the interim toolchain; `i686-elf` cross-compilation on the target toolchain).
See [`CLAUDE.md`](CLAUDE.md) and [`InitechOS-PRD.md`](InitechOS-PRD.md) for the
engineering standard and the product requirements.

---

<sub>&copy; 1992, 1993 Initech Systems Corporation. All Rights Reserved.
InitechOS, InitechDOS, FLAIR, InitechBase, InitechCalc, InitechPaint, and the
Initech logo are trademarks of Initech Systems Corporation. Turbo Initech is a
registered trademark of Initech Systems Corporation. Intel and 80386 are
trademarks of Intel Corporation. All other trademarks are the property of their
respective holders. Technical Support: extension 2504.</sub>
