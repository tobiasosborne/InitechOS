# MS-DOS 3.30 reference utility acquisition

The earlier Packard Bell boot image lacks SORT.EXE and FIND.EXE. On
2026-11-15, DOS owner 4 recovered genuine utilities from PCjs's Microsoft
MS-DOS 3.30 two-disk archive:

https://www.pcjs.org/software/pcx86/sys/dos/microsoft/3.30/
https://diskettes.pcjs.org/pcx86/sys/dos/microsoft/3.30/MSDOS330-DISK1.json
https://diskettes.pcjs.org/pcx86/sys/dos/microsoft/3.30/MSDOS330-DISK2.json

The JSON sector arrays reconstruct 40 cylinders, two heads, nine sectors of
512 bytes, using little-endian 32-bit words and repeating the last word for
an omitted sector tail. Both reconstructed raw images match the archive's
independently published original-image MD5s:

- Disk 1: 5c2c58602d688c636ea1c9801e6106f4
- Disk 2: 7e7ed2b3f9864a1a383238f263999fc3

Mtools extracted these files; SHA256:

| Utility | SHA256 |
|---|---|
| FIND.EXE | 513f58937c7713572d8f204ce3f8a769c93d35c5e66c008f21b4fa54af5ccfa4 |
| SORT.EXE | af20bdbc7b2a7b4d070156e1cc4e83fd16048c7820aa849bba1067363e2621f4 |
| MORE.COM | 9a71219dbcc0ebe9b082a1ee8450ca4ed7150ce069dfdd7dcdc15a641465bb06 |
| XCOPY.EXE | c4c694e1c1625e8d22bf1cfa658eb814ba1e1259400deef0fa4a9a1ee2bb8e31 |

No proprietary binary is checked in, installed or used to build the product.
The XCOPY large-file /V capture is pinned separately in
`dos/xcopy/tests/compat33-large/`. It establishes no read-fault behavior.

The resource study remains initech-0e5gu. Completed preliminary SORT size
cases show a 32766-byte fixture succeeds and 65535/65536/65537-byte fixtures
refuse with `SORT: Insufficient memory`, ERRORLEVEL 1, on the tested 8 MiB
configuration. A following line-heavy case did not complete within the
reference harness bound, so the full run is incomplete. Neither that timeout
nor the successful prefix establishes an exact numeric memory or line limit.
FIND's late-match, count, refusal/status and diagnostic-stream cases remain
uncaptured. Existing InitechDOS limits, refusal text, statuses and streams
remain stated local safety assumptions until the complete captures are pinned.
The original acquisition, decoder and incomplete-run receipts are retained in
the DOS owner handoff; none of those runs is counted as a green full capture.
