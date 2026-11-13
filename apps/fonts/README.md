# Font specimen (SDK example)

FONTS.EXE is a test/example application, never a product-volume entry.
Dependencies: SDK. The optional legacy pixel checker also uses FLAIR's
public generated Chicago art and its factory-owned frozen NFNT metrics.
The target uses plain mk/tenant.mk flags with no semantic defines.

Frozen specimen: a clipped long title, seven Chicago style/text specimens,
a deliberately overlong line clipped to the window, File Quit, and four
Style menu entries drawn in their own face. Those four entries are visible
no-ops: the specimen is fixed. No files are persisted.

make test runs 20 independent literal transport checks on sdk/host in <1s;
this is unit/regression. make mutants kills one wrong-face patch, proving
red once. No embedded mutation hooks existed in this source. The claimed
ppm_flair_fonts_check.c is retained as an optional checker, not counted as a
passing tier-0 pixel differential. Its full emulator journey is unimported;
make smoke fails explicitly. Font metrics/styles property and real-resource
host suites were already claimed by FLAIR and remain owned there. Remaining
font specimen traces in the migration tree belong to factory.
