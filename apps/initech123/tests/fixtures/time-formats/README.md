# Fixed AM/PM time formats

Real Lotus 1-2-3 R2.2 under initialized DOS33/QEMU 486/8MiB runtime, driven
by goldens/dataloss/drive.c. Exact steps, raw VGA, transcriptions, saved WK1s
and monitor logs are retained and hash-pinned. All expectations are reference
bytes, read independently; no expected strings/format bytes come from fmt.c.
Reference Manual p. 1-22 supplies fixed Time 1/2 meanings.

Tests compare 16-character cells to raw reference VGA, including alignment,
then compare saved cell/format/global-format records with a separate WK1
reader. Midnight/noon, single-digit hours (leading zero), second rounding,
minute truncation, fractional days, large whole-day values, negatives,
formula results, global/explicit overrides and narrow columns are covered.

The near-day-end carry is peculiar: rounding to 86400 seconds prints 12 PM,
not AM. Time 2 drops seconds after that rounding; a rounded 12:01:00 prints
12:01 PM while 12:00:30 remains 12:00 PM. Preserve these captured results.
Both formats print asterisks for negative values and for insufficient width.

The first global_minutes VGA still shows a menu/Global Settings overlay;
it is an observation, not a cell oracle. Tests use the READY global_seconds
and narrow_seconds frames. The initial final Save was stopped too quickly;
final/ replays that Save with settling sleeps and retains extracted NARROW.
