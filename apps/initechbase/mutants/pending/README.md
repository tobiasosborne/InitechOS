# Pending fault oracles

These patches are converted, not killed by the tier-0 sweep.

- `ibase-ovl-mutate-escape-callback.patch`: Retained slot pointer/section lifetime; legacy overlay packaging/boot guards, host selection survives.
- `ibase-ovl-mutate-escape-hook.patch`: Retained slot pointer/section lifetime; legacy overlay packaging/boot guards, host selection survives.
- `ibase-ovl-mutate-no-crc.patch`: Target overlay manager swap/open/CRC or reentry; needs the legacy corrupt/missing/stale overlay boot journey.
- `ibase-ovl-mutate-no-swap.patch`: Target overlay manager swap/open/CRC or reentry; needs the legacy corrupt/missing/stale overlay boot journey.
- `ibase-ovl-mutate-skip-open.patch`: Target overlay manager swap/open/CRC or reentry; needs the legacy corrupt/missing/stale overlay boot journey.
- `ibase-stream-mutate-drop-crlf.patch`: DOS PAL/keyboard/framebuffer path is not compiled into host programs; legacy console/screen emulator journey needed.
- `pal-dos-mutate-drop-write.patch`: DOS PAL/keyboard/framebuffer path is not compiled into host programs; legacy console/screen emulator journey needed.
- `pal-dos-mutate-short-read.patch`: DOS PAL/keyboard/framebuffer path is not compiled into host programs; legacy console/screen emulator journey needed.
- `proc-mutate-stack-args.patch`: Legacy target-stack pressure check; all normal host suites survive (host pointer/stack widths differ).
