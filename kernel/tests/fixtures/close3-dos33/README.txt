Genuine DOS 3.30 close/reopen probe, captured by the InitechBase owner.
Source evidence: initech-kyrcw, /tmp/ibase-dos-close3.P90Qfh (2026-11-14).
CLOSE3.ASM and CLOSE3.COM are the independent 16-bit reference probe; they
never execute under InitechDOS. The genuine runtime prints CLOSE3-OK when
closing inherited AUX handle3 lets OPEN MINT.PRG reuse exactly that JFT slot.
result.txt is the captured VGA text. Runtime provenance is the hash-pinned
DOS3.30 media used by goldens/dataloss; the disposable boot.img has SHA256
adc79af101c773435e7b3702f42827c243eecbb0342a723c7df13f25a4c38caf.
No reference operating system binary is redistributed here.

SHA256:
d1f480aac75164fac4c53c4061287428fd05b59ffe18e2212bca8448d7d42f4c CLOSE3.ASM
e80988cc8d2e98132e782fcace0cddd7ac85f9cbd0b6bc99cea5143cbe59ebc2 CLOSE3.COM
ac249551e17ef155f166bafcbaa81020b3723e7b0d09c3e91c4ea74b0c805314 result.txt

The host test adapts this register outcome to the native 32-bit dispatcher,
uses its own ordinary in-memory file fixture, and checks the captured slot3
reuse. Slots0,1,2,4 extend the same JFT invariant from DOS AH=3Eh, plus error6
on a second close and correct file-reference release in a standard-number slot.
The pinned resident SFT remains alive independently of every process JFT slot.
