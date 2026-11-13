# Product composition

Links the InitechDOS objects and resident COMMAND.COM with the FLAIR Toolbox,
region engine and desktop binding into build/system/INITDOS.SYS. The standalone
kernel artifact remains build/kernel/INITDOS.SYS. Dependencies: kernel, dos,
flair and flair/desktop. No source is compiled here and no ABI flags are changed.

S1 verification: the QEMU and Bochs DOS smoke passes (8 MiB); the composed
image is 186010 loaded bytes. A working disk with AUTOEXEC.BAT containing
FLAIR followed by ECHO hook-complete reports FLAIR is not installed and
reaches hook-complete. This is the weak-hook milestone, not a desktop claim.
