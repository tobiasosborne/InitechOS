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
