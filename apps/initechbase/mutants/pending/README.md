# Pending fault oracles

These patches are converted, not killed by the tier-0 sweep.

- `ibase-cursor25-mutant.patch`: DOS PAL/keyboard/framebuffer path is not compiled into host programs; legacy console/screen emulator journey needed.
- `ibase-dotprompt-mutant-blank79.patch`: Real screen/cursor capture comparison belongs to the deferred legacy emulator journey; all behavioral host suites survive.
- `ibase-dotprompt-mutant-bottom.patch`: Real screen/cursor capture comparison belongs to the deferred legacy emulator journey; all behavioral host suites survive.
- `ibase-dotprompt-mutant-clear.patch`: Real screen/cursor capture comparison belongs to the deferred legacy emulator journey; all behavioral host suites survive.
- `ibase-dotprompt-mutant-enter.patch`: DOS PAL/keyboard/framebuffer path is not compiled into host programs; legacy console/screen emulator journey needed.
- `ibase-forms-mutant-clear.patch`: Real screen/cursor capture comparison belongs to the deferred legacy emulator journey; all behavioral host suites survive.
- `ibase-forms-mutant-cursor.patch`: Real screen/cursor capture comparison belongs to the deferred legacy emulator journey; all behavioral host suites survive.
- `ibase-forms-mutant-glyph.patch`: Real screen/cursor capture comparison belongs to the deferred legacy emulator journey; all behavioral host suites survive.
- `ibase-forms-mutant-narrow.patch`: Real screen/cursor capture comparison belongs to the deferred legacy emulator journey; all behavioral host suites survive.
- `ibase-forms-mutant-subcmd.patch`: Real screen/cursor capture comparison belongs to the deferred legacy emulator journey; all behavioral host suites survive.
- `ibase-forms-mutant-wrap.patch`: DOS PAL/keyboard/framebuffer path is not compiled into host programs; legacy console/screen emulator journey needed.
- `ibase-ovl-mutate-cross.patch`: Target linker NOCROSSREFS/call-graph rejection; no host runtime kill (legacy static-overlay gate).
- `ibase-ovl-mutate-escape-callback.patch`: Retained slot pointer/section lifetime; legacy overlay packaging/boot guards, host selection survives.
- `ibase-ovl-mutate-escape-hook.patch`: Retained slot pointer/section lifetime; legacy overlay packaging/boot guards, host selection survives.
- `ibase-ovl-mutate-no-crc.patch`: Target overlay manager swap/open/CRC or reentry; needs the legacy corrupt/missing/stale overlay boot journey.
- `ibase-ovl-mutate-no-swap.patch`: Target overlay manager swap/open/CRC or reentry; needs the legacy corrupt/missing/stale overlay boot journey.
- `ibase-ovl-mutate-reenter.patch`: Target overlay manager swap/open/CRC or reentry; needs the legacy corrupt/missing/stale overlay boot journey.
- `ibase-ovl-mutate-resref.patch`: Target linker NOCROSSREFS/call-graph rejection; no host runtime kill (legacy static-overlay gate).
- `ibase-ovl-mutate-skip-open.patch`: Target overlay manager swap/open/CRC or reentry; needs the legacy corrupt/missing/stale overlay boot journey.
- `ibase-review-mutant-color-reset.patch`: DOS PAL/keyboard/framebuffer path is not compiled into host programs; legacy console/screen emulator journey needed.
- `ibase-review-mutant-color.patch`: DOS PAL/keyboard/framebuffer path is not compiled into host programs; legacy console/screen emulator journey needed.
- `ibase-review-mutant-keys.patch`: DOS PAL/keyboard/framebuffer path is not compiled into host programs; legacy console/screen emulator journey needed.
- `ibase-stream-mutate-drop-crlf.patch`: DOS PAL/keyboard/framebuffer path is not compiled into host programs; legacy console/screen emulator journey needed.
- `pal-dos-mutate-drop-write.patch`: DOS PAL/keyboard/framebuffer path is not compiled into host programs; legacy console/screen emulator journey needed.
- `pal-dos-mutate-short-read.patch`: DOS PAL/keyboard/framebuffer path is not compiled into host programs; legacy console/screen emulator journey needed.
- `proc-mutate-stack-args.patch`: Legacy target-stack pressure check; all normal host suites survive (host pointer/stack widths differ).
- `ibase-dotprompt-mutant-scoreboard.patch`: legacy dot-prompt screen capture; the selected prog_diff does not link screen.c and the strict repeat found no host kill.
- `ibase-review-mutant-redirect.patch`: DOS PAL redirection behavior; pal_dos.c is target-only and the strict repeat found no host kill.
