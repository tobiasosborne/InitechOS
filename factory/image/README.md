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
the working-volume manifest. The staged CONFIG.SYS and AUTOEXEC.BAT come
from system's installed/c-boot directory. The user starts at the C: prompt
and types FLAIR. The optional A: data volume, build/image/a-data.img, is
a 1.44 MB FAT12 ATA disk filled from system/data-volume.list.

Every image build compares the C: inventory, including hidden files, to the
manifest and its inputs. The layout reader compares both system file chains
to the actual loader and selected kernel. Checks cover the partition, BPB,
free space, both filesystems, DATA's attributes and its recorded window at
(46,115). The default boot.img and data.img serve the A: system until
activation step 20.

After `make image`, run `make -C factory smoke-staged-media` for the matching
corrupt sibling control and C-only/C+A prompt and persisted-file checks on
QEMU and Bochs. A driver running from the source root can source
`factory/image/test-media.sh` and call
`media_prepare staged with-data "$run"` (also accepts `canonical` and
`alone`). Prepare fixtures via `media_work`, then use
`media_run "$emulator" "$driver" ...` with ordinary harness options.
After a completed run, `media_readback "$emulator" "$report"` sets
`media_volume` to the raw working FAT filesystem and `media_written` to
the actual written master. `media_data` holds the optional slave path.

## Working disks

`make run` and `make run-bochs` use `user-disks/boot.img` and
`user-disks/data.img`. On first start they copy the product pair after a
fresh `make image`. They share a checkout lock and refuse an incomplete
pair. Bochs attaches the working boot disk in place for this launcher.
Smoke and repro still use the factory images.

Run `sh factory/image/tests/launch.sh` from a disposable checkout to probe
the launch path with stub emulators and mtools. It builds the image, inserts
markers in both drives, and reads them on restart after an image rebuild
and after `make clean`. It leaves the test's working disks in that checkout.
