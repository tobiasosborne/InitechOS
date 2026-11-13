# Product composition

Links the InitechDOS objects and resident COMMAND.COM with the FLAIR Toolbox,
region engine and desktop binding into build/system/INITDOS.SYS. The standalone
kernel artifact remains build/kernel/INITDOS.SYS. Dependencies: kernel, dos,
flair and flair/desktop. No source is compiled here and no ABI flags are changed.

S1 verification: the QEMU and Bochs DOS smoke passes (8 MiB); the composed
image is 186010 loaded bytes. A working disk with AUTOEXEC.BAT containing
FLAIR followed by ECHO hook-complete reports FLAIR is not installed and
reaches hook-complete. This is the weak-hook milestone, not a desktop claim.

S2: factory/smoke-desktop.sh now checks Finder startup, pointer movement,
Special > Exit to InitechDOS (including its confirmation), a real A:\>
prompt, and a second FLAIR launch. QEMU reports zero faults and two power-on
CPU resets. Screens: build/smoke-desktop/{desktop,dos,qemu}.ppm.

S3 working-volume.list supplies the APPS directory from imported application
artifacts. Product AUTOEXEC.BAT ends with FLAIR. The DOS smoke deletes that
file from its private writable volume copies, retaining the product kernel.
Finder opens INITECH then APPS; the APPS view contains 123.EXE and 123.MNU.
The QEMU folder screenshot is build/smoke-s3/apps.ppm.
