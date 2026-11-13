# FLAIR desktop

The first build milestone links the platform adapter, desktop painter and
cdecl INT 81h entry with the Toolbox. The Finder and live pump are still
parked in kmain_legacy.c pending extraction. The default DOS FLAIR hook reports
that the graphical environment is not installed and returns to the caller.

Dependencies: flair/include and assets, flair/region/include, sdk/include,
kernel/include. The adapter implements the fifteen flair_platform functions
specified in ../NOTES-FOR-KERNEL.md. Files use a shadow PSP and restore the
caller's PSP and CWD; tenant allocation stays owned by the Toolbox heap.
The assembly already passes cdecl arguments on the stack and cleans them.
