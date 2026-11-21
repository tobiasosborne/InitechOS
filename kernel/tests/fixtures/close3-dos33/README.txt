Genuine DOS 3.30 close/reopen probe, captured by the InitechBase owner.
Source evidence: initech-kyrcw, /tmp/ibase-dos-close3.P90Qfh (2026-11-14).
CLOSE3.ASM and CLOSE3.COM are the independent 16-bit reference probe; they
never execute under InitechDOS. The genuine runtime prints CLOSE3-OK when
closing inherited AUX handle3 lets OPEN MINT.PRG reuse exactly that JFT slot.
result.txt retains the nonblank VGA text with right padding removed. The raw
transcript's SHA-256 was ac249551e17ef155f166bafcbaa81020b3723e7b0d09c3e91c4ea74b0c805314.
The capture used the hash-pinned DOS 3.30 media from goldens/dataloss.
The disposable boot.img had SHA-256
adc79af101c773435e7b3702f42827c243eecbb0342a723c7df13f25a4c38caf.

The host test checks the captured slot 3 reuse on the 32-bit dispatcher.
The same rule covers all five standard handles in The MS-DOS Encyclopedia
(FILES), and IBM DOS 3.30 Technical Reference p. 4-15 describes how CLOSE
releases a handle and its counted reference.
https://msarchive.pcjs.org/mspl13/msdos/encyclopedia/section3/
https://bitsavers.org/pdf/ibm/pc/dos/80X0945_DOS_3.30_Technical_Reference_Apr87.pdf
