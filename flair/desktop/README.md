# FLAIR desktop

Builds libdesktop.a with the Finder, shell, desktop painter, live event pump,
INT 81h entry, platform adapter, alerts, Standard File, power and About box.
The production pump is extracted from legacy-final kmain.c; demo tenants,
boot personalities and mutation branches are excluded. The two Finder size
splits are rejoined. The quarry kmain_legacy.c is not built; final removal of
extracted ranges and test import are scheduled for S6 of the vertical slice.

Dependencies: flair/include and assets, flair/region/include, sdk/include,
kernel/include. All target C uses TARGET_CFLAGS with cdecl. The assembly
already passes cdecl arguments on the stack and cleans them.

The fifteen flair_platform functions follow ../NOTES-FOR-KERNEL.md. File
sessions use shadow PSPs and restore caller PSP/CWD. Allocation stays owned
by the Toolbox. Input hooks enqueue raw data; only the cooperative pump calls
the managers. Controller initialization polls with interrupts masked, as in
the legacy kernel. Exit detaches input/timer/fault hooks and returns through
the DOS graphical-environment hook after the kernel restores its console.

make builds. sh factory/smoke-desktop.sh from the root checks the S2 QEMU
startup/exit/restart session and preserves PPMs. The DOS smoke is separate.
Host tests remain parked until S6; no host-green claim is made yet.
