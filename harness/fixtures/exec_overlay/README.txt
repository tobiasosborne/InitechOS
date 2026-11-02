RED-first evidence, review follow-up initech-i15, 2026-10-25.

Base artifact: 91192754111a3d218f4477610700063de5d36d35.
Only os/milton/test_overlay.c and harness/exec_overlay.mk changed when run:

  make test-overlay                  exit 2 (test binary exit 1)
  make test-exec-overlay-flair        exit 2

atomic-red.txt: header/module/relocation reads fail AFTER copying their bytes.
The multi-chunk MZ fixture has six reads on the base implementation. Failed
reads 4, 5 and 6 change the destination, as do a failed raw read and the real
FAT short-chain read (1024 declared bytes, cluster 2 already EOC).
Malformed fixups in both chunks already refuse without destination writes.

flair-unbound-red.txt: the shipping flagship kernel, the existing overlay
fixture installed as SAMIR.COM, Ctrl-Alt-backslash through the real GUI pump.
The gate fails for missing OVL-MZ-CALLED after OVL-FAIL mz-load and rc=1.
These are observed failure transcripts, not success goldens.

PM supersession, initech-f9t: cb3c7910's full-module staging was rejected.
Validation refusals still preserve the whole destination, but genuine device
read errors during copy/fixup I/O may leave partial writes. This is an explicit
operator change to the error contract, not an inferred oracle relaxation.
stream-device-red.txt and stream-memory-red.txt were captured before changing
the loader: the rejected implementation fails the deliberately partial-read
fixture and uses 193280 B of linked byte buffers, above the 1024 B bound.
The older full-atomicity transcripts above remain historical evidence.
