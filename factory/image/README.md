# Product disk set

Layout is the legacy-final shell image, not a redesigned installer:

- boot.img: IDE primary master, 2944 sectors (46 cylinders, 2 heads, 32
  sectors/track). MBR at LBA0, stage2 in LBA1..16; active FAT12 BOOT partition
  at LBA63, 2880 sectors. Its first two root entries are hidden/system/read-only
  IO.SYS and INITDOS.SYS, in that order. The kernel mounts this as C:.
- data.img: IDE primary slave, 2880 sectors, unpartitioned FAT12, mounted as A:.
  It holds CONFIG.SYS, COMMAND.COM, HELLO.TXT and all external .COM utilities.
  AUTOEXEC.BAT, if supplied on A:, is run by the permanent interpreter. The
  shipped image runs FLAIR from it. PATH starts with
  the shell's legacy defaults; utilities resolve from A:'s root.
- File dates are fixed at 1980-01-01, volume serials fixed, source copy order
  lexical at every depth. copy-tree.sh creates directories and inserts files
  explicitly: mcopy -s uses host readdir order, even under LC_ALL=C. The
  order regression compares literal FAT entries and whole disk bytes for
  opposite host insertion orders. Host source paths and build dates never enter disk bytes. The
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

## C: system disk (staged; initech-tlhqy)

build.sh also mints build/image/c.img, the delivery owner's C: system disk:
20160 sectors (20 complete 16x63 CHS cylinders), MBR and stage2, one FAT12
partition at LBA 63 of 16384 sectors (8 MiB) with 8 sectors per cluster,
IO.SYS and INITDOS.SYS as root entries 0 and 1 (HSR), label INITECH, then
the same working volume as data.img with an AUTOEXEC.BAT putting C:\ and
C:\APPS on the PATH; and build/image/a-data.img, the optional A: data volume
(1.44 MB FAT12 ATA disk) filled by system/install-data.sh from
system/data-volume.list with the shipped 1980-01-01 timestamps.
c-disk-check.sh judges both independently (sfdisk, minfo, boot_system check,
the working volume byte for byte, the data manifest and its attributes,
>= 2 MiB free, fsck.fat), on every make image; repro.sh hashes both.
They are not yet the booted product: boot.img/data.img stay the product
until delivery's C: startup is on main and the smoke drivers move to
C:@@32256 (the agreed staging in factory-owner-2.md).
