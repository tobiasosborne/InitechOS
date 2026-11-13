# Retired reference windows

HELLO/NOTES and tenantfx.asm are deleted. The C windows used direct
Window/Process/heap/region APIs and kernel-held launch state, rather than
the SDK INT 81h application boundary. Keeping them as a third example would
teach the unsupported interface and duplicate FLAIR's own manager tests.
The assembly fixture duplicated the now-imported C gate example. Neither
was a product app; no product-volume entry is removed. Existing comments
and independently transcribed gate tests elsewhere may still cite their
legacy-final source as historical ground truth.
