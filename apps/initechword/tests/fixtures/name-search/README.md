Real WordPerfect 5.1 03-09-92, DOS 3.30, QEMU 486/8 MiB, fixed 1992 RTC.
Captured 2026-11-14 via goldens/dataloss/{runtime.sh,drive.c}. Each session
started with a fresh writable runtime image; no product code is linked.
input.steps lists a dedicated NAMES directory: ALDIR/ZZDIR and three copies
of real MINT02.WP5 named AONE/ATWO/BONE. wp-directory.steps uses the installed
WP51 directory, with the same three added files and ZZDIR. The unmodified
4000-byte VGA buffers retain the red selection band and command/query text.

Manual authority: WP5.1 Reference printed 351-352. Regular prefixes exclude
directories; / or backslash selects directories. First matching prefix wins;
no match selects its lexical predecessor, clamped to first/last. Thus az
selects ATWO and 0 selects AONE in NAMES, while atz selects ATWO rather than
ATI.VRS in WP51. Backspace recomputes; arrows dismiss then move; F7 and
Enter dismiss without opening. Names are case-insensitive, query echo is not.
