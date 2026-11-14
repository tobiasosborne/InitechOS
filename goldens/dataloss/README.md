# Data-loss reference captures

Captured locally on 2026-11-13, on branch `goldens/dataloss`. These are
independent observations of real software, not InitechOS output. No product
source was changed. `SHA256SUMS` pins every retained evidence byte and script.

## Reference software and method

- Packard Bell OEM MS-DOS 3.30 Sys and Util, 360 KB, disk 1; original SHA-256
  `3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30`.
  Provenance: `initech-manuals-ref/sidekick/dosboot/` and the existing
  `kernel/tests/fixtures/dos33_overlay/README.txt`.
- Lotus 1-2-3 Release 2.2, the initialized `lotus123-decomp/mint/work_ref`
  runtime described in that corpus's `SOURCES.md` section 1b. Its original
  CNF is retained; the runtime is copied to `C:\LOTUS`.
- WordPerfect 5.1, installed **03-09-92** corpus build, copied from
  `wordperfect51-decomp/mint/work/WP51` to `C:\WP51`. `FILES=40` and
  `BUFFERS=20` in DOS CONFIG.SYS allow this installed runtime to start.
  The default DOS file-handle setting did not start the editor here.
- DOSBox-X is absent on this machine's PATH and searched local executable
  locations. Both real installed applications run directly in QEMU with real
  DOS; neither application nor version was substituted. Existing corpus
  DOSBox-X step scripts supplied the menu/key conventions.

`inputs/` records software/reader hashes, corpus HEADs, boot commands and
versions. Original software is neither modified nor committed. A disposable
32 MiB MBR/FAT16 disk (partition starts at byte 32256) holds the apps. QEMU
8.2.2, CPU 486, 8 MiB, fixed RTC base, no network. One reference emulator at
a time; later emulator runs at priority 15. The driver follows the existing C HMP/VGA
mint in `dos/command/tests/batch_break_mint/`. It types actual keys, reads
physical VGA text bytes and asks QEMU for a framebuffer dump. PNGs are
lossless conversions of those dumps, with no retouching. `.txt` is a
25-line, 80-column transcription of `.vga` character bytes (NUL becomes
space); `.vga` retains attributes and all exact original bytes. The guest
clock and file times in captures are observations, not normalized output.

Every disk is read only after its emulator has stopped. `*.mdir` is mtools
output, `*.WK1`, `*.WP5`, `*.BK!`, `*.OUT` are exact extracted bytes. Dumps
use the corpus's independent Python format readers. `data.img.gz` is the
entire post-operation FAT12 user-data disk (no system/application binaries),
compressed with `gzip -n`; it permits independent re-extraction and FAT
inspection. Boot and application disks remain disposable local scratch.
Compressed HMP logs preserve the actual monitor commands and capture paths.

App captures are also copied to the corresponding external corpus's
`goldens/minted/dataloss/`, following its convention. These new directories are left
untracked in the external checkouts. The committed copies here make the research portable without
changing the pinned corpus HEAD or Lotus `mint/*.steps` enumeration in
`CORPORA.lock`. Existing tier-0 reference inputs and their lock are unchanged.

## Reproduce

Run from any directory, with absolute output directories that do not exist
(or are empty), and the existing reference checkouts at their recorded paths:

```
sh goldens/dataloss/runtime.sh /tmp/new-dataloss-runtime
sh goldens/dataloss/dos-copy.sh /tmp/new-dos-copy
sh goldens/dataloss/lotus.sh /tmp/new-lotus /tmp/new-dataloss-runtime/runtime.img /tmp/new-dataloss-runtime/lotus-boot.img
sh goldens/dataloss/wordperfect.sh /tmp/new-wp /tmp/new-dataloss-runtime/runtime.img /tmp/new-dataloss-runtime/wp-boot.img
```

Requires C compiler, QEMU, mtools,
sfdisk, ImageMagick, POSIX shell and Python only for independent readers.
This replays behavior; application timestamps/settings serialization and
host fixture times mean a fresh run is not promised identical disk bytes.
The immutable captured bytes, rather than a regenerated expected result,
are the oracle. SHA verification: `(cd goldens/dataloss && sha256sum -c SHA256SUMS)`.

## DOS COPY: initech-u6x8 and initech-rp8d

`dos/` records ordinary COPY and explicit `/B`, new/overwrite, 8192 bytes
free and zero free, `/A`, `/A`'s final Ctrl-Z exhaustion, and self-copy.
Each large source is 131072 bytes. The old destination is 4096 bytes.

Observed large-copy failure: target absent afterwards. A new name restores
its original free-space count; overwrite destroys the old target and frees
its former clusters. Exact diagnostic/count are `COPY.OUT`, not paraphrases.
Self-copy (including case plus `A:\.\source.bin` alias) refuses and leaves
the source hash unchanged.

This settles DOS COPY cleanup and wording. It does not make deleting an old
destination an acceptable fix for graphical FILE COPY: GOLDEN-MASTER's
preservation requirement is separate. No GUI alert, cancellation,
concatenation, interruption or media-I/O-error behavior was captured.

## Lotus: initech-uyi1

One name, TOTAL=A1, and four sentinel cells A10:B11. Name Table at A10 writes
TOTAL and A1 (not A1..A1), then clears A11:B11 on success. Cases:

- `disabled`: global protection off, success and following-row deletion.
- `allprotected`: global protection on, first output cell protected; refuses
  with `Protected cell`; complete WK1 before/after files are identical.
- `firstprotected`: only B10:B11 unprotected; same complete refusal.
- `secondprotected`: only A10:A11 unprotected; writes A10=TOTAL, then refuses
  at protected B10. B10 and both following cells retain their sentinels.
- `terminator`: only A10:B10 unprotected; writes both table cells, refuses at
  protected A11. Both following-row sentinels remain intact.
- `unprotected`: global protection on, A10:B11 unprotected; full success.
- `labels`: global protection on, all cells protected. `/Range Name Labels
  Right D10` rebinds existing TOTAL from A1 to E10. All cell records remain
  byte-identical; only the named-range coordinates change.

Implement protection at every actual cell write/erase to match the observed
sequential semantics. A transactional preflight would be a stronger product
policy, not parity established by these captures. Do not block Name Labels
on cell protection. Absent protected cells, boundary rows and multi-name
ordering are not separately exercised here.

## WordPerfect: initech-gj7z

All scenarios show Backup=Yes on screen, retrieve real MINT01 (495 bytes)
as B:\NAME.WP5, seed NAME.BK! with real MINT02 (344 bytes), append the literal
revision text in `input.steps`, and answer the normal replacement question.

`collision`: existing backup is replaced without a separate backup-collision
question. New document is 804 bytes; NAME.BK! is the exact old 495-byte file.
`full`: floppy initially has zero free bytes. At the disk-full error screen,
NAME.WP5 is absent; NAME.BK! is the exact old 495-byte file, and 512 bytes
are free. The earlier 344-byte backup is gone; no partial document remains.
The edited text is still displayed in memory. Evidence is taken at the error
prompt, before dismissal/retry or normal application exit.

Honor the preference, rotate the previous saved bytes to .BK!, and retain a
recoverable old document on failure. The observed failure uses .BK!, not
restoration under the original filename. A stronger rollback policy must be
identified as such. This does not establish every write/close/rename fault,
crash durability, timed backup, backup-off, or cross-extension behavior.

`readonly`: NAME.BK! starts with the DOS read-only attribute. The save
attempt returns to the filename prompt, leaving both existing files
byte-identical and the backup read-only attribute set. The screen sampling
does not establish the intervening diagnostic, so no error wording is
claimed for this case. `after.attrib` and the raw disk preserve attributes.

## Validation

Top-level `nice -n 15 make -j2 test` completed ALL GREEN (11 registered,
11 run, 154 seconds; complete log compressed in `validation/`). No interface
or product implementation changed, so no product smoke was needed for this
research-only branch. Captures have independent byte/hash and format-reader
checks, with a deliberately corrupted evidence copy rejected once by the
hash verifier. This is evidence validation, not a claim that the open bugs
have been fixed or passed a product regression gate.

The WP independent reader reports the saved document text through byte 804
with zero mirrored-trailer errors. Its legacy linked-packet/index dump also
shows stale fast-save packet entries; that dump is retained verbatim and
is not a claim that all WP packet variants have been validated.

`wordperfect/reopen/` starts the real WP again on a copy of the successful
collision data floppy with a pristine application runtime and retrieves NAME.WP5. Its screen independently confirms
the edited document reopens successfully; these keys perform no save.
To repeat the reopen check, copy the successful data image, supply a fresh
runtime image from runtime.sh, and pass `wordperfect/reopen/input.steps`
to `drive OUT BOOT DATA RUNTIME STEPS`, using a compiled copy of drive.c.
