# Product disk set

Layout is the legacy-final shell image, not a redesigned installer:

- boot.img: IDE primary master, 2944 sectors (46 cylinders, 2 heads, 32
  sectors/track). MBR at LBA0, stage2 in LBA1..16; active FAT12 BOOT partition
  at LBA63, 2880 sectors. Its first two root entries are hidden/system/read-only
  IO.SYS and INITDOS.SYS, in that order. The kernel mounts this as C:.
- data.img: IDE primary slave, 2880 sectors, unpartitioned FAT12, mounted as A:.
  It holds CONFIG.SYS, COMMAND.COM, HELLO.TXT and all external .COM utilities.
  AUTOEXEC.BAT, if supplied on A:, is run by the permanent interpreter. The
  shipped image omits it, preserving the date/time prompts. PATH starts with
  the shell's legacy defaults; utilities resolve from A:'s root.
- File dates are fixed at 1980-01-01, volume serials fixed, source copy order
  lexical. Host source paths and build dates never enter disk bytes. The
  independent mtools reader validates the guest-written files in smoke.

References: legacy Makefile tracer_boot.img, tools/mk_boot_system.sh,
block_device_map/drive_letters.h and shell startup. boot_system.c was claimed
from the legacy independent FAT differential; its partition/check modes are
used here. No attic code is built. The old overlay packaging checks and
optional FLAIR.OVL/COMMAND transient arguments are removed: neither retired
format is part of this disk set.

A single installable product should reconsider the split between BIOS boot
files on C: and the working system on A:, fixed drive-letter policy, shell
residency, CONFIG/AUTOEXEC placement and distribution of utilities. No such
redesign belongs in this spine import. SYS/FORMAT's inherited physical-drive
interfaces and protections also need an installation oracle before use as a
product installer.
